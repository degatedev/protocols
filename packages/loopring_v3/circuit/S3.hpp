#pragma once

#include <iostream>
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <fstream>

class S3Service{
	Aws::SDKOptions options;
	Aws::String bucket;
	Aws::String region;
	Aws::String access_key_id;
	Aws::String secret_access_key;
	std::string root_dir;
public:
	S3Service(nlohmann::json workerConf)
	{
		Aws::InitAPI(options);
		bucket = workerConf["S3_CONF"]["bucket"].get<Aws::String>();
		region = workerConf["S3_CONF"]["region_name"].get<Aws::String>();
		access_key_id = workerConf["S3_CONF"]["access_key_id"].get<Aws::String>();
		secret_access_key = workerConf["S3_CONF"]["secret_access_key"].get<Aws::String>();
		root_dir = workerConf["S3_CONF"]["root_dir"].get<std::string>();
	}
	~S3Service() { Aws::ShutdownAPI(options); }

	bool GetObjectToFile(const std::string objectKey, const std::string localFilePath)
	{
	    Aws::Client::ClientConfiguration config;

	    if (!region.empty())
	    {
	        config.region = region;
	    }

	    Aws::Auth::AWSCredentials cred(access_key_id, secret_access_key);
	    Aws::S3::S3Client s3_client(cred, config);

	    Aws::S3::Model::GetObjectRequest object_request;
	    object_request.SetBucket(bucket);
	    object_request.SetKey(Aws::String(root_dir + objectKey));

	    Aws::S3::Model::GetObjectOutcome get_object_outcome =
	        s3_client.GetObject(object_request);

	    if (get_object_outcome.IsSuccess())
	    {
	        auto& retrieved_file = get_object_outcome.GetResultWithOwnership().GetBody();
	        std::ofstream f(localFilePath);
	        f << retrieved_file.rdbuf();
	        f.close();
	        return true;
	    }
	    else
	    {
	        auto err = get_object_outcome.GetError();
	        std::cout << "Error: GetObject: " << err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
	        return false;
	    }
	}
};
