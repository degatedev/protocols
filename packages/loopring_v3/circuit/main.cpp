// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.

#include "ThirdParty/BigInt.hpp"
#include "Utils/Data.h"
#include "Circuits/UniversalCircuit.h"

#include "ThirdParty/httplib.h"
//#include "ThirdParty/json.hpp"
#include "ethsnarks.hpp"
#include "import.hpp"
#include "stubs.hpp"
#include <fstream>
#include <chrono>
#include <mutex>
#include <cstdio>
#include <iostream>
#include <exception>
#include <pthread.h>
#include <unistd.h>

#ifdef ZKP_WORKER_MODE
#include "KafkaConsumer.hpp"
#include "S3.hpp"
#endif

#ifdef MULTICORE
#include <omp.h>
#endif

#define WITH_MEMORY_STATS 0

#if WITH_MEMORY_STATS
#include <unistd.h>
#include <ios>
#include <string>

#include <malloc.h>

//////////////////////////////////////////////////////////////////////////////
//
// process_mem_usage(double &, double &) - takes two doubles by reference,
// attempts to read the system-dependent data for a process' virtual memory
// size and resident set size, and return the results in KB.
//
// On failure, returns 0.0, 0.0

void process_mem_usage(double &vm_usage, double &resident_set)
{
    using std::ifstream;
    using std::ios_base;
    using std::string;

    vm_usage = 0.0;
    resident_set = 0.0;

    // 'file' stat seems to give the most reliable results
    //
    ifstream stat_stream("/proc/self/stat", ios_base::in);

    // dummy vars for leading entries in stat that we don't care about
    //
    string pid, comm, state, ppid, pgrp, session, tty_nr;
    string tpgid, flags, minflt, cminflt, majflt, cmajflt;
    string utime, stime, cutime, cstime, priority, nice;
    string O, itrealvalue, starttime;

    // the two fields we want
    //
    unsigned long vsize;
    long rss;

    stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr >> tpgid >> flags >> minflt >> cminflt >>
      majflt >> cmajflt >> utime >> stime >> cutime >> cstime >> priority >> nice >> O >> itrealvalue >> starttime >>
      vsize >> rss; // don't care about the rest

    stat_stream.close();

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    vm_usage = vsize / 1024.0;
    resident_set = rss * page_size_kb;
}

void printMemoryUsage()
{
    malloc_trim(0);

    double vm, rss;
    process_mem_usage(vm, rss);
    std::cout << "VM: " << unsigned(vm * 0.001) << "MB; RSS: " << unsigned(rss * 0.001) << "MB" << std::endl;
}
#else
void printMemoryUsage()
{
}
#endif

using json = nlohmann::json;

enum class Mode
{
    CreateKeys = 0,
    Validate,
    Prove,
    ExportCircuit,
    ExportWitness,
    Server,
    Benchmark,
	Test
};

namespace libsnark
{
static void from_json(const nlohmann::json &j, libsnark::Config &config)
{
    if (j.contains("num_threads"))
    {
        config.num_threads = j.at("num_threads").get<unsigned int>();
    }
    if (j.contains("smt"))
    {
        config.smt = j.at("smt").get<bool>();
    }
    if (j.contains("fft"))
    {
        config.fft = j.at("fft").get<std::string>();
    }
    if (j.contains("radixes"))
    {
        config.radixes = j.at("radixes").get<std::vector<unsigned int>>();
    }
    if (j.contains("swapAB"))
    {
        config.swapAB = j.at("swapAB").get<bool>();
    }
    if (j.contains("multi_exp_c"))
    {
        config.multi_exp_c = j.at("multi_exp_c").get<unsigned int>();
    }
    if (j.contains("multi_exp_prefetch_locality"))
    {
        config.multi_exp_prefetch_locality = j.at("multi_exp_prefetch_locality").get<unsigned int>();
    }
    if (j.contains("prefetch_stride"))
    {
        config.prefetch_stride = j.at("prefetch_stride").get<unsigned int>();
    }
    if (j.contains("multi_exp_look_ahead"))
    {
        config.multi_exp_look_ahead = j.at("multi_exp_look_ahead").get<unsigned int>();
    }
}
} // namespace libsnark

struct BenchmarkConfig
{
    unsigned int num_iterations;
    std::vector<unsigned int> num_threads;
    std::vector<bool> smt;
    std::vector<std::string> fft;
    std::vector<std::vector<unsigned int>> radixes;
    std::vector<bool> swapAB;
    std::vector<unsigned int> multi_exp_c;
    std::vector<unsigned int> multi_exp_prefetch_locality; // 4 == no prefetching, [0, 3] prefetch
                                                           // locality
    std::vector<unsigned int> prefetch_stride;             // 4 * L1_CACHE_BYTES
    std::vector<unsigned int> multi_exp_look_ahead;
};

static void from_json(const nlohmann::json &j, BenchmarkConfig &config)
{
    config.num_iterations = j.at("num_iterations").get<unsigned int>();
    config.num_threads = j.at("num_threads").get<std::vector<unsigned int>>();
    config.smt = j.at("smt").get<std::vector<bool>>();
    config.fft = j.at("fft").get<std::vector<std::string>>();
    config.radixes = j.at("radixes").get<std::vector<std::vector<unsigned int>>>();
    config.swapAB = j.at("swapAB").get<std::vector<bool>>();
    config.multi_exp_c = j.at("multi_exp_c").get<std::vector<unsigned int>>();
    config.multi_exp_prefetch_locality = j.at("multi_exp_prefetch_locality").get<std::vector<unsigned int>>();
    config.prefetch_stride = j.at("prefetch_stride").get<std::vector<unsigned int>>();
    config.multi_exp_look_ahead = j.at("multi_exp_look_ahead").get<std::vector<unsigned int>>();
}

