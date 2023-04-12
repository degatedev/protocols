// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
#ifndef _BASETRANSACTIONCIRCUIT_H_
#define _BASETRANSACTIONCIRCUIT_H_

#include "Circuit.h"
#include "../Utils/Constants.h"
#include "../Utils/Data.h"

#include "../ThirdParty/BigIntHeader.hpp"
#include "ethsnarks.hpp"
#include "utils.hpp"

using namespace ethsnarks;

namespace Loopring
{

struct BaseTransactionAccountState : public GadgetT 
{
    std::vector<StorageGadget> storageArray;
    BalanceGadget balanceS;
    BalanceGadget balanceB;
    // split trading fee and gas fee
    BalanceGadget balanceFee;
    AccountGadget account;
    BaseTransactionAccountState( //
      ProtoboardT &pb,
      unsigned int storageArraySize,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          balanceS(pb, FMT(prefix, ".balanceS")),
          balanceB(pb, FMT(prefix, ".balanceB")),
          // split trading fee and gas fee
          balanceFee(pb, FMT(prefix, ".balanceFee")),
          account(pb, FMT(prefix, ".account"))
    {
      LOG(LogDebug, "in BaseTransactionAccountState", "");
      for (size_t i = 0; i < storageArraySize; i++) 
      {
        storageArray.emplace_back(pb, "BaseTransactionAccountState.storageArray_" + std::to_string(i));
      }
    }
};

struct TransactionAccountState : public BaseTransactionAccountState
{
    StorageGadget storage;

    TransactionAccountState( //
      ProtoboardT &pb,
      unsigned int storageArraySize,
      const std::string &prefix)
        : BaseTransactionAccountState(pb, storageArraySize, prefix),

          storage(pb, FMT(prefix, ".storage"))
    {
      LOG(LogDebug, "in TransactionAccountState", "");
    }

    void generate_r1cs_witness(
      const AccountLeaf &accountLeaf,
      const BalanceLeaf &balanceLeafS,
      const BalanceLeaf &balanceLeafB,
      const BalanceLeaf &balanceLeafFee,
      const StorageLeaf &storageLeaf,
      const std::vector<StorageUpdate> &storageUpdate_array)
    {
        LOG(LogDebug, "in TransactionAccountState", "generate_r1cs_witness");
        storage.generate_r1cs_witness(storageLeaf);
        balanceS.generate_r1cs_witness(balanceLeafS);
        balanceB.generate_r1cs_witness(balanceLeafB);
        // split trading fee and gas fee
        balanceFee.generate_r1cs_witness(balanceLeafFee);
        account.generate_r1cs_witness(accountLeaf);
        for (size_t i = 0; i < storageUpdate_array.size(); i ++) {
          storageArray[i].generate_r1cs_witness(storageUpdate_array[i].before);
        }
    }
};


struct TransactionBatchAccountState : public BaseTransactionAccountState
{

    TransactionBatchAccountState( //
      ProtoboardT &pb,
      unsigned int storageArraySize,
      const std::string &prefix)
        : BaseTransactionAccountState(pb, storageArraySize, prefix)
    {
      LOG(LogDebug, "in TransactionBatchAccountState", "");
    }

    void generate_r1cs_witness(
      const AccountLeaf &accountLeaf,
      const BalanceLeaf &balanceLeafS,
      const BalanceLeaf &balanceLeafB,
      const BalanceLeaf &balanceLeafFee,
      const std::vector<StorageUpdate> &storageUpdate_array)
    {
        LOG(LogDebug, "in TransactionBatchAccountState", "generate_r1cs_witness");
        balanceS.generate_r1cs_witness(balanceLeafS);
        balanceB.generate_r1cs_witness(balanceLeafB);
        // split trading fee and gas fee
        balanceFee.generate_r1cs_witness(balanceLeafFee);
        account.generate_r1cs_witness(accountLeaf);
        for (size_t i = 0; i < storageUpdate_array.size(); i ++) {
          storageArray[i].generate_r1cs_witness(storageUpdate_array[i].before);
        }
    }
};

struct TransactionAccountOperatorState : public GadgetT
{
    BalanceGadget balanceA;
    BalanceGadget balanceB;
    BalanceGadget balanceC;
    BalanceGadget balanceD;
    AccountGadget account;

    TransactionAccountOperatorState( //
      ProtoboardT &pb,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          balanceA(pb, FMT(prefix, ".balanceA")),
          balanceB(pb, FMT(prefix, ".balanceB")),
          balanceC(pb, FMT(prefix, ".balanceC")),
          balanceD(pb, FMT(prefix, ".balanceD")),
          account(pb, FMT(prefix, ".account"))
    {
        LOG(LogDebug, "in TransactionAccountOperatorState", "");
    }

    void generate_r1cs_witness(
      const AccountLeaf &accountLeaf,
      const BalanceLeaf &balanceLeafA,
      const BalanceLeaf &balanceLeafB,
      const BalanceLeaf &balanceLeafC,
      const BalanceLeaf &balanceLeafD)
    {
        LOG(LogDebug, "in TransactionAccountOperatorState", "generate_r1cs_witness");
        balanceA.generate_r1cs_witness(balanceLeafA);
        balanceB.generate_r1cs_witness(balanceLeafB);
        balanceC.generate_r1cs_witness(balanceLeafC);
        balanceD.generate_r1cs_witness(balanceLeafD);
        account.generate_r1cs_witness(accountLeaf);
    }
};

struct TransactionAccountBalancesState : public GadgetT
{
    BalanceGadget balanceA;
    BalanceGadget balanceB;
    BalanceGadget balanceC;

    TransactionAccountBalancesState( //
      ProtoboardT &pb,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          balanceA(pb, FMT(prefix, ".balanceA")),
          balanceB(pb, FMT(prefix, ".balanceB")),
          balanceC(pb, FMT(prefix, ".balanceC"))
    {
        LOG(LogDebug, "in TransactionAccountBalancesState", "");
    }

    void generate_r1cs_witness(const BalanceLeaf &balanceLeafA, const BalanceLeaf &balanceLeafB, const BalanceLeaf &balanceLeafC)
    {
        LOG(LogDebug, "in TransactionAccountBalancesState", "generate_r1cs_witness");
        balanceA.generate_r1cs_witness(balanceLeafA);
        balanceB.generate_r1cs_witness(balanceLeafB);
        balanceC.generate_r1cs_witness(balanceLeafC);
    }
};

struct TransactionState : public GadgetT
{
    const jubjub::Params &params;

