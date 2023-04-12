// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
#ifndef _UNIVERSALCIRCUIT_H_
#define _UNIVERSALCIRCUIT_H_

#include "Circuit.h"
#include "../Utils/Constants.h"
#include "../Utils/Data.h"
#include "../Utils/Utils.h"
#include "../Gadgets/MatchingGadgets.h"
#include "../Gadgets/AccountGadgets.h"
#include "../Gadgets/StorageGadgets.h"
#include "../Gadgets/MathGadgets.h"
#include "./BaseTransactionCircuit.h"
#include "./DepositCircuit.h"
#include "./TransferCircuit.h"
#include "./SpotTradeCircuit.h"
#include "./AccountUpdateCircuit.h"
#include "./AppKeyUpdateCircuit.h"
#include "./WithdrawCircuit.h"
#include "./NoopCircuit.h"
#include "./OrderCancelCircuit.h"
#include "./BatchSpotTradeCircuit.h"
// #include "./BatchSpotTradeCircuitTwo.h"
// #include "./BatchSpotTradeCircuitThree.h"
// #include "./BatchSpotTradeCircuitFour.h"
// #include "./BatchSpotTradeCircuitFive.h"
// #include "./BatchSpotTradeCircuitSix.h"
// #include "./BatchSpotTradeCircuitSeven.h"
// #include "./BatchSpotTradeCircuitEight.h"
// #include "./BatchSpotTradeCircuitNine.h"
// #include "./BatchSpotTradeCircuitTen.h"
// DEG-265 auto market
// #include "./AutoMarketUpdateCircuit.h"

#include "ethsnarks.hpp"
#include "utils.hpp"
#include "gadgets/subadd.hpp"

using namespace ethsnarks;

// Naming conventions:
// - O: operator
// - P: prrotocol fee
// - S：sell (send)
// - B: buy (receive)
// - A/B: tokens/accounts that are not equal

namespace Loopring
{

class SelectTransactionGadget : public BaseTransactionCircuit
{
  public:
    std::vector<SelectGadget> uSelects;
    // std::vector<ArraySelectGadget> uMapSelects;
    std::vector<ArraySelectGadget> aSelects;
    // std::vector<VectorArraySelectGadget> aMapSelects;
    std::vector<ArraySelectGadget> publicDataSelects;
    
    // std::map<TxVariable, std::vector<VariableT>> uOutputsMap;
    // std::map<TxVariable, std::vector<VariableArrayT>> aOutputsMap;
    SelectTransactionGadget(
      ProtoboardT &pb,
      const TransactionState &state,
      const VariableArrayT &selector,
      const std::vector<BaseTransactionCircuit *> &transactions,
      const std::string &prefix)
        : BaseTransactionCircuit(pb, state, prefix)
    {
        assert(selector.size() == transactions.size());

        // Unsigned outputs
        std::cout << "in SelectTransactionGadget before uSelects" << std::endl;
        uSelects.reserve(uOutputs.size());
        for (const auto &uPair : uOutputs)
        {
            std::vector<VariableT> variables;
            for (unsigned int i = 0; i < transactions.size(); i++)
            {
                variables.push_back(transactions[i]->getOutput(uPair.first));
            }
            uSelects.emplace_back(pb, state.constants, selector, variables, FMT(annotation_prefix, ".uSelects"));

            // Set the output variable
            setOutput(uPair.first, uSelects.back().result());
        }

        // // TODO need check the logic, copy all array or ergodic every item
        // // Unsigned map outputs
        // uMapSelects.reserve(uOutputsMap.size());
        // for (const auto &uMapPair : uOutputsMap)
        // {
        //     std::vector<VariableArrayT> variables;
        //     for (unsigned int i = 0; i < transactions.size(); i++)
        //     {
        //         variables.push_back(transactions[i]->getOutputArray(uMapPair.first));
        //     }
        //     uMapSelects.emplace_back(pb, state.constants, selector, variables, FMT(annotation_prefix, ".uMapSelects"));

        //     // Set the output variable
        //     setOutputArrayAll(uMapPair.first, uMapSelects.back().result());
        // }

        // Array outputs
        std::cout << "in SelectTransactionGadget before aSelects" << std::endl;
        aSelects.reserve(aOutputs.size());
        for (const auto &aPair : aOutputs)
        {
            std::vector<VariableArrayT> variables;
            for (unsigned int i = 0; i < transactions.size(); i++)
            {
                variables.push_back(transactions[i]->getArrayOutput(aPair.first));
            }
            aSelects.emplace_back(pb, state.constants, selector, variables, FMT(annotation_prefix, ".aSelects"));

            // Set the output variable
            setArrayOutput(aPair.first, aSelects.back().result());
        }

        // // TODO need check the logic, copy all array or ergodic every item
        // // Array map outputs
        // aMapSelects.reserve(aOutputsMap.size());
        // for (const auto &aMapPair : aOutputsMap)
        // {
        //     std::vector<std::vector<VariableArrayT>> variables;
        //     for (unsigned int i = 0; i < transactions.size(); i++)
        //     {
        //         variables.push_back(transactions[i]->getArrayOutputArray(aMapPair.first));
        //     }
        //     aMapSelects.emplace_back(pb, state.constants, selector, variables, FMT(annotation_prefix, ".aMapSelects"));

        //     // Set the output variable
        //     setArrayOutputArrayAll(aMapPair.first, aMapSelects.back().result());
        // }

        // Public data
        std::cout << "in SelectTransactionGadget before Public data" << std::endl;
        {
            std::vector<VariableArrayT> variables;
            for (unsigned int i = 0; i < transactions.size(); i++)
            {
                VariableArrayT da = transactions[i]->getPublicData();
                // assert(da.size() <= (TX_DATA_AVAILABILITY_SIZE - 1) * 8);
                assert(da.size() <= (TX_DATA_AVAILABILITY_SIZE) * 8);
                // Pad with zeros if needed
                // for (unsigned int j = da.size(); j < (TX_DATA_AVAILABILITY_SIZE - 1) * 8; j++)
                for (unsigned int j = da.size(); j < (TX_DATA_AVAILABILITY_SIZE) * 8; j++)
                {
                    da.emplace_back(state.constants._0);
                }
                variables.push_back(da);
                // std::cout << "da size: " << variables.back().size() << std::endl;
            }
            publicDataSelects.emplace_back(
              pb, state.constants, selector, variables, FMT(annotation_prefix, ".publicDataSelects"));
        }
    }

    void generate_r1cs_witness()
    {
        for (unsigned int i = 0; i < uSelects.size(); i++)
        {
            uSelects[i].generate_r1cs_witness();
        }
        for (unsigned int i = 0; i < aSelects.size(); i++)
        {
            aSelects[i].generate_r1cs_witness();
        }
        for (unsigned int i = 0; i < publicDataSelects.size(); i++)
        {
            publicDataSelects[i].generate_r1cs_witness();
        }
    }

    void generate_r1cs_constraints()
    {
        for (unsigned int i = 0; i < uSelects.size(); i++)
        {
            uSelects[i].generate_r1cs_constraints();
        }
        for (unsigned int i = 0; i < aSelects.size(); i++)
        {
            aSelects[i].generate_r1cs_constraints();
        }
        for (unsigned int i = 0; i < publicDataSelects.size(); i++)
        {
            publicDataSelects[i].generate_r1cs_constraints();
        }
    }

    const VariableArrayT getPublicData() const
    {
        printBits("[ZKS]spottrade publicData:", publicDataSelects.back().result().get_bits(pb), false);
        return publicDataSelects.back().result();
    }
};

// class BatchStorageUpdateGadget : public GadgetT 
// {
//   public:
//     SelectTransactionGadget tx;
//     TransactionAccountState account;
//     VariableT storageRoot;
//     std::vector<UpdateStorageGadget> updateStorages;
    
//     BatchStorageUpdateGadget(
//       ProtoboardT &pb,
//       const SelectTransactionGadget &_tx,
//       const TransactionAccountState &_account,
//       const VariableT &_storageRoot,
//       const TxVariable &tvAddress, 
//       const TxVariable &tvTokenSID, 
//       const TxVariable &tvTokenBID, 
//       const TxVariable &tvData, 
//       const TxVariable &tvStorageID, 
//       const TxVariable &tvGasFee,
//       const TxVariable &tvCancelled, 
//       const TxVariable &tvForward, 
//       const std::string &prefix)
//         : GadgetT(pb, prefix),
//         tx(_tx),
//         account(_account),
//         storageRoot(_storageRoot)
//     {
//       std::cout << "in BatchStorageUpdateGadget" << std::endl;
//       for (size_t i = 0; i < account.storageArray.size(); i++) 
//       {
//         updateStorages.emplace_back(UpdateStorageGadget(
//           pb,
//           (i == 0) ? storageRoot : updateStorages.back().result(),
//           tx.getArrayOutputFromArray(tvAddress, i),
//           {account.storageArray[i].tokenSID, 
//           account.storageArray[i].tokenBID, 
//           account.storageArray[i].data, 
//           account.storageArray[i].storageID, 
//           account.storageArray[i].gasFee, 
//           account.storageArray[i].cancelled, 
//           account.storageArray[i].forward},

//           {tx.getOutputFromArray(tvTokenSID, i), 
//           tx.getOutputFromArray(tvTokenBID, i), 
//           tx.getOutputFromArray(tvData, i), 
//           tx.getOutputFromArray(tvStorageID, i), 
//           tx.getOutputFromArray(tvGasFee, i), 
//           tx.getOutputFromArray(tvCancelled, i), 
//           tx.getOutputFromArray(tvForward, i)},
//           std::string("BatchStorageUpdateGadget.updateStorages_") + std::to_string(i)
//         ));
//       }
//     }
//     // void generate_r1cs_witness(ProtoboardT &pb, TxVariable tvAddress, TxVariable tvTokenSID, TxVariable tvTokenBID, 
//     //   TxVariable tvData, TxVariable tvStorageID, TxVariable tvGasFee, TxVariable tvCancelled, TxVariable tvForward, 
//     //   std::vector<StorageUpdate> storageUpdateArray) 
//     void generate_r1cs_witness(std::vector<StorageUpdate> storageUpdateArray) 
//     {
//       std::cout << "in BatchStorageUpdateGadget generate_r1cs_witness" << std::endl;
//       std::cout << "in BatchStorageUpdateGadget generate_r1cs_witness: " << account.storageArray.size() << std::endl;
//       for (size_t i = 0; i < account.storageArray.size(); i++) 
//       {
//         updateStorages[i].generate_r1cs_witness(storageUpdateArray[i]);
//       }
//     }
//     void generate_r1cs_constraints() 
//     {
//       std::cout << "in BatchStorageUpdateGadget generate_r1cs_constraints" << std::endl;
//       for (size_t i = 0; i < updateStorages.size(); i++) 
//       {
//         updateStorages[i].generate_r1cs_constraints();
//       }
//       std::cout << "in BatchStorageUpdateGadget generate_r1cs_constraints end" << std::endl;
//     }
//     const VariableT getHashRoot() const
//     {
//         std::cout << "in BatchStorageUpdateGadget getHashRoot updateStorages size:" << updateStorages.size() << std::endl;
//         return updateStorages.back().result();
//     }
// };

class BatchStorageAUpdateGadget : public GadgetT 
{
  public:
    SelectTransactionGadget tx;
    TransactionAccountState account;
    VariableT storageRoot;
    std::vector<UpdateStorageGadget> updateStorages;
    
    BatchStorageAUpdateGadget(
      ProtoboardT &pb,
      const SelectTransactionGadget &_tx,
      const TransactionAccountState &_account,
      const VariableT &_storageRoot,
      const std::string &prefix)
        : GadgetT(pb, prefix),
        tx(_tx),
        account(_account),
        storageRoot(_storageRoot)
    {
      std::cout << "in BatchStorageUpdateGadget" << std::endl;
      updateStorages.emplace_back(UpdateStorageGadget(
          pb,
          storageRoot,
          tx.getArrayOutput(TXV_STORAGE_A_ADDRESS_ARRAY_0),
          {account.storageArray[0].tokenSID, 
          account.storageArray[0].tokenBID, 
          account.storageArray[0].data, 
          account.storageArray[0].storageID, 
          account.storageArray[0].gasFee, 
          account.storageArray[0].cancelled, 
          account.storageArray[0].forward},

          {tx.getOutput(TXV_STORAGE_A_TOKENSID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_A_TOKENBID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_A_DATA_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_A_STORAGEID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_A_GASFEE_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_A_CANCELLED_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_A_FORWARD_ARRAY_0)},
          std::string("BatchStorageUpdateGadget.updateStorages_0")
        ));
      updateStorages.emplace_back(UpdateStorageGadget(
          pb,
          updateStorages.back().result(),
          tx.getArrayOutput(TXV_STORAGE_A_ADDRESS_ARRAY_1),
          {account.storageArray[1].tokenSID, 
          account.storageArray[1].tokenBID, 
          account.storageArray[1].data, 
          account.storageArray[1].storageID, 
          account.storageArray[1].gasFee, 
          account.storageArray[1].cancelled, 
          account.storageArray[1].forward},

          {tx.getOutput(TXV_STORAGE_A_TOKENSID_ARRAY_1), 
          tx.getOutput(TXV_STORAGE_A_TOKENBID_ARRAY_1), 
          tx.getOutput(TXV_STORAGE_A_DATA_ARRAY_1), 
          tx.getOutput(TXV_STORAGE_A_STORAGEID_ARRAY_1), 
          tx.getOutput(TXV_STORAGE_A_GASFEE_ARRAY_1), 
          tx.getOutput(TXV_STORAGE_A_CANCELLED_ARRAY_1), 
          tx.getOutput(TXV_STORAGE_A_FORWARD_ARRAY_1)},
          std::string("BatchStorageUpdateGadget.updateStorages_1")
        ));
      updateStorages.emplace_back(UpdateStorageGadget(
          pb,
          updateStorages.back().result(),
          tx.getArrayOutput(TXV_STORAGE_A_ADDRESS_ARRAY_2),
          {account.storageArray[2].tokenSID, 
          account.storageArray[2].tokenBID, 
          account.storageArray[2].data, 
          account.storageArray[2].storageID, 
          account.storageArray[2].gasFee, 
          account.storageArray[2].cancelled, 
          account.storageArray[2].forward},

          {tx.getOutput(TXV_STORAGE_A_TOKENSID_ARRAY_2), 
          tx.getOutput(TXV_STORAGE_A_TOKENBID_ARRAY_2), 
          tx.getOutput(TXV_STORAGE_A_DATA_ARRAY_2), 
          tx.getOutput(TXV_STORAGE_A_STORAGEID_ARRAY_2), 
          tx.getOutput(TXV_STORAGE_A_GASFEE_ARRAY_2), 
          tx.getOutput(TXV_STORAGE_A_CANCELLED_ARRAY_2), 
          tx.getOutput(TXV_STORAGE_A_FORWARD_ARRAY_2)},
          std::string("BatchStorageUpdateGadget.updateStorages_2")
        ));
    }
    void generate_r1cs_witness(std::vector<StorageUpdate> storageUpdateArray) 
    {
      std::cout << "in BatchStorageUpdateGadget generate_r1cs_witness" << std::endl;
      std::cout << "in BatchStorageUpdateGadget generate_r1cs_witness: " << account.storageArray.size() << std::endl;

      std::cout << "in BatchStorageAUpdateGadget generate_r1cs_witness TXV_STORAGE_A_STORAGEID_ARRAY 0:" << pb.val(tx.getOutput(TXV_STORAGE_A_STORAGEID_ARRAY_0)) << std::endl;
      std::cout << "in BatchStorageAUpdateGadget generate_r1cs_witness TXV_STORAGE_A_STORAGEID_ARRAY 1:" << pb.val(tx.getOutput(TXV_STORAGE_A_STORAGEID_ARRAY_1)) << std::endl;
      std::cout << "in BatchStorageAUpdateGadget generate_r1cs_witness TXV_STORAGE_A_STORAGEID_ARRAY 2:" << pb.val(tx.getOutput(TXV_STORAGE_A_STORAGEID_ARRAY_2)) << std::endl;
      
      std::cout << "in BatchStorageAUpdateGadget generate_r1cs_witness TXV_STORAGE_A_TOKENSID_ARRAY 0:" << pb.val(tx.getOutput(TXV_STORAGE_A_TOKENSID_ARRAY_0)) << std::endl;
      std::cout << "in BatchStorageAUpdateGadget generate_r1cs_witness TXV_STORAGE_A_TOKENBID_ARRAY 0:" << pb.val(tx.getOutput(TXV_STORAGE_A_TOKENBID_ARRAY_0)) << std::endl;

      std::cout << "in BatchStorageAUpdateGadget generate_r1cs_witness TXV_STORAGE_A_TOKENSID_ARRAY 1:" << pb.val(tx.getOutput(TXV_STORAGE_A_TOKENSID_ARRAY_1)) << std::endl;
      std::cout << "in BatchStorageAUpdateGadget generate_r1cs_witness TXV_STORAGE_A_TOKENBID_ARRAY 1:" << pb.val(tx.getOutput(TXV_STORAGE_A_TOKENBID_ARRAY_1)) << std::endl;

      std::cout << "in BatchStorageAUpdateGadget generate_r1cs_witness TXV_STORAGE_A_DATA_ARRAY 0:" << pb.val(tx.getOutput(TXV_STORAGE_A_DATA_ARRAY_0)) << std::endl;
      std::cout << "in BatchStorageAUpdateGadget generate_r1cs_witness TXV_STORAGE_A_DATA_ARRAY 1:" << pb.val(tx.getOutput(TXV_STORAGE_A_DATA_ARRAY_1)) << std::endl;

      for (size_t i = 0; i < account.storageArray.size(); i++) 
      {
        updateStorages[i].generate_r1cs_witness(storageUpdateArray[i]);
      }
    }
    void generate_r1cs_constraints() 
    {
      std::cout << "in BatchStorageUpdateGadget generate_r1cs_constraints" << std::endl;
      for (size_t i = 0; i < updateStorages.size(); i++) 
      {
        std::cout << "BatchStorageUpdateGadget before updateStorages:" << pb.num_constraints() << std::endl;
        updateStorages[i].generate_r1cs_constraints();
        std::cout << "BatchStorageUpdateGadget after updateStorages:" << pb.num_constraints() << std::endl;
      }
      std::cout << "in BatchStorageUpdateGadget generate_r1cs_constraints end" << std::endl;
    }
    const VariableT getHashRoot() const
    {
        std::cout << "in BatchStorageUpdateGadget getHashRoot updateStorages size:" << updateStorages.size() << std::endl;
        return updateStorages.back().result();
    }
};

class BatchStorageBUpdateGadget : public GadgetT 
{
  public:
    SelectTransactionGadget tx;
    TransactionAccountState account;
    VariableT storageRoot;
    std::vector<UpdateStorageGadget> updateStorages;
    
