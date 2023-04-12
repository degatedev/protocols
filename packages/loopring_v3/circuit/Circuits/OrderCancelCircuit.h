// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
#ifndef _ORDERCANCELCIRCUIT_H_
#define _ORDERCANCELCIRCUIT_H_

#include "Circuit.h"
#include "../Utils/Constants.h"
#include "../Utils/Data.h"

#include "ethsnarks.hpp"
#include "utils.hpp"

#include "../Gadgets/SignatureGadgets.h"

using namespace ethsnarks;

namespace Loopring
{
// DEG-146:order cancel
class OrderCancelCircuit : public BaseTransactionCircuit
{
  public:
    DualVariableGadget typeTx;
    // type is 3bits, need add 1bit, then 4bits can convert to 1 hex char
    DualVariableGadget typeTxPad;
    // Inputs
    // DualVariableGadget owner;
    DualVariableGadget accountID;
    // DualVariableGadget tokenID;
    DualVariableGadget storageID;
    DualVariableGadget feeTokenID;
    DualVariableGadget fee;
    DualVariableGadget maxFee;
    DualVariableGadget useAppKey;

    // Signature
    Poseidon_6 hash;
    // Poseidon_7 hash;

    // choose verify key
    TernaryGadget resolvedAuthorX;
    TernaryGadget resolvedAuthorY;

    // Validate
    // OwnerValidGadget ownerValid;
    RequireLeqGadget requireValidFee;
    EqualGadget isOrderCancelTx;

    // Balances
    DynamicBalanceGadget balanceS_A;
    DynamicBalanceGadget balanceB_O;
    // Fee as float
    FloatGadget fFee;
    RequireAccuracyGadget requireAccuracyFee;
    // Fee payment from From to the operator
    TransferGadget feePayment;    
    // DEG-148 Review fix
    OrderCancelledNonceGadget nonce;


    OrderCancelCircuit( //
      ProtoboardT &pb,
      const TransactionState &state,
      const std::string &prefix)
        : BaseTransactionCircuit(pb, state, prefix),

          typeTx(pb, NUM_BITS_TX_TYPE, FMT(prefix, ".typeTx")),
          typeTxPad(pb, NUM_BITS_BIT, FMT(prefix, ".typeTxPad")),
          // Inputs
          // owner(pb, NUM_BITS_ADDRESS, FMT(prefix, ".owner")),
          accountID(pb, NUM_BITS_ACCOUNT, FMT(prefix, ".accountID")),
          // tokenID(pb, NUM_BITS_TOKEN, FMT(prefix, ".tokenID")),
          storageID(pb, NUM_BITS_STORAGEID, FMT(prefix, ".storageID")),
          feeTokenID(pb, NUM_BITS_TOKEN, FMT(prefix, ".feeTokenID")),
          fee(pb, NUM_BITS_AMOUNT, FMT(prefix, ".fee")),
          maxFee(pb, NUM_BITS_AMOUNT, FMT(prefix, ".maxFee")),
          useAppKey(pb, NUM_BITS_BYTE, FMT(prefix, ".useAppKey")),

          // Signature
          hash(
            pb,
            var_array(
              {state.exchange,
              //  owner.packed,
               accountID.packed,
              //  tokenID.packed,
               storageID.packed,
               maxFee.packed,
               feeTokenID.packed,
               useAppKey.packed}),
            FMT(this->annotation_prefix, ".hash")),

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

          // Validate
          // ownerValid(pb, state.constants, state.accountA.account.owner, owner.packed, FMT(prefix, ".ownerValid")),
          requireValidFee(pb, fee.packed, maxFee.packed, NUM_BITS_AMOUNT, FMT(prefix, ".requireValidFee")),
          isOrderCancelTx(pb, state.type, state.constants.txTypeOrderCancel, FMT(prefix, ".isOrderCancelTx")),

