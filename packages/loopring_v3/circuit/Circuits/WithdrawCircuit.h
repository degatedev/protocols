// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
// Modified by DeGate DAO, 2022
#ifndef _OFFCHAINWITHDRAWALCIRCUIT_H_
#define _OFFCHAINWITHDRAWALCIRCUIT_H_

#include "Circuit.h"
#include "../Utils/Constants.h"
#include "../Utils/Data.h"
#include "../Utils/Utils.h"
#include "../Gadgets/AccountGadgets.h"

#include "ethsnarks.hpp"
#include "utils.hpp"
#include "gadgets/subadd.hpp"

using namespace ethsnarks;

namespace Loopring
{
/**
 Changes we made: 
   1. In order to reduce the gas used by each transaction, DeGate removed the protocol account. All fees collected go to the operator account.
   2. Every end user is allowed to do a withdrawal using their asset key or app key.
   3. The use of an asset key for withdrawal can be disabled.
 */ 
class WithdrawCircuit : public BaseTransactionCircuit
{
  public:
    // Inputs
    DualVariableGadget accountID;
    DualVariableGadget tokenID;
    DualVariableGadget amount;
    DualVariableGadget feeTokenID;
    DualVariableGadget fee;
    DualVariableGadget validUntil;
    DualVariableGadget onchainDataHash;
    DualVariableGadget maxFee;
    DualVariableGadget storageID;
    DualVariableGadget type;
    DualVariableGadget useAppKey;

    DualVariableGadget minGas;
    DualVariableGadget to;

    ToBitsGadget disableAppKeyWithdraw;

    // Special case protocol fee withdrawal
    EqualGadget isWithdrawalTx;
    EqualGadget isProtocolFeeWithdrawal;
    TernaryGadget ownerValue;
    ToBitsGadget owner;

    // choose verify key, appKey or assetKey
    TernaryGadget resolvedAuthorX;
    TernaryGadget resolvedAuthorY;

    IfThenRequireEqualGadget ifUseAppKey_then_require_enable_switch;

    // onChainDataHash is the hash of withdrawal attributes: minGas, to and amount
    OnChainDataHashGadget onchainDataHashCalculate;

    Poseidon_10 hash;

    RequireLtGadget requireValidUntil;
    RequireLeqGadget requireValidFee;
    std::unique_ptr<IfThenRequireEqualGadget> requireValidOnChainDataHash;


    // Type
    IsNonZero isConditional;
    NotGadget needsSignature;

    // Balances
    DynamicBalanceGadget balanceS_A;
    DynamicBalanceGadget balanceD_O;

    TernaryGadget fullBalance;
    EqualGadget amountIsZero;
    EqualGadget amountIsFullBalance;
    EqualGadget validFullWithdrawalType;
    EqualGadget invalidFullWithdrawalType;
    IfThenRequireGadget checkValidFullWithdrawal;
    IfThenRequireGadget checkInvalidFullWithdrawal;

    // Fee balances
    DynamicBalanceGadget balanceB_A;
    DynamicBalanceGadget balanceA_O;
    StorageReaderGadget storageReader;
    // Fee as float
    FloatGadget fFee;
    RequireAccuracyGadget requireAccuracyFee;
    // Fee payment from From to the operator
    TransferGadget feePayment;

    // Calculate the new balance
    TernaryGadget amountA;
    TernaryGadget amountP;
    SubGadget balanceA_after;
    SubGadget balanceO_after;
    ArrayTernaryGadget merkleTreeAccountA;

    // Update the nonce storage (unless it's a forced withdrawal)
    OrGadget isForcedWithdrawal;
    NotGadget isNotForcedWithdrawal;
    AndGadget doCheckNonce;
    NonceGadget nonce;
    TernaryGadget storageDataValue;
    TernaryGadget storageIdValue;
    TernaryGadget tokenSIDValue;
    TernaryGadget tokenBIDValue;
    TernaryGadget gasFeeValue;
    TernaryGadget cancelledValue;
    TernaryGadget forwardValue;

    // Increase the number of conditional transactions
    UnsafeAddGadget numConditionalTransactionsAfter;