    BatchStorageBUpdateGadget(
      ProtoboardT &pb,
      const SelectTransactionGadget &_tx,
      const TransactionAccountState &_account,
      const VariableT &_storageRoot,
      const std::string &prefix)
        : GadgetT(pb, prefix),
        tx(_tx),
        account(_account),
        storageRoot(_storageRoot)
    {
      std::cout << "in BatchStorageBUpdateGadget" << std::endl;
      updateStorages.emplace_back(UpdateStorageGadget(
          pb,
          storageRoot,
          tx.getArrayOutput(TXV_STORAGE_B_ADDRESS_ARRAY_0),
          {account.storageArray[0].tokenSID, 
          account.storageArray[0].tokenBID, 
          account.storageArray[0].data, 
          account.storageArray[0].storageID, 
          account.storageArray[0].gasFee, 
          account.storageArray[0].cancelled, 
          account.storageArray[0].forward},

          {tx.getOutput(TXV_STORAGE_B_TOKENSID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_B_TOKENBID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_B_DATA_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_B_STORAGEID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_B_GASFEE_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_B_CANCELLED_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_B_FORWARD_ARRAY_0)},
          std::string("BatchStorageBUpdateGadget.updateStorages_0")
        ));
      // updateStorages.emplace_back(UpdateStorageGadget(
      //     pb,
      //     updateStorages.back().result(),
      //     tx.getArrayOutput(TXV_STORAGE_B_ADDRESS_ARRAY_1),
      //     {account.storageArray[1].tokenSID, 
      //     account.storageArray[1].tokenBID, 
      //     account.storageArray[1].data, 
      //     account.storageArray[1].storageID, 
      //     account.storageArray[1].gasFee, 
      //     account.storageArray[1].cancelled, 
      //     account.storageArray[1].forward},

      //     {tx.getOutput(TXV_STORAGE_B_TOKENSID_ARRAY_1), 
      //     tx.getOutput(TXV_STORAGE_B_TOKENBID_ARRAY_1), 
      //     tx.getOutput(TXV_STORAGE_B_DATA_ARRAY_1), 
      //     tx.getOutput(TXV_STORAGE_B_STORAGEID_ARRAY_1), 
      //     tx.getOutput(TXV_STORAGE_B_GASFEE_ARRAY_1), 
      //     tx.getOutput(TXV_STORAGE_B_CANCELLED_ARRAY_1), 
      //     tx.getOutput(TXV_STORAGE_B_FORWARD_ARRAY_1)},
      //     std::string("BatchStorageBUpdateGadget.updateStorages_1")
      //   ));
    }
    void generate_r1cs_witness(std::vector<StorageUpdate> storageUpdateArray) 
    {
      std::cout << "in BatchStorageBUpdateGadget generate_r1cs_witness" << std::endl;
      std::cout << "in BatchStorageBUpdateGadget generate_r1cs_witness: " << account.storageArray.size() << std::endl;
      for (size_t i = 0; i < account.storageArray.size(); i++) 
      {
        updateStorages[i].generate_r1cs_witness(storageUpdateArray[i]);
      }
    }
    void generate_r1cs_constraints() 
    {
      std::cout << "in BatchStorageBUpdateGadget generate_r1cs_constraints" << std::endl;
      for (size_t i = 0; i < updateStorages.size(); i++) 
      {
        updateStorages[i].generate_r1cs_constraints();
      }
      std::cout << "in BatchStorageBUpdateGadget generate_r1cs_constraints end" << std::endl;
    }
    const VariableT getHashRoot() const
    {
        std::cout << "in BatchStorageBUpdateGadget getHashRoot updateStorages size:" << updateStorages.size() << std::endl;
        return updateStorages.back().result();
    }
};

class BatchStorageCUpdateGadget : public GadgetT 
{
  public:
    SelectTransactionGadget tx;
    TransactionAccountState account;
    VariableT storageRoot;
    std::vector<UpdateStorageGadget> updateStorages;
    
    BatchStorageCUpdateGadget(
      ProtoboardT &pb,
      const SelectTransactionGadget &_tx,
      const TransactionAccountState &_account,
      const VariableT &_storageRoot,
      const std::string &prefix)
        : GadgetT(pb, prefix),
        tx(_tx),
        account(_account),
        storageRoot(_storageRoot)
    {
      std::cout << "in BatchStorageCUpdateGadget" << std::endl;
      updateStorages.emplace_back(UpdateStorageGadget(
          pb,
          storageRoot,
          tx.getArrayOutput(TXV_STORAGE_C_ADDRESS_ARRAY_0),
          {account.storageArray[0].tokenSID, 
          account.storageArray[0].tokenBID, 
          account.storageArray[0].data, 
          account.storageArray[0].storageID, 
          account.storageArray[0].gasFee, 
          account.storageArray[0].cancelled, 
          account.storageArray[0].forward},

          {tx.getOutput(TXV_STORAGE_C_TOKENSID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_C_TOKENBID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_C_DATA_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_C_STORAGEID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_C_GASFEE_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_C_CANCELLED_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_C_FORWARD_ARRAY_0)},
          std::string("BatchStorageCUpdateGadget.updateStorages_0")
        ));
    }
    void generate_r1cs_witness(std::vector<StorageUpdate> storageUpdateArray) 
    {
      std::cout << "in BatchStorageCUpdateGadget generate_r1cs_witness" << std::endl;
      std::cout << "in BatchStorageCUpdateGadget generate_r1cs_witness: " << account.storageArray.size() << std::endl;
      for (size_t i = 0; i < account.storageArray.size(); i++) 
      {
        updateStorages[i].generate_r1cs_witness(storageUpdateArray[i]);
      }
    }
    void generate_r1cs_constraints() 
    {
      std::cout << "in BatchStorageCUpdateGadget generate_r1cs_constraints" << std::endl;
      for (size_t i = 0; i < updateStorages.size(); i++) 
      {
        updateStorages[i].generate_r1cs_constraints();
      }
      std::cout << "in BatchStorageCUpdateGadget generate_r1cs_constraints end" << std::endl;
    }
    const VariableT getHashRoot() const
    {
        std::cout << "in BatchStorageCUpdateGadget getHashRoot updateStorages size:" << updateStorages.size() << std::endl;
        return updateStorages.back().result();
    }
};

class BatchStorageDUpdateGadget : public GadgetT 
{
  public:
    SelectTransactionGadget tx;
    TransactionAccountState account;
    VariableT storageRoot;
    std::vector<UpdateStorageGadget> updateStorages;
    
    BatchStorageDUpdateGadget(
      ProtoboardT &pb,
      const SelectTransactionGadget &_tx,
      const TransactionAccountState &_account,
      const VariableT &_storageRoot,
      const std::string &prefix)
        : GadgetT(pb, prefix),
        tx(_tx),
        account(_account),
        storageRoot(_storageRoot)
    {
      std::cout << "in BatchStorageDUpdateGadget" << std::endl;
      updateStorages.emplace_back(UpdateStorageGadget(
          pb,
          storageRoot,
          tx.getArrayOutput(TXV_STORAGE_D_ADDRESS_ARRAY_0),
          {account.storageArray[0].tokenSID, 
          account.storageArray[0].tokenBID, 
          account.storageArray[0].data, 
          account.storageArray[0].storageID, 
          account.storageArray[0].gasFee, 
          account.storageArray[0].cancelled, 
          account.storageArray[0].forward},

          {tx.getOutput(TXV_STORAGE_D_TOKENSID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_D_TOKENBID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_D_DATA_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_D_STORAGEID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_D_GASFEE_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_D_CANCELLED_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_D_FORWARD_ARRAY_0)},
          std::string("BatchStorageDUpdateGadget.updateStorages_0")
        ));
    }
    void generate_r1cs_witness(std::vector<StorageUpdate> storageUpdateArray) 
    {
      std::cout << "in BatchStorageDUpdateGadget generate_r1cs_witness" << std::endl;
      std::cout << "in BatchStorageDUpdateGadget generate_r1cs_witness: " << account.storageArray.size() << std::endl;
      for (size_t i = 0; i < account.storageArray.size(); i++) 
      {
        updateStorages[i].generate_r1cs_witness(storageUpdateArray[i]);
      }
    }
    void generate_r1cs_constraints() 
    {
      std::cout << "in BatchStorageDUpdateGadget generate_r1cs_constraints" << std::endl;
      for (size_t i = 0; i < updateStorages.size(); i++) 
      {
        updateStorages[i].generate_r1cs_constraints();
      }
      std::cout << "in BatchStorageDUpdateGadget generate_r1cs_constraints end" << std::endl;
    }
    const VariableT getHashRoot() const
    {
        std::cout << "in BatchStorageDUpdateGadget getHashRoot updateStorages size:" << updateStorages.size() << std::endl;
        return updateStorages.back().result();
    }
};

class BatchStorageEUpdateGadget : public GadgetT 
{
  public:
    SelectTransactionGadget tx;
    TransactionAccountState account;
    VariableT storageRoot;
    std::vector<UpdateStorageGadget> updateStorages;
    
    BatchStorageEUpdateGadget(
      ProtoboardT &pb,
      const SelectTransactionGadget &_tx,
      const TransactionAccountState &_account,
      const VariableT &_storageRoot,
      const std::string &prefix)
        : GadgetT(pb, prefix),
        tx(_tx),
        account(_account),
        storageRoot(_storageRoot)
    {
      std::cout << "in BatchStorageEUpdateGadget" << std::endl;
      updateStorages.emplace_back(UpdateStorageGadget(
          pb,
          storageRoot,
          tx.getArrayOutput(TXV_STORAGE_E_ADDRESS_ARRAY_0),
          {account.storageArray[0].tokenSID, 
          account.storageArray[0].tokenBID, 
          account.storageArray[0].data, 
          account.storageArray[0].storageID, 
          account.storageArray[0].gasFee, 
          account.storageArray[0].cancelled, 
          account.storageArray[0].forward},

          {tx.getOutput(TXV_STORAGE_E_TOKENSID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_E_TOKENBID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_E_DATA_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_E_STORAGEID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_E_GASFEE_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_E_CANCELLED_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_E_FORWARD_ARRAY_0)},
          std::string("BatchStorageEUpdateGadget.updateStorages_0")
        ));
    }
    void generate_r1cs_witness(std::vector<StorageUpdate> storageUpdateArray) 
    {
      std::cout << "in BatchStorageEUpdateGadget generate_r1cs_witness" << std::endl;
      std::cout << "in BatchStorageEUpdateGadget generate_r1cs_witness: " << account.storageArray.size() << std::endl;
      for (size_t i = 0; i < account.storageArray.size(); i++) 
      {
        updateStorages[i].generate_r1cs_witness(storageUpdateArray[i]);
      }
    }
    void generate_r1cs_constraints() 
    {
      std::cout << "in BatchStorageEUpdateGadget generate_r1cs_constraints" << std::endl;
      for (size_t i = 0; i < updateStorages.size(); i++) 
      {
        updateStorages[i].generate_r1cs_constraints();
      }
      std::cout << "in BatchStorageEUpdateGadget generate_r1cs_constraints end" << std::endl;
    }
    const VariableT getHashRoot() const
    {
        std::cout << "in BatchStorageEUpdateGadget getHashRoot updateStorages size:" << updateStorages.size() << std::endl;
        return updateStorages.back().result();
    }
};

class BatchStorageFUpdateGadget : public GadgetT 
{
  public:
    SelectTransactionGadget tx;
    TransactionAccountState account;
    VariableT storageRoot;
    std::vector<UpdateStorageGadget> updateStorages;
    
