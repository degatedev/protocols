// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
#ifndef _APPKEYUPDATECIRCUIT_H_
#define _APPKEYUPDATECIRCUIT_H_

#include "Circuit.h"
#include "../Utils/Constants.h"
#include "../Utils/Data.h"

#include "ethsnarks.hpp"
#include "utils.hpp"

#include "../Gadgets/SignatureGadgets.h"

using namespace ethsnarks;

namespace Loopring
{
// To split the original key function and make the transaction more flexible. AssetKey(original key) is handed over to the institution manager, 
// who has the final right to manage the account. The manager can set the permission of AppKey(trade, transfer and withdraw), and then hand the AppKey to the trader for trading.
class AppKeyUpdateCircuit : public BaseTransactionCircuit
{
  public:
    DualVariableGadget typeTx;
    // type used 3bits, but 3bits can not convert to hex, can not less then 4bits, so need to pad 1bit
    DualVariableGadget typeTxPad;
    // Inputs
    DualVariableGadget accountID;
    DualVariableGadget validUntil;
    ToBitsGadget nonce;
    VariableT appKeyPublicKeyX;
    VariableT appKeyPublicKeyY;
    DualVariableGadget feeTokenID;
    DualVariableGadget fee;
    DualVariableGadget maxFee;
    DualVariableGadget disableAppKeySpotTrade;
    DualVariableGadget disableAppKeyWithdraw;
    DualVariableGadget disableAppKeyTransferToOther;

    // Signature
    Poseidon_11 hash;

    // Validate
    RequireLtGadget requireValidUntil;
    RequireLeqGadget requireValidFee;

    // Compress the public key
    CompressPublicKey compressPublicKey;

    // Balances
    DynamicBalanceGadget balanceS_A;
    DynamicBalanceGadget balanceB_O;
    // Fee as float
    FloatGadget fFee;
    RequireAccuracyGadget requireAccuracyFee;
    // Fee payment from From to the operator
    TransferGadget feePayment;

    // Increase the nonce
    AddGadget nonce_after;

    AppKeyUpdateCircuit( //
      ProtoboardT &pb,
      const TransactionState &state,
      const std::string &prefix)
        : BaseTransactionCircuit(pb, state, prefix),

          typeTx(pb, NUM_BITS_TX_TYPE, FMT(prefix, ".typeTx")),
          typeTxPad(pb, NUM_BITS_BIT, FMT(prefix, ".typeTxPad")),
          // Inputs
          accountID(pb, NUM_BITS_ACCOUNT, FMT(prefix, ".accountID")),
          validUntil(pb, NUM_BITS_TIMESTAMP, FMT(prefix, ".validUntil")),
          nonce(pb, state.accountA.account.nonce, NUM_BITS_NONCE, FMT(prefix, ".nonce")),
          appKeyPublicKeyX(make_variable(pb, FMT(prefix, ".appKeyPublicKeyX"))),
          appKeyPublicKeyY(make_variable(pb, FMT(prefix, ".appKeyPublicKeyY"))),
          feeTokenID(pb, NUM_BITS_TOKEN, FMT(prefix, ".feeTokenID")),
          fee(pb, NUM_BITS_AMOUNT, FMT(prefix, ".fee")),
          maxFee(pb, NUM_BITS_AMOUNT, FMT(prefix, ".maxFee")),
          disableAppKeySpotTrade(pb, NUM_BITS_BIT, FMT(prefix, ".disableAppKeySpotTrade")),
          disableAppKeyWithdraw(pb, NUM_BITS_BIT, FMT(prefix, ".disableAppKeyWithdraw")),
          disableAppKeyTransferToOther(pb, NUM_BITS_BIT, FMT(prefix, ".disableAppKeyTransferToOther")),

          // Signature
          hash(
            pb,
            var_array({
              state.exchange,
              accountID.packed,
              feeTokenID.packed,
              maxFee.packed,
              appKeyPublicKeyX,
              appKeyPublicKeyY,
              validUntil.packed,
              nonce.packed,
              disableAppKeySpotTrade.packed,
              disableAppKeyWithdraw.packed,
              disableAppKeyTransferToOther.packed
            }),
            FMT(this->annotation_prefix, ".hash")),

          requireValidUntil(
            pb,
            state.timestamp,
            validUntil.packed,
            NUM_BITS_TIMESTAMP,
            FMT(prefix, ".requireValidUntil")),
          requireValidFee(pb, fee.packed, maxFee.packed, NUM_BITS_AMOUNT, FMT(prefix, ".requireValidFee")),

          // Compress the public key
          compressPublicKey(
            pb,
            state.params,
            state.constants,
            appKeyPublicKeyX,
            appKeyPublicKeyY,
            FMT(this->annotation_prefix, ".compressPublicKey")),

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