    WithdrawCircuit( //
      ProtoboardT &pb,
      const TransactionState &state,
      const std::string &prefix)
        : BaseTransactionCircuit(pb, state, prefix),

          // Inputs
          accountID(pb, NUM_BITS_ACCOUNT, FMT(prefix, ".accountID")),
          tokenID(pb, NUM_BITS_TOKEN, FMT(prefix, ".tokenID")),
          // 248bits for withdraw
          amount(pb, NUM_BITS_AMOUNT_WITHDRAW, FMT(prefix, ".amount")),
          feeTokenID(pb, NUM_BITS_TOKEN, FMT(prefix, ".feeTokenID")),
          fee(pb, NUM_BITS_AMOUNT, FMT(prefix, ".fee")),
          validUntil(pb, NUM_BITS_TIMESTAMP, FMT(prefix, ".validUntil")),
          onchainDataHash(pb, NUM_BITS_HASH, FMT(prefix, ".onchainDataHash")),
          maxFee(pb, NUM_BITS_AMOUNT, FMT(prefix, ".maxFee")),
          storageID(pb, NUM_BITS_STORAGEID, FMT(prefix, ".storageID")),
          type(pb, NUM_BITS_TYPE, FMT(prefix, ".type")),
          useAppKey(pb, NUM_BITS_BYTE, FMT(prefix, ".useAppKey")),

          minGas(pb, NUM_BITS_MIN_GAS, FMT(prefix, ".minGas")),
          to(pb, NUM_BITS_ADDRESS, FMT(prefix, ".to")),

          disableAppKeyWithdraw(pb, state.accountA.account.disableAppKeyWithdraw, 1, FMT(prefix, ".disableAppKeyWithdraw")),

          // Special case protocol fee withdrawal
          isWithdrawalTx( //
            pb,
            state.type,
            state.constants.txTypeWithdrawal,
            FMT(prefix, ".isWithdrawalTx")),
          isProtocolFeeWithdrawal( //
            pb,
            state.constants._1,
            state.constants._0,
            FMT(prefix, ".isProtocolFeeWithdrawal")),
          ownerValue(
            pb,
            isProtocolFeeWithdrawal.result(),
            state.constants._0,
            state.accountA.account.owner,
            FMT(prefix, ".ownerValue")),
          owner(pb, ownerValue.result(), NUM_BITS_ADDRESS, FMT(prefix, ".owner")),

          resolvedAuthorX(
            pb,
            useAppKey.packed,
            state.accountA.account.appKeyPublicKey.x,
            state.accountA.account.publicKey.x,
            FMT(prefix, ".resolvedAuthorX")),
          resolvedAuthorY(
            pb,
            useAppKey.packed,
            state.accountA.account.appKeyPublicKey.y,
            state.accountA.account.publicKey.y,
            FMT(prefix, ".resolvedAuthorY")),

          ifUseAppKey_then_require_enable_switch(
            pb,
            useAppKey.packed,
            disableAppKeyWithdraw.packed,
            state.constants._0,
            FMT(prefix, ".ifUseAppKey_then_require_enable_switch")),
          // Signature
          onchainDataHashCalculate(
            pb,
            FMT(prefix, ".onchainDataHashCalculate")),
          hash(
            pb,
            var_array(
              {state.exchange,
               accountID.packed,
               tokenID.packed,
               amount.packed,
               feeTokenID.packed,
               maxFee.packed,
               onchainDataHash.packed,
               validUntil.packed,
               storageID.packed,
               useAppKey.packed
              }),
            FMT(this->annotation_prefix, ".hash")),

          // Validate
          requireValidUntil(
            pb,
            state.timestamp,
            validUntil.packed,
            NUM_BITS_TIMESTAMP,
            FMT(prefix, ".requireValidUntil")),
          requireValidFee(pb, fee.packed, maxFee.packed, NUM_BITS_AMOUNT, FMT(prefix, ".requireValidFee")),

          // Type
          isConditional(pb, type.packed, FMT(prefix, ".isConditional")),
          needsSignature(pb, isConditional.result(), FMT(prefix, ".needsSignature")),

