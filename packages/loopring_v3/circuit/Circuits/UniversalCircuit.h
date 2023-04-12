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
    std::vector<ArraySelectGadget> aSelects;
    std::vector<ArraySelectGadget> publicDataSelects;
    
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

        // Array outputs
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

        // Public data
        {
            std::vector<VariableArrayT> variables;
            for (unsigned int i = 0; i < transactions.size(); i++)
            {
                VariableArrayT da = transactions[i]->getPublicData();
                assert(da.size() <= (TX_DATA_AVAILABILITY_SIZE) * 8);
                // Pad with zeros if needed
                for (unsigned int j = da.size(); j < (TX_DATA_AVAILABILITY_SIZE) * 8; j++)
                {
                    da.emplace_back(state.constants._0);
                }
                variables.push_back(da);
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
        return publicDataSelects.back().result();
    }
};

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
      for (size_t i = 0; i < account.storageArray.size(); i++) 
      {
        updateStorages[i].generate_r1cs_witness(storageUpdateArray[i]);
      }
    }
    void generate_r1cs_constraints() 
    {
      for (size_t i = 0; i < updateStorages.size(); i++) 
      {
        updateStorages[i].generate_r1cs_constraints();
      }
    }
    const VariableT getHashRoot() const
    {
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
    }
    void generate_r1cs_witness(std::vector<StorageUpdate> storageUpdateArray) 
    {
      for (size_t i = 0; i < account.storageArray.size(); i++) 
      {
        updateStorages[i].generate_r1cs_witness(storageUpdateArray[i]);
      }
    }
    void generate_r1cs_constraints() 
    {
      for (size_t i = 0; i < updateStorages.size(); i++) 
      {
        updateStorages[i].generate_r1cs_constraints();
      }
    }
    const VariableT getHashRoot() const
    {
        return updateStorages.back().result();
    }
};

class BatchStorageCUpdateGadget : public GadgetT 
{
  public:
    SelectTransactionGadget tx;
    BaseTransactionAccountState account;
    VariableT storageRoot;
    std::vector<UpdateStorageGadget> updateStorages;
    
    BatchStorageCUpdateGadget(
      ProtoboardT &pb,
      const SelectTransactionGadget &_tx,
      const BaseTransactionAccountState &_account,
      const VariableT &_storageRoot,
      const std::string &prefix)
        : GadgetT(pb, prefix),
        tx(_tx),
        account(_account),
        storageRoot(_storageRoot)
    {
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
      for (size_t i = 0; i < account.storageArray.size(); i++) 
      {
        updateStorages[i].generate_r1cs_witness(storageUpdateArray[i]);
      }
    }
    void generate_r1cs_constraints() 
    {
      for (size_t i = 0; i < updateStorages.size(); i++) 
      {
        updateStorages[i].generate_r1cs_constraints();
      }
    }
    const VariableT getHashRoot() const
    {
        return updateStorages.back().result();
    }
};

class BatchStorageDUpdateGadget : public GadgetT 
{
  public:
    SelectTransactionGadget tx;
    BaseTransactionAccountState account;
    VariableT storageRoot;
    std::vector<UpdateStorageGadget> updateStorages;
    
    BatchStorageDUpdateGadget(
      ProtoboardT &pb,
      const SelectTransactionGadget &_tx,
      const BaseTransactionAccountState &_account,
      const VariableT &_storageRoot,
      const std::string &prefix)
        : GadgetT(pb, prefix),
        tx(_tx),
        account(_account),
        storageRoot(_storageRoot)
    {
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
      for (size_t i = 0; i < account.storageArray.size(); i++) 
      {
        updateStorages[i].generate_r1cs_witness(storageUpdateArray[i]);
      }
    }
    void generate_r1cs_constraints() 
    {
      for (size_t i = 0; i < updateStorages.size(); i++) 
      {
        updateStorages[i].generate_r1cs_constraints();
      }
    }
    const VariableT getHashRoot() const
    {
        return updateStorages.back().result();
    }
};