          // Increase the nonce
          nonce_after(
            pb,
            state.accountA.account.nonce,
            state.constants._1,
            NUM_BITS_NONCE,
            FMT(prefix, ".nonce_after"))
    {
        LOG(LogDebug, "in AppKeyUpdateCircuit", "");
        // Update the account data
        setArrayOutput(TXV_ACCOUNT_A_ADDRESS, accountID.bits);
        setOutput(TXV_ACCOUNT_A_APPKEY_PUBKEY_X, appKeyPublicKeyX);
        setOutput(TXV_ACCOUNT_A_APPKEY_PUBKEY_Y, appKeyPublicKeyY);
        setOutput(TXV_ACCOUNT_A_NONCE, nonce_after.result());

        setOutput(TXV_ACCOUNT_A_DISABLE_APPKEY_SPOTTRADE, disableAppKeySpotTrade.packed);
        setOutput(TXV_ACCOUNT_A_DISABLE_APPKEY_WITHDRAW_TO_OTHER, disableAppKeyWithdraw.packed);
        setOutput(TXV_ACCOUNT_A_DISABLE_APPKEY_TRANSFER_TO_OTHER, disableAppKeyTransferToOther.packed);

        // Update the account balance for the fee payment
        setArrayOutput(TXV_BALANCE_A_S_ADDRESS, feeTokenID.bits);
        setOutput(TXV_BALANCE_A_S_BALANCE, balanceS_A.balance());
        // Update the operator balance for the fee payment
        setArrayOutput(TXV_BALANCE_O_B_Address, feeTokenID.bits);
        setOutput(TXV_BALANCE_O_B_BALANCE, balanceB_O.balance());

        // We need a single signature of the account that's being updated if not
        // conditional
        setOutput(TXV_HASH_A, hash.result());
        setOutput(TXV_SIGNATURE_REQUIRED_A, state.constants._1);
        setOutput(TXV_SIGNATURE_REQUIRED_B, state.constants._0);
    }

    void generate_r1cs_witness(const AppKeyUpdate &update)
    {
        LOG(LogDebug, "in AppKeyUpdateCircuit", "generate_r1cs_witness");
        typeTx.generate_r1cs_witness(pb, ethsnarks::FieldT(int(Loopring::TransactionType::AppKeyUpdate)));
        typeTxPad.generate_r1cs_witness(pb, ethsnarks::FieldT(0));
        // Inputs
        accountID.generate_r1cs_witness(pb, update.accountID);
        validUntil.generate_r1cs_witness(pb, update.validUntil);
        nonce.generate_r1cs_witness();
        pb.val(appKeyPublicKeyX) = update.appKeyPublicKeyX;
        pb.val(appKeyPublicKeyY) = update.appKeyPublicKeyY;
        feeTokenID.generate_r1cs_witness(pb, update.feeTokenID);
        fee.generate_r1cs_witness(pb, update.fee);
        maxFee.generate_r1cs_witness(pb, update.maxFee);
        disableAppKeySpotTrade.generate_r1cs_witness(pb, update.disableAppKeySpotTrade);
        disableAppKeyWithdraw.generate_r1cs_witness(pb, update.disableAppKeyWithdraw);
        disableAppKeyTransferToOther.generate_r1cs_witness(pb, update.disableAppKeyTransferToOther);

        // Signature
        hash.generate_r1cs_witness();

        // Validate
        requireValidUntil.generate_r1cs_witness();
        requireValidFee.generate_r1cs_witness();

        // Compress the public key
        compressPublicKey.generate_r1cs_witness();

        // Balances
        balanceS_A.generate_r1cs_witness();
        balanceB_O.generate_r1cs_witness();
        // Fee as float
        fFee.generate_r1cs_witness(toFloat(update.fee, Float16Encoding));
        requireAccuracyFee.generate_r1cs_witness();
        // Fee payment from to the operator
        feePayment.generate_r1cs_witness();

        // Increase the nonce
        nonce_after.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        LOG(LogDebug, "in AppKeyUpdateCircuit", "generate_r1cs_constraints");
        typeTx.generate_r1cs_constraints(true);
        typeTxPad.generate_r1cs_constraints(true);
        // Inputs
        accountID.generate_r1cs_constraints(true);
        validUntil.generate_r1cs_constraints(true);
        nonce.generate_r1cs_constraints();
        feeTokenID.generate_r1cs_constraints(true);
        fee.generate_r1cs_constraints(true);
        maxFee.generate_r1cs_constraints(true);
        disableAppKeySpotTrade.generate_r1cs_constraints(true);
        disableAppKeyWithdraw.generate_r1cs_constraints(true);
        disableAppKeyTransferToOther.generate_r1cs_constraints(true);

        // Signature
        hash.generate_r1cs_constraints();

        // Validate
        requireValidUntil.generate_r1cs_constraints();
        requireValidFee.generate_r1cs_constraints();

        // Compress the public key
        compressPublicKey.generate_r1cs_constraints();

        // Balances
        balanceS_A.generate_r1cs_constraints();
        balanceB_O.generate_r1cs_constraints();
        // Fee as float
        fFee.generate_r1cs_constraints();
        requireAccuracyFee.generate_r1cs_constraints();
        // Fee payment from to the operator
        feePayment.generate_r1cs_constraints();

        // Increase the nonce
        nonce_after.generate_r1cs_constraints();
    }

    const VariableArrayT getPublicData() const
    {
        return flattenReverse({
          typeTx.bits,
          typeTxPad.bits,
          accountID.bits,
          feeTokenID.bits,
          fFee.bits(),
          nonce.bits
        });
    }
};

} // namespace Loopring

#endif