          // Balances
          balanceS_A(pb, state.accountA.balanceS, FMT(prefix, ".balanceS_A")),
          balanceD_O(pb, state.oper.balanceD, FMT(prefix, ".balanceD_O")),

          fullBalance(
            pb,
            isProtocolFeeWithdrawal.result(),
            balanceD_O.balance(),
            balanceS_A.balance(),
            FMT(prefix, ".fullBalance")),
          amountIsZero(pb, amount.packed, state.constants._0, FMT(prefix, ".amountIsZero")),
          amountIsFullBalance(pb, amount.packed, fullBalance.result(), FMT(prefix, ".amountIsFullBalance")),
          validFullWithdrawalType(pb, type.packed, state.constants._2, FMT(prefix, ".validFullWithdrawalType")),
          invalidFullWithdrawalType(pb, type.packed, state.constants._3, FMT(prefix, ".invalidFullWithdrawalType")),
          checkValidFullWithdrawal(
            pb,
            validFullWithdrawalType.result(),
            amountIsFullBalance.result(),
            FMT(prefix, ".checkValidFullWithdrawal")),
          checkInvalidFullWithdrawal(
            pb,
            invalidFullWithdrawalType.result(),
            amountIsZero.result(),
            FMT(prefix, ".checkInvalidFullWithdrawal")),

          // Fee balances
          balanceB_A(pb, state.accountA.balanceB, FMT(prefix, ".balanceB_A")),
          balanceA_O(pb, state.oper.balanceA, FMT(prefix, ".balanceA_O")),
          storageReader(
            pb,
            state.constants,
            state.accountA.storage,
            storageID,
            isWithdrawalTx.result(),
            FMT(prefix, ".storageReader")),
          // Fee as float
          fFee(pb, state.constants, Float16Encoding, FMT(prefix, ".fFee")),
          requireAccuracyFee(
            pb,
            fFee.value(),
            fee.packed,
            Float16Accuracy,
            NUM_BITS_AMOUNT,
            FMT(prefix, ".requireAccuracyFee")),

          feePayment(pb, balanceB_A, balanceA_O, fFee.value(), FMT(prefix, ".feePayment")),

          // Calculate the new balance
          amountA(pb, isProtocolFeeWithdrawal.result(), state.constants._0, amount.packed, FMT(prefix, ".amountA")),
          amountP(pb, isProtocolFeeWithdrawal.result(), amount.packed, state.constants._0, FMT(prefix, ".amountP")),
          balanceA_after(pb, balanceS_A.balance(), amountA.result(), NUM_BITS_AMOUNT_WITHDRAW, FMT(prefix, ".balanceA_after")),
          balanceO_after(pb, balanceD_O.balance(), amountP.result(), NUM_BITS_AMOUNT_WITHDRAW, FMT(prefix, ".balanceO_after")),
          merkleTreeAccountA(
            pb,
            isProtocolFeeWithdrawal.result(),
            flatten({VariableArrayT(1, state.constants._1), VariableArrayT(NUM_BITS_ACCOUNT - 1, state.constants._0)}),
            accountID.bits,
            FMT(prefix, ".merkleTreeAccountA")),

          // Update the nonce storage (unless it's a forced withdrawal)
          isForcedWithdrawal(
            pb,
            {validFullWithdrawalType.result(), invalidFullWithdrawalType.result()},
            FMT(prefix, ".isForcedWithdrawal")),
          isNotForcedWithdrawal(pb, isForcedWithdrawal.result(), FMT(prefix, ".isNotForcedWithdrawal")),
          doCheckNonce(pb, {isWithdrawalTx.result(), isNotForcedWithdrawal.result()}, FMT(prefix, ".doCheckNonce")),
          nonce( //
            pb,
            state.constants,
            state.accountA.storage,
            storageID,
            doCheckNonce.result(),
            FMT(prefix, ".nonce")),
          storageDataValue(
            pb,
            isForcedWithdrawal.result(),
            state.accountA.storage.data,
            nonce.getData(),
            FMT(prefix, ".storageDataValue")),
          storageIdValue(
            pb,
            isForcedWithdrawal.result(),
            state.accountA.storage.storageID,
            storageID.packed,
            FMT(prefix, ".storageIdValue")),
          tokenSIDValue(
            pb,
            isForcedWithdrawal.result(),
            state.accountA.storage.tokenSID,
            tokenID.packed,
            FMT(prefix, ".tokenSIDValue")),
          tokenBIDValue(
            pb,
            isForcedWithdrawal.result(),
            state.accountA.storage.tokenBID,
            storageReader.getTokenBID(),
            FMT(prefix, ".tokenBIDValue")),
          gasFeeValue(
            pb,
            isForcedWithdrawal.result(),
            state.accountA.storage.gasFee,
            storageReader.getGasFee(),
            FMT(prefix, ".gasFeeValue")),
          cancelledValue(
            pb,
            isForcedWithdrawal.result(),
            state.accountA.storage.cancelled,
            storageReader.getCancelled(),
            FMT(prefix, ".cancelledValue")),
          forwardValue(
            pb,
            isForcedWithdrawal.result(),
            state.accountA.storage.forward,
            storageReader.getForward(),
            FMT(prefix, ".forwardValue")),

