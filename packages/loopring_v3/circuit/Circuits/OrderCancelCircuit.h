// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 DeGate DAO
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

// OrderCancel is used for on-chain cancellation. Cancellation actually only involves the canceled field of the storage node.
// If the cancelled field is marked as 1, it means that the order has been cancelled successfully.
class OrderCancelCircuit : public BaseTransactionCircuit
{
  public:
    ToBitsGadget typeTx;
    ToBitsGadget typeTxPad;
    // Inputs
    DualVariableGadget accountID;
    DualVariableGadget storageID;
    DualVariableGadget feeTokenID;
    DualVariableGadget fee;
    DualVariableGadget maxFee;
    DualVariableGadget useAppKey;

    // Signature
    Poseidon_6 hash;

    // choose verify key
    // To cancel an order, assetKey or appKey can be used
    TernaryGadget resolvedAuthorX;
    TernaryGadget resolvedAuthorY;

    // Validate
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
    // need to ensure that: 
    // 1. the provided storageID is valid on the storageNode, storageID >= storageNode.storageID
    // 2. can't cancel repeatedly, storageNode.cancelled == 0
    OrderCancelledNonceGadget nonce;

    OrderCancelCircuit( //
      ProtoboardT &pb,
      const TransactionState &state,
      const std::string &prefix)
        : BaseTransactionCircuit(pb, state, prefix),

          typeTx(pb, state.constants.txTypeOrderCancel, NUM_BITS_TX_TYPE, FMT(prefix, ".typeTx")),
          typeTxPad(pb, state.constants._0, NUM_BITS_BIT, FMT(prefix, ".typeTxPad")),
          // Inputs
          accountID(pb, NUM_BITS_ACCOUNT, FMT(prefix, ".accountID")),
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
               accountID.packed,
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
          nonce(pb, state.constants, state.accountA.storage, storageID, isOrderCancelTx.result(), FMT(prefix, ".nonce"))
    {
        LOG(LogDebug, "in OrderCancelCircuit", "");
        // Update the account data
        setArrayOutput(TXV_ACCOUNT_A_ADDRESS, accountID.bits);
        // cancel order, cancelled variable must be 1
        setOutput(TXV_STORAGE_A_CANCELLED, nonce.getCancelled());
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
        typeTx.generate_r1cs_witness();
        typeTxPad.generate_r1cs_witness();

        accountID.generate_r1cs_witness(pb, update.accountID);
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
        nonce.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        LOG(LogDebug, "in OrderCancelCircuit", "generate_r1cs_constraints");
        typeTx.generate_r1cs_constraints();
        typeTxPad.generate_r1cs_constraints();
        // Inputs
        accountID.generate_r1cs_constraints(true);
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
        nonce.generate_r1cs_constraints();
    }

    const VariableArrayT getPublicData() const
    {
        return flattenReverse({
          typeTx.bits, 
          typeTxPad.bits,
          accountID.bits, 
          storageID.bits, 
          feeTokenID.bits, 
          fFee.bits()
          });
    }
};

} // namespace Loopring

#endif
