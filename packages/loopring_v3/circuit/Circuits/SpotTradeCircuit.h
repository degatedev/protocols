// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
// Modified by DeGate DAO, 2022
#ifndef _SPOTTRADECIRCUIT_H_
#define _SPOTTRADECIRCUIT_H_

#include "Circuit.h"
#include "../Utils/Constants.h"
#include "../Utils/Data.h"

#include "ethsnarks.hpp"
#include "utils.hpp"

using namespace ethsnarks;

namespace Loopring
{

class SpotTradeCircuit : public BaseTransactionCircuit
{
  public:
    ToBitsGadget typeTx;
    ToBitsGadget typeTxPad;
    // Orders
    OrderGadget orderA;
    OrderGadget orderB;

    // Balances
    DynamicBalanceGadget balanceS_A;
    DynamicBalanceGadget balanceB_A;
    DynamicBalanceGadget balanceS_B;
    DynamicBalanceGadget balanceB_B;
    DynamicBalanceGadget balanceA_O;
    DynamicBalanceGadget balanceB_O;
    DynamicBalanceGadget balanceC_O;
    DynamicBalanceGadget balanceD_O;

    DynamicBalanceGadget balanceFee_A;
    DynamicBalanceGadget balanceFee_B;

    // Order fills
    FloatGadget fillS_A;
    FloatGadget fillS_B;

    RangeCheckGadget checkFillS_A;
    RangeCheckGadget checkFillS_B;

    EqualGadget isSpotTradeTx;
    // read history data on storage node
    StorageReaderGadget tradeHistory_A;
    StorageReaderGadget tradeHistory_B;

    AutoMarketOrderCheck autoMarketOrderCheckA;
    AutoMarketOrderCheck autoMarketOrderCheckB;

    // Once grid order is matched, the data and gasFee fields should be reset
    StorageReaderForAutoMarketGadget tradeHistoryWithAutoMarket_A;
    StorageReaderForAutoMarketGadget tradeHistoryWithAutoMarket_B;


    // Match orders
    OrderMatchingGadget orderMatching;

    // Calculate fees
    FeeCalculatorGadget feeCalculatorA;
    FeeCalculatorGadget feeCalculatorB;

    DualVariableGadget appointTradingFeeA;
    DualVariableGadget appointTradingFeeB;
    FloatGadget fAppointTradingFeeA;
    FloatGadget fAppointTradingFeeB;
    RequireAccuracyGadget requireAccuracyTradingFeeA;
    RequireAccuracyGadget requireAccuracyTradingFeeB;

    /* Token Transfers */
    // Actual trade
    TransferGadget fillSA_from_balanceSA_to_balanceBB;
    TransferGadget fillSB_from_balanceSB_to_balanceBA;

    TransferGadget feeA_from_accountA_fee_to_balanceAO;
    TransferGadget feeB_from_accountB_fee_to_balanceBO;
    // Trading fees
    TransferGadget protocolFeeA_from_balanceBA_to_balanceAP;
    TransferGadget protocolFeeB_from_balanceBB_to_balanceBP;

    GasFeeMatchingGadget feeMatch_A;
    GasFeeMatchingGadget feeMatch_B;

    TernaryGadget resolvedAAuthorX;
    TernaryGadget resolvedAAuthorY;

    TernaryGadget resolvedBAuthorX;
    TernaryGadget resolvedBAuthorY;

    SpotTradeCircuit( //
      ProtoboardT &pb,
      const TransactionState &state,
      const std::string &prefix)
        : BaseTransactionCircuit(pb, state, prefix),

          typeTx(pb, state.constants.txTypeSpotTrade, NUM_BITS_TX_TYPE, FMT(prefix, ".typeTx")),
          typeTxPad(pb, state.constants._0, NUM_BITS_BIT, FMT(prefix, ".typeTxPad")),
          // Orders
          orderA(pb, state.constants, state.exchange, state.protocolFeeBips, state.constants._1, state.accountA.account.disableAppKeySpotTrade, FMT(prefix, ".orderA")),
          orderB(pb, state.constants, state.exchange, state.protocolFeeBips, state.constants._1, state.accountB.account.disableAppKeySpotTrade, FMT(prefix, ".orderB")),