    BatchStorageFUpdateGadget(
      ProtoboardT &pb,
      const SelectTransactionGadget &_tx,
      const TransactionAccountState &_account,
      const VariableT &_storageRoot,
      const std::string &prefix)
        : GadgetT(pb, prefix),
        tx(_tx),
        account(_account),
        storageRoot(_storageRoot)
    {
      std::cout << "in BatchStorageFUpdateGadget" << std::endl;
      updateStorages.emplace_back(UpdateStorageGadget(
          pb,
          storageRoot,
          tx.getArrayOutput(TXV_STORAGE_F_ADDRESS_ARRAY_0),
          {account.storageArray[0].tokenSID, 
          account.storageArray[0].tokenBID, 
          account.storageArray[0].data, 
          account.storageArray[0].storageID, 
          account.storageArray[0].gasFee, 
          account.storageArray[0].cancelled, 
          account.storageArray[0].forward},

          {tx.getOutput(TXV_STORAGE_F_TOKENSID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_F_TOKENBID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_F_DATA_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_F_STORAGEID_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_F_GASFEE_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_F_CANCELLED_ARRAY_0), 
          tx.getOutput(TXV_STORAGE_F_FORWARD_ARRAY_0)},
          std::string("BatchStorageFUpdateGadget.updateStorages_0")
        ));
    }
    void generate_r1cs_witness(std::vector<StorageUpdate> storageUpdateArray) 
    {
      std::cout << "in BatchStorageFUpdateGadget generate_r1cs_witness" << std::endl;
      std::cout << "in BatchStorageFUpdateGadget generate_r1cs_witness: " << account.storageArray.size() << std::endl;
      for (size_t i = 0; i < account.storageArray.size(); i++) 
      {
        updateStorages[i].generate_r1cs_witness(storageUpdateArray[i]);
      }
    }
    void generate_r1cs_constraints() 
    {
      std::cout << "in BatchStorageFUpdateGadget generate_r1cs_constraints" << std::endl;
      for (size_t i = 0; i < updateStorages.size(); i++) 
      {
        updateStorages[i].generate_r1cs_constraints();
      }
      std::cout << "in BatchStorageFUpdateGadget generate_r1cs_constraints end" << std::endl;
    }
    const VariableT getHashRoot() const
    {
        std::cout << "in BatchStorageFUpdateGadget getHashRoot updateStorages size:" << updateStorages.size() << std::endl;
        return updateStorages.back().result();
    }
};

class TransactionGadget : public GadgetT
{
  public:
    const Constants &constants;

    DualVariableGadget type;
    SelectorGadget selector;

    TransactionState state;

    // Process transaction
    NoopCircuit noop;
    SpotTradeCircuit spotTrade;
    DepositCircuit deposit;
    WithdrawCircuit withdraw;
    AccountUpdateCircuit accountUpdate;
    TransferCircuit transfer;
    // DEG-146:order cancel
    OrderCancelCircuit orderCancel;
    AppKeyUpdateCircuit appKeyUpdate;

    BatchSpotTradeCircuit batchSpotTrade;
    // // DEG-170 auto market - config add
    // AutoMarketUpdateCircuit autoMakretUpdate;
    SelectTransactionGadget tx;

    // General validation
    // FromBitsGadget accountA;
    // FromBitsGadget accountB;
    // RequireNotZeroGadget validateAccountA;
    // RequireNotZeroGadget validateAccountB;

    // Check signatures
    SignatureVerifier signatureVerifierA;
    SignatureVerifier signatureVerifierB;
    BatchSignatureVerifier batchSignatureVerifierA;
    BatchSignatureVerifier batchSignatureVerifierB;
    BatchSignatureVerifier batchSignatureVerifierC;
    BatchSignatureVerifier batchSignatureVerifierD;
    BatchSignatureVerifier batchSignatureVerifierE;
    BatchSignatureVerifier batchSignatureVerifierF;

    // Update UserA
    UpdateStorageGadget updateStorage_A;
    // BatchStorageUpdateGadget updateStorage_A_batch;
    BatchStorageAUpdateGadget updateStorage_A_batch;
    UpdateBalanceGadget updateBalanceS_A;
    UpdateBalanceGadget updateBalanceB_A;
    UpdateBalanceGadget updateBalanceFee_A;
    // DEG-265 auto market
    // // DEG-170 auto market - storage add
    // UpdateAutoMarketStorageGadget updateAutoMarketStorage_A;
    // // DEG-170 auto market - config add
    // UpdateAutoMarketGadget updateAutoMarket_A;
    UpdateAccountGadget updateAccount_A;

    // Update UserB
    UpdateStorageGadget updateStorage_B;
    // BatchStorageUpdateGadget updateStorage_B_batch;
    BatchStorageBUpdateGadget updateStorage_B_batch;
    UpdateBalanceGadget updateBalanceS_B;
    UpdateBalanceGadget updateBalanceB_B;
    // split trading fee and gas fee
    UpdateBalanceGadget updateBalanceFee_B;
    // DEG-265 auto market
    // // DEG-170 auto market - storage add
    // UpdateAutoMarketStorageGadget updateAutoMarketStorage_B;
    // // DEG-170 auto market - config add
    // UpdateAutoMarketGadget updateAutoMarket_B;
    UpdateAccountGadget updateAccount_B;

    //------UserC
    // BatchStorageUpdateGadget updateStorage_C_batch;
    BatchStorageCUpdateGadget updateStorage_C_batch;
    UpdateBalanceGadget updateBalanceS_C;
    UpdateBalanceGadget updateBalanceB_C;
    UpdateBalanceGadget updateBalanceFee_C;
    UpdateAccountGadget updateAccount_C;
    //------UserD
    // BatchStorageUpdateGadget updateStorage_D_batch;
    BatchStorageDUpdateGadget updateStorage_D_batch;
    UpdateBalanceGadget updateBalanceS_D;
    UpdateBalanceGadget updateBalanceB_D;
    UpdateBalanceGadget updateBalanceFee_D;
    UpdateAccountGadget updateAccount_D;

    //------UserE
    BatchStorageEUpdateGadget updateStorage_E_batch;
    UpdateBalanceGadget updateBalanceS_E;
    UpdateBalanceGadget updateBalanceB_E;
    UpdateBalanceGadget updateBalanceFee_E;
    UpdateAccountGadget updateAccount_E;

    //------UserF
    BatchStorageFUpdateGadget updateStorage_F_batch;
    UpdateBalanceGadget updateBalanceS_F;
    UpdateBalanceGadget updateBalanceB_F;
    UpdateBalanceGadget updateBalanceFee_F;
    UpdateAccountGadget updateAccount_F;

    // Update Operator
    UpdateBalanceGadget updateBalanceD_O;
    UpdateBalanceGadget updateBalanceC_O;
    UpdateBalanceGadget updateBalanceB_O;
    UpdateBalanceGadget updateBalanceA_O;
    UpdateAccountGadget updateAccount_O;

    // Update Protocol pool
    // UpdateBalanceGadget updateBalanceC_P;
    // UpdateBalanceGadget updateBalanceB_P;
    // UpdateBalanceGadget updateBalanceA_P;