    const Constants &constants;

    const VariableT &exchange;
    const VariableT &timestamp;
    const VariableT &protocolFeeBips;
    const VariableT &numConditionalTransactions;
    const VariableT &type;

    TransactionAccountState accountA;
    TransactionAccountState accountB;
    TransactionBatchAccountState accountC;
    TransactionBatchAccountState accountD;
    TransactionBatchAccountState accountE;
    TransactionBatchAccountState accountF;
    TransactionAccountOperatorState oper;

    TransactionState(
      ProtoboardT &pb,
      const jubjub::Params &_params,
      const Constants &_constants,
      const VariableT &_exchange,
      const VariableT &_timestamp,
      const VariableT &_protocolFeeBips,
      const VariableT &_numConditionalTransactions,
      const VariableT &_type,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          params(_params),

          constants(_constants),

          exchange(_exchange),
          timestamp(_timestamp),
          protocolFeeBips(_protocolFeeBips),
          numConditionalTransactions(_numConditionalTransactions),
          type(_type),

          accountA(pb, ORDER_SIZE_USER_A - 1, FMT(prefix, ".accountA")),
          accountB(pb, ORDER_SIZE_USER_B - 1, FMT(prefix, ".accountB")),
          accountC(pb, ORDER_SIZE_USER_C, FMT(prefix, ".accountC")),
          accountD(pb, ORDER_SIZE_USER_D, FMT(prefix, ".accountD")),
          accountE(pb, ORDER_SIZE_USER_E, FMT(prefix, ".accountE")),
          accountF(pb, ORDER_SIZE_USER_F, FMT(prefix, ".accountF")),
          oper(pb, FMT(prefix, ".oper"))
    {
      LOG(LogDebug, "in TransactionState", "");
    }

    void generate_r1cs_witness(
      const AccountLeaf &account_A,
      const BalanceLeaf &balanceLeafS_A,
      const BalanceLeaf &balanceLeafB_A,
      const BalanceLeaf &balanceLeafFee_A,
      const StorageLeaf &storageLeaf_A,
      const std::vector<StorageUpdate> &storageUpdate_A_array,
      const AccountLeaf &account_B,
      const BalanceLeaf &balanceLeafS_B,
      const BalanceLeaf &balanceLeafB_B,
      const BalanceLeaf &balanceLeafFee_B,
      const StorageLeaf &storageLeaf_B,
      const std::vector<StorageUpdate> &storageUpdate_B_array,
      // AccountC
      const AccountLeaf &account_C,
      const BalanceLeaf &balanceLeafS_C,
      const BalanceLeaf &balanceLeafB_C,
      const BalanceLeaf &balanceLeafFee_C,
      const std::vector<StorageUpdate> &storageUpdate_C_array,
      // AccountD
      const AccountLeaf &account_D,
      const BalanceLeaf &balanceLeafS_D,
      const BalanceLeaf &balanceLeafB_D,
      const BalanceLeaf &balanceLeafFee_D,
      const std::vector<StorageUpdate> &storageUpdate_D_array,
      // AccountE
      const AccountLeaf &account_E,
      const BalanceLeaf &balanceLeafS_E,
      const BalanceLeaf &balanceLeafB_E,
      const BalanceLeaf &balanceLeafFee_E,
      const std::vector<StorageUpdate> &storageUpdate_E_array,
      // AccountF
      const AccountLeaf &account_F,
      const BalanceLeaf &balanceLeafS_F,
      const BalanceLeaf &balanceLeafB_F,
      const BalanceLeaf &balanceLeafFee_F,
      const std::vector<StorageUpdate> &storageUpdate_F_array,
      const AccountLeaf &account_O,
      const BalanceLeaf &balanceLeafA_O,
      const BalanceLeaf &balanceLeafB_O,
      const BalanceLeaf &balanceLeafC_O,
      const BalanceLeaf &balanceLeafD_O
    )
    {
        LOG(LogDebug, "in TransactionState", "generate_r1cs_witness");
        accountA.generate_r1cs_witness(account_A, balanceLeafS_A, balanceLeafB_A, balanceLeafFee_A, storageLeaf_A, storageUpdate_A_array);
        accountB.generate_r1cs_witness(account_B, balanceLeafS_B, balanceLeafB_B, balanceLeafFee_B, storageLeaf_B, storageUpdate_B_array);
        // storageLeaf_A in next is not used, because the circuit is not easy to handle heavy load
        accountC.generate_r1cs_witness(account_C, balanceLeafS_C, balanceLeafB_C, balanceLeafFee_C, storageUpdate_C_array);
        accountD.generate_r1cs_witness(account_D, balanceLeafS_D, balanceLeafB_D, balanceLeafFee_D, storageUpdate_D_array);
        accountE.generate_r1cs_witness(account_E, balanceLeafS_E, balanceLeafB_E, balanceLeafFee_E, storageUpdate_E_array);
        accountF.generate_r1cs_witness(account_F, balanceLeafS_F, balanceLeafB_F, balanceLeafFee_F, storageUpdate_F_array);
        oper.generate_r1cs_witness(account_O, balanceLeafA_O, balanceLeafB_O, balanceLeafC_O, balanceLeafD_O);
    }
};

enum TxVariable
{
    // storage leaf: tokenSID, tokenBID, data, storageID, gasFee, cancelled, forward
    TXV_STORAGE_A_ADDRESS,
    TXV_STORAGE_A_TOKENSID,
    TXV_STORAGE_A_TOKENBID,
    TXV_STORAGE_A_DATA,
    TXV_STORAGE_A_STORAGEID,
    TXV_STORAGE_A_GASFEE,
    TXV_STORAGE_A_CANCELLED,
    TXV_STORAGE_A_FORWARD,

    // balance leaf: balance
    TXV_BALANCE_A_S_ADDRESS,
    TXV_BALANCE_A_S_BALANCE,

    TXV_BALANCE_A_B_ADDRESS,
    TXV_BALANCE_A_B_BALANCE,

