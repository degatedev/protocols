// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
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
    DualVariableGadget typeTx;
    // type is 3bits, need add 1bit, then 4bits can convert to 1 hex char
    DualVariableGadget typeTxPad;
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

    // split trading fee and gas fee
    DynamicBalanceGadget balanceFee_A;
    DynamicBalanceGadget balanceFee_B;

    // Order fills
    FloatGadget fillS_A;
    FloatGadget fillS_B;

    RangeCheckGadget checkFillS_A;
    RangeCheckGadget checkFillS_B;

    // Trade history
    EqualGadget isSpotTradeTx;
    StorageReaderGadget tradeHistory_A;
    StorageReaderGadget tradeHistory_B;

    AutoMarketOrderCheck autoMarketOrderCheckA;
    AutoMarketOrderCheck autoMarketOrderCheckB;

    StorageReaderForAutoMarketGadget tradeHistoryWithAutoMarket_A;
    StorageReaderForAutoMarketGadget tradeHistoryWithAutoMarket_B;


    // Match orders
    OrderMatchingGadget orderMatching;

    // Calculate fees
    FeeCalculatorGadget feeCalculatorA;
    FeeCalculatorGadget feeCalculatorB;

    /* Token Transfers */
    // Actual trade
    TransferGadget fillSA_from_balanceSA_to_balanceBB;
    TransferGadget fillSB_from_balanceSB_to_balanceBA;

    TransferGadget feeA_from_accountA_fee_to_balanceAO;
    TransferGadget feeB_from_accountB_fee_to_balanceBO;
    // Protocol fees
    TransferGadget protocolFeeA_from_balanceBA_to_balanceAP;
    TransferGadget protocolFeeB_from_balanceBB_to_balanceBP;

    GasFeeMatchingGadget feeMatch_A;
    GasFeeMatchingGadget feeMatch_B;

    // choose accountA appKey or assetKey
    TernaryGadget resolvedAAuthorX;
    TernaryGadget resolvedAAuthorY;

    // choose accountB appKey or assetKey
    TernaryGadget resolvedBAuthorX;
    TernaryGadget resolvedBAuthorY;

    SpotTradeCircuit( //
      ProtoboardT &pb,
      const TransactionState &state,
      const std::string &prefix)
        : BaseTransactionCircuit(pb, state, prefix),

          typeTx(pb, NUM_BITS_TX_TYPE, FMT(prefix, ".typeTx")),
          typeTxPad(pb, NUM_BITS_BIT, FMT(prefix, ".typeTxPad")),
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

          // split trading fee and gas fee
          balanceFee_A(pb, state.accountA.balanceFee, FMT(prefix, ".balanceFee_A")),
          balanceFee_B(pb, state.accountB.balanceFee, FMT(prefix, ".balanceFee_B")),

          // Order fills
          fillS_A(pb, state.constants, Float32Encoding, FMT(prefix, ".fillS_A")),
          fillS_B(pb, state.constants, Float32Encoding, FMT(prefix, ".fillS_B")),

          checkFillS_A(pb, fillS_A.value(), NUM_BITS_AMOUNT, FMT(prefix, ".checkFillS_A")),
          checkFillS_B(pb, fillS_B.value(), NUM_BITS_AMOUNT, FMT(prefix, ".checkFillS_B")),

          // Trade history
          isSpotTradeTx(pb, state.type, state.constants.txTypeSpotTrade, FMT(prefix, ".isSpotTradeTx")),
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
          // Fees gas fee
          // split trading fee and gas fee
          // gas fee pay to operator and trading fee pay to operator
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
          
          // pay trading fee, now, trading fee deduct from the token user bought, rather than operator account
          protocolFeeA_from_balanceBA_to_balanceAP(
            pb,
            balanceB_A,
            balanceC_O,
            feeCalculatorA.getFee(),
            FMT(prefix, ".protocolFeeA_from_balanceBA_to_balanceAP")),
          protocolFeeB_from_balanceBB_to_balanceBP(
            pb,
            balanceB_B,
            balanceD_O,
            feeCalculatorB.getFee(),
            FMT(prefix, ".protocolFeeB_from_balanceBB_to_balanceBP")),

          // split trading fee and gas fee - add up gas
          feeMatch_A(pb, state.constants, orderA.fee.packed, tradeHistoryWithAutoMarket_A.getGasFee(), orderA.maxFee.packed, state.constants._0, state.constants._0, isSpotTradeTx.result(), FMT(prefix, ".fee match A")),
          feeMatch_B(pb, state.constants, orderB.fee.packed, tradeHistoryWithAutoMarket_B.getGasFee(), orderB.maxFee.packed, state.constants._0, state.constants._0, isSpotTradeTx.result(), FMT(prefix, ".fee match B")),

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
        typeTx.generate_r1cs_witness(pb, ethsnarks::FieldT(int(Loopring::TransactionType::SpotTrade)));
        typeTxPad.generate_r1cs_witness(pb, ethsnarks::FieldT(0));
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

        // Trade history
        isSpotTradeTx.generate_r1cs_witness();
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

        /* Token Transfers */
        // Actual trade
        fillSA_from_balanceSA_to_balanceBB.generate_r1cs_witness();
        fillSB_from_balanceSB_to_balanceBA.generate_r1cs_witness();
        // Fees
        feeA_from_accountA_fee_to_balanceAO.generate_r1cs_witness();
        feeB_from_accountB_fee_to_balanceBO.generate_r1cs_witness();

        // Protocol fees
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
        typeTx.generate_r1cs_constraints(true);
        typeTxPad.generate_r1cs_constraints(true);
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

        // Trade history
        isSpotTradeTx.generate_r1cs_constraints();
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

        /* Token Transfers */
        // Actual trade
        fillSA_from_balanceSA_to_balanceBB.generate_r1cs_constraints();
        fillSB_from_balanceSB_to_balanceBA.generate_r1cs_constraints();
        // Fees gas fee
        feeA_from_accountA_fee_to_balanceAO.generate_r1cs_constraints();
        feeB_from_accountB_fee_to_balanceBO.generate_r1cs_constraints();

        // Protocol fees, trading fee
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

          orderA.fillAmountBorS.bits,
          orderA.feeBipsMultiplierFlag.bits,
          orderA.feeBipsData.bits,
          orderB.fillAmountBorS.bits,
          orderB.feeBipsMultiplierFlag.bits,
          orderB.feeBipsData.bits
        });
        
    }
};

} // namespace Loopring

#endif