    TransactionGadget(
      ProtoboardT &pb,
      const jubjub::Params &params,
      const Constants &_constants,
      const VariableT &exchange,
      const VariableT &accountsRoot,
      const VariableT &accountsAssetRoot,
      const VariableT &timestamp,
      const VariableT &protocolFeeBips,
      const VariableArrayT &operatorAccountID,
      const VariableT &numConditionalTransactionsBefore,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          constants(_constants),

          type(pb, NUM_BITS_TX_TYPE_FOR_SELECT, FMT(prefix, ".type")),
          selector(pb, constants, type.packed, (unsigned int)TransactionType::COUNT, FMT(prefix, ".selector")),

          state(
            pb,
            params,
            constants,
            exchange,
            timestamp,
            protocolFeeBips,
            numConditionalTransactionsBefore,
            type.packed,
            FMT(prefix, ".transactionState")),

          // Process transaction
          noop(pb, state, FMT(prefix, ".noop")),
          spotTrade(pb, state, FMT(prefix, ".spotTrade")),
          deposit(pb, state, FMT(prefix, ".deposit")),
          withdraw(pb, state, FMT(prefix, ".withdraw")),
          accountUpdate(pb, state, FMT(prefix, ".accountUpdate")),
          transfer(pb, state, FMT(prefix, ".transfer")),
          // DEG-146:order cancel
          orderCancel(pb, state, FMT(prefix, ".orderCancel")),
          appKeyUpdate(pb, state, FMT(prefix, ".appKeyUpdate")),
          batchSpotTrade(pb, state, FMT(prefix, ".batchSpotTrade")),

          // DEG-265 auto market
          // // DEG-170 auto market - config add
          // autoMakretUpdate(pb, state, FMT(prefix, ".autoMakretUpdate")),
          tx(
            pb,
            state,
            selector.result(),
            {&noop, &transfer, &spotTrade, &orderCancel, &appKeyUpdate, &batchSpotTrade, 
            &deposit, &accountUpdate, &withdraw},
            FMT(prefix, ".tx")),

          // // General validation
          // accountA(pb, tx.getArrayOutput(TXV_ACCOUNT_A_ADDRESS), FMT(prefix, ".packAccountA")),
          // accountB(pb, tx.getArrayOutput(TXV_ACCOUNT_B_ADDRESS), FMT(prefix, ".packAccountA")),
          // validateAccountA(pb, accountA.packed, FMT(prefix, ".validateAccountA")),
          // validateAccountB(pb, accountB.packed, FMT(prefix, ".validateAccountB")),

          // Check signatures
          signatureVerifierA(
            pb,
            params,
            state.constants,
            jubjub::VariablePointT(tx.getOutput(TXV_PUBKEY_X_A), tx.getOutput(TXV_PUBKEY_Y_A)),
            tx.getOutput(TXV_HASH_A),
            tx.getOutput(TXV_SIGNATURE_REQUIRED_A),
            FMT(prefix, ".signatureVerifierA")),
          signatureVerifierB(
            pb,
            params,
            state.constants,
            jubjub::VariablePointT(tx.getOutput(TXV_PUBKEY_X_B), tx.getOutput(TXV_PUBKEY_Y_B)),
            tx.getOutput(TXV_HASH_B),
            tx.getOutput(TXV_SIGNATURE_REQUIRED_B),
            FMT(prefix, ".signatureVerifierB")),
          batchSignatureVerifierA(
            pb,
            params,
            state.constants,
            tx.getArrayOutput(TXV_PUBKEY_X_A_ARRAY),
            tx.getArrayOutput(TXV_PUBKEY_Y_A_ARRAY),
            tx.getArrayOutput(TXV_HASH_A_ARRAY),
            tx.getArrayOutput(TXV_SIGNATURE_REQUIRED_A_ARRAY),
            FMT(prefix, ".batchSignatureVerifierA")),
          batchSignatureVerifierB(
            pb,
            params,
            state.constants,
            tx.getArrayOutput(TXV_PUBKEY_X_B_ARRAY),
            tx.getArrayOutput(TXV_PUBKEY_Y_B_ARRAY),
            tx.getArrayOutput(TXV_HASH_B_ARRAY),
            tx.getArrayOutput(TXV_SIGNATURE_REQUIRED_B_ARRAY),
            FMT(prefix, ".batchSignatureVerifierB")),
          batchSignatureVerifierC(
            pb,
            params,
            state.constants,
            tx.getArrayOutput(TXV_PUBKEY_X_C_ARRAY),
            tx.getArrayOutput(TXV_PUBKEY_Y_C_ARRAY),
            tx.getArrayOutput(TXV_HASH_C_ARRAY),
            tx.getArrayOutput(TXV_SIGNATURE_REQUIRED_C_ARRAY),
            FMT(prefix, ".batchSignatureVerifierC")),
          batchSignatureVerifierD(
            pb,
            params,
            state.constants,
            tx.getArrayOutput(TXV_PUBKEY_X_D_ARRAY),
            tx.getArrayOutput(TXV_PUBKEY_Y_D_ARRAY),
            tx.getArrayOutput(TXV_HASH_D_ARRAY),
            tx.getArrayOutput(TXV_SIGNATURE_REQUIRED_D_ARRAY),
            FMT(prefix, ".batchSignatureVerifierD")),
          batchSignatureVerifierE(
            pb,
            params,
            state.constants,
            tx.getArrayOutput(TXV_PUBKEY_X_E_ARRAY),
            tx.getArrayOutput(TXV_PUBKEY_Y_E_ARRAY),
            tx.getArrayOutput(TXV_HASH_E_ARRAY),
            tx.getArrayOutput(TXV_SIGNATURE_REQUIRED_E_ARRAY),
            FMT(prefix, ".batchSignatureVerifierE")),
          batchSignatureVerifierF(
            pb,
            params,
            state.constants,
            tx.getArrayOutput(TXV_PUBKEY_X_F_ARRAY),
            tx.getArrayOutput(TXV_PUBKEY_Y_F_ARRAY),
            tx.getArrayOutput(TXV_HASH_F_ARRAY),
            tx.getArrayOutput(TXV_SIGNATURE_REQUIRED_F_ARRAY),
            FMT(prefix, ".batchSignatureVerifierF")),

          // Update UserA
          updateStorage_A(
            pb,
            // DEG-347 Storage location change
            // state.accountA.balanceS.storageRoot,
            state.accountA.account.storageRoot,
            tx.getArrayOutput(TXV_STORAGE_A_ADDRESS),
            // split trading fee and gas fee - add up gas
            // {state.accountA.storage.data, state.accountA.storage.storageID},
            // {tx.getOutput(TXV_STORAGE_A_DATA), tx.getOutput(TXV_STORAGE_A_STORAGEID)},
            // DEG-146:order cancel
            // DEG-347 Storage move
            {state.accountA.storage.tokenSID, 
            state.accountA.storage.tokenBID, 
            state.accountA.storage.data, 
            state.accountA.storage.storageID, 
            state.accountA.storage.gasFee, 
            state.accountA.storage.cancelled, 
            state.accountA.storage.forward},
            {tx.getOutput(TXV_STORAGE_A_TOKENSID), 
            tx.getOutput(TXV_STORAGE_A_TOKENBID), 
            tx.getOutput(TXV_STORAGE_A_DATA), 
            tx.getOutput(TXV_STORAGE_A_STORAGEID), 
            tx.getOutput(TXV_STORAGE_A_GASFEE), 
            tx.getOutput(TXV_STORAGE_A_CANCELLED), 
            tx.getOutput(TXV_STORAGE_A_FORWARD)},
            FMT(prefix, ".updateStorage_A")),
          updateStorage_A_batch(
            pb, 
            tx, 
            state.accountA, 
            updateStorage_A.result(), 
            FMT(prefix, ".updateStorage_A_batch")),
          updateBalanceS_A(
            pb,
            state.accountA.account.balancesRoot,
            tx.getArrayOutput(TXV_BALANCE_A_S_ADDRESS),
            // DEG-347 Storage location change
            // {state.accountA.balanceS.balance, state.accountA.balanceS.storageRoot},
            {state.accountA.balanceS.balance},
            // DEG-347 Storage location change
            // {tx.getOutput(TXV_BALANCE_A_S_BALANCE), updateStorage_A.result()},
            {tx.getOutput(TXV_BALANCE_A_S_BALANCE)},
            FMT(prefix, ".updateBalanceS_A")),
          updateBalanceB_A(
            pb,
            updateBalanceS_A.result(),
            // tx.getArrayOutput(TXV_BALANCE_B_S_ADDRESS),
            tx.getArrayOutput(TXV_BALANCE_A_B_ADDRESS),
            // DEG-347 Storage location change
            // {state.accountA.balanceB.balance, state.accountA.balanceB.storageRoot},
            {state.accountA.balanceB.balance},
            // {tx.getOutput(TXV_BALANCE_A_B_BALANCE),
            // DEG-347 Storage location change
            //  state.accountA.balanceB.storageRoot},
            {tx.getOutput(TXV_BALANCE_A_B_BALANCE)},
            FMT(prefix, ".updateBalanceB_A")),
          // split trading fee and gas fee
          updateBalanceFee_A(
            pb,
            updateBalanceB_A.result(),
            // DEG-127 split trading fee and gas fee
            tx.getArrayOutput(TXV_BALANCE_A_FEE_Address),
            // DEG-347 Storage location change
            // {state.accountA.balanceFee.balance, state.accountA.balanceFee.storageRoot},
            {state.accountA.balanceFee.balance},
            // DEG-347 Storage location change
            // {tx.getOutput(TXV_BALANCE_A_FEE_BALANCE),
            //  state.accountA.balanceFee.storageRoot},
            {tx.getOutput(TXV_BALANCE_A_FEE_BALANCE)},
            FMT(prefix, ".updateBalanceFee_A")),
          // DEG-265 auto market
          // // DEG-170 auto market - storage add
          // updateAutoMarketStorage_A(
          //   pb,
          //   state.accountA.autoMarket.autoMarketStorageRoot,
          //   tx.getArrayOutput(TXV_AUTO_MARKET_STORAGE_A_ADDRESS),
          //   {state.accountA.autoMarketStorage.autoMarketID, state.accountA.autoMarketStorage.tokenStorageID, state.accountA.autoMarketStorage.forward, state.accountA.autoMarketStorage.data},
          //   {tx.getOutput(TXV_AUTO_MARKET_STORAGE_A_AUTO_MARKET_ID), tx.getOutput(TXV_AUTO_MARKET_STORAGE_A_TOKEN_STORAGE_ID), tx.getOutput(TXV_AUTO_MARKET_STORAGE_A_FORWARD), tx.getOutput(TXV_AUTO_MARKET_STORAGE_A_DATA)},
          //   FMT(prefix, ".updateAutoMarketStorage_A")
          // ),
          // // DEG-170 auto market - config add
          // updateAutoMarket_A(
          //   pb,
          //   state.accountA.account.autoMarketRoot,
          //   tx.getArrayOutput(TXV_AUTOMARKET_A_ADDRESS),
          //   {state.accountA.autoMarket.autoMarketID, state.accountA.autoMarket.tokenSID, state.accountA.autoMarket.tokenBID, state.accountA.autoMarket.maxLevel, 
          //   // DEG-170 auto market - storage add
          //   state.accountA.autoMarket.gridOffset, state.accountA.autoMarket.orderOffset, state.accountA.autoMarket.cancelled, state.accountA.autoMarket.validUntil, state.accountA.autoMarket.autoMarketStorageRoot},
          //   {tx.getOutput(TXV_AUTOMARKET_A_ID), tx.getOutput(TXV_AUTOMARKET_A_TOKENS_ID), tx.getOutput(TXV_AUTOMARKET_A_TOKENB_ID), tx.getOutput(TXV_AUTOMARKET_A_MAX_LEVEL), 
          //   // DEG-170 auto market - storage add
          //   tx.getOutput(TXV_AUTOMARKET_A_GRID_OFFSET), tx.getOutput(TXV_AUTOMARKET_A_ORDER_OFFSET), tx.getOutput(TXV_AUTOMARKET_A_CANCELLED), tx.getOutput(TXV_AUTOMARKET_A_VALIDUNTIL), updateAutoMarketStorage_A.result()},
          //   FMT(prefix, ".updateAutoMarket_A")
          // ),
          updateAccount_A(
            pb,
            accountsRoot,
            accountsAssetRoot,
            tx.getArrayOutput(TXV_ACCOUNT_A_ADDRESS),
            {state.accountA.account.owner,
             state.accountA.account.publicKey.x,
             state.accountA.account.publicKey.y,
             state.accountA.account.appKeyPublicKey.x,
             state.accountA.account.appKeyPublicKey.y,
             state.accountA.account.nonce,
             state.accountA.account.disableAppKeySpotTrade,
             state.accountA.account.disableAppKeyWithdraw,
             state.accountA.account.disableAppKeyTransferToOther,
             state.accountA.account.balancesRoot,
            //  // DEG-170 auto market - config add
            //  state.accountA.account.autoMarketRoot, 
             // DEG-347 Storage location change
             state.accountA.account.storageRoot},
            {tx.getOutput(TXV_ACCOUNT_A_OWNER),
             tx.getOutput(TXV_ACCOUNT_A_PUBKEY_X),
             tx.getOutput(TXV_ACCOUNT_A_PUBKEY_Y),
             tx.getOutput(TXV_ACCOUNT_A_APPKEY_PUBKEY_X),
             tx.getOutput(TXV_ACCOUNT_A_APPKEY_PUBKEY_Y),
             tx.getOutput(TXV_ACCOUNT_A_NONCE),
             tx.getOutput(TXV_ACCOUNT_A_DISABLE_APPKEY_SPOTTRADE),
             tx.getOutput(TXV_ACCOUNT_A_DISABLE_APPKEY_WITHDRAW_TO_OTHER),
             tx.getOutput(TXV_ACCOUNT_A_DISABLE_APPKEY_TRANSFER_TO_OTHER),
            // split trading fee and gas fee
             updateBalanceFee_A.result(),
            //  // DEG-170 auto market - config add
            //  updateAutoMarket_A.result(), 
             // DEG-347 Storage location change
             updateStorage_A_batch.getHashRoot()},
            FMT(prefix, ".updateAccount_A")),

          // Update UserB
          updateStorage_B(
            pb,
            // DEG-347 Storage location change
            // state.accountB.balanceS.storageRoot,
            state.accountB.account.storageRoot,
            tx.getArrayOutput(TXV_STORAGE_B_ADDRESS),
            // split trading fee and gas fee - add up gas
            // {state.accountB.storage.data, state.accountB.storage.storageID},
            // {tx.getOutput(TXV_STORAGE_B_DATA), tx.getOutput(TXV_STORAGE_B_STORAGEID)},
            // DEG-146:order cancel
            // DEG-347 Storage move
            {state.accountB.storage.tokenSID, 
            state.accountB.storage.tokenBID, 
            state.accountB.storage.data, 
            state.accountB.storage.storageID, 
            state.accountB.storage.gasFee, 
            state.accountB.storage.cancelled, 
            state.accountB.storage.forward},
            {tx.getOutput(TXV_STORAGE_B_TOKENSID), 
            tx.getOutput(TXV_STORAGE_B_TOKENBID), 
            tx.getOutput(TXV_STORAGE_B_DATA), 
            tx.getOutput(TXV_STORAGE_B_STORAGEID), 
            tx.getOutput(TXV_STORAGE_B_GASFEE), 
            tx.getOutput(TXV_STORAGE_B_CANCELLED), 
            tx.getOutput(TXV_STORAGE_B_FORWARD)},
            FMT(prefix, ".updateStorage_B")),
          updateStorage_B_batch(
            pb, 
            tx, 
            state.accountB, 
            updateStorage_B.result(), 
            FMT(prefix, ".updateStorage_B_batch")),
          updateBalanceS_B(
            pb,
            state.accountB.account.balancesRoot,
            tx.getArrayOutput(TXV_BALANCE_B_S_ADDRESS),
            // DEG-347 Storage location change
            // {state.accountB.balanceS.balance, state.accountB.balanceS.storageRoot},
            {state.accountB.balanceS.balance},
            // DEG-347 Storage location change
            // {tx.getOutput(TXV_BALANCE_B_S_BALANCE), updateStorage_B.result()},
            {tx.getOutput(TXV_BALANCE_B_S_BALANCE)},
            FMT(prefix, ".updateBalanceS_B")),
          updateBalanceB_B(
            pb,
            updateBalanceS_B.result(),
            // tx.getArrayOutput(TXV_BALANCE_A_S_ADDRESS),
            tx.getArrayOutput(TXV_BALANCE_B_B_ADDRESS),
            // DEG-347 Storage location change
            // {state.accountB.balanceB.balance, state.accountB.balanceB.storageRoot},
            {state.accountB.balanceB.balance},
            // DEG-347 Storage location change
            // {tx.getOutput(TXV_BALANCE_B_B_BALANCE),
            //  state.accountB.balanceB.storageRoot},
            {tx.getOutput(TXV_BALANCE_B_B_BALANCE)},
            FMT(prefix, ".updateBalanceB_B")),
          // split trading fee and gas fee
          updateBalanceFee_B(
            pb,
            updateBalanceB_B.result(),
            // DEG-127 split trading fee and gas fee
            tx.getArrayOutput(TXV_BALANCE_B_FEE_Address),
            // DEG-347 Storage location change
            // {state.accountB.balanceFee.balance, state.accountB.balanceFee.storageRoot},
            {state.accountB.balanceFee.balance},
            // DEG-347 Storage location change
            // {tx.getOutput(TXV_BALANCE_B_FEE_BALANCE),
            //  state.accountB.balanceFee.storageRoot},
            {tx.getOutput(TXV_BALANCE_B_FEE_BALANCE)},
            FMT(prefix, ".updateBalanceFee_B")),
          // DEG-265 auto market
          // // DEG-170 auto market - storage add
          // updateAutoMarketStorage_B(
          //   pb,
          //   state.accountB.autoMarket.autoMarketStorageRoot,
          //   tx.getArrayOutput(TXV_AUTO_MARKET_STORAGE_B_ADDRESS),
          //   {state.accountB.autoMarketStorage.autoMarketID, state.accountB.autoMarketStorage.tokenStorageID, state.accountB.autoMarketStorage.forward, state.accountB.autoMarketStorage.data},
          //   {tx.getOutput(TXV_AUTO_MARKET_STORAGE_B_AUTO_MARKET_ID), tx.getOutput(TXV_AUTO_MARKET_STORAGE_B_TOKEN_STORAGE_ID), tx.getOutput(TXV_AUTO_MARKET_STORAGE_B_FORWARD), tx.getOutput(TXV_AUTO_MARKET_STORAGE_B_DATA)},
          //   FMT(prefix, ".updateAutoMarketStorage_B")
          // ),
          // // DEG-170 auto market - config add
          // updateAutoMarket_B(
          //   pb,
          //   state.accountB.account.autoMarketRoot,
          //   tx.getArrayOutput(TXV_AUTOMARKET_B_ADDRESS),
          //   {state.accountB.autoMarket.autoMarketID, state.accountB.autoMarket.tokenSID, state.accountB.autoMarket.tokenBID, state.accountB.autoMarket.maxLevel, 
          //   // DEG-170 auto market - storage add
          //   state.accountB.autoMarket.gridOffset, state.accountB.autoMarket.orderOffset, state.accountB.autoMarket.cancelled, state.accountB.autoMarket.validUntil, state.accountB.autoMarket.autoMarketStorageRoot},
          //   {state.accountB.autoMarket.autoMarketID, state.accountB.autoMarket.tokenSID, state.accountB.autoMarket.tokenBID, state.accountB.autoMarket.maxLevel, 
          //   // DEG-170 auto market - storage add
          //   state.accountB.autoMarket.gridOffset, state.accountB.autoMarket.orderOffset, state.accountB.autoMarket.cancelled, state.accountB.autoMarket.validUntil, updateAutoMarketStorage_B.result()},
          //   FMT(prefix, ".updateAutoMarket_B")
          // ),
          updateAccount_B(
            pb,
            updateAccount_A.result(),
            updateAccount_A.assetResult(),
            tx.getArrayOutput(TXV_ACCOUNT_B_ADDRESS),
            {state.accountB.account.owner,
             state.accountB.account.publicKey.x,
             state.accountB.account.publicKey.y,
             state.accountB.account.appKeyPublicKey.x,
             state.accountB.account.appKeyPublicKey.y,
             state.accountB.account.nonce,
             state.accountB.account.disableAppKeySpotTrade,
             state.accountB.account.disableAppKeyWithdraw,
             state.accountB.account.disableAppKeyTransferToOther,
             state.accountB.account.balancesRoot,
            //  // DEG-170 auto market - config add
            //  state.accountB.account.autoMarketRoot,
             // DEG-347 Storage location change
             state.accountB.account.storageRoot},
            {tx.getOutput(TXV_ACCOUNT_B_OWNER),
             tx.getOutput(TXV_ACCOUNT_B_PUBKEY_X),
             tx.getOutput(TXV_ACCOUNT_B_PUBKEY_Y),
             state.accountB.account.appKeyPublicKey.x,
             state.accountB.account.appKeyPublicKey.y,
             tx.getOutput(TXV_ACCOUNT_B_NONCE),
             state.accountB.account.disableAppKeySpotTrade,
             state.accountB.account.disableAppKeyWithdraw,
             state.accountB.account.disableAppKeyTransferToOther,
            // split trading fee and gas fee
             updateBalanceFee_B.result(),
            //  // DEG-170 auto market - config add
            //  state.accountB.account.autoMarketRoot, 
             // DEG-347 Storage location change
             updateStorage_B_batch.getHashRoot()},
            FMT(prefix, ".updateAccount_B")),
          // ------------UserC
          updateStorage_C_batch(
            pb, 
            tx, 
            state.accountC, 
            state.accountC.account.storageRoot, 
            FMT(prefix, ".updateStorage_C_batch")),
          updateBalanceS_C(
            pb,
            state.accountC.account.balancesRoot,
            tx.getArrayOutput(TXV_BALANCE_C_S_ADDRESS),
            {state.accountC.balanceS.balance},
            {tx.getOutput(TXV_BALANCE_C_S_BALANCE)},
            FMT(prefix, ".updateBalanceS_C")),
          updateBalanceB_C(
            pb,
            updateBalanceS_C.result(),
            // tx.getArrayOutput(TXV_BALANCE_B_S_ADDRESS),
            tx.getArrayOutput(TXV_BALANCE_C_B_ADDRESS),
            {state.accountC.balanceB.balance},
            {tx.getOutput(TXV_BALANCE_C_B_BALANCE)},
            FMT(prefix, ".updateBalanceB_C")),
          updateBalanceFee_C(
            pb,
            updateBalanceB_C.result(),
            tx.getArrayOutput(TXV_BALANCE_C_FEE_Address),
            {state.accountC.balanceFee.balance},
            {tx.getOutput(TXV_BALANCE_C_FEE_BALANCE)},
            FMT(prefix, ".updateBalanceFee_C")),
          updateAccount_C(
            pb,
            updateAccount_B.result(),
            updateAccount_B.assetResult(),
            tx.getArrayOutput(TXV_ACCOUNT_C_ADDRESS),
            {state.accountC.account.owner,
             state.accountC.account.publicKey.x,
             state.accountC.account.publicKey.y,
             state.accountC.account.appKeyPublicKey.x,
             state.accountC.account.appKeyPublicKey.y,
             state.accountC.account.nonce,
             state.accountC.account.disableAppKeySpotTrade,
             state.accountC.account.disableAppKeyWithdraw,
             state.accountC.account.disableAppKeyTransferToOther,
             state.accountC.account.balancesRoot,
             state.accountC.account.storageRoot},
            {tx.getOutput(TXV_ACCOUNT_C_OWNER),
             tx.getOutput(TXV_ACCOUNT_C_PUBKEY_X),
             tx.getOutput(TXV_ACCOUNT_C_PUBKEY_Y),
             state.accountC.account.appKeyPublicKey.x,
             state.accountC.account.appKeyPublicKey.y,
             tx.getOutput(TXV_ACCOUNT_C_NONCE),
             state.accountC.account.disableAppKeySpotTrade,
             state.accountC.account.disableAppKeyWithdraw,
             state.accountC.account.disableAppKeyTransferToOther,
             updateBalanceFee_C.result(),
             updateStorage_C_batch.getHashRoot()},
            FMT(prefix, ".updateAccount_C")),          
          //------------UserD
          updateStorage_D_batch(
            pb, 
            tx, 
            state.accountD, 
            state.accountD.account.storageRoot, 
            FMT(prefix, ".updateStorage_D_batch")),
          updateBalanceS_D(
            pb,
            state.accountD.account.balancesRoot,
            tx.getArrayOutput(TXV_BALANCE_D_S_ADDRESS),
            {state.accountD.balanceS.balance},
            {tx.getOutput(TXV_BALANCE_D_S_BALANCE)},
            FMT(prefix, ".updateBalanceS_D")),
          updateBalanceB_D(
            pb,
            updateBalanceS_D.result(),
            // tx.getArrayOutput(TXV_BALANCE_B_S_ADDRESS),
            tx.getArrayOutput(TXV_BALANCE_D_B_ADDRESS),
            {state.accountD.balanceB.balance},
            {tx.getOutput(TXV_BALANCE_D_B_BALANCE)},
            FMT(prefix, ".updateBalanceB_D")),
          updateBalanceFee_D(
            pb,
            updateBalanceB_D.result(),
            tx.getArrayOutput(TXV_BALANCE_D_FEE_Address),
            {state.accountD.balanceFee.balance},
            {tx.getOutput(TXV_BALANCE_D_FEE_BALANCE)},
            FMT(prefix, ".updateBalanceFee_D")),
          updateAccount_D(
            pb,
            updateAccount_C.result(),
            updateAccount_C.assetResult(),
            tx.getArrayOutput(TXV_ACCOUNT_D_ADDRESS),
            {state.accountD.account.owner,
             state.accountD.account.publicKey.x,
             state.accountD.account.publicKey.y,
             state.accountD.account.appKeyPublicKey.x,
             state.accountD.account.appKeyPublicKey.y,
             state.accountD.account.nonce,
             state.accountD.account.disableAppKeySpotTrade,
             state.accountD.account.disableAppKeyWithdraw,
             state.accountD.account.disableAppKeyTransferToOther,
             state.accountD.account.balancesRoot,
             state.accountD.account.storageRoot},
            {tx.getOutput(TXV_ACCOUNT_D_OWNER),
             tx.getOutput(TXV_ACCOUNT_D_PUBKEY_X),
             tx.getOutput(TXV_ACCOUNT_D_PUBKEY_Y),
             state.accountD.account.appKeyPublicKey.x,
             state.accountD.account.appKeyPublicKey.y,
             tx.getOutput(TXV_ACCOUNT_D_NONCE),
             state.accountD.account.disableAppKeySpotTrade,
             state.accountD.account.disableAppKeyWithdraw,
             state.accountD.account.disableAppKeyTransferToOther,
             updateBalanceFee_D.result(),
             updateStorage_D_batch.getHashRoot()},
            FMT(prefix, ".updateAccount_D")),
          //------------UserE
          updateStorage_E_batch(
            pb, 
            tx, 
            state.accountE, 
            state.accountE.account.storageRoot, 
            FMT(prefix, ".updateStorage_E_batch")),
          updateBalanceS_E(
            pb,
            state.accountE.account.balancesRoot,
            tx.getArrayOutput(TXV_BALANCE_E_S_ADDRESS),
            {state.accountE.balanceS.balance},
            {tx.getOutput(TXV_BALANCE_E_S_BALANCE)},
            FMT(prefix, ".updateBalanceS_E")),
          updateBalanceB_E(
            pb,
            updateBalanceS_E.result(),
            // tx.getArrayOutput(TXV_BALANCE_B_S_ADDRESS),
            tx.getArrayOutput(TXV_BALANCE_E_B_ADDRESS),
            {state.accountE.balanceB.balance},
            {tx.getOutput(TXV_BALANCE_E_B_BALANCE)},
            FMT(prefix, ".updateBalanceB_E")),
          updateBalanceFee_E(
            pb,
            updateBalanceB_E.result(),
            tx.getArrayOutput(TXV_BALANCE_E_FEE_Address),
            {state.accountE.balanceFee.balance},
            {tx.getOutput(TXV_BALANCE_E_FEE_BALANCE)},
            FMT(prefix, ".updateBalanceFee_E")),
          updateAccount_E(
            pb,
            updateAccount_D.result(),
            updateAccount_D.assetResult(),
            tx.getArrayOutput(TXV_ACCOUNT_E_ADDRESS),
            {state.accountE.account.owner,
             state.accountE.account.publicKey.x,
             state.accountE.account.publicKey.y,
             state.accountE.account.appKeyPublicKey.x,
             state.accountE.account.appKeyPublicKey.y,
             state.accountE.account.nonce,
             state.accountE.account.disableAppKeySpotTrade,
             state.accountE.account.disableAppKeyWithdraw,
             state.accountE.account.disableAppKeyTransferToOther,
             state.accountE.account.balancesRoot,
             state.accountE.account.storageRoot},
            {tx.getOutput(TXV_ACCOUNT_E_OWNER),
             tx.getOutput(TXV_ACCOUNT_E_PUBKEY_X),
             tx.getOutput(TXV_ACCOUNT_E_PUBKEY_Y),
             state.accountE.account.appKeyPublicKey.x,
             state.accountE.account.appKeyPublicKey.y,
             tx.getOutput(TXV_ACCOUNT_E_NONCE),
             state.accountE.account.disableAppKeySpotTrade,
             state.accountE.account.disableAppKeyWithdraw,
             state.accountE.account.disableAppKeyTransferToOther,
             updateBalanceFee_E.result(),
             updateStorage_E_batch.getHashRoot()},
            FMT(prefix, ".updateAccount_E")),   

          //------------UserF
          updateStorage_F_batch(
            pb, 
            tx, 
            state.accountF, 
            state.accountF.account.storageRoot, 
            FMT(prefix, ".updateStorage_F_batch")),
          updateBalanceS_F(
            pb,
            state.accountF.account.balancesRoot,
            tx.getArrayOutput(TXV_BALANCE_F_S_ADDRESS),
            {state.accountF.balanceS.balance},
            {tx.getOutput(TXV_BALANCE_F_S_BALANCE)},
            FMT(prefix, ".updateBalanceS_F")),
          updateBalanceB_F(
            pb,
            updateBalanceS_F.result(),
            // tx.getArrayOutput(TXV_BALANCE_B_S_ADDRESS),
            tx.getArrayOutput(TXV_BALANCE_F_B_ADDRESS),
            {state.accountF.balanceB.balance},
            {tx.getOutput(TXV_BALANCE_F_B_BALANCE)},
            FMT(prefix, ".updateBalanceB_F")),
          updateBalanceFee_F(
            pb,
            updateBalanceB_F.result(),
            tx.getArrayOutput(TXV_BALANCE_F_FEE_Address),
            {state.accountF.balanceFee.balance},
            {tx.getOutput(TXV_BALANCE_F_FEE_BALANCE)},
            FMT(prefix, ".updateBalanceFee_F")),
          updateAccount_F(
            pb,
            updateAccount_E.result(),
            updateAccount_E.assetResult(),
            tx.getArrayOutput(TXV_ACCOUNT_F_ADDRESS),
            {state.accountF.account.owner,
             state.accountF.account.publicKey.x,
             state.accountF.account.publicKey.y,
             state.accountF.account.appKeyPublicKey.x,
             state.accountF.account.appKeyPublicKey.y,
             state.accountF.account.nonce,
             state.accountF.account.disableAppKeySpotTrade,
             state.accountF.account.disableAppKeyWithdraw,
             state.accountF.account.disableAppKeyTransferToOther,
             state.accountF.account.balancesRoot,
             state.accountF.account.storageRoot},
            {tx.getOutput(TXV_ACCOUNT_F_OWNER),
             tx.getOutput(TXV_ACCOUNT_F_PUBKEY_X),
             tx.getOutput(TXV_ACCOUNT_F_PUBKEY_Y),
             state.accountF.account.appKeyPublicKey.x,
             state.accountF.account.appKeyPublicKey.y,
             tx.getOutput(TXV_ACCOUNT_F_NONCE),
             state.accountF.account.disableAppKeySpotTrade,
             state.accountF.account.disableAppKeyWithdraw,
             state.accountF.account.disableAppKeyTransferToOther,
             updateBalanceFee_F.result(),
             updateStorage_F_batch.getHashRoot()},
            FMT(prefix, ".updateAccount_F")),   

          // Update Operator
          updateBalanceD_O(
            pb,
            state.oper.account.balancesRoot,
            tx.getArrayOutput(TXV_BALANCE_O_D_Address),
            {state.oper.balanceD.balance},
            {tx.getOutput(TXV_BALANCE_O_D_BALANCE)},
            FMT(prefix, ".updateBalanceD_O")),
          updateBalanceC_O(
            pb,
            updateBalanceD_O.result(),
            tx.getArrayOutput(TXV_BALANCE_O_C_Address),
            {state.oper.balanceC.balance},
            {tx.getOutput(TXV_BALANCE_O_C_BALANCE)},
            FMT(prefix, ".updateBalanceC_O")),
          updateBalanceB_O(
            pb,
            // state.oper.account.balancesRoot,
            updateBalanceC_O.result(),
            // split trading fee and gas fee
            // tx.getArrayOutput(TXV_BALANCE_A_S_ADDRESS),
            tx.getArrayOutput(TXV_BALANCE_O_B_Address),
            // DEG-347 Storage location change
            // {state.oper.balanceB.balance, state.oper.balanceB.storageRoot},
            {state.oper.balanceB.balance},
            // DEG-347 Storage location change
            // {tx.getOutput(TXV_BALANCE_O_B_BALANCE), state.oper.balanceB.storageRoot},
            {tx.getOutput(TXV_BALANCE_O_B_BALANCE)},
            FMT(prefix, ".updateBalanceB_O")),
          updateBalanceA_O(
            pb,
            updateBalanceB_O.result(),
            // split trading fee and gas fee
            // tx.getArrayOutput(TXV_BALANCE_B_S_ADDRESS),
            tx.getArrayOutput(TXV_BALANCE_O_A_Address),
            // DEG-347 Storage location change
            // {state.oper.balanceA.balance, state.oper.balanceA.storageRoot},
            {state.oper.balanceA.balance},
            // DEG-347 Storage location change
            // {tx.getOutput(TXV_BALANCE_O_A_BALANCE), state.oper.balanceA.storageRoot},
            {tx.getOutput(TXV_BALANCE_O_A_BALANCE)},
            FMT(prefix, ".updateBalanceA_O")),
          updateAccount_O(
            pb,
            // updateAccount_B.result(),
            updateAccount_F.result(),
            updateAccount_F.assetResult(),
            operatorAccountID,
            {state.oper.account.owner,
             state.oper.account.publicKey.x,
             state.oper.account.publicKey.y,
             state.oper.account.appKeyPublicKey.x,
             state.oper.account.appKeyPublicKey.y,
             state.oper.account.nonce,
             state.oper.account.disableAppKeySpotTrade,
             state.oper.account.disableAppKeyWithdraw,
             state.oper.account.disableAppKeyTransferToOther,
             state.oper.account.balancesRoot,
            //  // DEG-170 auto market - config add
            //  state.oper.account.autoMarketRoot,
             // DEG-347 Storage location change
             state.oper.account.storageRoot},
            {state.oper.account.owner,
             state.oper.account.publicKey.x,
             state.oper.account.publicKey.y,
             state.oper.account.appKeyPublicKey.x,
             state.oper.account.appKeyPublicKey.y,
             state.oper.account.nonce,
             state.oper.account.disableAppKeySpotTrade,
             state.oper.account.disableAppKeyWithdraw,
             state.oper.account.disableAppKeyTransferToOther,
             updateBalanceA_O.result(),
            //  // DEG-170 auto market - config add
            //  state.oper.account.autoMarketRoot,
             // DEG-347 Storage location change
             state.oper.account.storageRoot},
            FMT(prefix, ".updateAccount_O"))

          // Update Protocol pool

          // updateBalanceC_P(
          //   pb,
          //   protocolBalancesRoot,
          //   tx.getArrayOutput(TXV_BALANCE_P_C_Address),
          //   {state.pool.balanceC.balance},
          //   {tx.getOutput(TXV_BALANCE_P_C_BALANCE)},
          //   FMT(prefix, ".updateBalanceC_P")),
          // updateBalanceB_P(
          //   pb,
          //   // protocolBalancesRoot,
          //   updateBalanceC_P.result(),
          //   // tx.getArrayOutput(TXV_BALANCE_A_S_ADDRESS),
          //   tx.getArrayOutput(TXV_BALANCE_P_B_Address),
          //   // DEG-347 Storage location change
          //   // {state.pool.balanceB.balance, state.pool.balanceB.storageRoot},
          //   {state.pool.balanceB.balance},
          //   // DEG-347 Storage location change
          //   // {tx.getOutput(TXV_BALANCE_P_B_BALANCE), state.pool.balanceB.storageRoot},
          //   {tx.getOutput(TXV_BALANCE_P_B_BALANCE)},
          //   FMT(prefix, ".updateBalanceB_P")),
          // updateBalanceA_P(
          //   pb,
          //   updateBalanceB_P.result(),
          //   // tx.getArrayOutput(TXV_BALANCE_B_S_ADDRESS),
          //   tx.getArrayOutput(TXV_BALANCE_P_A_Address),
          //   // DEG-347 Storage location change
          //   // {state.pool.balanceA.balance, state.pool.balanceA.storageRoot},
          //   {state.pool.balanceA.balance},
          //   // DEG-347 Storage location change
          //   // {tx.getOutput(TXV_BALANCE_P_A_BALANCE), state.pool.balanceA.storageRoot},
          //   {tx.getOutput(TXV_BALANCE_P_A_BALANCE)},
          //   FMT(prefix, ".updateBalanceA_P"))
    {
      std::cout << "in Transaction TransactionGadget" << std::endl;
      
    }

    void generate_r1cs_witness(const UniversalTransaction &uTx)
    {
        std::cout << "in Transaction: before type" << std::endl;
        type.generate_r1cs_witness(pb, uTx.type);
        std::cout << "in Transaction: before selecter" << std::endl;
        selector.generate_r1cs_witness();

        std::cout << "in Transaction: before state" << std::endl;

        std::cout << "in Transaction: before state fee A before:" << uTx.witness.balanceUpdateB_A.before.balance << std::endl;
        std::cout << "in Transaction: before state fee A after:" << uTx.witness.balanceUpdateB_A.after.balance << std::endl;
        std::cout << "in Transaction: before state fee A before:" << uTx.witness.balanceUpdateFee_A.before.balance << std::endl;
        std::cout << "in Transaction: before state fee A after:" << uTx.witness.balanceUpdateFee_A.after.balance << std::endl;
        std::cout << "in Transaction: before state storage A before data after:" << uTx.witness.storageUpdate_A.before.data << std::endl;
        std::cout << "in Transaction: before state uTx.witness.balanceUpdateA_O.before after:" << uTx.witness.balanceUpdateA_O.before.balance << std::endl;
        std::cout << "in Transaction: before state uTx.witness.balanceUpdateB_O.before after:" << uTx.witness.balanceUpdateB_O.before.balance << std::endl;
        std::cout << "in Transaction: before state uTx.witness.balanceUpdateC_O.before after:" << uTx.witness.balanceUpdateC_O.before.balance << std::endl;
        std::cout << "in Transaction: before state uTx.witness.balanceUpdateD_O.before after:" << uTx.witness.balanceUpdateD_O.before.balance << std::endl;
        state.generate_r1cs_witness(
          uTx.witness.accountUpdate_A.before,
          uTx.witness.balanceUpdateS_A.before,
          uTx.witness.balanceUpdateB_A.before,
          // split trading fee and gas fee
          uTx.witness.balanceUpdateFee_A.before,
          uTx.witness.storageUpdate_A.before,
          uTx.witness.storageUpdate_A_array,
          // DEG-265 auto market
          // // DEG-170 auto market - storage add
          // uTx.witness.autoMarketStorageUpdate_A.before,
          // // DEG-170 auto market - config add
          // uTx.witness.autoMarketUpdate_A.before,
          uTx.witness.accountUpdate_B.before,
          uTx.witness.balanceUpdateS_B.before,
          uTx.witness.balanceUpdateB_B.before,
          // split trading fee and gas fee
          uTx.witness.balanceUpdateFee_B.before,
          uTx.witness.storageUpdate_B.before,
          uTx.witness.storageUpdate_B_array,
          //---- UserC
          uTx.witness.accountUpdate_C.before,
          uTx.witness.balanceUpdateS_C.before,
          uTx.witness.balanceUpdateB_C.before,
          uTx.witness.balanceUpdateFee_C.before,
          uTx.witness.storageUpdate_C_array,
          //---- UserD
          uTx.witness.accountUpdate_D.before,
          uTx.witness.balanceUpdateS_D.before,
          uTx.witness.balanceUpdateB_D.before,
          uTx.witness.balanceUpdateFee_D.before,
          uTx.witness.storageUpdate_D_array,
          //---- UserE
          uTx.witness.accountUpdate_E.before,
          uTx.witness.balanceUpdateS_E.before,
          uTx.witness.balanceUpdateB_E.before,
          uTx.witness.balanceUpdateFee_E.before,
          uTx.witness.storageUpdate_E_array,
          //---- UserF
          uTx.witness.accountUpdate_F.before,
          uTx.witness.balanceUpdateS_F.before,
          uTx.witness.balanceUpdateB_F.before,
          uTx.witness.balanceUpdateFee_F.before,
          uTx.witness.storageUpdate_F_array,
          // DEG-265 auto market
          // // DEG-170 auto market - storage add
          // uTx.witness.autoMarketStorageUpdate_B.before,
          // // DEG-170 auto market - config add
          // uTx.witness.autoMarketUpdate_B.before,
          uTx.witness.accountUpdate_O.before,
          uTx.witness.balanceUpdateA_O.before,
          uTx.witness.balanceUpdateB_O.before,
          uTx.witness.balanceUpdateC_O.before,
          uTx.witness.balanceUpdateD_O.before
          // uTx.witness.balanceUpdateA_P.before,
          // uTx.witness.balanceUpdateB_P.before,
          // uTx.witness.balanceUpdateC_P.before
          );
        // std::cout << "in Transaction: uTx.witness.autoMarketUpdate_A.before ==============:" << uTx.witness.autoMarketUpdate_A.before.autoMarketStorageRoot << std::endl;

        std::cout << "in Transaction: before state accountA disableAppKeySpotTrade:" << pb.val(state.accountA.account.disableAppKeySpotTrade) << std::endl;
        std::cout << "in Transaction: before state accountA disableAppKeyWithdraw:" << pb.val(state.accountA.account.disableAppKeyWithdraw) << std::endl;
        std::cout << "in Transaction: before state accountA disableAppKeyTransferToOther:" << pb.val(state.accountA.account.disableAppKeyTransferToOther) << std::endl;

        std::cout << "in Transaction: before state state.oper.balanceA.balance() after:" << pb.val(state.oper.balanceA.balance) << std::endl;
        std::cout << "in Transaction: before state state.oper.balanceB.balance() after:" << pb.val(state.oper.balanceB.balance) << std::endl;
        std::cout << "in Transaction: before state state.oper.balanceC.balance() after:" << pb.val(state.oper.balanceC.balance) << std::endl;
        std::cout << "in Transaction: before state state.oper.balanceD.balance() after:" << pb.val(state.oper.balanceD.balance) << std::endl;
        noop.generate_r1cs_witness();
        spotTrade.generate_r1cs_witness(uTx.spotTrade);
        deposit.generate_r1cs_witness(uTx.deposit);
        withdraw.generate_r1cs_witness(uTx.withdraw);
        accountUpdate.generate_r1cs_witness(uTx.accountUpdate);
        transfer.generate_r1cs_witness(uTx.transfer);
        // DEG-146:order cancel
        orderCancel.generate_r1cs_witness(uTx.orderCancel);
        appKeyUpdate.generate_r1cs_witness(uTx.appKeyUpdate);
        batchSpotTrade.generate_r1cs_witness(uTx.batchSpotTrade);
        // // DEG-170 auto market - config add
        // autoMakretUpdate.generate_r1cs_witness(uTx.autoMarketUpdate);
        std::cout << "in Transaction: before tx" << std::endl;
        tx.generate_r1cs_witness();

        std::cout << "in Transaction: before accountA" << std::endl;
        // // General validation
        // accountA.generate_r1cs_witness();
        // std::cout << "in Transaction: before accountB" << std::endl;
        // accountB.generate_r1cs_witness();
        // std::cout << "in Transaction: before valide account a" << std::endl;
        // validateAccountA.generate_r1cs_witness();
        // std::cout << "in Transaction: before valide account b" << std::endl;
        // validateAccountB.generate_r1cs_witness();

        // Check signatures
        std::cout << "in Transaction: before signatureVerifierA" << std::endl;
        signatureVerifierA.generate_r1cs_witness(uTx.witness.signatureA);
        std::cout << "in Transaction: before signatureVerifierB" << std::endl;
        signatureVerifierB.generate_r1cs_witness(uTx.witness.signatureB);

        batchSignatureVerifierA.generate_r1cs_witness(uTx.witness.signatureArray[0]);
        batchSignatureVerifierB.generate_r1cs_witness(uTx.witness.signatureArray[1]);
        batchSignatureVerifierC.generate_r1cs_witness(uTx.witness.signatureArray[2]);
        batchSignatureVerifierD.generate_r1cs_witness(uTx.witness.signatureArray[3]);
        batchSignatureVerifierE.generate_r1cs_witness(uTx.witness.signatureArray[4]);
        batchSignatureVerifierF.generate_r1cs_witness(uTx.witness.signatureArray[5]);
        // Update UserA
        std::cout << "in Transaction: before updateStorage_A" << std::endl;
        updateStorage_A.generate_r1cs_witness(uTx.witness.storageUpdate_A);

        // batch spot trade Storage
        std::cout << "in Transaction: before storageUpdate_A_array" << std::endl;
        updateStorage_A_batch.generate_r1cs_witness(uTx.witness.storageUpdate_A_array);

        std::cout << "in Transaction: before updateBalanceS_A" << std::endl;
        updateBalanceS_A.generate_r1cs_witness(uTx.witness.balanceUpdateS_A);
        std::cout << "in Transaction: before updateBalanceB_A" << std::endl;
        updateBalanceB_A.generate_r1cs_witness(uTx.witness.balanceUpdateB_A);
        std::cout << "in Transaction: before fee update A" << std::endl;
        // split trading fee and gas fee
        updateBalanceFee_A.generate_r1cs_witness(uTx.witness.balanceUpdateFee_A);
        // DEG-265 auto market
        // // DEG-170 auto market - storage add
        // std::cout << "in Transaction: before updateAutoMarketStorage_A" << std::endl;
        // updateAutoMarketStorage_A.generate_r1cs_witness(uTx.witness.autoMarketStorageUpdate_A);
        // // DEG-170 auto market - config add
        // std::cout << "in Transaction: before updateAutoMarket_A" << std::endl;
        // updateAutoMarket_A.generate_r1cs_witness(uTx.witness.autoMarketUpdate_A);
        std::cout << "in Transaction: before account update A" << std::endl;
        updateAccount_A.generate_r1cs_witness(uTx.witness.accountUpdate_A);

        // Update UserB
        updateStorage_B.generate_r1cs_witness(uTx.witness.storageUpdate_B);

        // batch spot trade Storage
        updateStorage_B_batch.generate_r1cs_witness(uTx.witness.storageUpdate_B_array);
        updateBalanceS_B.generate_r1cs_witness(uTx.witness.balanceUpdateS_B);
        updateBalanceB_B.generate_r1cs_witness(uTx.witness.balanceUpdateB_B);
        std::cout << "in Transaction: before fee update B" << std::endl;
        // split trading fee and gas fee
        updateBalanceFee_B.generate_r1cs_witness(uTx.witness.balanceUpdateFee_B);
        // DEG-265 auto market
        // // DEG-170 auto market - storage add
        // std::cout << "in Transaction: before updateAutoMarketStorage_B" << std::endl;
        // updateAutoMarketStorage_B.generate_r1cs_witness(uTx.witness.autoMarketStorageUpdate_B);
        // // DEG-170 auto market - config add
        // std::cout << "in Transaction: before fee updateAutoMarket_B" << std::endl;
        // updateAutoMarket_B.generate_r1cs_witness(uTx.witness.autoMarketUpdate_B);
        std::cout << "in Transaction: before account update B" << std::endl;
        updateAccount_B.generate_r1cs_witness(uTx.witness.accountUpdate_B);

        //---------UserC
        // Update UserC
        std::cout << "in Transaction: before updateStorage_C" << std::endl;
        // batch spot trade Storage
        updateStorage_C_batch.generate_r1cs_witness(uTx.witness.storageUpdate_C_array);

        std::cout << "in Transaction: before updateBalanceS_C" << std::endl;
        updateBalanceS_C.generate_r1cs_witness(uTx.witness.balanceUpdateS_C);
        std::cout << "in Transaction: before updateBalanceB_C" << std::endl;
        updateBalanceB_C.generate_r1cs_witness(uTx.witness.balanceUpdateB_C);
        std::cout << "in Transaction: before fee update C" << std::endl;
        updateBalanceFee_C.generate_r1cs_witness(uTx.witness.balanceUpdateFee_C);
        std::cout << "in Transaction: before account update C" << std::endl;
        updateAccount_C.generate_r1cs_witness(uTx.witness.accountUpdate_C);

        //---------UserD
        // Update UserD
        std::cout << "in Transaction: before updateStorage_D" << std::endl;
        // batch spot trade Storage
        updateStorage_D_batch.generate_r1cs_witness(uTx.witness.storageUpdate_D_array);

        std::cout << "in Transaction: before updateBalanceS_D" << std::endl;
        updateBalanceS_D.generate_r1cs_witness(uTx.witness.balanceUpdateS_D);
        std::cout << "in Transaction: before updateBalanceB_D" << std::endl;
        updateBalanceB_D.generate_r1cs_witness(uTx.witness.balanceUpdateB_D);
        std::cout << "in Transaction: before fee update D" << std::endl;
        updateBalanceFee_D.generate_r1cs_witness(uTx.witness.balanceUpdateFee_D);
        std::cout << "in Transaction: before account update D" << std::endl;
        updateAccount_D.generate_r1cs_witness(uTx.witness.accountUpdate_D);

        //---------UserE
        // Update UserE
        std::cout << "in Transaction: before updateStorage_E" << std::endl;
        // batch spot trade Storage
        updateStorage_E_batch.generate_r1cs_witness(uTx.witness.storageUpdate_E_array);

        std::cout << "in Transaction: before updateBalanceS_E" << std::endl;
        updateBalanceS_E.generate_r1cs_witness(uTx.witness.balanceUpdateS_E);
        std::cout << "in Transaction: before updateBalanceB_E" << std::endl;
        updateBalanceB_E.generate_r1cs_witness(uTx.witness.balanceUpdateB_E);
        std::cout << "in Transaction: before fee update E" << std::endl;
        updateBalanceFee_E.generate_r1cs_witness(uTx.witness.balanceUpdateFee_E);
        std::cout << "in Transaction: before account update E" << std::endl;
        updateAccount_E.generate_r1cs_witness(uTx.witness.accountUpdate_E);

        //---------UserF
        // Update UserF
        std::cout << "in Transaction: before updateStorage_F" << std::endl;
        // batch spot trade Storage
        updateStorage_F_batch.generate_r1cs_witness(uTx.witness.storageUpdate_F_array);

        std::cout << "in Transaction: before updateBalanceS_F" << std::endl;
        updateBalanceS_F.generate_r1cs_witness(uTx.witness.balanceUpdateS_F);
        std::cout << "in Transaction: before updateBalanceB_F" << std::endl;
        updateBalanceB_F.generate_r1cs_witness(uTx.witness.balanceUpdateB_F);
        std::cout << "in Transaction: before fee update F" << std::endl;
        updateBalanceFee_F.generate_r1cs_witness(uTx.witness.balanceUpdateFee_F);
        std::cout << "in Transaction: before account update F" << std::endl;
        updateAccount_F.generate_r1cs_witness(uTx.witness.accountUpdate_F);

        // Update Operator
        std::cout << "in Transaction: before updateBalanceD_O" << std::endl;
        updateBalanceD_O.generate_r1cs_witness(uTx.witness.balanceUpdateD_O);
        std::cout << "in Transaction: before updateBalanceC_O" << std::endl;
        updateBalanceC_O.generate_r1cs_witness(uTx.witness.balanceUpdateC_O);
        std::cout << "in Transaction: before updateBalanceB_O" << std::endl;
        updateBalanceB_O.generate_r1cs_witness(uTx.witness.balanceUpdateB_O);
        std::cout << "in Transaction: before updateBalanceA_O" << std::endl;
        updateBalanceA_O.generate_r1cs_witness(uTx.witness.balanceUpdateA_O);
        std::cout << "in Transaction: before account update operator" << std::endl;
        updateAccount_O.generate_r1cs_witness(uTx.witness.accountUpdate_O);

        // // Update Protocol pool
        // updateBalanceC_P.generate_r1cs_witness(uTx.witness.balanceUpdateC_P);
        // std::cout << "in Transaction: before updateBalanceB_P" << std::endl;
        // updateBalanceB_P.generate_r1cs_witness(uTx.witness.balanceUpdateB_P);
        // std::cout << "in Transaction: before updateBalanceA_P" << std::endl;
        // updateBalanceA_P.generate_r1cs_witness(uTx.witness.balanceUpdateA_P);
    }

    void generate_r1cs_constraints()
    {
        std::cout << "in TransactionGadget: before type" << std::endl;
        type.generate_r1cs_constraints(true);
        selector.generate_r1cs_constraints();

        noop.generate_r1cs_constraints();
        spotTrade.generate_r1cs_constraints();
        deposit.generate_r1cs_constraints();
        withdraw.generate_r1cs_constraints();
        accountUpdate.generate_r1cs_constraints();
        transfer.generate_r1cs_constraints();
        // DEG-146:order cancel
        orderCancel.generate_r1cs_constraints();
        appKeyUpdate.generate_r1cs_constraints();

        std::cout << "before batchSpotTrade:" << pb.num_constraints() << std::endl;
        batchSpotTrade.generate_r1cs_constraints();
        std::cout << "after batchSpotTrade:" << pb.num_constraints()  << std::endl;
        // DEG-265 auto market
        // // DEG-170 auto market - config add
        // autoMakretUpdate.generate_r1cs_constraints();
        tx.generate_r1cs_constraints();

        // // General validation
        // accountA.generate_r1cs_constraints();
        // accountB.generate_r1cs_constraints();
        // validateAccountA.generate_r1cs_constraints();
        // validateAccountB.generate_r1cs_constraints();

        // Check signatures
        signatureVerifierA.generate_r1cs_constraints();
        signatureVerifierB.generate_r1cs_constraints();

        std::cout << "before batchSignatureVerifierA:" << pb.num_constraints() << std::endl;
        batchSignatureVerifierA.generate_r1cs_constraints();
        std::cout << "after batchSignatureVerifierA:" << pb.num_constraints() << std::endl;
        batchSignatureVerifierB.generate_r1cs_constraints();
        std::cout << "after batchSignatureVerifierB:" << pb.num_constraints() << std::endl;
        batchSignatureVerifierC.generate_r1cs_constraints();
        batchSignatureVerifierD.generate_r1cs_constraints();
        batchSignatureVerifierE.generate_r1cs_constraints();
        batchSignatureVerifierF.generate_r1cs_constraints();

        // Update UserA
        updateStorage_A.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateStorage_A_batch" << std::endl;
        updateStorage_A_batch.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceS_A" << std::endl;
        updateBalanceS_A.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceB_A" << std::endl;
        updateBalanceB_A.generate_r1cs_constraints();
        // split trading fee and gas fee
        std::cout << "in TransactionGadget: before updateBalanceFee_A" << std::endl;
        updateBalanceFee_A.generate_r1cs_constraints();
        // DEG-265 auto market
        // // DEG-170 auto market - storage add
        // updateAutoMarketStorage_A.generate_r1cs_constraints();
        // // DEG-170 auto market - config add
        // updateAutoMarket_A.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateAccount_A" << std::endl;
        updateAccount_A.generate_r1cs_constraints();

        // Update UserB
        updateStorage_B.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateStorage_B_batch" << std::endl;
        updateStorage_B_batch.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceS_B" << std::endl;
        updateBalanceS_B.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceB_B" << std::endl;
        updateBalanceB_B.generate_r1cs_constraints();
        // split trading fee and gas fee
        std::cout << "in TransactionGadget: before updateBalanceFee_B" << std::endl;
        updateBalanceFee_B.generate_r1cs_constraints();
        // DEG-265 auto market
        // // DEG-170 auto market - storage add
        // updateAutoMarketStorage_B.generate_r1cs_constraints();
        // // DEG-170 auto market - config add
        // updateAutoMarket_B.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateAccount_B" << std::endl;
        updateAccount_B.generate_r1cs_constraints();

        //--------UserC
        std::cout << "in TransactionGadget: before updateStorage_C_batch" << std::endl;
        updateStorage_C_batch.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceS_C" << std::endl;
        updateBalanceS_C.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceB_C" << std::endl;
        updateBalanceB_C.generate_r1cs_constraints();
        // split trading fee and gas fee
        std::cout << "in TransactionGadget: before updateBalanceFee_C" << std::endl;
        updateBalanceFee_C.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateAccount_C" << std::endl;
        updateAccount_C.generate_r1cs_constraints();

        //--------UserD
        std::cout << "in TransactionGadget: before updateStorage_D_batch" << std::endl;
        updateStorage_D_batch.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceS_D" << std::endl;
        updateBalanceS_D.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceB_D" << std::endl;
        updateBalanceB_D.generate_r1cs_constraints();
        // split trading fee and gas fee
        std::cout << "in TransactionGadget: before updateBalanceFee_D" << std::endl;
        updateBalanceFee_D.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateAccount_D" << std::endl;
        updateAccount_D.generate_r1cs_constraints();

        //--------UserE
        std::cout << "in TransactionGadget: before updateStorage_E_batch" << std::endl;
        updateStorage_E_batch.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceS_E" << std::endl;
        updateBalanceS_E.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceB_E" << std::endl;
        updateBalanceB_E.generate_r1cs_constraints();
        // split trading fee and gas fee
        std::cout << "in TransactionGadget: before updateBalanceFee_E" << std::endl;
        updateBalanceFee_E.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateAccount_E" << std::endl;
        updateAccount_E.generate_r1cs_constraints();

        //--------UserF
        std::cout << "in TransactionGadget: before updateStorage_F_batch" << std::endl;
        updateStorage_F_batch.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceS_F" << std::endl;
        updateBalanceS_F.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceB_F" << std::endl;
        updateBalanceB_F.generate_r1cs_constraints();
        // split trading fee and gas fee
        std::cout << "in TransactionGadget: before updateBalanceFee_F" << std::endl;
        updateBalanceFee_F.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateAccount_F" << std::endl;
        updateAccount_F.generate_r1cs_constraints();

        // Update Operator
        std::cout << "in TransactionGadget: before updateBalanceD_O" << std::endl;
        updateBalanceD_O.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceC_O" << std::endl;
        updateBalanceC_O.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceB_O" << std::endl;
        updateBalanceB_O.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateBalanceA_O" << std::endl;
        updateBalanceA_O.generate_r1cs_constraints();
        std::cout << "in TransactionGadget: before updateAccount_O" << std::endl;
        updateAccount_O.generate_r1cs_constraints();

        // // Update Protocol fee pool
        // updateBalanceC_P.generate_r1cs_constraints();
        // updateBalanceB_P.generate_r1cs_constraints();
        // updateBalanceA_P.generate_r1cs_constraints();
    }

    const VariableArrayT getPublicData() const
    {
        // return flatten({reverse(type.bits), tx.getPublicData()});
        // return flatten({reverse(typeBitsIntoPublicData.result()), tx.getPublicData()});
        return flatten({tx.getPublicData()});
    }

    const VariableT &getNewAccountsRoot() const
    {
        return updateAccount_O.result();
    }

    const VariableT &getNewAccountsAssetRoot() const
    {
        return updateAccount_O.assetResult();
    }

    // const VariableT &getNewProtocolBalancesRoot() const
    // {
    //     return updateBalanceA_P.result();
    // }
};

class UniversalCircuit : public Circuit
{
  public:
    PublicDataGadget publicData;
    Constants constants;
    jubjub::Params params;