          // Balances
          balanceS_A(pb, state.accountA.balanceS, FMT(prefix, ".balanceS_A")),
          balanceB_O(pb, state.oper.balanceB, FMT(prefix, ".balanceB_O")),
          // Fee as float
          fFee(pb, state.constants, Float16Encoding, FMT(prefix, ".fFee")),
          requireAccuracyFee(
            pb,
            fFee.value(),
            fee.packed,
            Float16Accuracy,
            NUM_BITS_AMOUNT,
            FMT(prefix, ".requireAccuracyFee")),
          // Fee payment from to the operator
          feePayment(pb, balanceS_A, balanceB_O, fFee.value(), FMT(prefix, ".feePayment")),
          // DEG-148 Review fix
          nonce(pb, state.constants, state.accountA.storage, storageID, isOrderCancelTx.result(), FMT(prefix, ".nonce"))
    {
        LOG(LogDebug, "in OrderCancelCircuit", "");
        // Update the account data
        setArrayOutput(TXV_ACCOUNT_A_ADDRESS, accountID.bits);
        // setOutput(TXV_ACCOUNT_A_OWNER, owner.packed);
        // cancel order, cancelled variable must be 1
        // DEG-148 Review fix
        setOutput(TXV_STORAGE_A_CANCELLED, nonce.getCancelled());
        // setOutput(TXV_STORAGE_A_CANCELLED, state.constants._1);
        // DEG-146 order cancel
        setArrayOutput(TXV_STORAGE_A_ADDRESS, subArray(storageID.bits, 0, NUM_BITS_STORAGE_ADDRESS));
        setOutput(TXV_STORAGE_A_STORAGEID, storageID.packed);

        // Update the account balance for the fee payment
        setArrayOutput(TXV_BALANCE_A_S_ADDRESS, feeTokenID.bits);
        setOutput(TXV_BALANCE_A_S_BALANCE, balanceS_A.balance());

        // Update the operator balance for the fee payment
        setArrayOutput(TXV_BALANCE_O_B_Address, feeTokenID.bits);
        setOutput(TXV_BALANCE_O_B_BALANCE, balanceB_O.balance());

        setOutput(TXV_HASH_A, hash.result());
        setOutput(TXV_PUBKEY_X_A, resolvedAuthorX.result());
        setOutput(TXV_PUBKEY_Y_A, resolvedAuthorY.result());
        setOutput(TXV_SIGNATURE_REQUIRED_A, state.constants._1);
        setOutput(TXV_SIGNATURE_REQUIRED_B, state.constants._0);
    }

    void generate_r1cs_witness(const OrderCancel &update)
    {
        LOG(LogDebug, "in OrderCancelCircuit", "generate_r1cs_witness");
        // Inputs
        // std::cout << "in OrderCancel: before owner:" << update.owner << std::endl;
        LOG(LogDebug, "in OrderCancelCircuit before update.accountID", update.accountID);
        // std::cout << "in OrderCancel: before update.tokenID:" << update.tokenID << std::endl;
        LOG(LogDebug, "in OrderCancelCircuit before update.storageID", update.storageID);
        typeTx.generate_r1cs_witness(pb, ethsnarks::FieldT(int(Loopring::TransactionType::OrderCancel)));
        typeTxPad.generate_r1cs_witness(pb, ethsnarks::FieldT(0));

        // owner.generate_r1cs_witness(pb, update.owner);
        accountID.generate_r1cs_witness(pb, update.accountID);
        // tokenID.generate_r1cs_witness(pb, update.tokenID);
        storageID.generate_r1cs_witness(pb, update.storageID);

        feeTokenID.generate_r1cs_witness(pb, update.feeTokenID);
        fee.generate_r1cs_witness(pb, update.fee);
        maxFee.generate_r1cs_witness(pb, update.maxFee);

        useAppKey.generate_r1cs_witness(pb, update.useAppKey);

        // Signature
        hash.generate_r1cs_witness();

        resolvedAuthorX.generate_r1cs_witness();
        resolvedAuthorY.generate_r1cs_witness();

        // Validate
        // ownerValid.generate_r1cs_witness();
        requireValidFee.generate_r1cs_witness();
        isOrderCancelTx.generate_r1cs_witness();

        // Balances
        balanceS_A.generate_r1cs_witness();
        balanceB_O.generate_r1cs_witness();
        // Fee as float
        fFee.generate_r1cs_witness(toFloat(update.fee, Float16Encoding));
        requireAccuracyFee.generate_r1cs_witness();
        // Fee payment from to the operator
        feePayment.generate_r1cs_witness();
        // DEG-148 Review fix
        nonce.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        LOG(LogDebug, "in OrderCancelCircuit", "generate_r1cs_constraints");
        typeTx.generate_r1cs_constraints(true);
        typeTxPad.generate_r1cs_constraints(true);
        // Inputs
        // owner.generate_r1cs_constraints();
        accountID.generate_r1cs_constraints(true);
        // tokenID.generate_r1cs_constraints(true);
        storageID.generate_r1cs_constraints(true);
        feeTokenID.generate_r1cs_constraints(true);
        fee.generate_r1cs_constraints(true);
        maxFee.generate_r1cs_constraints(true);
        useAppKey.generate_r1cs_constraints(true);

        // Signature
        hash.generate_r1cs_constraints();

        resolvedAuthorX.generate_r1cs_constraints();
        resolvedAuthorY.generate_r1cs_constraints();

        // Validate
        // ownerValid.generate_r1cs_constraints();
        requireValidFee.generate_r1cs_constraints();
        isOrderCancelTx.generate_r1cs_constraints();

        // Balances
        balanceS_A.generate_r1cs_constraints();
        balanceB_O.generate_r1cs_constraints();
        // Fee as float
        fFee.generate_r1cs_constraints();
        requireAccuracyFee.generate_r1cs_constraints();
        // Fee payment from to the operator
        feePayment.generate_r1cs_constraints();
        // DEG-148 Review fix
        nonce.generate_r1cs_constraints();
    }

    const VariableArrayT getPublicData() const
    {
        return flattenReverse({
          typeTx.bits, 
          typeTxPad.bits,
          // owner.bits, 
          accountID.bits, 
          storageID.bits, 
          feeTokenID.bits, 
          fFee.bits()
          });
    }
};

} // namespace Loopring

#endif