static inline auto now() -> decltype(std::chrono::high_resolution_clock::now())
{
    return std::chrono::high_resolution_clock::now();
}

template <typename T> unsigned int elapsed_time_ms(const T &t1)
{
    auto t2 = std::chrono::high_resolution_clock::now();
    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    return time_ms;
}

template <typename T> void print_time(const T &t1, const char *str)
{
    printf("%s (%dms)\n", str, elapsed_time_ms(t1));
}

bool fileExists(const std::string &fileName)
{
    std::ifstream infile(fileName.c_str());
    return infile.good();
}

void initProverContextBuffers(ProverContextT &context)
{
    context.scratch_exponents.resize(std::max(context.constraint_system->num_variables() + 1, context.domain->m - 1));
    context.aA.resize(context.domain->m + 1, FieldT::one());
    context.aB.resize(context.domain->m + 1, FieldT::one());
    context.aH.resize(context.domain->m + 1, FieldT::one());
}

bool generateKeyPair(ethsnarks::ProtoboardT &pb, std::string &baseFilename)
{
    std::string provingKeyFilename = baseFilename + "_pk.raw";
    std::string verificationKeyFilename = baseFilename + "_vk.json";
#ifdef GPU_PROVE
    std::string paramsFilename = baseFilename + "_params.raw";
#endif
    if (
      fileExists(provingKeyFilename.c_str()) && fileExists(verificationKeyFilename.c_str())
#ifdef GPU_PROVE
      && fileExists(paramsFilename.c_str())
#endif
    )
    {
        return true;
    }
#ifdef GPU_PROVE
    std::cout << "Generating keys and params..." << std::endl;
    int result = stub_genkeys_params_from_pb(
      pb, provingKeyFilename.c_str(), verificationKeyFilename.c_str(), paramsFilename.c_str());
#else
    std::cout << "Generating keys..." << std::endl;
    int result = stub_genkeys_from_pb(pb, provingKeyFilename.c_str(), verificationKeyFilename.c_str());
#endif
    return (result == 0);
}

json loadJSON(const std::string &filename)
{
    // Read the JSON file
    std::ifstream file(filename.c_str());
    if (!file.is_open())
    {
        std::cerr << "Cannot open json file: " << filename << std::endl;
        return json();
    }
    json input;
    file >> input;
    file.close();
    return input;
}

libsnark::Config loadConfig(const std::string &filename)
{
    return loadJSON(filename).get<libsnark::Config>();
}

void loadProvingKey(const std::string &pk_file, ethsnarks::ProvingKeyT &proving_key)
{
    std::cout << "Loading proving key " << pk_file << "..." << std::endl;
    auto begin = now();
    auto pk = ethsnarks::load_proving_key(pk_file.c_str());
    proving_key.alpha_g1 = std::move(pk.alpha_g1);
    proving_key.beta_g1 = std::move(pk.beta_g1);
    proving_key.beta_g2 = std::move(pk.beta_g2);
    proving_key.delta_g1 = std::move(pk.delta_g1);
    proving_key.delta_g2 = std::move(pk.delta_g2);
    proving_key.A_query = std::move(pk.A_query);
    proving_key.B_query = std::move(pk.B_query);
    proving_key.H_query = std::move(pk.H_query);
    proving_key.L_query = std::move(pk.L_query);
    print_time(begin, "Proving key loaded");
}

VerificationKeyT loadVerificationKey(const std::string &vk_file)
{
    std::cout << "Loading verification key " << vk_file << "..." << std::endl;
    return vk_from_json(loadJSON(vk_file));
}

std::string proveCircuit(ProverContextT &context, Loopring::Circuit *circuit)
{
    std::cout << "Generating proof..." << std::endl;
    auto begin = now();
    std::string jProof = ethsnarks::prove(context, circuit->getPb());
    unsigned int elapsed_ms = elapsed_time_ms(begin);
    elapsed_ms = elapsed_ms == 0 ? 1 : elapsed_ms;
    std::cout << "Proof generated in " << float(elapsed_ms) / 1000.0f << " seconds ("
              << (circuit->getPb().num_constraints() * 10) / (elapsed_ms / 100) << " constraints/second)" << std::endl;
    return jProof;
}

bool writeProof(const std::string &jProof, const std::string &proofFilename)
{
    std::ofstream fproof(proofFilename);
    if (!fproof.is_open())
    {
        std::cerr << "Cannot create proof file: " << proofFilename << std::endl;
        return false;
    }
    fproof << jProof;
    fproof.close();
    std::cout << "Proof written to: " << proofFilename << std::endl;
    return true;
}

Loopring::Circuit *newCircuit(unsigned int blockType, ethsnarks::ProtoboardT &outPb)
{
    return new Loopring::UniversalCircuit(outPb, "circuit");
}

Loopring::Circuit *createCircuit(unsigned int blockType, unsigned int blockSize, ethsnarks::ProtoboardT &outPb)
{
    std::cout << "Creating circuit... " << std::endl;
    auto begin = now();
    Loopring::Circuit *circuit = newCircuit(blockType, outPb);
    circuit->generateConstraints(blockSize);
    circuit->printInfo();
    print_time(begin, "Circuit created");
    return circuit;
}