    // State
    AccountGadget accountBefore_P;
    AccountGadget accountBefore_O;

    // Inputs
    DualVariableGadget exchange;
    DualVariableGadget merkleRootBefore;
    DualVariableGadget merkleRootAfter;
    DualVariableGadget merkleAssetRootBefore;
    DualVariableGadget merkleAssetRootAfter;
    DualVariableGadget timestamp;
    // DualVariableGadget protocolTakerFeeBips;
    // DualVariableGadget protocolMakerFeeBips;
    DualVariableGadget protocolFeeBips;
    std::unique_ptr<ToBitsGadget> numConditionalTransactions;
    DualVariableGadget operatorAccountID;

    // Increment the nonce of the Operator
    AddGadget nonce_after;

    // Signature
    Poseidon_2 hash;
    SignatureVerifier signatureVerifier;

    // Transactions
    unsigned int numTransactions;
    std::vector<TransactionGadget> transactions;

    // Update Protocol pool
    std::unique_ptr<UpdateAccountGadget> updateAccount_P;

    // Update Operator
    std::unique_ptr<UpdateAccountGadget> updateAccount_O;

    std::vector<DualVariableGadget> txTypes;

    std::vector<EqualGadget> isDeposit;
    std::vector<EqualGadget> isAccountUpdate;
    std::vector<EqualGadget> isWithdraw;
    // deposit or accountUpdate or withdraw
    std::vector<OrThreeGadget> isSpecialTransaction;
    std::vector<NotGadget> isOtherTransaction;