    // asset tree account leaf: owner, publicKeyX, publicKeyY, nonce, balanceRoot
    // full tree account leaf: owner, publicKeyX, publicKeyY, appKeyPublicKeyX, appKeyPublicKeyY, nonce, disableAppKeySpotTrade, 
    //   disableAppKeyWithdraw, disableAppKeyTransferToOther, balanceRoot, storageRoot
    TXV_ACCOUNT_A_ADDRESS,
    TXV_ACCOUNT_A_OWNER,
    TXV_ACCOUNT_A_PUBKEY_X,
    TXV_ACCOUNT_A_PUBKEY_Y,
    TXV_ACCOUNT_A_APPKEY_PUBKEY_X,
    TXV_ACCOUNT_A_APPKEY_PUBKEY_Y,
    TXV_ACCOUNT_A_NONCE,
    TXV_ACCOUNT_A_DISABLE_APPKEY_SPOTTRADE,
    TXV_ACCOUNT_A_DISABLE_APPKEY_WITHDRAW_TO_OTHER,
    TXV_ACCOUNT_A_DISABLE_APPKEY_TRANSFER_TO_OTHER,

    TXV_STORAGE_A_ADDRESS_ARRAY_0,
    TXV_STORAGE_A_TOKENSID_ARRAY_0,
    TXV_STORAGE_A_TOKENBID_ARRAY_0,
    TXV_STORAGE_A_DATA_ARRAY_0,
    TXV_STORAGE_A_STORAGEID_ARRAY_0,
    TXV_STORAGE_A_GASFEE_ARRAY_0,
    TXV_STORAGE_A_CANCELLED_ARRAY_0,
    TXV_STORAGE_A_FORWARD_ARRAY_0,

    TXV_STORAGE_A_ADDRESS_ARRAY_1,
    TXV_STORAGE_A_TOKENSID_ARRAY_1,
    TXV_STORAGE_A_TOKENBID_ARRAY_1,
    TXV_STORAGE_A_DATA_ARRAY_1,
    TXV_STORAGE_A_STORAGEID_ARRAY_1,
    TXV_STORAGE_A_GASFEE_ARRAY_1,
    TXV_STORAGE_A_CANCELLED_ARRAY_1,
    TXV_STORAGE_A_FORWARD_ARRAY_1,

    TXV_STORAGE_A_ADDRESS_ARRAY_2,
    TXV_STORAGE_A_TOKENSID_ARRAY_2,
    TXV_STORAGE_A_TOKENBID_ARRAY_2,
    TXV_STORAGE_A_DATA_ARRAY_2,
    TXV_STORAGE_A_STORAGEID_ARRAY_2,
    TXV_STORAGE_A_GASFEE_ARRAY_2,
    TXV_STORAGE_A_CANCELLED_ARRAY_2,
    TXV_STORAGE_A_FORWARD_ARRAY_2,

    TXV_STORAGE_B_ADDRESS,
    TXV_STORAGE_B_TOKENSID,
    TXV_STORAGE_B_TOKENBID,
    TXV_STORAGE_B_DATA,
    TXV_STORAGE_B_STORAGEID,
    TXV_STORAGE_B_GASFEE,
    TXV_STORAGE_B_CANCELLED,
    TXV_STORAGE_B_FORWARD,

    TXV_BALANCE_B_S_ADDRESS,
    TXV_BALANCE_B_S_BALANCE,

    TXV_BALANCE_B_B_ADDRESS,
    TXV_BALANCE_B_B_BALANCE,

    TXV_ACCOUNT_B_ADDRESS,
    TXV_ACCOUNT_B_OWNER,
    TXV_ACCOUNT_B_PUBKEY_X,
    TXV_ACCOUNT_B_PUBKEY_Y,
    TXV_ACCOUNT_B_NONCE,

    TXV_STORAGE_B_ADDRESS_ARRAY_0,
    TXV_STORAGE_B_TOKENSID_ARRAY_0,
    TXV_STORAGE_B_TOKENBID_ARRAY_0,
    TXV_STORAGE_B_DATA_ARRAY_0,
    TXV_STORAGE_B_STORAGEID_ARRAY_0,
    TXV_STORAGE_B_GASFEE_ARRAY_0,
    TXV_STORAGE_B_CANCELLED_ARRAY_0,
    TXV_STORAGE_B_FORWARD_ARRAY_0,

    //-----------UserC
    TXV_BALANCE_C_S_ADDRESS,
    TXV_BALANCE_C_S_BALANCE,

    TXV_BALANCE_C_B_ADDRESS,
    TXV_BALANCE_C_B_BALANCE,

    TXV_ACCOUNT_C_ADDRESS,
    TXV_ACCOUNT_C_OWNER,
    TXV_ACCOUNT_C_PUBKEY_X,
    TXV_ACCOUNT_C_PUBKEY_Y,
    TXV_ACCOUNT_C_NONCE,

    TXV_STORAGE_C_ADDRESS_ARRAY_0,
    TXV_STORAGE_C_TOKENSID_ARRAY_0,
    TXV_STORAGE_C_TOKENBID_ARRAY_0,
    TXV_STORAGE_C_DATA_ARRAY_0,
    TXV_STORAGE_C_STORAGEID_ARRAY_0,
    TXV_STORAGE_C_GASFEE_ARRAY_0,
    TXV_STORAGE_C_CANCELLED_ARRAY_0,
    TXV_STORAGE_C_FORWARD_ARRAY_0,

    //-----------UserD
    TXV_BALANCE_D_S_ADDRESS,
    TXV_BALANCE_D_S_BALANCE,

    TXV_BALANCE_D_B_ADDRESS,
    TXV_BALANCE_D_B_BALANCE,

    TXV_ACCOUNT_D_ADDRESS,
    TXV_ACCOUNT_D_OWNER,
    TXV_ACCOUNT_D_PUBKEY_X,
    TXV_ACCOUNT_D_PUBKEY_Y,
    TXV_ACCOUNT_D_NONCE,

    TXV_STORAGE_D_ADDRESS_ARRAY_0,
    TXV_STORAGE_D_TOKENSID_ARRAY_0,
    TXV_STORAGE_D_TOKENBID_ARRAY_0,
    TXV_STORAGE_D_DATA_ARRAY_0,
    TXV_STORAGE_D_STORAGEID_ARRAY_0,
    TXV_STORAGE_D_GASFEE_ARRAY_0,
    TXV_STORAGE_D_CANCELLED_ARRAY_0,
    TXV_STORAGE_D_FORWARD_ARRAY_0,