bool generateWitness(Loopring::Circuit *circuit, const json &input)
{
    std::cout << "Generating witness... " << std::endl;
    auto begin = now();
    if (!circuit->generateWitness(input))
    {
        std::cerr << "Could not generate witness!" << std::endl;
        return false;
    }
    print_time(begin, "Witness generated");
    return true;
}

bool validateCircuit(Loopring::Circuit *circuit)
{
    std::cout << "Validating block..." << std::endl;
    auto begin = now();
    // Check if the inputs are valid for the circuit
    #ifndef NDEBUG
        circuit->printInfo();
    #endif
    if (!circuit->getPb().is_satisfied())
    {
        std::cerr << "Block is not valid!" << std::endl;
        return false;
    }
    print_time(begin, "Block is valid");
    return true;
}

std::string getBaseName(unsigned int blockType)
{
    switch (blockType)
    {
        default:
            return "all";
    }
}

std::string getProvingKeyFilename(const std::string &baseFilename)
{
    return baseFilename + "_pk.raw";
}

void runServer(
  ProverContextT &context,
  Loopring::Circuit *circuit,
  const std::string &provingKeyFilename,
  const libsnark::Config &config,
  unsigned int port)
{
    using namespace httplib;

    struct ProverStatus
    {
        bool proving = false;
        std::string blockFilename;
        std::string proofFilename;
    };

    struct ProverStatusRAII
    {
        ProverStatus &proverStatus;
        ProverStatusRAII(
          ProverStatus &_proverStatus,
          const std::string &blockFilename,
          const std::string &proofFilename)
            : proverStatus(_proverStatus)
        {
            proverStatus.proving = true;
            proverStatus.blockFilename = blockFilename;
            proverStatus.proofFilename = proofFilename;
        }

        ~ProverStatusRAII()
        {
            proverStatus.proving = false;
        }
    };

    // Prover status info
    ProverStatus proverStatus;
    // Lock for the prover
    std::mutex mtx;
    // Setup the server
    Server svr;
    // Called to prove blocks
    svr.Get("/prove", [&](const Request &req, Response &res) {
    		try
    		{
			const std::lock_guard<std::mutex> lock(mtx);

			// Parse the parameters
			std::string blockFilename = req.get_param_value("block_filename");
			std::string proofFilename = req.get_param_value("proof_filename");
			std::string strValidate = req.get_param_value("validate");
			bool validate = (strValidate.compare("true") == 0) ? true : false;
			if (blockFilename.length() == 0)
			{
				res.set_content("Error: block_filename missing!\n", "text/plain");
				return;
			}
			std::string strDelFile = req.get_param_value("delFile");
			bool delFileAfterSuccess = (strDelFile.compare("true") == 0) ? true : false;

			// Set the prover status for this session
			ProverStatusRAII statusRAII(proverStatus, blockFilename, proofFilename);

			// Prove the block
			json input = loadJSON(blockFilename);
			if (input == json())
			{
				res.set_content("Error: Failed to load block!\n", "text/plain");
				return;
			}

			// Some checks to see if this block is compatible with the loaded circuit
			int iBlockType = input["blockType"].get<int>();
			unsigned int blockSize = input["blockSize"].get<int>();
			if (/*iBlockType & circuit->getBlockType() != 1 || */ blockSize != circuit->getBlockSize())
			{
				res.set_content(
				  "Error: Incompatible block requested! Use /info to check "
				  "which blocks can be proven.\n",
				  "text/plain");
				return;
			}

			if (!generateWitness(circuit, input))
			{
				res.set_content("Error: Failed to generate witness for block!\n", "text/plain");
				return;
			}
			if (validate)
			{
				if (!validateCircuit(circuit))
				{
					res.set_content("Error: Block is invalid!\n", "text/plain");
					return;
				}
			}
			std::string jProof = proveCircuit(context, circuit);
			if (jProof.length() == 0)
			{
				res.set_content("Error: Failed to prove block!\n", "text/plain");
				return;
			}
			if (proofFilename.length() != 0)
			{
				if (!writeProof(jProof, proofFilename))
				{
					res.set_content("Error: Failed to write proof!\n", "text/plain");
					return;
				}
			}

			// verify the proof.
			VerificationKeyT vk = loadVerificationKey(provingKeyFilename.substr(0, provingKeyFilename.length() - 6) + "vk.json");
            std::stringstream proof_stream;
            proof_stream << jProof;
            auto proof_pair = proof_from_json(proof_stream);

            std::cout << "proof:" << jProof;
            bool verified = libsnark::r1cs_gg_ppzksnark_zok_verifier_strong_IC<ppT>(vk, proof_pair.first, proof_pair.second);
            std::cout << "verified:" << verified << std::endl;

			// Return the proof
			res.set_content(jProof + "\n", "text/plain");
			if(delFileAfterSuccess){
					std::remove(blockFilename.c_str());
			}
    		}catch(std::exception& e)
    		{
    			std::string message = std::string("Prove error, exception:") + std::string(e.what());
    			std::cout << message << std::endl;
    			res.set_content(message, "text/plain");
    			return;
    		}
    });
    // Retuns the status of the server
    svr.Get("/status", [&](const Request &req, Response &res) {
        if (proverStatus.proving)
        {
            std::string status = std::string("Proving ") + proverStatus.blockFilename;
            res.set_content(status + "\n", "text/plain");
        }
        else
        {
            res.set_content("Idle\n", "text/plain");
        }
    });
    // Info of this prover server
    svr.Get("/info", [&](const Request &req, Response &res) {
        std::string info = std::string("BlockType: ") + std::to_string(int(circuit->getBlockType())) +
                           std::string("; BlockSize: ") + std::to_string(circuit->getBlockSize()) + "\n";
        res.set_content(info, "text/plain");
    });
    // Stops the prover server
    svr.Get("/stop", [&](const Request &req, Response &res) {
        const std::lock_guard<std::mutex> lock(mtx);
        svr.stop();
    });
    // Default page contains help
    svr.Get("/", [&](const Request &req, Response &res) {
        std::string content;
        content += "Prover server:\n";
        content += "- Prove a block: "
                   "/prove?block_filename=<block.json>&proof_filename=<proof.json>&"
                   "validate=true (proof_filename and validate are optional)\n";
        content += "- Status of the server: /status (busy proving a block or not)\n";
        content += "- Info of the server: /info (which blocks can be proven)\n";
        content += "- Shut down the server: /stop (will first finish generating "
                   "the proof if busy)\n";
        res.set_content(content, "text/plain");
    });

    std::cout << "Running server on 'localhost' on port " << port << std::endl;
    svr.listen("0.0.0.0", port);
}