          // Balances
          balanceS_A(pb, state.accountA.balanceS, FMT(prefix, ".balanceS_A")),
          balanceB_A(pb, state.accountA.balanceB, FMT(prefix, ".balanceB_A")),
          balanceS_B(pb, state.accountB.balanceS, FMT(prefix, ".balanceS_B")),
          balanceB_B(pb, state.accountB.balanceB, FMT(prefix, ".balanceB_B")),
          balanceC_O(pb, state.oper.balanceC, FMT(prefix, ".balanceC_O")),
          balanceD_O(pb, state.oper.balanceD, FMT(prefix, ".balanceD_O")),
          balanceA_O(pb, state.oper.balanceA, FMT(prefix, ".balanceA_O")),
          balanceB_O(pb, state.oper.balanceB, FMT(prefix, ".balanceB_O")),

          balanceFee_A(pb, state.accountA.balanceFee, FMT(prefix, ".balanceFee_A")),
          balanceFee_B(pb, state.accountB.balanceFee, FMT(prefix, ".balanceFee_B")),

          // Order fills
          fillS_A(pb, state.constants, Float32Encoding, FMT(prefix, ".fillS_A")),
          fillS_B(pb, state.constants, Float32Encoding, FMT(prefix, ".fillS_B")),

          checkFillS_A(pb, fillS_A.value(), NUM_BITS_AMOUNT, FMT(prefix, ".checkFillS_A")),
          checkFillS_B(pb, fillS_B.value(), NUM_BITS_AMOUNT, FMT(prefix, ".checkFillS_B")),

          isSpotTradeTx(pb, state.type, state.constants.txTypeSpotTrade, FMT(prefix, ".isSpotTradeTx")),
          // Trade history
          tradeHistory_A(
            pb,
            state.constants,
            state.accountA.storage,
            orderA.storageID,
            isSpotTradeTx.result(),
            FMT(prefix, ".tradeHistoryA")),
          tradeHistory_B(
            pb,
            state.constants,
            state.accountB.storage,
            orderB.storageID,
            isSpotTradeTx.result(),
            FMT(prefix, ".tradeHistoryB")),

          autoMarketOrderCheckA(pb, state.constants, state.timestamp, state.exchange, orderA, tradeHistory_A, FMT(prefix, ".autoMarketOrderCheckA")),
          autoMarketOrderCheckB(pb, state.constants, state.timestamp, state.exchange, orderB, tradeHistory_B, FMT(prefix, ".autoMarketOrderCheckB")),

          tradeHistoryWithAutoMarket_A(pb, state.constants, tradeHistory_A, autoMarketOrderCheckA.isNewOrder(), FMT(prefix, ".tradeHistoryWithAutoMarket_A")),
          tradeHistoryWithAutoMarket_B(pb, state.constants, tradeHistory_B, autoMarketOrderCheckB.isNewOrder(), FMT(prefix, ".tradeHistoryWithAutoMarket_B")),
          // Match orders
          orderMatching(
            pb,
            state.constants,
            state.timestamp,
            orderA,
            orderB,
            state.accountA.account.owner,
            state.accountB.account.owner,
            tradeHistoryWithAutoMarket_A.getData(),
            tradeHistoryWithAutoMarket_B.getData(),
            tradeHistory_A.getCancelled(),
            tradeHistory_B.getCancelled(),
            fillS_A.value(),
            fillS_B.value(),
            isSpotTradeTx.result(),
            FMT(prefix, ".orderMatching")),
          // Calculate fees
          feeCalculatorA(
            pb,
            state.constants,
            fillS_B.value(),
            orderA.feeBips.packed,
            FMT(prefix, ".feeCalculatorA")),
          feeCalculatorB(
            pb,
            state.constants,
            fillS_A.value(),
            orderB.feeBips.packed,
            FMT(prefix, ".feeCalculatorB")),
          
          appointTradingFeeA(pb, NUM_BITS_AMOUNT, FMT(prefix, ".appointTradingFeeA")),
          appointTradingFeeB(pb, NUM_BITS_AMOUNT, FMT(prefix, ".appointTradingFeeB")),