    //-----------UserE
    TXV_BALANCE_E_S_ADDRESS,
    TXV_BALANCE_E_S_BALANCE,

    TXV_BALANCE_E_B_ADDRESS,
    TXV_BALANCE_E_B_BALANCE,

    TXV_ACCOUNT_E_ADDRESS,
    TXV_ACCOUNT_E_OWNER,
    TXV_ACCOUNT_E_PUBKEY_X,
    TXV_ACCOUNT_E_PUBKEY_Y,
    TXV_ACCOUNT_E_NONCE,

    TXV_STORAGE_E_ADDRESS_ARRAY_0,
    TXV_STORAGE_E_TOKENSID_ARRAY_0,
    TXV_STORAGE_E_TOKENBID_ARRAY_0,
    TXV_STORAGE_E_DATA_ARRAY_0,
    TXV_STORAGE_E_STORAGEID_ARRAY_0,
    TXV_STORAGE_E_GASFEE_ARRAY_0,
    TXV_STORAGE_E_CANCELLED_ARRAY_0,
    TXV_STORAGE_E_FORWARD_ARRAY_0,

    //-----------UserF
    TXV_BALANCE_F_S_ADDRESS,
    TXV_BALANCE_F_S_BALANCE,

    TXV_BALANCE_F_B_ADDRESS,
    TXV_BALANCE_F_B_BALANCE,

    TXV_ACCOUNT_F_ADDRESS,
    TXV_ACCOUNT_F_OWNER,
    TXV_ACCOUNT_F_PUBKEY_X,
    TXV_ACCOUNT_F_PUBKEY_Y,
    TXV_ACCOUNT_F_NONCE,

    TXV_STORAGE_F_ADDRESS_ARRAY_0,
    TXV_STORAGE_F_TOKENSID_ARRAY_0,
    TXV_STORAGE_F_TOKENBID_ARRAY_0,
    TXV_STORAGE_F_DATA_ARRAY_0,
    TXV_STORAGE_F_STORAGEID_ARRAY_0,
    TXV_STORAGE_F_GASFEE_ARRAY_0,
    TXV_STORAGE_F_CANCELLED_ARRAY_0,
    TXV_STORAGE_F_FORWARD_ARRAY_0,



    TXV_BALANCE_O_A_Address,
    TXV_BALANCE_O_B_Address,
    TXV_BALANCE_O_C_Address,
    TXV_BALANCE_O_D_Address,
    // add C and D，The A, B, C and D mean different tokens rather than accounts 
    TXV_BALANCE_O_A_BALANCE,
    TXV_BALANCE_O_B_BALANCE,
    TXV_BALANCE_O_C_BALANCE,
    TXV_BALANCE_O_D_BALANCE,

    TXV_BALANCE_A_FEE_BALANCE,
    TXV_BALANCE_B_FEE_BALANCE,
    TXV_BALANCE_C_FEE_BALANCE,
    TXV_BALANCE_D_FEE_BALANCE,
    TXV_BALANCE_E_FEE_BALANCE,
    TXV_BALANCE_F_FEE_BALANCE,

    TXV_BALANCE_A_FEE_Address,
    TXV_BALANCE_B_FEE_Address,
    TXV_BALANCE_C_FEE_Address,
    TXV_BALANCE_D_FEE_Address,
    TXV_BALANCE_E_FEE_Address,
    TXV_BALANCE_F_FEE_Address,

    TXV_HASH_A,
    TXV_HASH_A_ARRAY,
    TXV_PUBKEY_X_A,
    TXV_PUBKEY_Y_A,
    TXV_PUBKEY_X_A_ARRAY,
    TXV_PUBKEY_Y_A_ARRAY,
    TXV_SIGNATURE_REQUIRED_A,
    TXV_SIGNATURE_REQUIRED_A_ARRAY,

    TXV_HASH_B,
    TXV_HASH_B_ARRAY,
    TXV_PUBKEY_X_B,
    TXV_PUBKEY_Y_B,
    TXV_PUBKEY_X_B_ARRAY,
    TXV_PUBKEY_Y_B_ARRAY,
    TXV_SIGNATURE_REQUIRED_B,
    TXV_SIGNATURE_REQUIRED_B_ARRAY,


    TXV_HASH_C_ARRAY,
    TXV_PUBKEY_X_C_ARRAY,
    TXV_PUBKEY_Y_C_ARRAY,
    TXV_SIGNATURE_REQUIRED_C_ARRAY,

    TXV_HASH_D_ARRAY,
    TXV_PUBKEY_X_D_ARRAY,
    TXV_PUBKEY_Y_D_ARRAY,
    TXV_SIGNATURE_REQUIRED_D_ARRAY,

    TXV_HASH_E_ARRAY,
    TXV_PUBKEY_X_E_ARRAY,
    TXV_PUBKEY_Y_E_ARRAY,
    TXV_SIGNATURE_REQUIRED_E_ARRAY,

    TXV_HASH_F_ARRAY,
    TXV_PUBKEY_X_F_ARRAY,
    TXV_PUBKEY_Y_F_ARRAY,
    TXV_SIGNATURE_REQUIRED_F_ARRAY,

    TXV_NUM_CONDITIONAL_TXS

};

class BaseTransactionCircuit : public GadgetT
{
  public:
    const TransactionState &state;

    std::map<TxVariable, VariableT> uOutputs;
    std::map<TxVariable, VariableArrayT> aOutputs;