          // Increase the number of conditional transactions
          numConditionalTransactionsAfter(
            pb,
            state.numConditionalTransactions,
            state.constants._1,
            FMT(prefix, ".numConditionalTransactionsAfter"))
    {
        LOG(LogDebug, "in WithdrawCircuit", "");
        // Set the account
        setArrayOutput(TXV_ACCOUNT_A_ADDRESS, merkleTreeAccountA.result());

        // Update the account balances (withdrawal + fee)
        setArrayOutput(TXV_BALANCE_A_S_ADDRESS, tokenID.bits);
        setOutput(TXV_BALANCE_A_S_BALANCE, balanceA_after.result());
        setArrayOutput(TXV_BALANCE_A_B_ADDRESS, feeTokenID.bits);
        setOutput(TXV_BALANCE_A_B_BALANCE, balanceB_A.balance());

        // Update the protocol fee pool balance when withdrawing from the protocol
        // pool
        setOutput(TXV_BALANCE_O_D_BALANCE, balanceO_after.result());
        setArrayOutput(TXV_BALANCE_O_D_Address, tokenID.bits);

        // Update the operator balance for the fee payment
        setArrayOutput(TXV_BALANCE_O_A_Address, feeTokenID.bits);
        setOutput(TXV_BALANCE_O_A_BALANCE, balanceA_O.balance());

        // Verify a single signature of the account owner (if not conditional)
        setOutput(TXV_HASH_A, hash.result());
        setOutput(TXV_PUBKEY_X_A, resolvedAuthorX.result());
        setOutput(TXV_PUBKEY_Y_A, resolvedAuthorY.result());
        setOutput(TXV_SIGNATURE_REQUIRED_A, needsSignature.result());
        setOutput(TXV_SIGNATURE_REQUIRED_B, state.constants._0);

        // Increase the number of conditional transactions
        setOutput(TXV_NUM_CONDITIONAL_TXS, numConditionalTransactionsAfter.result());

        // Nonce
        setArrayOutput(TXV_STORAGE_A_ADDRESS, subArray(storageID.bits, 0, NUM_BITS_STORAGE_ADDRESS));
        setOutput(TXV_STORAGE_A_TOKENSID, tokenSIDValue.result());
        setOutput(TXV_STORAGE_A_TOKENBID, tokenBIDValue.result());
        setOutput(TXV_STORAGE_A_DATA, storageDataValue.result());
        setOutput(TXV_STORAGE_A_STORAGEID, storageIdValue.result());
        setOutput(TXV_STORAGE_A_GASFEE, gasFeeValue.result());
        setOutput(TXV_STORAGE_A_CANCELLED, cancelledValue.result());
        setOutput(TXV_STORAGE_A_FORWARD, forwardValue.result());
    }