          fAppointTradingFeeA(pb, state.constants, Float32Encoding, FMT(prefix, ".fAppointTradingFeeA")),
          fAppointTradingFeeB(pb, state.constants, Float32Encoding, FMT(prefix, ".fAppointTradingFeeB")),

          requireAccuracyTradingFeeA(
            pb,
            fAppointTradingFeeA.value(),
            appointTradingFeeA.packed,
            Float32Accuracy,
            NUM_BITS_AMOUNT,
            FMT(prefix, ".requireAccuracyTradingFeeA")),
          
          requireAccuracyTradingFeeB(
            pb,
            fAppointTradingFeeB.value(),
            appointTradingFeeB.packed,
            Float32Accuracy,
            NUM_BITS_AMOUNT,
            FMT(prefix, ".requireAccuracyTradingFeeB")),


          /* Token Transfers */
          // Actual trade
          fillSA_from_balanceSA_to_balanceBB(
            pb,
            balanceS_A,
            balanceB_B,
            fillS_A.value(),
            FMT(prefix, ".fillSA_from_balanceSA_to_balanceBB")),
          fillSB_from_balanceSB_to_balanceBA(
            pb,
            balanceS_B,
            balanceB_A,
            fillS_B.value(),
            FMT(prefix, ".fillSB_from_balanceSB_to_balanceBA")),
          // gas fee
          // pay gas fee and trading fee to operator
          // order A pay gas fee
          feeA_from_accountA_fee_to_balanceAO(
            pb,
            balanceFee_A,
            balanceA_O,
            orderA.fFee.value(),
            FMT(prefix, ".feeA_from_accountA_fee_to_balanceAO")),
          // order B pay gas fee
          feeB_from_accountB_fee_to_balanceBO(
            pb,
            balanceFee_B,
            balanceB_O,
            orderB.fFee.value(),
            FMT(prefix, ".feeB_from_accountB_fee_to_balanceBO")),
          
          // pay trading fee, trading fee deduct from the tokenB
          protocolFeeA_from_balanceBA_to_balanceAP(
            pb,
            balanceB_A,
            balanceC_O,
            // feeCalculatorA.getFee(),
            fAppointTradingFeeA.value(),
            FMT(prefix, ".protocolFeeA_from_balanceBA_to_balanceAP")),
          protocolFeeB_from_balanceBB_to_balanceBP(
            pb,
            balanceB_B,
            balanceD_O,
            // feeCalculatorB.getFee(),
            fAppointTradingFeeB.value(),
            FMT(prefix, ".protocolFeeB_from_balanceBB_to_balanceBP")),

          feeMatch_A(pb, state.constants, orderA.fee.packed, tradeHistoryWithAutoMarket_A.getGasFee(), orderA.maxFee.packed, feeCalculatorA.getFee(), appointTradingFeeA.packed, isSpotTradeTx.result(), FMT(prefix, ".fee match A")),
          feeMatch_B(pb, state.constants, orderB.fee.packed, tradeHistoryWithAutoMarket_B.getGasFee(), orderB.maxFee.packed, feeCalculatorB.getFee(), appointTradingFeeB.packed, isSpotTradeTx.result(), FMT(prefix, ".fee match B")),

          resolvedAAuthorX(
            pb,
            orderA.useAppKey.packed,
            state.accountA.account.appKeyPublicKey.x,
            state.accountA.account.publicKey.x,
            FMT(prefix, ".resolvedAAuthorX")),
          resolvedAAuthorY(
            pb,
            orderA.useAppKey.packed,
            state.accountA.account.appKeyPublicKey.y,
            state.accountA.account.publicKey.y,
            FMT(prefix, ".resolvedAAuthorY")),