class BatchStorageEUpdateGadget : public GadgetT 
{
  public:
    SelectTransactionGadget tx;
    BaseTransactionAccountState account;
    VariableT storageRoot;
    std::vector<UpdateStorageGadget> updateStorages;
    
    BatchStorageEUpdateGadget(
      ProtoboardT &pb,
      const SelectTransactionGadget &_tx,
      const BaseTransactionAccountState &_account,
      const VariableT &_storageRoot,
      const std::string &prefix)
        : GadgetT(pb, prefix),
        tx(_tx),
        account(_account),
        storageRoot(_storageRoot)
    {
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
      for (size_t i = 0; i < account.storageArray.size(); i++) 
      {
        updateStorages[i].generate_r1cs_witness(storageUpdateArray[i]);
      }
    }
    void generate_r1cs_constraints() 
    {
      for (size_t i = 0; i < updateStorages.size(); i++) 
      {
        updateStorages[i].generate_r1cs_constraints();
      }
    }
    const VariableT getHashRoot() const
    {
        return updateStorages.back().result();
    }
};

class BatchStorageFUpdateGadget : public GadgetT 
{
  public:
    SelectTransactionGadget tx;
    BaseTransactionAccountState account;
    VariableT storageRoot;
    std::vector<UpdateStorageGadget> updateStorages;
    
    BatchStorageFUpdateGadget(
      ProtoboardT &pb,
      const SelectTransactionGadget &_tx,
      const BaseTransactionAccountState &_account,
      const VariableT &_storageRoot,
      const std::string &prefix)
        : GadgetT(pb, prefix),
        tx(_tx),
        account(_account),
        storageRoot(_storageRoot)
    {
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
      for (size_t i = 0; i < account.storageArray.size(); i++) 
      {
        updateStorages[i].generate_r1cs_witness(storageUpdateArray[i]);
      }
    }
    void generate_r1cs_constraints() 
    {
      for (size_t i = 0; i < updateStorages.size(); i++) 
      {
        updateStorages[i].generate_r1cs_constraints();
      }
    }
    const VariableT getHashRoot() const
    {
        return updateStorages.back().result();
    }
};

class TransactionGadget : public GadgetT
{
  public:
    const Constants &constants;

    // DualVariableGadget type;
    SelectorGadget selector;

    TransactionState state;

    // Process transaction
    NoopCircuit noop;
    SpotTradeCircuit spotTrade;
    DepositCircuit deposit;
    WithdrawCircuit withdraw;
    AccountUpdateCircuit accountUpdate;
    TransferCircuit transfer;
    OrderCancelCircuit orderCancel;
    AppKeyUpdateCircuit appKeyUpdate;

    BatchSpotTradeCircuit batchSpotTrade;

    SelectTransactionGadget tx;

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
    BatchStorageAUpdateGadget updateStorage_A_batch;
    UpdateBalanceGadget updateBalanceS_A;
    UpdateBalanceGadget updateBalanceB_A;
    UpdateBalanceGadget updateBalanceFee_A;
    UpdateAccountGadget updateAccount_A;

    // Update UserB
    UpdateStorageGadget updateStorage_B;
    BatchStorageBUpdateGadget updateStorage_B_batch;
    UpdateBalanceGadget updateBalanceS_B;
    UpdateBalanceGadget updateBalanceB_B;
    UpdateBalanceGadget updateBalanceFee_B;
    UpdateAccountGadget updateAccount_B;

    //------UserC
    BatchStorageCUpdateGadget updateStorage_C_batch;
    UpdateBalanceGadget updateBalanceS_C;
    UpdateBalanceGadget updateBalanceB_C;
    UpdateBalanceGadget updateBalanceFee_C;
    UpdateAccountGadget updateAccount_C;
    //------UserD
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
      const VariableT type,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          constants(_constants),