    void generate_r1cs_witness(const Withdrawal &withdrawal)
    {
        LOG(LogDebug, "in WithdrawCircuit", "generate_r1cs_witness");
        // Inputs
        accountID.generate_r1cs_witness(pb, withdrawal.accountID);
        tokenID.generate_r1cs_witness(pb, withdrawal.tokenID);
        amount.generate_r1cs_witness(pb, withdrawal.amount);
        feeTokenID.generate_r1cs_witness(pb, withdrawal.feeTokenID);
        fee.generate_r1cs_witness(pb, withdrawal.fee);
        validUntil.generate_r1cs_witness(pb, withdrawal.validUntil);
        onchainDataHash.generate_r1cs_witness(pb, withdrawal.onchainDataHash);
        maxFee.generate_r1cs_witness(pb, withdrawal.maxFee);
        storageID.generate_r1cs_witness(pb, withdrawal.storageID);
        type.generate_r1cs_witness(pb, withdrawal.type);
        useAppKey.generate_r1cs_witness(pb, withdrawal.useAppKey);

        minGas.generate_r1cs_witness(pb, withdrawal.minGas);
        to.generate_r1cs_witness(pb, withdrawal.to);

        disableAppKeyWithdraw.generate_r1cs_witness();

        // Special case protocol fee withdrawal
        isWithdrawalTx.generate_r1cs_witness();
        isProtocolFeeWithdrawal.generate_r1cs_witness();
        ownerValue.generate_r1cs_witness();
        owner.generate_r1cs_witness();

        resolvedAuthorX.generate_r1cs_witness();
        resolvedAuthorY.generate_r1cs_witness();

        ifUseAppKey_then_require_enable_switch.generate_r1cs_witness();

        // Signature
        onchainDataHashCalculate.generate_r1cs_witness();
        hash.generate_r1cs_witness();

        // Validate
        requireValidUntil.generate_r1cs_witness();
        requireValidFee.generate_r1cs_witness();
        
        requireValidOnChainDataHash->generate_r1cs_witness();

        // Type
        isConditional.generate_r1cs_witness();
        needsSignature.generate_r1cs_witness();

        // Balances
        balanceS_A.generate_r1cs_witness();
        balanceD_O.generate_r1cs_witness();

        fullBalance.generate_r1cs_witness();
        amountIsZero.generate_r1cs_witness();
        amountIsFullBalance.generate_r1cs_witness();
        validFullWithdrawalType.generate_r1cs_witness();
        invalidFullWithdrawalType.generate_r1cs_witness();
        checkValidFullWithdrawal.generate_r1cs_witness();
        checkInvalidFullWithdrawal.generate_r1cs_witness();

        // Fee balances
        balanceB_A.generate_r1cs_witness();
        balanceA_O.generate_r1cs_witness();
        storageReader.generate_r1cs_witness();
        // Fee as float
        fFee.generate_r1cs_witness(toFloat(withdrawal.fee, Float16Encoding));
        requireAccuracyFee.generate_r1cs_witness();
        // Fee payment from to the operator
        feePayment.generate_r1cs_witness();

        // Calculate the new balance
        amountA.generate_r1cs_witness();
        amountP.generate_r1cs_witness();
        balanceA_after.generate_r1cs_witness();
        balanceO_after.generate_r1cs_witness();
        merkleTreeAccountA.generate_r1cs_witness();

        // Update the nonce storage (unless it's a forced withdrawal)
        isForcedWithdrawal.generate_r1cs_witness();
        isNotForcedWithdrawal.generate_r1cs_witness();
        doCheckNonce.generate_r1cs_witness();
        nonce.generate_r1cs_witness();
        storageDataValue.generate_r1cs_witness();
        storageIdValue.generate_r1cs_witness();
        tokenSIDValue.generate_r1cs_witness();
        tokenBIDValue.generate_r1cs_witness();
        gasFeeValue.generate_r1cs_witness();
        cancelledValue.generate_r1cs_witness();
        forwardValue.generate_r1cs_witness();

        // Increase the number of conditional transactions
        numConditionalTransactionsAfter.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        LOG(LogDebug, "in WithdrawCircuit: generate_r1cs_constraints", "");
        // Inputs
        accountID.generate_r1cs_constraints(true);
        tokenID.generate_r1cs_constraints(true);
        amount.generate_r1cs_constraints(true);
        feeTokenID.generate_r1cs_constraints(true);
        fee.generate_r1cs_constraints(true);
        validUntil.generate_r1cs_constraints(true);
        onchainDataHash.generate_r1cs_constraints(true);
        maxFee.generate_r1cs_constraints(true);
        storageID.generate_r1cs_constraints(true);
        type.generate_r1cs_constraints(true);
        useAppKey.generate_r1cs_constraints(true);

        minGas.generate_r1cs_constraints(true);
        to.generate_r1cs_constraints(true);

        disableAppKeyWithdraw.generate_r1cs_constraints();

        // Special case protocol fee withdrawal
        isWithdrawalTx.generate_r1cs_constraints();
        isProtocolFeeWithdrawal.generate_r1cs_constraints();
        ownerValue.generate_r1cs_constraints();
        owner.generate_r1cs_constraints();

        resolvedAuthorX.generate_r1cs_constraints();
        resolvedAuthorY.generate_r1cs_constraints();

        ifUseAppKey_then_require_enable_switch.generate_r1cs_constraints();
        
        // Signature
        onchainDataHashCalculate.add(minGas.bits);
        onchainDataHashCalculate.add(to.bits);
        onchainDataHashCalculate.add(amount.bits);
        onchainDataHashCalculate.generate_r1cs_constraints();
        hash.generate_r1cs_constraints();

        // Validate
        requireValidUntil.generate_r1cs_constraints();
        requireValidFee.generate_r1cs_constraints();

        requireValidOnChainDataHash.reset(new IfThenRequireEqualGadget(pb, isWithdrawalTx.result(), onchainDataHash.packed, onchainDataHashCalculate.result(), FMT(annotation_prefix, ".requireValidOnChainDataHash"))),
        requireValidOnChainDataHash->generate_r1cs_constraints();

        // Type
        isConditional.generate_r1cs_constraints();
        needsSignature.generate_r1cs_constraints();

        // Balances
        balanceS_A.generate_r1cs_constraints();
        balanceD_O.generate_r1cs_constraints();

        fullBalance.generate_r1cs_constraints();
        amountIsZero.generate_r1cs_constraints();
        amountIsFullBalance.generate_r1cs_constraints();
        validFullWithdrawalType.generate_r1cs_constraints();
        invalidFullWithdrawalType.generate_r1cs_constraints();
        checkValidFullWithdrawal.generate_r1cs_constraints();
        checkInvalidFullWithdrawal.generate_r1cs_constraints();

        // Fee balances
        balanceB_A.generate_r1cs_constraints();
        balanceA_O.generate_r1cs_constraints();
        storageReader.generate_r1cs_constraints();
        // Fee as float
        fFee.generate_r1cs_constraints();
        requireAccuracyFee.generate_r1cs_constraints();
        // Fee payment from to the operator
        feePayment.generate_r1cs_constraints();

        // Calculate the new balance
        amountA.generate_r1cs_constraints();
        amountP.generate_r1cs_constraints();
        balanceA_after.generate_r1cs_constraints();
        balanceO_after.generate_r1cs_constraints();
        merkleTreeAccountA.generate_r1cs_constraints();

        // Update the nonce storage (unless it's a forced withdrawal)
        isForcedWithdrawal.generate_r1cs_constraints();
        isNotForcedWithdrawal.generate_r1cs_constraints();
        doCheckNonce.generate_r1cs_constraints();
        nonce.generate_r1cs_constraints();
        storageDataValue.generate_r1cs_constraints();
        storageIdValue.generate_r1cs_constraints();
        tokenSIDValue.generate_r1cs_constraints();
        tokenBIDValue.generate_r1cs_constraints();
        gasFeeValue.generate_r1cs_constraints();
        cancelledValue.generate_r1cs_constraints();
        forwardValue.generate_r1cs_constraints();

        // Increase the number of conditional transactions
        numConditionalTransactionsAfter.generate_r1cs_constraints();
    }

    const VariableArrayT getPublicData() const
    {
        return flattenReverse(
          {
           type.bits,
           owner.bits,
           accountID.bits,
           tokenID.bits,
           feeTokenID.bits,
           fFee.bits(),
           storageID.bits,
           onchainDataHash.bits
           });
    }
};

} // namespace Loopring

#endif