          resolvedBAuthorX(
            pb,
            orderB.useAppKey.packed,
            state.accountB.account.appKeyPublicKey.x,
            state.accountB.account.publicKey.x,
            FMT(prefix, ".resolvedBAuthorX")),
          resolvedBAuthorY(
            pb,
            orderB.useAppKey.packed,
            state.accountB.account.appKeyPublicKey.y,
            state.accountB.account.publicKey.y,
            FMT(prefix, ".resolvedBAuthorY"))
    {
        LOG(LogDebug, "in SpotTradeCircuit", "");
        // Set tokens
        setArrayOutput(TXV_BALANCE_A_S_ADDRESS, orderA.tokenS.bits);
        setArrayOutput(TXV_BALANCE_A_B_ADDRESS, orderA.tokenB.bits);
        setArrayOutput(TXV_BALANCE_B_S_ADDRESS, orderB.tokenS.bits);
        setArrayOutput(TXV_BALANCE_B_B_ADDRESS, orderB.tokenB.bits);

        // Update account A
        setArrayOutput(TXV_STORAGE_A_ADDRESS, subArray(orderA.storageID.bits, 0, NUM_BITS_STORAGE_ADDRESS));

        setOutput(TXV_STORAGE_A_TOKENSID, autoMarketOrderCheckA.getTokenSIDForStorageUpdate());
        setOutput(TXV_STORAGE_A_TOKENBID, autoMarketOrderCheckA.getTokenBIDForStorageUpdate());
        setOutput(TXV_STORAGE_A_DATA, orderMatching.getFilledAfter_A());
        setOutput(TXV_STORAGE_A_STORAGEID, orderA.storageID.packed);
        setOutput(TXV_STORAGE_A_GASFEE, feeMatch_A.getFeeSum());
        setOutput(TXV_STORAGE_A_CANCELLED, tradeHistory_A.getCancelled());
        setOutput(TXV_STORAGE_A_FORWARD, autoMarketOrderCheckA.getNewForwardForStorageUpdate());

        setOutput(TXV_BALANCE_A_S_BALANCE, balanceS_A.balance());
        setOutput(TXV_BALANCE_A_B_BALANCE, balanceB_A.balance());
        setOutput(TXV_BALANCE_A_FEE_BALANCE, balanceFee_A.balance());

        setArrayOutput(TXV_ACCOUNT_A_ADDRESS, orderA.accountID.bits);

        // Update account B
        setArrayOutput(TXV_STORAGE_B_ADDRESS, subArray(orderB.storageID.bits, 0, NUM_BITS_STORAGE_ADDRESS));

        setOutput(TXV_STORAGE_B_TOKENSID, autoMarketOrderCheckB.getTokenSIDForStorageUpdate());
        setOutput(TXV_STORAGE_B_TOKENBID, autoMarketOrderCheckB.getTokenBIDForStorageUpdate());
        setOutput(TXV_STORAGE_B_DATA, orderMatching.getFilledAfter_B());
        setOutput(TXV_STORAGE_B_STORAGEID, orderB.storageID.packed);
        setOutput(TXV_STORAGE_B_GASFEE, feeMatch_B.getFeeSum());
        setOutput(TXV_STORAGE_B_CANCELLED, tradeHistory_B.getCancelled());
        setOutput(TXV_STORAGE_B_FORWARD, autoMarketOrderCheckB.getNewForwardForStorageUpdate());

        setOutput(TXV_BALANCE_B_S_BALANCE, balanceS_B.balance());
        setOutput(TXV_BALANCE_B_B_BALANCE, balanceB_B.balance());
        setOutput(TXV_BALANCE_B_FEE_BALANCE, balanceFee_B.balance());

        setArrayOutput(TXV_ACCOUNT_B_ADDRESS, orderB.accountID.bits);

        setArrayOutput(TXV_BALANCE_O_C_Address, orderB.tokenS.bits);
        setArrayOutput(TXV_BALANCE_O_D_Address, orderA.tokenS.bits);

        setOutput(TXV_BALANCE_O_C_BALANCE, balanceC_O.balance());
        setOutput(TXV_BALANCE_O_D_BALANCE, balanceD_O.balance());

        setArrayOutput(TXV_BALANCE_O_A_Address, orderA.feeTokenID.bits);
        setArrayOutput(TXV_BALANCE_O_B_Address, orderB.feeTokenID.bits);

        setArrayOutput(TXV_BALANCE_A_FEE_Address, orderA.feeTokenID.bits);
        setArrayOutput(TXV_BALANCE_B_FEE_Address, orderB.feeTokenID.bits);

        setOutput(TXV_BALANCE_O_A_BALANCE, balanceA_O.balance());
        setOutput(TXV_BALANCE_O_B_BALANCE, balanceB_O.balance());

        // A signature is required for each order
        setOutput(TXV_HASH_A, autoMarketOrderCheckA.getVerifyHash());
        setOutput(TXV_HASH_B, autoMarketOrderCheckB.getVerifyHash());

        setOutput(TXV_PUBKEY_X_A, resolvedAAuthorX.result());
        setOutput(TXV_PUBKEY_Y_A, resolvedAAuthorY.result());
        setOutput(TXV_PUBKEY_X_B, resolvedBAuthorX.result());
        setOutput(TXV_PUBKEY_Y_B, resolvedBAuthorY.result());
        setOutput(TXV_SIGNATURE_REQUIRED_A, state.constants._1);
        setOutput(TXV_SIGNATURE_REQUIRED_B, state.constants._1);
    }