#ifdef ZKP_WORKER_MODE
int msg_consume(std::vector<char> &data, RdKafka::Message* message, void* opaque) {
    int len = -1;
    switch (message->err()) {
    case RdKafka::ERR__TIMED_OUT:
        //std::cout << "RdKafka::ERR__TIMED_OUT" << std::endl;
        break;
    case RdKafka::ERR_NO_ERROR:
    {
        if(message->payload())
        {
            len = static_cast<int>(message->len());
            data.resize(len);
            const char *msg = static_cast<const char *>(message->payload());
            memcpy(&data[0], msg, len);
        }
    }
        break;

    case RdKafka::ERR__PARTITION_EOF:
    {
        len = 0;
    }
        break;

    case RdKafka::ERR__UNKNOWN_TOPIC:
    case RdKafka::ERR__UNKNOWN_PARTITION:
    default:
        /* Errors */
        std::cerr << "Consume failed: " << message->errstr() << std::endl;
        len = -1;
    }
    return len;
}
#endif

std::string& replace_all(std::string& str,const std::string& old_value,const std::string& new_value)
{
	for(std::string::size_type   pos(0);   pos!=std::string::npos;   pos+=new_value.length())
	{
		if((pos=str.find(old_value,pos))!=std::string::npos)
		{
           str.replace(pos,old_value.length(),new_value);
		}
		else
		{
			break;
		}
    }
    return str;
}

#ifdef ZKP_WORKER_MODE
struct WorkerParams  
{  
    ProverContextT context;
    Loopring::Circuit *circuit;
    std::string provingKeyFilename;
};  