    BaseTransactionCircuit(
      ProtoboardT &pb,
      const TransactionState &_state,
      const std::string &prefix)
        : GadgetT(pb, prefix), state(_state)
    {
        LOG(LogDebug, "in BaseTransactionCircuit", "");
        aOutputs[TXV_STORAGE_A_ADDRESS] = VariableArrayT(NUM_BITS_STORAGE_ADDRESS, state.constants._0);
        uOutputs[TXV_STORAGE_A_TOKENSID] = state.accountA.storage.tokenSID;
        uOutputs[TXV_STORAGE_A_TOKENBID] = state.accountA.storage.tokenBID;
        uOutputs[TXV_STORAGE_A_DATA] = state.accountA.storage.data;
        uOutputs[TXV_STORAGE_A_STORAGEID] = state.accountA.storage.storageID;
        uOutputs[TXV_STORAGE_A_GASFEE] = state.accountA.storage.gasFee;
        uOutputs[TXV_STORAGE_A_CANCELLED] = state.accountA.storage.cancelled;
        uOutputs[TXV_STORAGE_A_FORWARD] = state.accountA.storage.forward;

        aOutputs[TXV_BALANCE_A_S_ADDRESS] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        uOutputs[TXV_BALANCE_A_S_BALANCE] = state.accountA.balanceS.balance;

        aOutputs[TXV_BALANCE_A_B_ADDRESS] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        uOutputs[TXV_BALANCE_A_B_BALANCE] = state.accountA.balanceB.balance;
        uOutputs[TXV_BALANCE_A_FEE_BALANCE] = state.accountA.balanceFee.balance;
        aOutputs[TXV_BALANCE_A_FEE_Address] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);

        // default account ID is 0 rather than 1
        aOutputs[TXV_ACCOUNT_A_ADDRESS] = VariableArrayT(NUM_BITS_ACCOUNT, state.constants._0);
        uOutputs[TXV_ACCOUNT_A_OWNER] = state.accountA.account.owner;
        uOutputs[TXV_ACCOUNT_A_PUBKEY_X] = state.accountA.account.publicKey.x;
        uOutputs[TXV_ACCOUNT_A_PUBKEY_Y] = state.accountA.account.publicKey.y;
        uOutputs[TXV_ACCOUNT_A_APPKEY_PUBKEY_X] = state.accountA.account.appKeyPublicKey.x;
        uOutputs[TXV_ACCOUNT_A_APPKEY_PUBKEY_Y] = state.accountA.account.appKeyPublicKey.y;
        uOutputs[TXV_ACCOUNT_A_NONCE] = state.accountA.account.nonce;
        uOutputs[TXV_ACCOUNT_A_DISABLE_APPKEY_SPOTTRADE] = state.accountA.account.disableAppKeySpotTrade;
        uOutputs[TXV_ACCOUNT_A_DISABLE_APPKEY_WITHDRAW_TO_OTHER] = state.accountA.account.disableAppKeyWithdraw;
        uOutputs[TXV_ACCOUNT_A_DISABLE_APPKEY_TRANSFER_TO_OTHER] = state.accountA.account.disableAppKeyTransferToOther;

        // Batch SpotTrade, account A have 4 orders, the first one had been handled before, now need to handle last 3
        aOutputs[TXV_STORAGE_A_ADDRESS_ARRAY_0] = (VariableArrayT(NUM_BITS_STORAGE_ADDRESS, state.constants._0));
        uOutputs[TXV_STORAGE_A_TOKENSID_ARRAY_0] = (state.accountA.storageArray[0].tokenSID);
        uOutputs[TXV_STORAGE_A_TOKENBID_ARRAY_0] = (state.accountA.storageArray[0].tokenBID);
        uOutputs[TXV_STORAGE_A_DATA_ARRAY_0] = (state.accountA.storageArray[0].data);
        uOutputs[TXV_STORAGE_A_STORAGEID_ARRAY_0] = (state.accountA.storageArray[0].storageID);
        uOutputs[TXV_STORAGE_A_GASFEE_ARRAY_0] = (state.accountA.storageArray[0].gasFee);
        uOutputs[TXV_STORAGE_A_CANCELLED_ARRAY_0] = (state.accountA.storageArray[0].cancelled);
        uOutputs[TXV_STORAGE_A_FORWARD_ARRAY_0] = (state.accountA.storageArray[0].forward);

        aOutputs[TXV_STORAGE_A_ADDRESS_ARRAY_1] = (VariableArrayT(NUM_BITS_STORAGE_ADDRESS, state.constants._0));
        uOutputs[TXV_STORAGE_A_TOKENSID_ARRAY_1] = (state.accountA.storageArray[1].tokenSID);
        uOutputs[TXV_STORAGE_A_TOKENBID_ARRAY_1] = (state.accountA.storageArray[1].tokenBID);
        uOutputs[TXV_STORAGE_A_DATA_ARRAY_1] = (state.accountA.storageArray[1].data);
        uOutputs[TXV_STORAGE_A_STORAGEID_ARRAY_1] = (state.accountA.storageArray[1].storageID);
        uOutputs[TXV_STORAGE_A_GASFEE_ARRAY_1] = (state.accountA.storageArray[1].gasFee);
        uOutputs[TXV_STORAGE_A_CANCELLED_ARRAY_1] = (state.accountA.storageArray[1].cancelled);
        uOutputs[TXV_STORAGE_A_FORWARD_ARRAY_1] = (state.accountA.storageArray[1].forward);

        aOutputs[TXV_STORAGE_A_ADDRESS_ARRAY_2] = (VariableArrayT(NUM_BITS_STORAGE_ADDRESS, state.constants._0));
        uOutputs[TXV_STORAGE_A_TOKENSID_ARRAY_2] = (state.accountA.storageArray[2].tokenSID);
        uOutputs[TXV_STORAGE_A_TOKENBID_ARRAY_2] = (state.accountA.storageArray[2].tokenBID);
        uOutputs[TXV_STORAGE_A_DATA_ARRAY_2] = (state.accountA.storageArray[2].data);
        uOutputs[TXV_STORAGE_A_STORAGEID_ARRAY_2] = (state.accountA.storageArray[2].storageID);
        uOutputs[TXV_STORAGE_A_GASFEE_ARRAY_2] = (state.accountA.storageArray[2].gasFee);
        uOutputs[TXV_STORAGE_A_CANCELLED_ARRAY_2] = (state.accountA.storageArray[2].cancelled);
        uOutputs[TXV_STORAGE_A_FORWARD_ARRAY_2] = (state.accountA.storageArray[2].forward);