    void generate_r1cs_witness(const SpotTrade &spotTrade)
    {
        LOG(LogDebug, "in SpotTradeCircuit", "generate_r1cs_witness");
        typeTx.generate_r1cs_witness();
        typeTxPad.generate_r1cs_witness();
        // Orders
        orderA.generate_r1cs_witness(spotTrade.orderA);
        orderB.generate_r1cs_witness(spotTrade.orderB);

        // Balances
        balanceS_A.generate_r1cs_witness();
        balanceB_A.generate_r1cs_witness();
        balanceS_B.generate_r1cs_witness();
        balanceB_B.generate_r1cs_witness();

        balanceC_O.generate_r1cs_witness();
        balanceD_O.generate_r1cs_witness();
        balanceA_O.generate_r1cs_witness();
        balanceB_O.generate_r1cs_witness();

        balanceFee_A.generate_r1cs_witness();
        balanceFee_B.generate_r1cs_witness();

        // Order fills
        fillS_A.generate_r1cs_witness(spotTrade.fillS_A);
        fillS_B.generate_r1cs_witness(spotTrade.fillS_B);

        checkFillS_A.generate_r1cs_witness();
        checkFillS_B.generate_r1cs_witness();

        isSpotTradeTx.generate_r1cs_witness();
        // Trade history
        tradeHistory_A.generate_r1cs_witness();
        tradeHistory_B.generate_r1cs_witness();

        autoMarketOrderCheckA.generate_r1cs_witness(spotTrade.orderA.startOrder);
        autoMarketOrderCheckB.generate_r1cs_witness(spotTrade.orderB.startOrder);

        tradeHistoryWithAutoMarket_A.generate_r1cs_witness();
        tradeHistoryWithAutoMarket_B.generate_r1cs_witness();

        // Match orders
        orderMatching.generate_r1cs_witness();

        // Calculate fees
        feeCalculatorA.generate_r1cs_witness();
        feeCalculatorB.generate_r1cs_witness();
        appointTradingFeeA.generate_r1cs_witness(pb, spotTrade.orderA.tradingFee);
        appointTradingFeeB.generate_r1cs_witness(pb, spotTrade.orderB.tradingFee);
        fAppointTradingFeeA.generate_r1cs_witness(toFloat(spotTrade.orderA.tradingFee, Float32Encoding));
        fAppointTradingFeeB.generate_r1cs_witness(toFloat(spotTrade.orderB.tradingFee, Float32Encoding));
        requireAccuracyTradingFeeA.generate_r1cs_witness();
        requireAccuracyTradingFeeB.generate_r1cs_witness();

        /* Token Transfers */
        // Actual trade
        fillSA_from_balanceSA_to_balanceBB.generate_r1cs_witness();
        fillSB_from_balanceSB_to_balanceBA.generate_r1cs_witness();
        // Fees
        feeA_from_accountA_fee_to_balanceAO.generate_r1cs_witness();
        feeB_from_accountB_fee_to_balanceBO.generate_r1cs_witness();

        // trading fees
        protocolFeeA_from_balanceBA_to_balanceAP.generate_r1cs_witness();
        protocolFeeB_from_balanceBB_to_balanceBP.generate_r1cs_witness();

        feeMatch_A.generate_r1cs_witness();
        feeMatch_B.generate_r1cs_witness();

        resolvedAAuthorX.generate_r1cs_witness();
        resolvedAAuthorY.generate_r1cs_witness();
        resolvedBAuthorX.generate_r1cs_witness();
        resolvedBAuthorY.generate_r1cs_witness();
        LOG(LogDebug, "in SpotTradeCircuit", "end");
    }