    std::vector<EqualGadget> depositSizeIsZero;
    std::vector<EqualGadget> accountUpdateSizeIsZero;
    std::vector<EqualGadget> otherTransactionSizeIsZero;
    std::vector<EqualGadget> withdrawSizeIsZero;

    // Conditions for deposit quantity accumulation
    // if transaction is deposit, then need match the condition, accountUpdateSize == 0 and withdrawSize == 0 and otherSize == 0
    std::vector<AndThreeGadget> depositCondition;
    // account update quantity accumulation require withdrawSize == 0 and otherSize == 0
    std::vector<AndTwoGadget> accountUpdateCondition;
    // other transaction quantity accumulation require withdrawSize == 0
    // Withdraw not need condition

    std::vector<IfThenRequireGadget> requireValidDeposit;
    std::vector<IfThenRequireGadget> requireValidAccountUpdate;
    std::vector<IfThenRequireGadget> requireValidOtherTransaction;

    std::vector<AddGadget> depositSizeAdd;
    std::vector<AddGadget> accountUpdateSizeAdd;
    std::vector<AddGadget> otherTransactionSizeAdd;
    std::vector<AddGadget> withdrawSizeAdd;

    std::unique_ptr<ToBitsGadget> depositSize;
    std::unique_ptr<ToBitsGadget> accountUpdateSize;
    std::unique_ptr<ToBitsGadget> withdrawSize;
    UniversalCircuit( //
      ProtoboardT &pb,
      const std::string &prefix)
        : Circuit(pb, prefix),