        // default account ID is 0 rather than 1
        aOutputs[TXV_STORAGE_B_ADDRESS] = VariableArrayT(NUM_BITS_STORAGE_ADDRESS, state.constants._0);
        uOutputs[TXV_STORAGE_B_TOKENSID] = state.accountB.storage.tokenSID;
        uOutputs[TXV_STORAGE_B_TOKENBID] = state.accountB.storage.tokenBID;
        uOutputs[TXV_STORAGE_B_DATA] = state.accountB.storage.data;
        uOutputs[TXV_STORAGE_B_STORAGEID] = state.accountB.storage.storageID;
        uOutputs[TXV_STORAGE_B_GASFEE] = state.accountB.storage.gasFee;
        uOutputs[TXV_STORAGE_B_CANCELLED] = state.accountB.storage.cancelled;
        uOutputs[TXV_STORAGE_B_FORWARD] = state.accountB.storage.forward;

        aOutputs[TXV_BALANCE_B_S_ADDRESS] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        uOutputs[TXV_BALANCE_B_S_BALANCE] = state.accountB.balanceS.balance;

        aOutputs[TXV_BALANCE_B_B_ADDRESS] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        uOutputs[TXV_BALANCE_B_B_BALANCE] = state.accountB.balanceB.balance;
        uOutputs[TXV_BALANCE_B_FEE_BALANCE] = state.accountB.balanceFee.balance;
        aOutputs[TXV_BALANCE_B_FEE_Address] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);

        aOutputs[TXV_ACCOUNT_B_ADDRESS] = VariableArrayT(NUM_BITS_ACCOUNT, state.constants._0);
        uOutputs[TXV_ACCOUNT_B_OWNER] = state.accountB.account.owner;
        uOutputs[TXV_ACCOUNT_B_PUBKEY_X] = state.accountB.account.publicKey.x;
        uOutputs[TXV_ACCOUNT_B_PUBKEY_Y] = state.accountB.account.publicKey.y;
        uOutputs[TXV_ACCOUNT_B_NONCE] = state.accountB.account.nonce;

        // Batch SpotTrade, account B have 2 orders, the first one had been handled before, now need to handle last 1
        aOutputs[TXV_STORAGE_B_ADDRESS_ARRAY_0] = (VariableArrayT(NUM_BITS_STORAGE_ADDRESS, state.constants._0));
        uOutputs[TXV_STORAGE_B_TOKENSID_ARRAY_0] = (state.accountB.storageArray[0].tokenSID);
        uOutputs[TXV_STORAGE_B_TOKENBID_ARRAY_0] = (state.accountB.storageArray[0].tokenBID);
        uOutputs[TXV_STORAGE_B_DATA_ARRAY_0] = (state.accountB.storageArray[0].data);
        uOutputs[TXV_STORAGE_B_STORAGEID_ARRAY_0] = (state.accountB.storageArray[0].storageID);
        uOutputs[TXV_STORAGE_B_GASFEE_ARRAY_0] = (state.accountB.storageArray[0].gasFee);
        uOutputs[TXV_STORAGE_B_CANCELLED_ARRAY_0] = (state.accountB.storageArray[0].cancelled);
        uOutputs[TXV_STORAGE_B_FORWARD_ARRAY_0] = (state.accountB.storageArray[0].forward);
        
        //------------------UserC
        aOutputs[TXV_BALANCE_C_S_ADDRESS] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        uOutputs[TXV_BALANCE_C_S_BALANCE] = state.accountC.balanceS.balance;

        aOutputs[TXV_BALANCE_C_B_ADDRESS] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        uOutputs[TXV_BALANCE_C_B_BALANCE] = state.accountC.balanceB.balance;
        // split trading fee and gas fee
        uOutputs[TXV_BALANCE_C_FEE_BALANCE] = state.accountC.balanceFee.balance;
        aOutputs[TXV_BALANCE_C_FEE_Address] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);

        // default account ID is 0 rather than 1
        aOutputs[TXV_ACCOUNT_C_ADDRESS] = VariableArrayT(NUM_BITS_ACCOUNT, state.constants._0);
        uOutputs[TXV_ACCOUNT_C_OWNER] = state.accountC.account.owner;
        uOutputs[TXV_ACCOUNT_C_PUBKEY_X] = state.accountC.account.publicKey.x;
        uOutputs[TXV_ACCOUNT_C_PUBKEY_Y] = state.accountC.account.publicKey.y;
        uOutputs[TXV_ACCOUNT_C_NONCE] = state.accountC.account.nonce;

        // Batch SpotTrade, account C have 1 orders
        aOutputs[TXV_STORAGE_C_ADDRESS_ARRAY_0] = (VariableArrayT(NUM_BITS_STORAGE_ADDRESS, state.constants._0));
        uOutputs[TXV_STORAGE_C_TOKENSID_ARRAY_0] = (state.accountC.storageArray[0].tokenSID);
        uOutputs[TXV_STORAGE_C_TOKENBID_ARRAY_0] = (state.accountC.storageArray[0].tokenBID);
        uOutputs[TXV_STORAGE_C_DATA_ARRAY_0] = (state.accountC.storageArray[0].data);
        uOutputs[TXV_STORAGE_C_STORAGEID_ARRAY_0] = (state.accountC.storageArray[0].storageID);
        uOutputs[TXV_STORAGE_C_GASFEE_ARRAY_0] = (state.accountC.storageArray[0].gasFee);
        uOutputs[TXV_STORAGE_C_CANCELLED_ARRAY_0] = (state.accountC.storageArray[0].cancelled);
        uOutputs[TXV_STORAGE_C_FORWARD_ARRAY_0] = (state.accountC.storageArray[0].forward);

        //------------------UserD
        aOutputs[TXV_BALANCE_D_S_ADDRESS] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        uOutputs[TXV_BALANCE_D_S_BALANCE] = state.accountD.balanceS.balance;

        aOutputs[TXV_BALANCE_D_B_ADDRESS] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        uOutputs[TXV_BALANCE_D_B_BALANCE] = state.accountD.balanceB.balance;
        uOutputs[TXV_BALANCE_D_FEE_BALANCE] = state.accountD.balanceFee.balance;
        aOutputs[TXV_BALANCE_D_FEE_Address] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);

        // default account ID is 0 rather than 1
        aOutputs[TXV_ACCOUNT_D_ADDRESS] = VariableArrayT(NUM_BITS_ACCOUNT, state.constants._0);
        uOutputs[TXV_ACCOUNT_D_OWNER] = state.accountD.account.owner;
        uOutputs[TXV_ACCOUNT_D_PUBKEY_X] = state.accountD.account.publicKey.x;
        uOutputs[TXV_ACCOUNT_D_PUBKEY_Y] = state.accountD.account.publicKey.y;
        uOutputs[TXV_ACCOUNT_D_NONCE] = state.accountD.account.nonce;