//1.consume zkp task message from kafka.
//2.call local api general zkp.
//3.produce zkp_result message to kafka.
void *runWorker(void *workerParams)
{
    struct WorkerParams *params =(struct WorkerParams*)workerParams;
    ProverContextT context = params->context;
    Loopring::Circuit *circuit = params->circuit;
    std::string provingKeyFilename = params->provingKeyFilename;

	std::string workerConfFile = "worker_config.json";
	json workerConf = loadJSON(workerConfFile);
	std::cout << "Run zkp worker start." << std::endl;

	//s3 client init.
	S3Service s3 = S3Service(workerConf);

	//kafka consumer
	std::string topic = workerConf["KAFKA_CONF"]["topic"].get<std::string>();
	std::string servers = workerConf["KAFKA_CONF"]["servers"].get<std::string>();
	std::string groupId = workerConf["KAFKA_CONF"]["group_id"].get<std::string>();
	KafkaConsumer consumer;
	consumer.SetTopic(topic);
	consumer.Init(servers, groupId);

	//kafka producer.
	std::string errstr;
	RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
	conf->set("bootstrap.servers", servers, errstr);
	std::shared_ptr<EventCB> m_eventcb = std::shared_ptr<EventCB>(new EventCB);
	conf->set("event_cb", m_eventcb.get(), errstr);
	RdKafka::Producer *producer = RdKafka::Producer::create(conf, errstr);
	if(!producer)
	{
		std::cerr << "Failed to create Kafka producer:" << errstr << std::endl;
		exit(1);
	}

	while(true)
	{
		std::vector<char> data;
		std::shared_ptr<RdKafka::Message> msg = consumer.consume(1000);
		RdKafka::Message* message = msg.get();
        if(message->err() != RdKafka::ERR_NO_ERROR)
        {
            if(message->err() != RdKafka::ERR__TIMED_OUT)
            {
                std::cerr << "Consume failed: " << message->errstr() << std::endl;
            }
            sleep(10);
        }
        else if(message->err() == RdKafka::ERR_NO_ERROR){
            if(message->payload()){
                std::string payload = reinterpret_cast<char *>(message->payload());
                std::cout << "Get zkp-request message, offset:" << message->offset() << ", value:" << payload << std::endl;
                auto value = json::parse(payload);

                //download block.json.
                std::string blockId = value["id"];
                std::string objectKey = std::string("blocks/block_0_" + blockId + ".json");
                std::string localFile = std::string("s3_data/block_0_" + blockId + ".json");
                bool downloadSuccess = s3.GetObjectToFile(objectKey, localFile);

                if(downloadSuccess){
                    std::cout <<"Download block.json from s3 success, key:" << objectKey << std::endl;

                    value["message"] = "";
                    value["proof"] = "";
                    value["success"] = false;
                    // Prove the block
                    json input = loadJSON(localFile);
                    if (input == json())
                    {
                        std::cout << "Error: Failed to load block!" << std::endl;
                        value["message"] = "Error: Failed to load block!";
                    }
                    else
                    {
                        // Some checks to see if this block is compatible with the loaded circuit
                        int iBlockType = input["blockType"].get<int>();
                        unsigned int blockSize = input["blockSize"].get<int>();
                        if (/*iBlockType & circuit->getBlockType() != 1 || */ blockSize != circuit->getBlockSize())
                        {
                            std::cout <<
                                    "Error: Incompatible block requested! Use /info to check "
                                    "which blocks can be proven.\n" << std::endl;
                            value["message"] = "Error: Incompatible block requested! Use /info to check which blocks can be proven.";
                        }
                        else
                        {
                            if (!generateWitness(circuit, input))
                            {
                                std::cout << "Error: Failed to generate witness for block!"<< std::endl;
                                value["message"] = "Error: Failed to generate witness for block!";
                            }
                            else
                            {
                                if (!validateCircuit(circuit))
                                {
                                    value["message"] = "Error: Validate Block Failed!";
                                }else
                                {
                                    std::string jProof = proveCircuit(context, circuit);
                                    if (jProof.length() == 0)
                                    {
                                        std::cout << "Error: Failed to prove block!" << std::endl;
                                        value["message"] = "Error: Failed to prove block!";
                                    }
                                    else
                                    {
                                        VerificationKeyT vk = loadVerificationKey(provingKeyFilename.substr(0, provingKeyFilename.length() - 6) + "vk.json");
                                        std::stringstream proof_stream;
                                        proof_stream << jProof;
                                        auto proof_pair = proof_from_json(proof_stream);

                                        std::cout << "proof:" << jProof;
                                        value["success"] = libsnark::r1cs_gg_ppzksnark_zok_verifier_strong_IC<ppT>(vk, proof_pair.first, proof_pair.second);
                                        std::cout << "verified:" << value["success"] << std::endl;
                                        value["proof"] = jProof;
                                    }
                                }
                            }
                        }
                    }
                }else{
                    std::cout <<"Download block.json from s3 error, key:" << objectKey << std::endl;
                    value["proof"] = "";
                    value["success"] = false;
                    value["message"] = "Download block.json from s3 error";
                }

                //produce zkp-result message.
                std::string message = value.dump();
                RdKafka::ErrorCode resp = producer->produce(workerConf["KAFKA_CONF"]["zkp-result-topic"].get<std::string>(),
                                                    RdKafka::Topic::PARTITION_UA,
                                                    RdKafka::Producer::RK_MSG_COPY,
                                                    const_cast<char *>(message.c_str()), message.size(), NULL, 0, 0, NULL);
                if (resp != RdKafka::ERR_NO_ERROR)
                {
                    std::cerr << "% Produce failed: " << RdKafka::err2str(resp) << std::endl;
                }
                else
                {
                    std::cerr << "% Produced zkp-result message:" << message << std::endl;
                    producer->poll(0);
                }
            }
        }
	}
}
#endif