    void generate_r1cs_constraints()
    {
        LOG(LogDebug, "in SpotTradeCircuit", "generate_r1cs_constraints");
        typeTx.generate_r1cs_constraints();
        typeTxPad.generate_r1cs_constraints();
        // Orders
        orderA.generate_r1cs_constraints();
        orderB.generate_r1cs_constraints();

        // Balances
        balanceS_A.generate_r1cs_constraints();
        balanceB_A.generate_r1cs_constraints();
        balanceS_B.generate_r1cs_constraints();
        balanceB_B.generate_r1cs_constraints();

        balanceC_O.generate_r1cs_constraints();
        balanceD_O.generate_r1cs_constraints();
        balanceA_O.generate_r1cs_constraints();
        balanceB_O.generate_r1cs_constraints();

        balanceFee_A.generate_r1cs_constraints();
        balanceFee_B.generate_r1cs_constraints();

        // Order fills
        fillS_A.generate_r1cs_constraints();
        fillS_B.generate_r1cs_constraints();

        checkFillS_A.generate_r1cs_constraints();
        checkFillS_B.generate_r1cs_constraints();

        isSpotTradeTx.generate_r1cs_constraints();
        // Trade history
        tradeHistory_A.generate_r1cs_constraints();
        tradeHistory_B.generate_r1cs_constraints();

        autoMarketOrderCheckA.generate_r1cs_constraints();
        autoMarketOrderCheckB.generate_r1cs_constraints();

        tradeHistoryWithAutoMarket_A.generate_r1cs_constraints();
        tradeHistoryWithAutoMarket_B.generate_r1cs_constraints();
        // Match orders
        orderMatching.generate_r1cs_constraints();
        // Calculate fees
        feeCalculatorA.generate_r1cs_constraints();
        feeCalculatorB.generate_r1cs_constraints();
        appointTradingFeeA.generate_r1cs_constraints();
        appointTradingFeeB.generate_r1cs_constraints();

        fAppointTradingFeeA.generate_r1cs_constraints();
        fAppointTradingFeeB.generate_r1cs_constraints();
        requireAccuracyTradingFeeA.generate_r1cs_constraints();
        requireAccuracyTradingFeeB.generate_r1cs_constraints();

        /* Token Transfers */
        // Actual trade
        fillSA_from_balanceSA_to_balanceBB.generate_r1cs_constraints();
        fillSB_from_balanceSB_to_balanceBA.generate_r1cs_constraints();
        // Fees gas fee
        feeA_from_accountA_fee_to_balanceAO.generate_r1cs_constraints();
        feeB_from_accountB_fee_to_balanceBO.generate_r1cs_constraints();

        // trading fees
        protocolFeeA_from_balanceBA_to_balanceAP.generate_r1cs_constraints();
        protocolFeeB_from_balanceBB_to_balanceBP.generate_r1cs_constraints();

        feeMatch_A.generate_r1cs_constraints();
        feeMatch_B.generate_r1cs_constraints();

        resolvedAAuthorX.generate_r1cs_constraints();
        resolvedAAuthorY.generate_r1cs_constraints();
        resolvedBAuthorX.generate_r1cs_constraints();
        resolvedBAuthorY.generate_r1cs_constraints();
    }

    const VariableArrayT getPublicData() const
    {
        return flattenReverse({
          typeTx.bits,
          typeTxPad.bits,

          orderA.accountID.bits,
          orderB.accountID.bits,

          orderA.tokenS.bits,
          orderB.tokenS.bits,

          fillS_A.bits(),
          fillS_B.bits(),

          orderA.feeTokenID.bits,
          orderA.fFee.bits(),

          orderB.feeTokenID.bits,
          orderB.fFee.bits(),

          fAppointTradingFeeA.bits(),
          fAppointTradingFeeB.bits(),

          orderA.fillAmountBorS.bits,
          orderB.fillAmountBorS.bits,
        });
        
    }
};

} // namespace Loopring

#endif