        // Batch SpotTrade, account D have 1 orders
        aOutputs[TXV_STORAGE_D_ADDRESS_ARRAY_0] = (VariableArrayT(NUM_BITS_STORAGE_ADDRESS, state.constants._0));
        uOutputs[TXV_STORAGE_D_TOKENSID_ARRAY_0] = (state.accountD.storageArray[0].tokenSID);
        uOutputs[TXV_STORAGE_D_TOKENBID_ARRAY_0] = (state.accountD.storageArray[0].tokenBID);
        uOutputs[TXV_STORAGE_D_DATA_ARRAY_0] = (state.accountD.storageArray[0].data);
        uOutputs[TXV_STORAGE_D_STORAGEID_ARRAY_0] = (state.accountD.storageArray[0].storageID);
        uOutputs[TXV_STORAGE_D_GASFEE_ARRAY_0] = (state.accountD.storageArray[0].gasFee);
        uOutputs[TXV_STORAGE_D_CANCELLED_ARRAY_0] = (state.accountD.storageArray[0].cancelled);
        uOutputs[TXV_STORAGE_D_FORWARD_ARRAY_0] = (state.accountD.storageArray[0].forward);

        //------------------UserE
        aOutputs[TXV_BALANCE_E_S_ADDRESS] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        uOutputs[TXV_BALANCE_E_S_BALANCE] = state.accountE.balanceS.balance;

        aOutputs[TXV_BALANCE_E_B_ADDRESS] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        uOutputs[TXV_BALANCE_E_B_BALANCE] = state.accountE.balanceB.balance;
        uOutputs[TXV_BALANCE_E_FEE_BALANCE] = state.accountE.balanceFee.balance;
        aOutputs[TXV_BALANCE_E_FEE_Address] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);

        // default account ID is 0 rather than 1
        aOutputs[TXV_ACCOUNT_E_ADDRESS] = VariableArrayT(NUM_BITS_ACCOUNT, state.constants._0);
        uOutputs[TXV_ACCOUNT_E_OWNER] = state.accountE.account.owner;
        uOutputs[TXV_ACCOUNT_E_PUBKEY_X] = state.accountE.account.publicKey.x;
        uOutputs[TXV_ACCOUNT_E_PUBKEY_Y] = state.accountE.account.publicKey.y;
        uOutputs[TXV_ACCOUNT_E_NONCE] = state.accountE.account.nonce;

        // Batch SpotTrade, account E have 1 orders
        aOutputs[TXV_STORAGE_E_ADDRESS_ARRAY_0] = (VariableArrayT(NUM_BITS_STORAGE_ADDRESS, state.constants._0));
        uOutputs[TXV_STORAGE_E_TOKENSID_ARRAY_0] = (state.accountE.storageArray[0].tokenSID);
        uOutputs[TXV_STORAGE_E_TOKENBID_ARRAY_0] = (state.accountE.storageArray[0].tokenBID);
        uOutputs[TXV_STORAGE_E_DATA_ARRAY_0] = (state.accountE.storageArray[0].data);
        uOutputs[TXV_STORAGE_E_STORAGEID_ARRAY_0] = (state.accountE.storageArray[0].storageID);
        uOutputs[TXV_STORAGE_E_GASFEE_ARRAY_0] = (state.accountE.storageArray[0].gasFee);
        uOutputs[TXV_STORAGE_E_CANCELLED_ARRAY_0] = (state.accountE.storageArray[0].cancelled);
        uOutputs[TXV_STORAGE_E_FORWARD_ARRAY_0] = (state.accountE.storageArray[0].forward);

        //------------------UserF
        aOutputs[TXV_BALANCE_F_S_ADDRESS] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        uOutputs[TXV_BALANCE_F_S_BALANCE] = state.accountF.balanceS.balance;

        aOutputs[TXV_BALANCE_F_B_ADDRESS] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        uOutputs[TXV_BALANCE_F_B_BALANCE] = state.accountF.balanceB.balance;
        uOutputs[TXV_BALANCE_F_FEE_BALANCE] = state.accountF.balanceFee.balance;
        aOutputs[TXV_BALANCE_F_FEE_Address] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);

        // default account ID is 0 rather than 1
        aOutputs[TXV_ACCOUNT_F_ADDRESS] = VariableArrayT(NUM_BITS_ACCOUNT, state.constants._0);
        uOutputs[TXV_ACCOUNT_F_OWNER] = state.accountF.account.owner;
        uOutputs[TXV_ACCOUNT_F_PUBKEY_X] = state.accountF.account.publicKey.x;
        uOutputs[TXV_ACCOUNT_F_PUBKEY_Y] = state.accountF.account.publicKey.y;
        uOutputs[TXV_ACCOUNT_F_NONCE] = state.accountF.account.nonce;

        // Batch SpotTrade, account F have 1 orders
        aOutputs[TXV_STORAGE_F_ADDRESS_ARRAY_0] = (VariableArrayT(NUM_BITS_STORAGE_ADDRESS, state.constants._0));
        uOutputs[TXV_STORAGE_F_TOKENSID_ARRAY_0] = (state.accountF.storageArray[0].tokenSID);
        uOutputs[TXV_STORAGE_F_TOKENBID_ARRAY_0] = (state.accountF.storageArray[0].tokenBID);
        uOutputs[TXV_STORAGE_F_DATA_ARRAY_0] = (state.accountF.storageArray[0].data);
        uOutputs[TXV_STORAGE_F_STORAGEID_ARRAY_0] = (state.accountF.storageArray[0].storageID);
        uOutputs[TXV_STORAGE_F_GASFEE_ARRAY_0] = (state.accountF.storageArray[0].gasFee);
        uOutputs[TXV_STORAGE_F_CANCELLED_ARRAY_0] = (state.accountF.storageArray[0].cancelled);
        uOutputs[TXV_STORAGE_F_FORWARD_ARRAY_0] = (state.accountF.storageArray[0].forward);