          publicData(pb, FMT(prefix, ".publicData")),
          constants(pb, FMT(prefix, ".constants")),

          // State
          accountBefore_P(pb, FMT(prefix, ".accountBefore_P")),
          accountBefore_O(pb, FMT(prefix, ".accountBefore_O")),

          // Inputs
          exchange(pb, NUM_BITS_ADDRESS, FMT(prefix, ".exchange")),
          merkleRootBefore(pb, 256, FMT(prefix, ".merkleRootBefore")),
          merkleRootAfter(pb, 256, FMT(prefix, ".merkleRootAfter")),
          merkleAssetRootBefore(pb, 256, FMT(prefix, ".merkleAssetRootBefore")),
          merkleAssetRootAfter(pb, 256, FMT(prefix, ".merkleAssetRootAfter")),
          timestamp(pb, NUM_BITS_TIMESTAMP, FMT(prefix, ".timestamp")),
          // protocolTakerFeeBips(pb, NUM_BITS_PROTOCOL_FEE_BIPS, FMT(prefix, ".protocolTakerFeeBips")),
          // protocolMakerFeeBips(pb, NUM_BITS_PROTOCOL_FEE_BIPS, FMT(prefix, ".protocolMakerFeeBips")),
          protocolFeeBips(pb, NUM_BITS_PROTOCOL_FEE_BIPS, FMT(prefix, ".protocolFeeBips")),
          operatorAccountID(pb, NUM_BITS_ACCOUNT, FMT(prefix, ".operatorAccountID")),

          // Increment the nonce of the Operator
          nonce_after(pb, accountBefore_O.nonce, constants._1, NUM_BITS_NONCE, FMT(prefix, ".nonce_after")),

          // Signature
          hash(pb, var_array({publicData.publicInput, accountBefore_O.nonce}), FMT(this->annotation_prefix, ".hash")),
          signatureVerifier(
            pb,
            params,
            constants,
            accountBefore_O.publicKey,
            hash.result(),
            constants._1,
            FMT(prefix, ".signatureVerifier"))
    {
    }