bool runBenchmark(Loopring::Circuit *circuit, const std::string &provingKeyFilename)
{
    // Load the proving key a single time
    ProverContextT context;
    loadProvingKey(provingKeyFilename, context.provingKey);
    context.constraint_system = &(circuit->getPb().constraint_system);

    VerificationKeyT vk =
      loadVerificationKey(provingKeyFilename.substr(0, provingKeyFilename.length() - 6) + "vk.json");

    if (!validateCircuit(circuit))
    {
        return false;
    }

    // Get all configs to benchmark from the benchmark config
    BenchmarkConfig benchmarkConfig = loadJSON("benchmark.json").get<BenchmarkConfig>();

    // Create all configs
    std::vector<libsnark::Config> configs;
    for (auto num_threads : benchmarkConfig.num_threads)
    {
        for (auto smt : benchmarkConfig.smt)
        {
            for (auto prefetch_stride : benchmarkConfig.prefetch_stride)
            {
                for (auto swapAB : benchmarkConfig.swapAB)
                {
                    for (auto fft : benchmarkConfig.fft)
                    {
                        for (auto radixes : benchmarkConfig.radixes)
                        {
                            for (auto multi_exp_c : benchmarkConfig.multi_exp_c)
                            {
                                for (auto multi_exp_prefetch_locality : benchmarkConfig.multi_exp_prefetch_locality)
                                {
                                    for (auto multi_exp_look_ahead : benchmarkConfig.multi_exp_look_ahead)
                                    {
                                        libsnark::Config config;
                                        config.num_threads = num_threads;
                                        config.smt = smt;
                                        config.prefetch_stride = prefetch_stride;
                                        config.swapAB = swapAB;
                                        config.fft = fft;
                                        config.radixes = radixes;
                                        config.multi_exp_c = multi_exp_c;
                                        config.multi_exp_prefetch_locality = multi_exp_prefetch_locality;
                                        config.multi_exp_look_ahead = multi_exp_look_ahead;
                                        configs.push_back(config);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    unsigned int num_iterations = benchmarkConfig.num_iterations;

    struct Result
    {
        libsnark::Config config;
        unsigned int duration_ms;

        static bool compareResult(Result a, Result b)
        {
            return (a.duration_ms < b.duration_ms);
        }
    };
    std::vector<Result> results;
    for (auto config : configs)
    {
        std::cout << "*****************************" << std::endl;
        std::cout << "Config: " << config << std::endl;
        std::cout << "*****************************" << std::endl;
#ifdef MULTICORE
        omp_set_num_threads(config.num_threads);
#endif

        context.config = config;
        context.domain = get_domain(circuit->getPb(), context.provingKey, config);
        initProverContextBuffers(context);

        unsigned int totalTime = 0;
        for (unsigned int l = 0; l < num_iterations; l++)
        {
            auto begin = now();
            std::string jProof = proveCircuit(context, circuit);
            totalTime += elapsed_time_ms(begin);
            if (jProof.length() == 0)
            {
                return false;
            }

            std::stringstream proof_stream;
            proof_stream << jProof;
            auto proof_pair = proof_from_json(proof_stream);

            if (!libsnark::r1cs_gg_ppzksnark_zok_verifier_strong_IC<ppT>(vk, proof_pair.first, proof_pair.second))
            {
                std::cerr << "Invalid proof!" << std::endl;
                return false;
            }
        }

        Result result;
        result.config = config;
        result.duration_ms = totalTime / num_iterations;
        results.push_back(result);
    }

    std::sort(results.begin(), results.end(), Result::compareResult);

    std::cout << "Benchmark results:" << std::endl;
    for (unsigned int i = 0; i < results.size(); i++)
    {
        const libsnark::Config &config = results[i].config;
        std::cout << i << ". " << config << " (" << results[i].duration_ms << "ms)" << std::endl;
    }

    return true;
}

#ifdef ZKP_WORKER_MODE
void test(std::string provingKeyFilename)
{
	std::cout << "Test start." << std::endl;
//	Aws::SDKOptions options;
//	Aws::InitAPI(options);
//	{
//	    const Aws::String bucket_name = "degate-dev";
//	    const Aws::String object_name = "blocks/block_0_1.json";
//	    const Aws::String region = "ap-northeast-2";
//
//	    if (!GetObjectToFile(object_name, bucket_name, region, "../../s3_data/block_0_1.json"))
//	    {
//	    		std::cout << "Get s3 object error." << std::endl;
//	    }
//	}
//	Aws::ShutdownAPI(options);
//	std::cout << "Test end." << std::endl;
//    VerificationKeyT vk = loadVerificationKey(provingKeyFilename.substr(0, provingKeyFilename.length() - 6) + "vk.json");
//	std::string jProof = "";
//    if (jProof.length() == 0)
//    {
//         return false;
//    }
//
//    std::stringstream proof_stream;
//    proof_stream << jProof;
//    auto proof_pair = proof_from_json(proof_stream);
//
//    if (!libsnark::r1cs_gg_ppzksnark_zok_verifier_strong_IC<ppT>(vk, proof_pair.first, proof_pair.second))
//    {
//        std::cerr << "Invalid proof!" << std::endl;
//        return false;
//    }
}
#endif

int main(int argc, char **argv)
{
    std::cout << "in main: " << std::endl;
    ethsnarks::ppT::init_public_params();

    // Load in the config
    libsnark::Config config = loadConfig("config.json");
    std::cout << "Config: " << config << std::endl;

#ifdef MULTICORE
    // omp_set_nested is needed for gcc for some reason
    omp_set_nested(1);
    omp_set_max_active_levels(5);
    std::cout << "Num threads available: " << omp_get_max_threads() << std::endl;
    std::cout << "Num processors available: " << omp_get_num_procs() << std::endl;
#endif

    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << std::endl;
        std::cerr << "-validate <block.json>: Validates a block" << std::endl;
        std::cerr << "-prove <block.json> <out_proof.json>: Proves a block" << std::endl;
        std::cerr << "-createkeys <protoBlock.json>: Creates prover/verifier keys" << std::endl;
        std::cerr << "-verify <vk.json> <proof.json>: Verify a proof" << std::endl;
        std::cerr << "-exportcircuit <block.json> <circuit.json>: Exports the rc1s "
                     "circuit to json (circom - not all fields)"
                  << std::endl;
        std::cerr << "-exportwitness <block.json> <witness.json>: Exports the "
                     "witness to json (circom)"
                  << std::endl;
        std::cerr << "-createpk <pk.json> <pk.raw>: Creates the "
                     "proving key using a bellman pk"
                  << std::endl;
        std::cerr << "-pk_alt2mcl <pk_alt.raw> <pk_mcl.raw>: Converts the proving "
                     "key from the alt format to the mcl format"
                  << std::endl;
        std::cerr << "-pk_mcl2nozk <pk_mlc.raw> <pk_nozk.raw>: Converts the "
                     "proving key from the mcl format to the nozk format"
                  << std::endl;
        std::cerr << "-server <block.json> <port>: Keeps the program running as an "
                     "HTTP server to prove blocks on demand"
                  << std::endl;
        std::cerr << "-benchmark <block.json>: Try out multiple prover options to "
                     "find the fastest configuration on the system"
                  << std::endl;
        return 1;
    }

    const char *proofFilename = NULL;
    Mode mode = Mode::Validate;

    #ifdef ZKP_WORKER_MODE
        std::string baseFilename = "/data/keys/";
    #else
        std::string baseFilename = "keys/";
    #endif

    if (strcmp(argv[1], "-validate") == 0)
    {
        mode = Mode::Validate;
        std::cout << "Validating " << argv[2] << "..." << std::endl;
    }
    else if (strcmp(argv[1], "-prove") == 0)
    {
        if (argc != 4)
        {
            std::cout << "Invalid number of arguments!" << std::endl;
            return 1;
        }
        mode = Mode::Prove;
        proofFilename = argv[3];
        std::cout << "Proving " << argv[2] << "..." << std::endl;
    }
    else if (strcmp(argv[1], "-createkeys") == 0)
    {
        if (argc != 3)
        {
            std::cout << "Invalid number of arguments!" << std::endl;
            return 1;
        }
        mode = Mode::CreateKeys;
        std::cout << "Creating keys for " << argv[2] << "..." << std::endl;
    }
    else if (strcmp(argv[1], "-verify") == 0)
    {
        if (argc != 4)
        {
            std::cout << "Invalid number of arguments!" << std::endl;
            return 1;
        }
        std::cout << "Verify for " << argv[3] << " ..." << std::endl;
        if (stub_main_verify(argv[0], argc - 1, (const char **)(argv + 1)))
        {
            return 1;
        }
        std::cout << "Proof is valid" << std::endl;
        return 0;
    }
    else if (strcmp(argv[1], "-exportcircuit") == 0)
    {
        if (argc != 4)
        {
            std::cout << "Invalid number of arguments!" << std::endl;
            return 1;
        }
        mode = Mode::ExportCircuit;
        std::cout << "Exporting circuit for " << argv[2] << "..." << std::endl;
    }
    else if (strcmp(argv[1], "-exportwitness") == 0)
    {
        if (argc != 4)
        {
            std::cout << "Invalid number of arguments!" << std::endl;
            return 1;
        }
        mode = Mode::ExportWitness;
        std::cout << "Exporting witness for " << argv[2] << "..." << std::endl;
    }
    else if (strcmp(argv[1], "-createpk") == 0)
    {
        if (argc != 4)
        {
            std::cout << "Invalid number of arguments!" << std::endl;
            return 1;
        }
        std::cout << "Converting pk from " << argv[2] << " to " << argv[3] << " ..." << std::endl;
        if (!pk_bellman2ethsnarks(argv[2], argv[3]))
        {
            return 1;
        }
        std::cout << "Successfully created pk " << argv[3] << "." << std::endl;
        return 0;
    }
    else if (strcmp(argv[1], "-pk_alt2mcl") == 0)
    {
        if (argc != 4)
        {
            std::cout << "Invalid number of arguments!" << std::endl;
            return 1;
        }
        std::cout << "Converting pk from " << argv[2] << " to " << argv[3] << " ..." << std::endl;
        if (!pk_alt2mcl(argv[2], argv[3]))
        {
            std::cout << "Could not convert pk." << std::endl;
            return 1;
        }
        std::cout << "Successfully created pk " << argv[3] << "." << std::endl;
        return 0;
    }
    else if (strcmp(argv[1], "-pk_mcl2nozk") == 0)
    {
        if (argc != 4)
        {
            std::cout << "Invalid number of arguments!" << std::endl;
            return 1;
        }
        std::cout << "Converting pk from " << argv[2] << " to " << argv[3] << " ..." << std::endl;
        if (!pk_mcl2nozk(argv[2], argv[3]))
        {
            std::cout << "Failed to convert!" << std::endl;
            return 1;
        }
        std::cout << "Successfully created pk " << argv[3] << "." << std::endl;
        return 0;
    }
    else if (strcmp(argv[1], "-server") == 0)
    {
        if (argc != 4)
        {
            std::cout << "Invalid number of arguments!" << std::endl;
            return 1;
        }
        mode = Mode::Server;
        std::cout << "Starting proving server for " << argv[2] << " on port " << argv[3] << "..." << std::endl;
    }
    else if (strcmp(argv[1], "-benchmark") == 0)
    {
        if (argc != 3)
        {
            std::cout << "Invalid number of arguments!" << std::endl;
            return 1;
        }
        mode = Mode::Benchmark;
        std::cout << "Benchmarking " << argv[2] << "..." << std::endl;
    }
    else if (strcmp(argv[1], "-test") == 0)
    {
        if (argc != 3)
        {
            std::cout << "Invalid number of arguments!" << std::endl;
            return 1;
        }
        mode = Mode::Test;
        std::cout << "Test ..." << std::endl;
    }
    else
    {
        std::cerr << "Unknown option: " << argv[1] << std::endl;
        return 1;
    }

    // Read the block file
    std::cout << "in main before loadJSON" << std::endl;
    json input = loadJSON(argv[2]);
    std::cout << "in main after loadJSON" << std::endl;
    if (input == json())
    {
        return 1;
    }

    // Read meta data
    int iBlockType = input["blockType"].get<int>();
    unsigned int blockSize = input["blockSize"].get<int>();
    std::string postFix = "_" + std::to_string(blockSize);

    /*if (iBlockType >= int(Loopring::BlockType::COUNT))
    {
        std::cerr << "Invalid block type: " << iBlockType << std::endl;
        return 1;
    }*/
    unsigned int blockType = iBlockType;
    baseFilename += getBaseName(blockType) + postFix;
    std::string provingKeyFilename = getProvingKeyFilename(baseFilename);

    #ifdef ZKP_WORKER_MODE
        if (mode == Mode::Test)
        {
                test(provingKeyFilename);
                return 0;
        }
    #endif

    if (mode == Mode::Prove || mode == Mode::Server)
    {
        if (!fileExists(provingKeyFilename))
        {
            std::cerr << "Failed to find pk!" << std::endl;
            return 1;
        }
    }

    ethsnarks::ProtoboardT pb;
    Loopring::Circuit *circuit = createCircuit(blockType, blockSize, pb);
    if (config.swapAB)
    {
        // pb.constraint_system.swap_AB_if_beneficial();
    }
    pb.constraint_system.constraints.shrink_to_fit();
    pb.values.shrink_to_fit();
    libsnark::ConstantStorage<FieldT>::getInstance().constants.shrink_to_fit();

    printMemoryUsage();

#if 0
    unsigned int totalCoeffs = 0;
    for (size_t i = 0; i < pb.constraint_system.constraints.size(); ++i)
    {
        totalCoeffs += pb.constraint_system.constraints[i]->getA().getTerms().size();
        totalCoeffs += pb.constraint_system.constraints[i]->getB().getTerms().size();
        totalCoeffs += pb.constraint_system.constraints[i]->getC().getTerms().size();
    }
    std::cout << "num coefficients: " << totalCoeffs << std::endl;
    std::cout << "num unique coefficients: " << libsnark::ConstantStorage<FieldT>::getInstance().constants.size() << std::endl;
#endif

    if (mode == Mode::Benchmark)
    {
        if (!generateWitness(circuit, input))
        {
            return 1;
        }
        runBenchmark(circuit, provingKeyFilename);
    }

#ifdef MULTICORE
    omp_set_num_threads(config.num_threads);
    std::cout << "Num threads used: " << omp_get_max_threads() << std::endl;
#endif

    if (mode == Mode::Server)
    {
    	// Setup the context a single time
    	ProverContextT context;
    	loadProvingKey(provingKeyFilename, context.provingKey);
    	context.constraint_system = &(circuit->getPb().constraint_system);
    	context.config = config;
    	context.domain = get_domain(circuit->getPb(), context.provingKey, config);
    	initProverContextBuffers(context);

        pthread_t tid;

        #ifdef ZKP_WORKER_MODE
            struct WorkerParams p;
            p.context = context;
            p.circuit = circuit;
            p.provingKeyFilename = provingKeyFilename;
            int ret = pthread_create(&tid, NULL, runWorker, &p);
            std::cout << "worker thread start, result:" << ret << std::endl;
            if(ret == 0)
            {
                runServer(context, circuit, provingKeyFilename, config, std::stoi(argv[3]));
            }
        #else
            runServer(context, circuit, provingKeyFilename, config, std::stoi(argv[3]));
        #endif
    }

    if (mode == Mode::Validate || mode == Mode::Prove)
    {
        if (!generateWitness(circuit, input))
        {
            return 1;
        }
    }

    if (mode == Mode::Validate || mode == Mode::Prove)
    {
        if (!validateCircuit(circuit))
        {
            return 1;
        }
    }

    if (mode == Mode::CreateKeys)
    {
        if (!generateKeyPair(pb, baseFilename))
        {
            std::cerr << "Failed to generate keys!" << std::endl;
            return 1;
        }
    }

    if (mode == Mode::Prove)
    {
#ifdef GPU_PROVE
        std::cout << "GPU Prove: Generate inputsFile." << std::endl;
        std::string inputsFilename = baseFilename + "_inputs.raw";
        auto begin = now();
        stub_write_input_from_pb(pb, provingKeyFilename.c_str(), inputsFilename.c_str());
        print_time(begin, "write input");
#else
        ProverContextT context;
        loadProvingKey(provingKeyFilename, context.provingKey);
        context.constraint_system = &pb.constraint_system;
        context.config = config;
        context.domain = get_domain(pb, context.provingKey, config);
        initProverContextBuffers(context);
        printMemoryUsage();
        std::string jProof = proveCircuit(context, circuit);
        if (jProof.length() == 0)
        {
            return 1;
        }
        if (!writeProof(jProof, proofFilename))
        {
            return 1;
        }

        VerificationKeyT vk = loadVerificationKey(provingKeyFilename.substr(0, provingKeyFilename.length() - 6) + "vk.json");
        std::stringstream proof_stream;
        proof_stream << jProof;
        auto proof_pair = proof_from_json(proof_stream);

        std::cout << "proof:" << jProof;
        bool verified = libsnark::r1cs_gg_ppzksnark_zok_verifier_strong_IC<ppT>(vk, proof_pair.first, proof_pair.second);
        std::cout << "verified:" << verified << std::endl;
#endif
    }

    if (mode == Mode::ExportCircuit)
    {
        if (!r1cs2json(pb, argv[3]))
        {
            std::cerr << "Failed to export circuit!" << std::endl;
            return 1;
        }
    }

    if (mode == Mode::ExportWitness)
    {
        if (!witness2json(pb, argv[3]))
        {
            std::cerr << "Failed to export witness!" << std::endl;
            return 1;
        }
    }

    pthread_exit(NULL);
    return 0;
}