          selector(pb, constants, type, (unsigned int)TransactionType::COUNT, FMT(prefix, ".selector")),

          state(
            pb,
            params,
            constants,
            exchange,
            timestamp,
            protocolFeeBips,
            numConditionalTransactionsBefore,
            type,
            FMT(prefix, ".transactionState")),

          // Process transaction
          noop(pb, state, FMT(prefix, ".noop")),
          spotTrade(pb, state, FMT(prefix, ".spotTrade")),
          deposit(pb, state, FMT(prefix, ".deposit")),
          withdraw(pb, state, FMT(prefix, ".withdraw")),
          accountUpdate(pb, state, FMT(prefix, ".accountUpdate")),
          transfer(pb, state, FMT(prefix, ".transfer")),
          orderCancel(pb, state, FMT(prefix, ".orderCancel")),
          appKeyUpdate(pb, state, FMT(prefix, ".appKeyUpdate")),
          batchSpotTrade(pb, state, FMT(prefix, ".batchSpotTrade")),

          tx(
            pb,
            state,
            selector.result(),
            {&noop, &transfer, &spotTrade, &orderCancel, &appKeyUpdate, &batchSpotTrade, 
            &deposit, &accountUpdate, &withdraw},
            FMT(prefix, ".tx")),

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
            state.accountA.account.storageRoot,
            tx.getArrayOutput(TXV_STORAGE_A_ADDRESS),
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
            {state.accountA.balanceS.balance},
            {tx.getOutput(TXV_BALANCE_A_S_BALANCE)},
            FMT(prefix, ".updateBalanceS_A")),
          updateBalanceB_A(
            pb,
            updateBalanceS_A.result(),
            tx.getArrayOutput(TXV_BALANCE_A_B_ADDRESS),
            {state.accountA.balanceB.balance},
            {tx.getOutput(TXV_BALANCE_A_B_BALANCE)},
            FMT(prefix, ".updateBalanceB_A")),
          updateBalanceFee_A(
            pb,
            updateBalanceB_A.result(),
            tx.getArrayOutput(TXV_BALANCE_A_FEE_Address),
            {state.accountA.balanceFee.balance},
            {tx.getOutput(TXV_BALANCE_A_FEE_BALANCE)},
            FMT(prefix, ".updateBalanceFee_A")),
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
             updateBalanceFee_A.result(),
             updateStorage_A_batch.getHashRoot()},
            FMT(prefix, ".updateAccount_A")),

          // Update UserB
          updateStorage_B(
            pb,
            state.accountB.account.storageRoot,
            tx.getArrayOutput(TXV_STORAGE_B_ADDRESS),
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
            {state.accountB.balanceS.balance},
            {tx.getOutput(TXV_BALANCE_B_S_BALANCE)},
            FMT(prefix, ".updateBalanceS_B")),
          updateBalanceB_B(
            pb,
            updateBalanceS_B.result(),
            tx.getArrayOutput(TXV_BALANCE_B_B_ADDRESS),
            {state.accountB.balanceB.balance},
            {tx.getOutput(TXV_BALANCE_B_B_BALANCE)},
            FMT(prefix, ".updateBalanceB_B")),
          updateBalanceFee_B(
            pb,
            updateBalanceB_B.result(),
            tx.getArrayOutput(TXV_BALANCE_B_FEE_Address),
            {state.accountB.balanceFee.balance},
            {tx.getOutput(TXV_BALANCE_B_FEE_BALANCE)},
            FMT(prefix, ".updateBalanceFee_B")),
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
             updateBalanceFee_B.result(),
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
            updateBalanceC_O.result(),
            tx.getArrayOutput(TXV_BALANCE_O_B_Address),
            {state.oper.balanceB.balance},
            {tx.getOutput(TXV_BALANCE_O_B_BALANCE)},
            FMT(prefix, ".updateBalanceB_O")),
          updateBalanceA_O(
            pb,
            updateBalanceB_O.result(),
            tx.getArrayOutput(TXV_BALANCE_O_A_Address),
            {state.oper.balanceA.balance},
            {tx.getOutput(TXV_BALANCE_O_A_BALANCE)},
            FMT(prefix, ".updateBalanceA_O")),
          updateAccount_O(
            pb,
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
             state.oper.account.storageRoot},
            FMT(prefix, ".updateAccount_O"))

    {
      
    }

    void generate_r1cs_witness(const UniversalTransaction &uTx)
    {
        selector.generate_r1cs_witness();

        state.generate_r1cs_witness(
          uTx.witness.accountUpdate_A.before,
          uTx.witness.balanceUpdateS_A.before,
          uTx.witness.balanceUpdateB_A.before,
          uTx.witness.balanceUpdateFee_A.before,
          uTx.witness.storageUpdate_A.before,
          uTx.witness.storageUpdate_A_array,
          uTx.witness.accountUpdate_B.before,
          uTx.witness.balanceUpdateS_B.before,
          uTx.witness.balanceUpdateB_B.before,
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
          uTx.witness.accountUpdate_O.before,
          uTx.witness.balanceUpdateA_O.before,
          uTx.witness.balanceUpdateB_O.before,
          uTx.witness.balanceUpdateC_O.before,
          uTx.witness.balanceUpdateD_O.before
          );

        noop.generate_r1cs_witness();
        spotTrade.generate_r1cs_witness(uTx.spotTrade);
        deposit.generate_r1cs_witness(uTx.deposit);
        withdraw.generate_r1cs_witness(uTx.withdraw);
        accountUpdate.generate_r1cs_witness(uTx.accountUpdate);
        transfer.generate_r1cs_witness(uTx.transfer);
        orderCancel.generate_r1cs_witness(uTx.orderCancel);
        appKeyUpdate.generate_r1cs_witness(uTx.appKeyUpdate);
        batchSpotTrade.generate_r1cs_witness(uTx.batchSpotTrade);
        tx.generate_r1cs_witness();


        // Check signatures
        signatureVerifierA.generate_r1cs_witness(uTx.witness.signatureA);
        signatureVerifierB.generate_r1cs_witness(uTx.witness.signatureB);

        batchSignatureVerifierA.generate_r1cs_witness(uTx.witness.signatureArray[0]);
        batchSignatureVerifierB.generate_r1cs_witness(uTx.witness.signatureArray[1]);
        batchSignatureVerifierC.generate_r1cs_witness(uTx.witness.signatureArray[2]);
        batchSignatureVerifierD.generate_r1cs_witness(uTx.witness.signatureArray[3]);
        batchSignatureVerifierE.generate_r1cs_witness(uTx.witness.signatureArray[4]);
        batchSignatureVerifierF.generate_r1cs_witness(uTx.witness.signatureArray[5]);
        // Update UserA
        updateStorage_A.generate_r1cs_witness(uTx.witness.storageUpdate_A);

        // batch spot trade Storage
        updateStorage_A_batch.generate_r1cs_witness(uTx.witness.storageUpdate_A_array);

        updateBalanceS_A.generate_r1cs_witness(uTx.witness.balanceUpdateS_A);
        updateBalanceB_A.generate_r1cs_witness(uTx.witness.balanceUpdateB_A);
        updateBalanceFee_A.generate_r1cs_witness(uTx.witness.balanceUpdateFee_A);
        updateAccount_A.generate_r1cs_witness(uTx.witness.accountUpdate_A);

        // Update UserB
        updateStorage_B.generate_r1cs_witness(uTx.witness.storageUpdate_B);

        // batch spot trade Storage
        updateStorage_B_batch.generate_r1cs_witness(uTx.witness.storageUpdate_B_array);
        updateBalanceS_B.generate_r1cs_witness(uTx.witness.balanceUpdateS_B);
        updateBalanceB_B.generate_r1cs_witness(uTx.witness.balanceUpdateB_B);
        updateBalanceFee_B.generate_r1cs_witness(uTx.witness.balanceUpdateFee_B);
        updateAccount_B.generate_r1cs_witness(uTx.witness.accountUpdate_B);

        //---------UserC
        // Update UserC
        // batch spot trade Storage
        updateStorage_C_batch.generate_r1cs_witness(uTx.witness.storageUpdate_C_array);

        updateBalanceS_C.generate_r1cs_witness(uTx.witness.balanceUpdateS_C);
        updateBalanceB_C.generate_r1cs_witness(uTx.witness.balanceUpdateB_C);
        updateBalanceFee_C.generate_r1cs_witness(uTx.witness.balanceUpdateFee_C);
        updateAccount_C.generate_r1cs_witness(uTx.witness.accountUpdate_C);

        //---------UserD
        // Update UserD
        // batch spot trade Storage
        updateStorage_D_batch.generate_r1cs_witness(uTx.witness.storageUpdate_D_array);

        updateBalanceS_D.generate_r1cs_witness(uTx.witness.balanceUpdateS_D);
        updateBalanceB_D.generate_r1cs_witness(uTx.witness.balanceUpdateB_D);
        updateBalanceFee_D.generate_r1cs_witness(uTx.witness.balanceUpdateFee_D);
        updateAccount_D.generate_r1cs_witness(uTx.witness.accountUpdate_D);

        //---------UserE
        // Update UserE
        // batch spot trade Storage
        updateStorage_E_batch.generate_r1cs_witness(uTx.witness.storageUpdate_E_array);

        updateBalanceS_E.generate_r1cs_witness(uTx.witness.balanceUpdateS_E);
        updateBalanceB_E.generate_r1cs_witness(uTx.witness.balanceUpdateB_E);
        updateBalanceFee_E.generate_r1cs_witness(uTx.witness.balanceUpdateFee_E);
        updateAccount_E.generate_r1cs_witness(uTx.witness.accountUpdate_E);

        //---------UserF
        // Update UserF
        // batch spot trade Storage
        updateStorage_F_batch.generate_r1cs_witness(uTx.witness.storageUpdate_F_array);

        updateBalanceS_F.generate_r1cs_witness(uTx.witness.balanceUpdateS_F);
        updateBalanceB_F.generate_r1cs_witness(uTx.witness.balanceUpdateB_F);
        updateBalanceFee_F.generate_r1cs_witness(uTx.witness.balanceUpdateFee_F);
        updateAccount_F.generate_r1cs_witness(uTx.witness.accountUpdate_F);

        // Update Operator
        updateBalanceD_O.generate_r1cs_witness(uTx.witness.balanceUpdateD_O);
        updateBalanceC_O.generate_r1cs_witness(uTx.witness.balanceUpdateC_O);
        updateBalanceB_O.generate_r1cs_witness(uTx.witness.balanceUpdateB_O);
        updateBalanceA_O.generate_r1cs_witness(uTx.witness.balanceUpdateA_O);
        updateAccount_O.generate_r1cs_witness(uTx.witness.accountUpdate_O);

    }

    void generate_r1cs_constraints()
    {
        selector.generate_r1cs_constraints();

        noop.generate_r1cs_constraints();
        spotTrade.generate_r1cs_constraints();
        deposit.generate_r1cs_constraints();
        withdraw.generate_r1cs_constraints();
        accountUpdate.generate_r1cs_constraints();
        transfer.generate_r1cs_constraints();
        orderCancel.generate_r1cs_constraints();
        appKeyUpdate.generate_r1cs_constraints();

        batchSpotTrade.generate_r1cs_constraints();
        tx.generate_r1cs_constraints();

        // Check signatures
        signatureVerifierA.generate_r1cs_constraints();
        signatureVerifierB.generate_r1cs_constraints();

        batchSignatureVerifierA.generate_r1cs_constraints();
        batchSignatureVerifierB.generate_r1cs_constraints();
        batchSignatureVerifierC.generate_r1cs_constraints();
        batchSignatureVerifierD.generate_r1cs_constraints();
        batchSignatureVerifierE.generate_r1cs_constraints();
        batchSignatureVerifierF.generate_r1cs_constraints();

        // Update UserA
        updateStorage_A.generate_r1cs_constraints();
        updateStorage_A_batch.generate_r1cs_constraints();
        updateBalanceS_A.generate_r1cs_constraints();
        updateBalanceB_A.generate_r1cs_constraints();
        updateBalanceFee_A.generate_r1cs_constraints();
        updateAccount_A.generate_r1cs_constraints();

        // Update UserB
        updateStorage_B.generate_r1cs_constraints();
        updateStorage_B_batch.generate_r1cs_constraints();
        updateBalanceS_B.generate_r1cs_constraints();
        updateBalanceB_B.generate_r1cs_constraints();
        updateBalanceFee_B.generate_r1cs_constraints();
        updateAccount_B.generate_r1cs_constraints();

        //--------UserC
        updateStorage_C_batch.generate_r1cs_constraints();
        updateBalanceS_C.generate_r1cs_constraints();
        updateBalanceB_C.generate_r1cs_constraints();
        updateBalanceFee_C.generate_r1cs_constraints();
        updateAccount_C.generate_r1cs_constraints();

        //--------UserD
        updateStorage_D_batch.generate_r1cs_constraints();
        updateBalanceS_D.generate_r1cs_constraints();
        updateBalanceB_D.generate_r1cs_constraints();
        updateBalanceFee_D.generate_r1cs_constraints();
        updateAccount_D.generate_r1cs_constraints();

        //--------UserE
        updateStorage_E_batch.generate_r1cs_constraints();
        updateBalanceS_E.generate_r1cs_constraints();
        updateBalanceB_E.generate_r1cs_constraints();
        updateBalanceFee_E.generate_r1cs_constraints();
        updateAccount_E.generate_r1cs_constraints();

        //--------UserF
        updateStorage_F_batch.generate_r1cs_constraints();
        updateBalanceS_F.generate_r1cs_constraints();
        updateBalanceB_F.generate_r1cs_constraints();
        updateBalanceFee_F.generate_r1cs_constraints();
        updateAccount_F.generate_r1cs_constraints();

        // Update Operator
        updateBalanceD_O.generate_r1cs_constraints();
        updateBalanceC_O.generate_r1cs_constraints();
        updateBalanceB_O.generate_r1cs_constraints();
        updateBalanceA_O.generate_r1cs_constraints();
        updateAccount_O.generate_r1cs_constraints();

    }

    const VariableArrayT getPublicData() const
    {
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

        constants.generate_r1cs_constraints();

        // Inputs
        exchange.generate_r1cs_constraints(true);
        merkleRootBefore.generate_r1cs_constraints(true);
        merkleRootAfter.generate_r1cs_constraints(true);
        merkleAssetRootBefore.generate_r1cs_constraints(true);
        merkleAssetRootAfter.generate_r1cs_constraints(true);
        timestamp.generate_r1cs_constraints(true);
        protocolFeeBips.generate_r1cs_constraints(true);
        operatorAccountID.generate_r1cs_constraints(true);

        // Increment the nonce of the Operator
        nonce_after.generate_r1cs_constraints();

        // Transactions
        transactions.reserve(numTransactions);
        for (size_t j = 0; j < numTransactions; j++)
        {
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
            transactions.emplace_back(
              pb,
              params,
              constants,
              exchange.packed,
              txAccountsRoot,
              txAccountsAssetRoot,
              timestamp.packed,
              protocolFeeBips.packed,
              operatorAccountID.bits,
              (j == 0) ? constants._0 : transactions.back().tx.getOutput(TXV_NUM_CONDITIONAL_TXS),
              txTypes.back().packed,
              std::string("tx_") + std::to_string(j));
            transactions.back().generate_r1cs_constraints();
        }

        depositSize.reset(new ToBitsGadget(pb, depositSizeAdd.back().result(), NUM_BITS_TX_SIZE, FMT(annotation_prefix, ".depositSize")));
        depositSize->generate_r1cs_constraints();
        accountUpdateSize.reset(new ToBitsGadget(pb, accountUpdateSizeAdd.back().result(), NUM_BITS_TX_SIZE, FMT(annotation_prefix, ".accountUpdateSize")));
        accountUpdateSize->generate_r1cs_constraints();
        withdrawSize.reset(new ToBitsGadget(pb, withdrawSizeAdd.back().result(), NUM_BITS_TX_SIZE, FMT(annotation_prefix, ".withdrawSize")));
        withdrawSize->generate_r1cs_constraints();
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
           accountBefore_P.storageRoot},
          FMT(annotation_prefix, ".updateAccount_P")));
        updateAccount_P->generate_r1cs_constraints();

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
           accountBefore_O.storageRoot},
          FMT(annotation_prefix, ".updateAccount_O")));
        updateAccount_O->generate_r1cs_constraints();

        // Num conditional transactions
        numConditionalTransactions.reset(new ToBitsGadget(
          pb, transactions.back().tx.getOutput(TXV_NUM_CONDITIONAL_TXS), 32, ".numConditionalTransactions"));
        numConditionalTransactions->generate_r1cs_constraints();

        // Public data
        publicData.add(exchange.bits);
        publicData.add(merkleRootBefore.bits);
        publicData.add(merkleRootAfter.bits);
        publicData.add(merkleAssetRootBefore.bits);
        publicData.add(merkleAssetRootAfter.bits);
        publicData.add(timestamp.bits);
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

        // Signature
        hash.generate_r1cs_constraints();
        signatureVerifier.generate_r1cs_constraints();

        // Check the new merkle root
        requireEqual(pb, updateAccount_O->result(), merkleRootAfter.packed, "newMerkleRoot");
        requireEqual(pb, updateAccount_O->assetResult(), merkleAssetRootAfter.packed, "newMerkleAssetRoot");
    }

    bool generateWitness(const Block &block)
    {
        if (block.transactions.size() != numTransactions)
        {
            std::cout << "Invalid number of transactions: " << block.transactions.size() << std::endl;
            return false;
        }

        constants.generate_r1cs_witness();

        // State
        accountBefore_P.generate_r1cs_witness(block.accountUpdate_P.before);
        accountBefore_O.generate_r1cs_witness(block.accountUpdate_O.before);

        // Inputs
        exchange.generate_r1cs_witness(pb, block.exchange);
        merkleRootBefore.generate_r1cs_witness(pb, block.merkleRootBefore);
        merkleRootAfter.generate_r1cs_witness(pb, block.merkleRootAfter);
        merkleAssetRootBefore.generate_r1cs_witness(pb, block.merkleAssetRootBefore);
        merkleAssetRootAfter.generate_r1cs_witness(pb, block.merkleAssetRootAfter);
        timestamp.generate_r1cs_witness(pb, block.timestamp);
        protocolFeeBips.generate_r1cs_witness(pb, block.protocolFeeBips);
        operatorAccountID.generate_r1cs_witness(pb, block.operatorAccountID);

        // Increment the nonce of the Operator
        nonce_after.generate_r1cs_witness();

        // Transactions
        // First set numConditionalTransactionsAfter which is a dependency between
        // transactions. Once this is set the transactions can be processed in
        // parallel.
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

        // Update Protocol pool
        updateAccount_P->generate_r1cs_witness(block.accountUpdate_P);

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