    void generateConstraints(unsigned int blockSize) override
    {
        this->numTransactions = blockSize;

        std::cout << "in generateConstraints: before constants" << std::endl;
        constants.generate_r1cs_constraints();

        std::cout << "in generateConstraints: before exchange" << std::endl;
        // Inputs
        exchange.generate_r1cs_constraints(true);
        merkleRootBefore.generate_r1cs_constraints(true);
        merkleRootAfter.generate_r1cs_constraints(true);
        merkleAssetRootBefore.generate_r1cs_constraints(true);
        merkleAssetRootAfter.generate_r1cs_constraints(true);
        timestamp.generate_r1cs_constraints(true);
        // protocolTakerFeeBips.generate_r1cs_constraints(true);
        // protocolMakerFeeBips.generate_r1cs_constraints(true);
        protocolFeeBips.generate_r1cs_constraints(true);
        operatorAccountID.generate_r1cs_constraints(true);

        std::cout << "in generateConstraints: before nonce_after" << std::endl;
        // Increment the nonce of the Operator
        nonce_after.generate_r1cs_constraints();

        // Transactions
        transactions.reserve(numTransactions);
        std::cout << "in generateConstraints: before transactions" << std::endl;
        for (size_t j = 0; j < numTransactions; j++)
        {
            std::cout << "------------------- tx: " << j << std::endl;
            txTypes.emplace_back(pb, NUM_BITS_TX_TYPE_FOR_SELECT, FMT(annotation_prefix, ".txTypes"));
            txTypes.back().generate_r1cs_constraints();

            isDeposit.emplace_back(pb, txTypes.back().packed, constants.depositType, FMT(annotation_prefix, ".isDeposit"));
            isDeposit.back().generate_r1cs_constraints();

            isAccountUpdate.emplace_back(pb, txTypes.back().packed, constants.accountUpdateType, FMT(annotation_prefix, ".isAccountUpdate"));
            isAccountUpdate.back().generate_r1cs_constraints();
            isWithdraw.emplace_back(pb, txTypes.back().packed, constants.withdrawType, FMT(annotation_prefix, ".isWithdraw"));
            isWithdraw.back().generate_r1cs_constraints();

            isSpecialTransaction.emplace_back(pb, isDeposit.back().result(), isAccountUpdate.back().result(), isWithdraw.back().result(), FMT(annotation_prefix, ".isSpecialTransaction"));
            isSpecialTransaction.back().generate_r1cs_constraints();
            isOtherTransaction.emplace_back(pb, isSpecialTransaction.back().result(), FMT(annotation_prefix, ".isOtherTransaction"));
            isOtherTransaction.back().generate_r1cs_constraints();

            depositSizeIsZero.emplace_back(pb, (j == 0) ? constants._0 : depositSizeAdd.back().result(), constants._0, FMT(annotation_prefix, ".depositSizeIsZero"));
            depositSizeIsZero.back().generate_r1cs_constraints();
            accountUpdateSizeIsZero.emplace_back(pb, (j == 0) ? constants._0 : accountUpdateSizeAdd.back().result(), constants._0, FMT(annotation_prefix, ".accountUpdateSizeIsZero"));
            accountUpdateSizeIsZero.back().generate_r1cs_constraints();
            otherTransactionSizeIsZero.emplace_back(pb, (j == 0) ? constants._0 : otherTransactionSizeAdd.back().result(), constants._0, FMT(annotation_prefix, ".otherTransactionSizeIsZero"));
            otherTransactionSizeIsZero.back().generate_r1cs_constraints();
            withdrawSizeIsZero.emplace_back(pb, (j == 0) ? constants._0 : withdrawSizeAdd.back().result(), constants._0, FMT(annotation_prefix, ".withdrawSizeIsZero"));
            withdrawSizeIsZero.back().generate_r1cs_constraints();

            depositCondition.emplace_back(pb, accountUpdateSizeIsZero.back().result(), otherTransactionSizeIsZero.back().result(), withdrawSizeIsZero.back().result(), FMT(annotation_prefix, ".depositCondition"));
            depositCondition.back().generate_r1cs_constraints();
            accountUpdateCondition.emplace_back(pb, otherTransactionSizeIsZero.back().result(), withdrawSizeIsZero.back().result(), FMT(annotation_prefix, ".accountUpdateCondition"));
            accountUpdateCondition.back().generate_r1cs_constraints();

            requireValidDeposit.emplace_back(pb, isDeposit.back().result(), depositCondition.back().result(), FMT(annotation_prefix, ".requireValidDeposit"));
            requireValidDeposit.back().generate_r1cs_constraints();
            requireValidAccountUpdate.emplace_back(pb, isAccountUpdate.back().result(), accountUpdateCondition.back().result(), FMT(annotation_prefix, ".requireValidAccountUpdate"));
            requireValidAccountUpdate.back().generate_r1cs_constraints();
            requireValidOtherTransaction.emplace_back(pb, isOtherTransaction.back().result(), withdrawSizeIsZero.back().result(), FMT(annotation_prefix, ".requireValidOtherTransaction"));
            requireValidOtherTransaction.back().generate_r1cs_constraints();

            depositSizeAdd.emplace_back(pb, (j == 0) ? constants._0 : depositSizeAdd.back().result(), isDeposit.back().result(), NUM_BITS_TX_SIZE, FMT(annotation_prefix, ".depositSizeAdd"));
            depositSizeAdd.back().generate_r1cs_constraints();
            accountUpdateSizeAdd.emplace_back(pb, (j == 0) ? constants._0 : accountUpdateSizeAdd.back().result(), isAccountUpdate.back().result(), NUM_BITS_TX_SIZE, FMT(annotation_prefix, ".accountUpdateSizeAdd"));
            accountUpdateSizeAdd.back().generate_r1cs_constraints();
            otherTransactionSizeAdd.emplace_back(pb, (j == 0) ? constants._0 : otherTransactionSizeAdd.back().result(), isOtherTransaction.back().result(), NUM_BITS_TX_SIZE, FMT(annotation_prefix, ".otherTransactionSizeAdd"));
            otherTransactionSizeAdd.back().generate_r1cs_constraints();
            withdrawSizeAdd.emplace_back(pb, (j == 0) ? constants._0 : withdrawSizeAdd.back().result(), isWithdraw.back().result(), NUM_BITS_TX_SIZE, FMT(annotation_prefix, ".withdrawSizeAdd"));
            withdrawSizeAdd.back().generate_r1cs_constraints();


            const VariableT txAccountsRoot =
              (j == 0) ? merkleRootBefore.packed : transactions.back().getNewAccountsRoot();
            const VariableT txAccountsAssetRoot =
              (j == 0) ? merkleAssetRootBefore.packed : transactions.back().getNewAccountsAssetRoot();
            // const VariableT &txProtocolBalancesRoot =
            //   (j == 0) ? accountBefore_P.balancesRoot : transactions.back().getNewProtocolBalancesRoot();
            std::cout << "------------------- before transactions" << std::endl;
            transactions.emplace_back(
              pb,
              params,
              constants,
              exchange.packed,
              txAccountsRoot,
              txAccountsAssetRoot,
              timestamp.packed,
              // protocolTakerFeeBips.packed,
              // protocolMakerFeeBips.packed,
              protocolFeeBips.packed,
              operatorAccountID.bits,
              // txProtocolBalancesRoot,
              (j == 0) ? constants._0 : transactions.back().tx.getOutput(TXV_NUM_CONDITIONAL_TXS),
              std::string("tx_") + std::to_string(j));
            std::cout << "------------------- after transactions" << std::endl;
            transactions.back().generate_r1cs_constraints();
        }

        depositSize.reset(new ToBitsGadget(pb, depositSizeAdd.back().result(), NUM_BITS_TX_SIZE, FMT(annotation_prefix, ".depositSize")));
        depositSize->generate_r1cs_constraints();
        accountUpdateSize.reset(new ToBitsGadget(pb, accountUpdateSizeAdd.back().result(), NUM_BITS_TX_SIZE, FMT(annotation_prefix, ".accountUpdateSize")));
        accountUpdateSize->generate_r1cs_constraints();
        withdrawSize.reset(new ToBitsGadget(pb, withdrawSizeAdd.back().result(), NUM_BITS_TX_SIZE, FMT(annotation_prefix, ".withdrawSize")));
        withdrawSize->generate_r1cs_constraints();
        std::cout << "in generateConstraints: before updateAccount_P" << std::endl;
        // Update Protocol pool
        updateAccount_P.reset(new UpdateAccountGadget(
          pb,
          transactions.back().getNewAccountsRoot(),
          transactions.back().getNewAccountsAssetRoot(),
          constants.zeroAccount,
          {accountBefore_P.owner,
           accountBefore_P.publicKey.x,
           accountBefore_P.publicKey.y,
           accountBefore_P.appKeyPublicKey.x,
           accountBefore_P.appKeyPublicKey.y,
           accountBefore_P.nonce,
           accountBefore_P.disableAppKeySpotTrade,
           accountBefore_P.disableAppKeyWithdraw,
           accountBefore_P.disableAppKeyTransferToOther,
           accountBefore_P.balancesRoot,
          //  // DEG-170 auto market - config add
          //  accountBefore_P.autoMarketRoot,
           // DEG-347 Storage location change
           accountBefore_P.storageRoot},
          {accountBefore_P.owner,
           accountBefore_P.publicKey.x,
           accountBefore_P.publicKey.y,
           accountBefore_P.appKeyPublicKey.x,
           accountBefore_P.appKeyPublicKey.y,
           accountBefore_P.nonce,
           accountBefore_P.disableAppKeySpotTrade,
           accountBefore_P.disableAppKeyWithdraw,
           accountBefore_P.disableAppKeyTransferToOther,
           accountBefore_P.balancesRoot,
          //  // DEG-170 auto market - config add
          //  accountBefore_P.autoMarketRoot,
           // DEG-347 Storage location change
           accountBefore_P.storageRoot},
          FMT(annotation_prefix, ".updateAccount_P")));
        updateAccount_P->generate_r1cs_constraints();

        std::cout << "in generateConstraints: before updateAccount_O" << std::endl;
        // Update Operator
        updateAccount_O.reset(new UpdateAccountGadget(
          pb,
          updateAccount_P->result(),
          updateAccount_P->assetResult(),
          operatorAccountID.bits,
          {accountBefore_O.owner,
           accountBefore_O.publicKey.x,
           accountBefore_O.publicKey.y,
           accountBefore_O.appKeyPublicKey.x,
           accountBefore_O.appKeyPublicKey.y,
           accountBefore_O.nonce,
           accountBefore_O.disableAppKeySpotTrade,
           accountBefore_O.disableAppKeyWithdraw,
           accountBefore_O.disableAppKeyTransferToOther,
           accountBefore_O.balancesRoot,
          // DEG-265 auto market
          //  // DEG-170 auto market - config add
          //  accountBefore_O.autoMarketRoot,
           // DEG-347 Storage location change
           accountBefore_O.storageRoot},
          {accountBefore_O.owner,
           accountBefore_O.publicKey.x,
           accountBefore_O.publicKey.y,
           accountBefore_O.appKeyPublicKey.x,
           accountBefore_O.appKeyPublicKey.y,
           nonce_after.result(),
           accountBefore_O.disableAppKeySpotTrade,
           accountBefore_O.disableAppKeyWithdraw,
           accountBefore_O.disableAppKeyTransferToOther,
           accountBefore_O.balancesRoot,
          // DEG-265 auto market
          //  // DEG-170 auto market - config add
          //  accountBefore_O.autoMarketRoot,
           // DEG-347 Storage location change
           accountBefore_O.storageRoot},
          FMT(annotation_prefix, ".updateAccount_O")));
        updateAccount_O->generate_r1cs_constraints();

        // Num conditional transactions
        numConditionalTransactions.reset(new ToBitsGadget(
          pb, transactions.back().tx.getOutput(TXV_NUM_CONDITIONAL_TXS), 32, ".numConditionalTransactions"));
        numConditionalTransactions->generate_r1cs_constraints();

        std::cout << "in generateConstraints: before publicData" << std::endl;
        // Public data
        publicData.add(exchange.bits);
        publicData.add(merkleRootBefore.bits);
        publicData.add(merkleRootAfter.bits);
        publicData.add(merkleAssetRootBefore.bits);
        publicData.add(merkleAssetRootAfter.bits);
        publicData.add(timestamp.bits);
        // publicData.add(protocolTakerFeeBips.bits);
        // publicData.add(protocolMakerFeeBips.bits);
        publicData.add(protocolFeeBips.bits);
        publicData.add(numConditionalTransactions->bits);
        publicData.add(operatorAccountID.bits);
        publicData.add(depositSize->bits);
        publicData.add(accountUpdateSize->bits);
        publicData.add(withdrawSize->bits);
        unsigned int start = publicData.publicDataBits.size();
        for (size_t j = 0; j < numTransactions; j++)
        {
            publicData.add(reverse(transactions[j].getPublicData()));
        }
        publicData.transform(start, numTransactions, TX_DATA_AVAILABILITY_SIZE * 8);
        publicData.generate_r1cs_constraints();

        std::cout << "in generateConstraints: before hash" << std::endl;
        // Signature
        hash.generate_r1cs_constraints();
        std::cout << "in generateConstraints: before signatureVerifier" << std::endl;
        signatureVerifier.generate_r1cs_constraints();

        std::cout << "in generateConstraints: before requireEqual" << std::endl;
        // Check the new merkle root
        requireEqual(pb, updateAccount_O->result(), merkleRootAfter.packed, "newMerkleRoot");
        requireEqual(pb, updateAccount_O->assetResult(), merkleAssetRootAfter.packed, "newMerkleAssetRoot");
        std::cout << "in generateConstraints: after requireEqual" << std::endl;
    }

    bool generateWitness(const Block &block)
    {
        std::cout << "in generateWitness: " << std::endl;
        if (block.transactions.size() != numTransactions)
        {
            std::cout << "Invalid number of transactions: " << block.transactions.size() << std::endl;
            return false;
        }

        std::cout << "in generateWitness: before constants" << std::endl;
        constants.generate_r1cs_witness();

        std::cout << "in generateWitness: before account p and o before" << std::endl;
        // State
        accountBefore_P.generate_r1cs_witness(block.accountUpdate_P.before);
        accountBefore_O.generate_r1cs_witness(block.accountUpdate_O.before);

        std::cout << "in generateWitness: before exchange" << std::endl;
        // Inputs
        exchange.generate_r1cs_witness(pb, block.exchange);
        merkleRootBefore.generate_r1cs_witness(pb, block.merkleRootBefore);
        merkleRootAfter.generate_r1cs_witness(pb, block.merkleRootAfter);
        merkleAssetRootBefore.generate_r1cs_witness(pb, block.merkleAssetRootBefore);
        merkleAssetRootAfter.generate_r1cs_witness(pb, block.merkleAssetRootAfter);
        timestamp.generate_r1cs_witness(pb, block.timestamp);
        // protocolTakerFeeBips.generate_r1cs_witness(pb, block.protocolTakerFeeBips);
        // protocolMakerFeeBips.generate_r1cs_witness(pb, block.protocolMakerFeeBips);
        protocolFeeBips.generate_r1cs_witness(pb, block.protocolFeeBips);
        operatorAccountID.generate_r1cs_witness(pb, block.operatorAccountID);

        // Increment the nonce of the Operator
        nonce_after.generate_r1cs_witness();

        // Transactions
        // First set numConditionalTransactionsAfter which is a dependency between
        // transactions. Once this is set the transactions can be processed in
        // parallel.
        std::cout << "in generateWitness: before transactions" << std::endl;
        for (unsigned int i = 0; i < block.transactions.size(); i++)
        {
            pb.val(transactions[i].tx.getOutput(TXV_NUM_CONDITIONAL_TXS)) =
              block.transactions[i].witness.numConditionalTransactionsAfter;
            
            txTypes[i].generate_r1cs_witness(pb, block.transactions[i].type);

            isDeposit[i].generate_r1cs_witness();
            isAccountUpdate[i].generate_r1cs_witness();
            isWithdraw[i].generate_r1cs_witness();

            isSpecialTransaction[i].generate_r1cs_witness();
            isOtherTransaction[i].generate_r1cs_witness();

            depositSizeIsZero[i].generate_r1cs_witness();
            accountUpdateSizeIsZero[i].generate_r1cs_witness();
            otherTransactionSizeIsZero[i].generate_r1cs_witness();
            withdrawSizeIsZero[i].generate_r1cs_witness();

            depositCondition[i].generate_r1cs_witness();
            accountUpdateCondition[i].generate_r1cs_witness();

            requireValidDeposit[i].generate_r1cs_witness();
            requireValidAccountUpdate[i].generate_r1cs_witness();
            requireValidOtherTransaction[i].generate_r1cs_witness();

            depositSizeAdd[i].generate_r1cs_witness();
            accountUpdateSizeAdd[i].generate_r1cs_witness();
            otherTransactionSizeAdd[i].generate_r1cs_witness();
            withdrawSizeAdd[i].generate_r1cs_witness();
        }
#ifdef MULTICORE
#pragma omp parallel for
#endif
        for (unsigned int i = 0; i < block.transactions.size(); i++)
        {
            std::cout << "--------------- tx: " << i << " ( " << block.transactions[i].type << " ) " << std::endl;
            transactions[i].generate_r1cs_witness(block.transactions[i]);
        }
        depositSize->generate_r1cs_witness();
        accountUpdateSize->generate_r1cs_witness();
        withdrawSize->generate_r1cs_witness();

        std::cout << "in generateWitness: before account p update" << std::endl;
        // Update Protocol pool
        updateAccount_P->generate_r1cs_witness(block.accountUpdate_P);

        std::cout << "in generateWitness: before account o update" << std::endl;
        // Update Operator
        updateAccount_O->generate_r1cs_witness(block.accountUpdate_O);

        // Num conditional transactions
        numConditionalTransactions->generate_r1cs_witness_from_packed();

        // Public data
        publicData.generate_r1cs_witness();

        // Signature
        hash.generate_r1cs_witness();
        signatureVerifier.generate_r1cs_witness(block.signature);

        return true;
    }

    bool generateWitness(const json &input) override
    {
        std::cout << "in generateWitness: before blcok" << std::endl;
        return generateWitness(input.get<Block>());
    }

    unsigned int getBlockType() override
    {
        return 0;
    }

    unsigned int getBlockSize() override
    {
        return numTransactions;
    }

    void printInfo() override
    {
        std::cout << pb.num_constraints() << " constraints (" << (pb.num_constraints() / numTransactions) << "/tx)"
                  << ";num_variables:" << pb.num_variables() << ";num_inputs:" << pb.num_inputs() << std::endl;
    }
};

} // namespace Loopring

#endif