        // split trading fee and gas fee，default tokenID is 0
        aOutputs[TXV_BALANCE_O_A_Address] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        aOutputs[TXV_BALANCE_O_B_Address] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        aOutputs[TXV_BALANCE_O_C_Address] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);
        aOutputs[TXV_BALANCE_O_D_Address] = VariableArrayT(NUM_BITS_TOKEN, state.constants._0);

        uOutputs[TXV_BALANCE_O_A_BALANCE] = state.oper.balanceA.balance;
        uOutputs[TXV_BALANCE_O_B_BALANCE] = state.oper.balanceB.balance;
        uOutputs[TXV_BALANCE_O_C_BALANCE] = state.oper.balanceC.balance;
        uOutputs[TXV_BALANCE_O_D_BALANCE] = state.oper.balanceD.balance;

        uOutputs[TXV_HASH_A] = state.constants._0;
        aOutputs[TXV_HASH_A_ARRAY] = VariableArrayT(ORDER_SIZE_USER_A - 1, state.constants._0);
        uOutputs[TXV_PUBKEY_X_A] = state.accountA.account.publicKey.x;
        uOutputs[TXV_PUBKEY_Y_A] = state.accountA.account.publicKey.y;
        aOutputs[TXV_PUBKEY_X_A_ARRAY] = VariableArrayT(ORDER_SIZE_USER_A - 1, state.accountA.account.publicKey.x);
        aOutputs[TXV_PUBKEY_Y_A_ARRAY] = VariableArrayT(ORDER_SIZE_USER_A - 1, state.accountA.account.publicKey.y);
        uOutputs[TXV_SIGNATURE_REQUIRED_A] = state.constants._1;
        aOutputs[TXV_SIGNATURE_REQUIRED_A_ARRAY] = VariableArrayT(ORDER_SIZE_USER_A - 1, state.constants._0);

        uOutputs[TXV_HASH_B] = state.constants._0;
        aOutputs[TXV_HASH_B_ARRAY] = VariableArrayT(ORDER_SIZE_USER_B - 1, state.constants._0);
        uOutputs[TXV_PUBKEY_X_B] = state.accountB.account.publicKey.x;
        uOutputs[TXV_PUBKEY_Y_B] = state.accountB.account.publicKey.y;
        aOutputs[TXV_PUBKEY_X_B_ARRAY] = VariableArrayT(ORDER_SIZE_USER_B - 1, state.accountB.account.publicKey.x);
        aOutputs[TXV_PUBKEY_Y_B_ARRAY] = VariableArrayT(ORDER_SIZE_USER_B - 1, state.accountB.account.publicKey.y);
        uOutputs[TXV_SIGNATURE_REQUIRED_B] = state.constants._1;
        aOutputs[TXV_SIGNATURE_REQUIRED_B_ARRAY] = VariableArrayT(ORDER_SIZE_USER_B - 1, state.constants._0);


        aOutputs[TXV_HASH_C_ARRAY] = VariableArrayT(ORDER_SIZE_USER_C, state.constants._0);
        aOutputs[TXV_PUBKEY_X_C_ARRAY] = VariableArrayT(ORDER_SIZE_USER_C, state.accountC.account.publicKey.x);
        aOutputs[TXV_PUBKEY_Y_C_ARRAY] = VariableArrayT(ORDER_SIZE_USER_C, state.accountC.account.publicKey.y);
        aOutputs[TXV_SIGNATURE_REQUIRED_C_ARRAY] = VariableArrayT(ORDER_SIZE_USER_C, state.constants._0);


        aOutputs[TXV_HASH_D_ARRAY] = VariableArrayT(ORDER_SIZE_USER_D, state.constants._0);
        aOutputs[TXV_PUBKEY_X_D_ARRAY] = VariableArrayT(ORDER_SIZE_USER_D, state.accountD.account.publicKey.x);
        aOutputs[TXV_PUBKEY_Y_D_ARRAY] = VariableArrayT(ORDER_SIZE_USER_D, state.accountD.account.publicKey.y);
        aOutputs[TXV_SIGNATURE_REQUIRED_D_ARRAY] = VariableArrayT(ORDER_SIZE_USER_D, state.constants._0);

        aOutputs[TXV_HASH_E_ARRAY] = VariableArrayT(ORDER_SIZE_USER_E, state.constants._0);
        aOutputs[TXV_PUBKEY_X_E_ARRAY] = VariableArrayT(ORDER_SIZE_USER_E, state.accountE.account.publicKey.x);
        aOutputs[TXV_PUBKEY_Y_E_ARRAY] = VariableArrayT(ORDER_SIZE_USER_E, state.accountE.account.publicKey.y);
        aOutputs[TXV_SIGNATURE_REQUIRED_E_ARRAY] = VariableArrayT(ORDER_SIZE_USER_E, state.constants._0);

        aOutputs[TXV_HASH_F_ARRAY] = VariableArrayT(ORDER_SIZE_USER_F, state.constants._0);
        aOutputs[TXV_PUBKEY_X_F_ARRAY] = VariableArrayT(ORDER_SIZE_USER_F, state.accountF.account.publicKey.x);
        aOutputs[TXV_PUBKEY_Y_F_ARRAY] = VariableArrayT(ORDER_SIZE_USER_F, state.accountF.account.publicKey.y);
        aOutputs[TXV_SIGNATURE_REQUIRED_F_ARRAY] = VariableArrayT(ORDER_SIZE_USER_F, state.constants._0);

        uOutputs[TXV_NUM_CONDITIONAL_TXS] = state.numConditionalTransactions;
        LOG(LogDebug, "in BaseTransactionCircuit", "End");
    }

    const VariableT &getOutput(TxVariable txVariable) const
    {
        return uOutputs.at(txVariable);
    }

    const VariableArrayT &getArrayOutput(TxVariable txVariable) const
    {
        return aOutputs.at(txVariable);
    }
    
    void setOutput(TxVariable txVariable, const VariableT &var)
    {
        assert(uOutputs.find(txVariable) != uOutputs.end());
        uOutputs[txVariable] = var;
    }

    void setArrayOutput(TxVariable txVariable, const VariableArrayT &var)
    {
        assert(aOutputs.find(txVariable) != aOutputs.end());
        aOutputs[txVariable] = var;
    }
    
    virtual const VariableArrayT getPublicData() const = 0;
};

} // namespace Loopring

#endif