// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
#ifndef _BATCHORDERGADGETS_H_
#define _BATCHORDERGADGETS_H_

#include "../Utils/Constants.h"
#include "../Utils/Data.h"
#include "StorageGadgets.h"
#include "AccountGadgets.h"
#include "MatchingGadgets.h"

#include "ethsnarks.hpp"
#include "gadgets/poseidon.hpp"
#include "utils.hpp"

using namespace ethsnarks;

namespace Loopring
{

// order just contain two token, supply three token, the three token must different
class SelectOneTokenAmountGadget : public GadgetT 
{
    public:
        EqualGadget tokenX_eq_tokenA;
        EqualGadget tokenY_eq_tokenA;
        EqualGadget tokenZ_eq_tokenA;

        TernaryGadget tokenXAmountInTokenA;
        TernaryGadget tokenYAmountInTokenA;
        TernaryGadget tokenZAmountInTokenA;

        TernaryGadget signX;
        TernaryGadget signY;
        TernaryGadget signZ;

        OrGadget validTokenA;
        IfThenRequireGadget requireValidTokenA;
        SelectOneTokenAmountGadget(
            ProtoboardT &pb,
            const Constants &constants,
            const VariableT &tokenX,
            const VariableT &tokenY,
            const VariableT &tokenZ,
            const VariableT &tokenA,
            const VariableT &amountA,
            // 0: no sign, belong fee or not match; 1: amount increase;2: amount reduce
            const VariableT &sign,
            const VariableT &verify,
            const std::string &prefix)
            : GadgetT(pb, prefix),
            tokenX_eq_tokenA(pb, tokenX, tokenA, FMT(prefix, ".tokenX_eq_tokenA")),
            tokenY_eq_tokenA(pb, tokenY, tokenA, FMT(prefix, ".tokenY_eq_tokenA")),
            tokenZ_eq_tokenA(pb, tokenZ, tokenA, FMT(prefix, ".tokenZ_eq_tokenA")),

            tokenXAmountInTokenA(pb, tokenX_eq_tokenA.result(), amountA, constants._0, FMT(prefix, ".tokenXAmountInTokenA")),
            tokenYAmountInTokenA(pb, tokenY_eq_tokenA.result(), amountA, constants._0, FMT(prefix, ".tokenYAmountInTokenA")),
            tokenZAmountInTokenA(pb, tokenZ_eq_tokenA.result(), amountA, constants._0, FMT(prefix, ".tokenZAmountInTokenA")),

            // 0: no sign, belong fee or not match; 1: amount increase;2: amount reduce
            signX(pb, tokenX_eq_tokenA.result(), sign, constants._0, FMT(prefix, ".signX")),
            signY(pb, tokenY_eq_tokenA.result(), sign, constants._0, FMT(prefix, ".signY")),
            signZ(pb, tokenZ_eq_tokenA.result(), sign, constants._0, FMT(prefix, ".signZ")),

            validTokenA(pb, {tokenX_eq_tokenA.result(), tokenY_eq_tokenA.result(), tokenZ_eq_tokenA.result()}, FMT(prefix, ".validTokenA")),
            requireValidTokenA(pb, verify, validTokenA.result(), FMT(prefix, ".requireValidTokenA"))
        {

        }

        void generate_r1cs_witness() 
        {
            tokenX_eq_tokenA.generate_r1cs_witness();
            tokenY_eq_tokenA.generate_r1cs_witness();
            tokenZ_eq_tokenA.generate_r1cs_witness();

            tokenXAmountInTokenA.generate_r1cs_witness();
            tokenYAmountInTokenA.generate_r1cs_witness();
            tokenZAmountInTokenA.generate_r1cs_witness();

            signX.generate_r1cs_witness();
            signY.generate_r1cs_witness();
            signZ.generate_r1cs_witness();

            validTokenA.generate_r1cs_witness();
            requireValidTokenA.generate_r1cs_witness();
        }

        void generate_r1cs_constraints() 
        {
            tokenX_eq_tokenA.generate_r1cs_constraints();
            tokenY_eq_tokenA.generate_r1cs_constraints();
            tokenZ_eq_tokenA.generate_r1cs_constraints();

            tokenXAmountInTokenA.generate_r1cs_constraints();
            tokenYAmountInTokenA.generate_r1cs_constraints();
            tokenZAmountInTokenA.generate_r1cs_constraints();

            signX.generate_r1cs_constraints();
            signY.generate_r1cs_constraints();
            signZ.generate_r1cs_constraints();

            validTokenA.generate_r1cs_constraints();
            requireValidTokenA.generate_r1cs_constraints();
        }

        const VariableT &getAmountX() const
        {
            return tokenXAmountInTokenA.result();
        }
        const VariableT &getAmountY() const
        {
            return tokenYAmountInTokenA.result();
        }
        const VariableT &getAmountZ() const
        {
            return tokenZAmountInTokenA.result();
        }

        const VariableT &getSignX() const
        {
            return signX.result();
        }
        const VariableT &getSignY() const
        {
            return signY.result();
        }
        const VariableT &getSignZ() const
        {
            return signZ.result();
        }
};

class IsBatchSpotTradeGadget : public GadgetT 
{
    public:
        EqualGadget isBatchSpotTradeTx;
        IsBatchSpotTradeGadget(
            ProtoboardT &pb,
            const Constants &constants,
            const VariableT &currentType,
            const VariableT &circuitType,
            const std::string &prefix)
                : GadgetT(pb, prefix),
                isBatchSpotTradeTx(pb, currentType, circuitType, FMT(prefix, ".isBatchSpotTradeTx"))
        {
            
        }
        void generate_r1cs_witness()
        {
            isBatchSpotTradeTx.generate_r1cs_witness();
        }
        void generate_r1cs_constraints()
        {
            isBatchSpotTradeTx.generate_r1cs_constraints();
        }

        const VariableT &result() const
        {
            return isBatchSpotTradeTx.result();
        }
};

class BatchOrderGadget : public GadgetT
{
  public:
    Constants constants;
    std::vector<VariableT> tokens;
    OrderGadget order;

    DualVariableGadget isNoop;
    NotGadget isNotNoop;
    AndGadget needCheckOrderValid;
    
    StorageReaderGadget tradeHistory;
    AutoMarketOrderCheck autoMarketOrderCheck;

    StorageReaderForAutoMarketGadget tradeHistoryWithAutoMarket;
    DualVariableGadget deltaFilledS;
    DualVariableGadget deltaFilledB;

    // calculate deltaFilledS, deltaFilledB legitimacy
    // the old logic about order matching need put into batch spot order
    BatchOrderMatchingGadget batchOrderMatching;

    FeeCalculatorGadget tradingFeeCalculator;
    DualVariableGadget appointTradingFee;
    GasFeeMatchingGadget gasFeeMatch;
    SubGadget tokenBExchange;

    SelectOneTokenAmountGadget tokenSSelect;
    SelectOneTokenAmountGadget tokenBSelect;
    AddGadget tokenOneAmount;
    AddGadget tokenOneSign;
    AddGadget tokenTwoAmount;
    AddGadget tokenTwoSign;
    AddGadget tokenThreeAmount;
    AddGadget tokenThreeSign;

    // calculate token B exchange, this amount will reduce trading fee
    SelectOneTokenAmountGadget tokenBExchangeSelect;
    AddGadget tokenOneExchangeAmount;
    AddGadget tokenOneExchangeSign;
    AddGadget tokenTwoExchangeAmount;
    AddGadget tokenTwoExchangeSign;
    AddGadget tokenThreeExchangeAmount;
    AddGadget tokenThreeExchangeSign;

    // calculate GasFee
    SelectOneTokenAmountGadget tokenFeeSelect;

    SubGadget tokenOneTradingFee;
    SubGadget tokenTwoTradingFee;
    SubGadget tokenThreeTradingFee;

    // select public key; appkey or asset key
    TernaryGadget resolvedAuthorX;
    TernaryGadget resolvedAuthorY;

    BatchOrderGadget(
      ProtoboardT &pb,
      const Constants &_constants,
      const VariableT &timestamp,
      const VariableT &blockExchange,
    //   const TransactionAccountState &account,
      const StorageGadget &storageGadget,
    //   const VariableT &type,
      // split TradingFee and GasFee - ProtocolFeeBips as the max TradingFee
      const VariableT &maxFeeBips,
      const std::vector<VariableT> &_tokens,
      const TransactionAccountState &account,
      const VariableT &isBatchSpotTradeTx,
      const std::string &prefix)
        : GadgetT(pb, prefix),
        constants(_constants),
        tokens(_tokens),
        order(pb, constants, blockExchange, maxFeeBips, constants._0, account.account.disableAppKeySpotTrade, FMT(prefix, ".order")),

        isNoop(pb, NUM_BITS_BIT, FMT(prefix, ".isNoop")),
        isNotNoop(pb, isNoop.packed, FMT(prefix, ".isNotNoop")),

        needCheckOrderValid(pb, {isBatchSpotTradeTx, isNotNoop.result()}, FMT(prefix, ".needCheckOrderValid")),

        tradeHistory(
            pb,
            constants,
            storageGadget,
            order.storageID,
            needCheckOrderValid.result(),
            FMT(prefix, ".tradeHistory")),
        autoMarketOrderCheck(pb, constants, timestamp, blockExchange, order, tradeHistory, FMT(prefix, ".autoMarketOrderCheck")),

        tradeHistoryWithAutoMarket(pb, constants, tradeHistory, autoMarketOrderCheck.isNewOrder(), FMT(prefix, ".tradeHistoryWithAutoMarket")),
        deltaFilledS(pb, NUM_BITS_AMOUNT, FMT(prefix, ".deltaFilledS")),
        deltaFilledB(pb, NUM_BITS_AMOUNT, FMT(prefix, ".deltaFilledB")),

        // order matching legitimacy check
        batchOrderMatching(
            pb, 
            constants, 
            timestamp, 
            order,
            tradeHistoryWithAutoMarket.getData(),
            tradeHistory.getCancelled(),
            deltaFilledS.packed,
            deltaFilledB.packed,
            isNotNoop.result(),
            FMT(prefix, ".batchOrderMatching")
        ),
        tradingFeeCalculator(
            pb,
            constants,
            deltaFilledB.packed,
            order.feeBips.packed,
            FMT(prefix, ".tradingFeeCalculator")),
        appointTradingFee(pb, NUM_BITS_AMOUNT, FMT(prefix, ".appointTradingFee")),
        gasFeeMatch(
            pb, 
            constants, 
            order.fee.packed, 
            tradeHistoryWithAutoMarket.getGasFee(), 
            order.maxFee.packed, 
            tradingFeeCalculator.getFee(),
            appointTradingFee.packed,
            isNotNoop.result(), 
            FMT(prefix, ".gas fee match")),
        tokenBExchange(pb, deltaFilledB.packed, appointTradingFee.packed, NUM_BITS_AMOUNT, FMT(prefix, ".tokenBExchange")),
        tokenSSelect(
            pb,
            constants,
            tokens[0],
            tokens[1],
            tokens[2],
            order.tokenS.packed,
            deltaFilledS.packed,
            constants._2,
            isNotNoop.result(),
            FMT(prefix, ".tokenSSelect")),
        tokenBSelect(
            pb,
            constants,
            tokens[0],
            tokens[1],
            tokens[2],
            order.tokenB.packed,
            deltaFilledB.packed,
            constants._1,
            isNotNoop.result(),
            FMT(prefix, ".tokenBSelect")),
        tokenOneAmount(pb, tokenSSelect.getAmountX(), tokenBSelect.getAmountX(), NUM_BITS_AMOUNT, FMT(prefix, ".tokenOneAmount")),
        tokenOneSign(pb, tokenSSelect.getSignX(), tokenBSelect.getSignX(), NUM_BITS_TYPE, FMT(prefix, ".tokenOneSign")),
        tokenTwoAmount(pb, tokenSSelect.getAmountY(), tokenBSelect.getAmountY(), NUM_BITS_AMOUNT, FMT(prefix, ".tokenTwoAmount")),
        tokenTwoSign(pb, tokenSSelect.getSignY(), tokenBSelect.getSignY(), NUM_BITS_TYPE, FMT(prefix, ".tokenTwoSign")),
        tokenThreeAmount(pb, tokenSSelect.getAmountZ(), tokenBSelect.getAmountZ(), NUM_BITS_AMOUNT, FMT(prefix, ".tokenThreeAmount")),
        tokenThreeSign(pb, tokenSSelect.getSignZ(), tokenBSelect.getSignZ(), NUM_BITS_TYPE, FMT(prefix, ".tokenThreeSign")),

        tokenBExchangeSelect(
            pb,
            constants,
            tokens[0],
            tokens[1],
            tokens[2],
            order.tokenB.packed,
            tokenBExchange.result(),
            constants._1,
            isNotNoop.result(),
            FMT(prefix, ".tokenBExchangeSelect")),
        tokenOneExchangeAmount(pb, tokenSSelect.getAmountX(), tokenBExchangeSelect.getAmountX(), NUM_BITS_AMOUNT, FMT(prefix, ".tokenOneExchangeAmount")),
        tokenOneExchangeSign(pb, tokenSSelect.getSignX(), tokenBExchangeSelect.getSignX(), NUM_BITS_TYPE, FMT(prefix, ".tokenOneExchangeSign")),
        tokenTwoExchangeAmount(pb, tokenSSelect.getAmountY(), tokenBExchangeSelect.getAmountY(), NUM_BITS_AMOUNT, FMT(prefix, ".tokenTwoExchangeAmount")),
        tokenTwoExchangeSign(pb, tokenSSelect.getSignY(), tokenBExchangeSelect.getSignY(), NUM_BITS_TYPE, FMT(prefix, ".tokenTwoExchangeSign")),
        tokenThreeExchangeAmount(pb, tokenSSelect.getAmountZ(), tokenBExchangeSelect.getAmountZ(), NUM_BITS_AMOUNT, FMT(prefix, ".tokenThreeExchangeAmount")),
        tokenThreeExchangeSign(pb, tokenSSelect.getSignZ(), tokenBExchangeSelect.getSignZ(), NUM_BITS_TYPE, FMT(prefix, ".tokenThreeExchangeSign")),
        
        tokenFeeSelect(
            pb,
            constants,
            tokens[0],
            tokens[1],
            tokens[2],
            order.feeTokenID.packed,
            order.fFee.value(),
            constants._1,
            isNotNoop.result(),
            FMT(prefix, ".tokenFeeSelect")),
        
        tokenOneTradingFee(pb, tokenOneAmount.result(), tokenOneExchangeAmount.result(), NUM_BITS_AMOUNT, FMT(prefix, ".tokenOneTradingFee")),
        tokenTwoTradingFee(pb, tokenTwoAmount.result(), tokenTwoExchangeAmount.result(), NUM_BITS_AMOUNT, FMT(prefix, ".tokenTwoTradingFee")),
        tokenThreeTradingFee(pb, tokenThreeAmount.result(), tokenThreeExchangeAmount.result(), NUM_BITS_AMOUNT, FMT(prefix, ".tokenThreeTradingFee")),
        
        resolvedAuthorX(
            pb,
            order.useAppKey.packed,
            account.account.appKeyPublicKey.x,
            account.account.publicKey.x,
            FMT(prefix, ".resolvedAuthorX")),
        resolvedAuthorY(
            pb,
            order.useAppKey.packed,
            account.account.appKeyPublicKey.y,
            account.account.publicKey.y,
            FMT(prefix, ".resolvedAuthorY"))

    {
    }

    void generate_r1cs_witness(const Order &orderEntity)
    {
        // Inputs
        LOG(LogDebug, "in BatchOrderGadget", "generate_r1cs_witness");
        order.generate_r1cs_witness(orderEntity);

        isNoop.generate_r1cs_witness(pb, orderEntity.isNoop);
        isNotNoop.generate_r1cs_witness();

        needCheckOrderValid.generate_r1cs_witness();

        tradeHistory.generate_r1cs_witness();

        autoMarketOrderCheck.generate_r1cs_witness(orderEntity.startOrder);
        tradeHistoryWithAutoMarket.generate_r1cs_witness();

        LOG(LogDebug, "in BatchOrderGadget before order deltaFilledS", orderEntity.deltaFilledS);
        deltaFilledS.generate_r1cs_witness(pb, orderEntity.deltaFilledS);
        LOG(LogDebug, "in BatchOrderGadget before order deltaFilledB", orderEntity.deltaFilledB);
        deltaFilledB.generate_r1cs_witness(pb, orderEntity.deltaFilledB);

        LOG(LogDebug, "in BatchOrderGadget before order batchOrderMatching", "");
        batchOrderMatching.generate_r1cs_witness();
        LOG(LogDebug, "in BatchOrderGadget before order tradingFeeCalculator", "");
        tradingFeeCalculator.generate_r1cs_witness();
        LOG(LogDebug, "in BatchOrderGadget before order appointTradingFee", "");
        appointTradingFee.generate_r1cs_witness(pb, orderEntity.tradingFee);
        LOG(LogDebug, "in BatchOrderGadget before order gasFeeMatch", "");
        gasFeeMatch.generate_r1cs_witness();
        LOG(LogDebug, "in BatchOrderGadget before order tokenBExchange", "");
        tokenBExchange.generate_r1cs_witness();

        LOG(LogDebug, "in BatchOrderGadget before order tokenSSelect", "");
        tokenSSelect.generate_r1cs_witness();
        tokenBSelect.generate_r1cs_witness();
        tokenOneAmount.generate_r1cs_witness();
        tokenOneSign.generate_r1cs_witness();
        tokenTwoAmount.generate_r1cs_witness();
        tokenTwoSign.generate_r1cs_witness();
        tokenThreeAmount.generate_r1cs_witness();
        tokenThreeSign.generate_r1cs_witness();

        LOG(LogDebug, "in BatchOrderGadget before order tokenBExchangeSelect", "");
        tokenBExchangeSelect.generate_r1cs_witness();
        tokenOneExchangeAmount.generate_r1cs_witness();
        tokenOneExchangeSign.generate_r1cs_witness();
        tokenTwoExchangeAmount.generate_r1cs_witness();
        tokenTwoExchangeSign.generate_r1cs_witness();
        tokenThreeExchangeAmount.generate_r1cs_witness();
        tokenThreeExchangeSign.generate_r1cs_witness();

        LOG(LogDebug, "in BatchOrderGadget before order tokenFeeSelect", "");
        tokenFeeSelect.generate_r1cs_witness();

        LOG(LogDebug, "in BatchOrderGadget before order tokenOneTradingFee", "");
        tokenOneTradingFee.generate_r1cs_witness();
        tokenTwoTradingFee.generate_r1cs_witness();
        tokenThreeTradingFee.generate_r1cs_witness();

        LOG(LogDebug, "in BatchOrderGadget before order resolvedAuthorX", "");
        resolvedAuthorX.generate_r1cs_witness();
        resolvedAuthorY.generate_r1cs_witness();

        LOG(LogDebug, "in BatchOrderGadget before tokenOneAmount", pb.val(tokenOneAmount.result()));
        LOG(LogDebug, "in BatchOrderGadget before tokenTwoAmount", pb.val(tokenTwoAmount.result()));
        LOG(LogDebug, "in BatchOrderGadget before tokenThreeAmount", pb.val(tokenThreeAmount.result()));

        LOG(LogDebug, "in BatchOrderGadget before tokenOneSign", pb.val(tokenOneSign.result()));
        LOG(LogDebug, "in BatchOrderGadget before tokenTwoSign", pb.val(tokenTwoSign.result()));
        LOG(LogDebug, "in BatchOrderGadget before tokenThreeSign", pb.val(tokenThreeSign.result()));

        LOG(LogDebug, "in BatchOrderGadget before tokenOneExchangeAmount", pb.val(tokenOneExchangeAmount.result()));
        LOG(LogDebug, "in BatchOrderGadget before tokenTwoExchangeAmount", pb.val(tokenTwoExchangeAmount.result()));
        LOG(LogDebug, "in BatchOrderGadget before tokenThreeExchangeAmount", pb.val(tokenThreeExchangeAmount.result()));

        LOG(LogDebug, "in BatchOrderGadget before tokenOneExchangeSign", pb.val(tokenOneExchangeSign.result()));
        LOG(LogDebug, "in BatchOrderGadget before tokenTwoExchangeSign", pb.val(tokenTwoExchangeSign.result()));
        LOG(LogDebug, "in BatchOrderGadget before tokenThreeExchangeSign", pb.val(tokenThreeExchangeSign.result()));

        LOG(LogDebug, "in BatchOrderGadget before tokenOneGasFeeCalculate", pb.val(tokenFeeSelect.getAmountX()));
        LOG(LogDebug, "in BatchOrderGadget before tokenTwoGasFeeCalculate", pb.val(tokenFeeSelect.getAmountY()));
        LOG(LogDebug, "in BatchOrderGadget before tokenThreeGasFeeCalculate", pb.val(tokenFeeSelect.getAmountZ()));
        LOG(LogDebug, "in BatchOrderGadget before order.fFee.value()", pb.val(order.fFee.value()));
        
    }

    void generate_r1cs_constraints()
    {
        order.generate_r1cs_constraints();

        isNoop.generate_r1cs_constraints(true);
        isNotNoop.generate_r1cs_constraints();

        needCheckOrderValid.generate_r1cs_witness();

        tradeHistory.generate_r1cs_constraints();

        autoMarketOrderCheck.generate_r1cs_constraints();
        tradeHistoryWithAutoMarket.generate_r1cs_constraints();

        deltaFilledS.generate_r1cs_constraints(true);
        deltaFilledB.generate_r1cs_constraints(true);

        batchOrderMatching.generate_r1cs_constraints();
        tradingFeeCalculator.generate_r1cs_constraints();
        appointTradingFee.generate_r1cs_constraints();
        gasFeeMatch.generate_r1cs_constraints();
        tokenBExchange.generate_r1cs_constraints();

        tokenSSelect.generate_r1cs_constraints();
        tokenBSelect.generate_r1cs_constraints();
        tokenOneAmount.generate_r1cs_constraints();
        tokenOneSign.generate_r1cs_constraints();
        tokenTwoAmount.generate_r1cs_constraints();
        tokenTwoSign.generate_r1cs_constraints();
        tokenThreeAmount.generate_r1cs_constraints();
        tokenThreeSign.generate_r1cs_constraints();

        tokenBExchangeSelect.generate_r1cs_constraints();
        tokenOneExchangeAmount.generate_r1cs_constraints();
        tokenOneExchangeSign.generate_r1cs_constraints();
        tokenTwoExchangeAmount.generate_r1cs_constraints();
        tokenTwoExchangeSign.generate_r1cs_constraints();
        tokenThreeExchangeAmount.generate_r1cs_constraints();
        tokenThreeExchangeSign.generate_r1cs_constraints();

        tokenFeeSelect.generate_r1cs_constraints();

        tokenOneTradingFee.generate_r1cs_constraints();
        tokenTwoTradingFee.generate_r1cs_constraints();
        tokenThreeTradingFee.generate_r1cs_constraints();

        resolvedAuthorX.generate_r1cs_constraints();
        resolvedAuthorY.generate_r1cs_constraints();
    }

    // token amunt exchange, contains trading fee
    const VariableT &getSelectTokenOneAmount() const
    {
        return tokenOneAmount.result();
    }

    const VariableT &getSelectTokenOneSign() const
    {
        return tokenOneSign.result();
    }

    const VariableT &getSelectTokenTwoAmount() const
    {
        return tokenTwoAmount.result();
    }

    const VariableT &getSelectTokenTwoSign() const
    {
        return tokenTwoSign.result();
    }

    const VariableT &getSelectTokenThreeAmount() const
    {
        return tokenThreeAmount.result();
    }

    const VariableT &getSelectTokenThreeSign() const
    {
        return tokenThreeSign.result();
    }

    // token amunt exchange, reduce trading fee
    const VariableT &getSelectTokenOneExchangeAmount() const
    {
        return tokenOneExchangeAmount.result();
    }

    const VariableT &getSelectTokenOneExchangeSign() const
    {
        return tokenOneExchangeSign.result();
    }

    const VariableT &getSelectTokenTwoExchangeAmount() const
    {
        return tokenTwoExchangeAmount.result();
    }

    const VariableT &getSelectTokenTwoExchangeSign() const
    {
        return tokenTwoExchangeSign.result();
    }

    const VariableT &getSelectTokenThreeExchangeAmount() const
    {
        return tokenThreeExchangeAmount.result();
    }

    const VariableT &getSelectTokenThreeExchangeSign() const
    {
        return tokenThreeExchangeSign.result();
    }

    // GasFee, the gas fee may be belong one token
    const VariableT &getSelectTokenOneGasFee() const
    {
        return tokenFeeSelect.getAmountX();
    }
    const VariableT &getSelectTokenTwoGasFee() const
    {
        return tokenFeeSelect.getAmountY();
    }
    const VariableT &getSelectTokenThreeGasFee() const
    {
        return tokenFeeSelect.getAmountZ();
    }

    const VariableT &getSelectTokenOneTradingFee() const
    {
        return tokenOneTradingFee.result();
    }

    const VariableT &getSelectTokenTwoTradingFee() const
    {
        return tokenTwoTradingFee.result();
    }

    const VariableT &getSelectTokenThreeTradingFee() const
    {
        return tokenThreeTradingFee.result();
    }

    const VariableT &getTokenSID() const
    {
        return order.tokenS.packed;
    }

    const VariableT &getTokenBID() const
    {
        return order.tokenS.packed;
    }

    const VariableT &getFeeTokenID() const
    {
        return order.tokenS.packed;
    }

    const VariableT &getTokenSExchange() const
    {
        return deltaFilledS.packed;
    }

    const VariableT &getTokenBExchange() const
    {
        return tokenBExchange.result();
    }

    const VariableT &getOperatorExchange() const
    {
        return order.fFee.value();
    }

    const VariableT &getProtocolExchange() const
    {
        // return tradingFeeCalculator.getFee();
        return appointTradingFee.packed;
    }

    const VariableT &requireSignature() const
    {
        return isNotNoop.result();
    }

    const VariableT &hash() const
    {
        return autoMarketOrderCheck.getVerifyHash();
    }

    const VariableT &getResolvedAuthorX() const
    {
        return resolvedAuthorX.result();
    }

    const VariableT &getResolvedAuthorY() const
    {
        return resolvedAuthorY.result();
    }

};

// According to the provided tokenamounts and tokensigns, accumulate the data to forwardamount and reversemount respectively
// if sign[i] == 1, then  forwardAmount = forwardAmount + tokenAmounts[i]
// if sign[i] == 2, then  reverseAmount = reverseAmount + tokenAmounts[i]
class BatchTokenAmountSumGadget : public GadgetT
{
    public:
        std::vector<EqualGadget> forwardSignsSelect;
        std::vector<TernaryGadget> forwardAmountsSelect;
        std::vector<AddGadget> forwardAmounts;

        std::vector<EqualGadget> reverseSignsSelect;
        std::vector<TernaryGadget> reverseAmountsSelect;
        std::vector<AddGadget> reverseAmounts;
        BatchTokenAmountSumGadget(
            ProtoboardT &pb,
            const Constants &constants,
            const std::vector<VariableT> &tokenAmounts,
            const std::vector<VariableT> &tokenSigns,
            unsigned int n,
            const std::string &prefix)
            : GadgetT(pb, prefix)
        {
            for (size_t i = 0; i < tokenAmounts.size(); i++) 
            {
                forwardSignsSelect.emplace_back(pb, tokenSigns[i], constants._1, std::string("forwardSignsSelect_") + std::to_string(i));
                forwardAmountsSelect.emplace_back(pb, forwardSignsSelect.back().result(), tokenAmounts[i], constants._0, std::string("forwardAmountsSelect_") + std::to_string(i));
                forwardAmounts.emplace_back(pb, (i == 0) ? constants._0 : forwardAmounts.back().result(), forwardAmountsSelect.back().result(), n, std::string("forwardAmounts_") + std::to_string(i));
                
                reverseSignsSelect.emplace_back(pb, tokenSigns[i], constants._2, std::string("reverseSignsSelect_") + std::to_string(i));
                reverseAmountsSelect.emplace_back(pb, reverseSignsSelect.back().result(), tokenAmounts[i], constants._0, std::string("reverseAmountsSelect_") + std::to_string(i));
                reverseAmounts.emplace_back(pb, (i == 0) ? constants._0 : reverseAmounts.back().result(), reverseAmountsSelect.back().result(), n, std::string("reverseAmounts_") + std::to_string(i));
            }
        }
        void generate_r1cs_witness() 
        {
            LOG(LogDebug, "in BatchTokenAmountSumGadget", "generate_r1cs_witness");
            for (size_t i = 0; i < forwardSignsSelect.size(); i++) 
            {
                forwardSignsSelect[i].generate_r1cs_witness();
                forwardAmountsSelect[i].generate_r1cs_witness();
                forwardAmounts[i].generate_r1cs_witness();

                reverseSignsSelect[i].generate_r1cs_witness();
                reverseAmountsSelect[i].generate_r1cs_witness();
                reverseAmounts[i].generate_r1cs_witness();
            }
            LOG(LogDebug, "in BatchTokenAmountSumGadget forwardAmounts", pb.val(forwardAmounts.back().result()));
            LOG(LogDebug, "in BatchTokenAmountSumGadget reverseAmounts", pb.val(reverseAmounts.back().result()));
        }
        void generate_r1cs_constraints() 
        {
            for (size_t i = 0; i < forwardSignsSelect.size(); i++) 
            {
                forwardSignsSelect[i].generate_r1cs_constraints();
                forwardAmountsSelect[i].generate_r1cs_constraints();
                forwardAmounts[i].generate_r1cs_constraints();

                reverseSignsSelect[i].generate_r1cs_constraints();
                reverseAmountsSelect[i].generate_r1cs_constraints();
                reverseAmounts[i].generate_r1cs_constraints();
            }
        }

        const VariableT &getForwardAmount() const
        {
            return forwardAmounts.back().result();
        }

        const VariableT &getReverseAmount() const
        {
            return reverseAmounts.back().result();
        }
};

// Calculate the change of balance. If it increases, the returned data is positive. If it decreases, the returned data is negative. 
// The return method of negative number is the 32nd power of 2 minus the data of float31 type
class CalculateBalanceDifGadget : public GadgetT 
{
    public:
        LeqGadget before_leq_after;
        EqualGadget increase;
        EqualGadget reduce;
        MinGadget minBalance;

        IfThenSubGadget increaseAmount;
        IfThenSubGadget reduceAmount;
        FloatGadget fIncreaseAmount;
        FloatGadget fReduceAmount;

        SubGadget reduceNegativeAmount;
        TernaryGadget balanceDif;
        ToBitsGadget balanceDifBits;

        EqualGadget increaseAmountEqual;
        EqualGadget reduceAmountEqual;
        IfThenRequireGadget requireValidIncreaseAmount;
        IfThenRequireGadget requireValidReduceAmount;

        TernaryGadget realIncreaseFloatAmount;
        TernaryGadget realReduceFloatAmount;
        CalculateBalanceDifGadget( //
            ProtoboardT &pb,
            const Constants &constants,
            const DynamicVariableGadget &balanceBefore,
            const DynamicVariableGadget &balanceAfter,
            const std::string &prefix)
                : GadgetT(pb, prefix),
                before_leq_after(pb, balanceBefore.back(), balanceAfter.back(), NUM_BITS_AMOUNT, FMT(prefix, ".before_leq_after")),

                increase(pb, before_leq_after.leq(), constants._1, FMT(prefix, ".increase")),
                reduce(pb, before_leq_after.gt(), constants._1, FMT(prefix, ".reduce")),

                minBalance(pb, balanceBefore.back(), balanceAfter.back(), NUM_BITS_AMOUNT, FMT(prefix, ".minBalance")),

                increaseAmount(pb, constants, increase.result(), balanceAfter.back(), minBalance.result(), NUM_BITS_AMOUNT, FMT(prefix, ".increaseAmount")),
                reduceAmount(pb, constants, reduce.result(), balanceBefore.back(), minBalance.result(), NUM_BITS_AMOUNT, FMT(prefix, ".reduceAmount")),
                // increaseAmount(pb, increase.result(), balanceAfter.balance(), balanceBefore.balance(), NUM_BITS_AMOUNT, FMT(prefix, ".increaseAmount")),
                // reduceAmount(pb, reduce.result(), balanceBefore.balance(), balanceAfter.balance(), NUM_BITS_AMOUNT, FMT(prefix, ".reduceAmount")),
                
                fIncreaseAmount(pb, constants, Float29Encoding, FMT(prefix, ".fIncreaseAmount")),
                fReduceAmount(pb, constants, Float29Encoding, FMT(prefix, ".fReduceAmount")),
                // _ 2pow30 is at the length of byte31, so byte31 is used as the constraint, but it becomes byte30 as long as any non-zero value is subtracted
                reduceNegativeAmount(pb, constants._2Pow30, fReduceAmount.getFArrayValue(), NUM_BITS_FLOAT_31, FMT(prefix, ".reduceNegativeAmount")),
                
                balanceDif(pb, increase.result(), fIncreaseAmount.getFArrayValue(), reduceNegativeAmount.result(), FMT(prefix, ".balanceDifFloat")),
                balanceDifBits(pb, balanceDif.result(), NUM_BITS_FLOAT_30, FMT(prefix, ".balanceDifBits")),

                increaseAmountEqual(pb, fIncreaseAmount.value(), increaseAmount.result(), FMT(prefix, ".increaseAmountEqual")),
                reduceAmountEqual(pb, fReduceAmount.value(), reduceAmount.result(), FMT(prefix, ".reduceAmountEqual")),
                requireValidIncreaseAmount(pb, increase.result(), increaseAmountEqual.result(), FMT(prefix, ".requireValidIncreaseAmount")),
                requireValidReduceAmount(pb, reduce.result(), reduceAmountEqual.result(), FMT(prefix, ".requireValidReduceAmount")),

                realIncreaseFloatAmount(pb, increase.result(), fIncreaseAmount.value(), constants._0, FMT(prefix, ".realIncreaseFloatAmount")),
                realReduceFloatAmount(pb, reduce.result(), fReduceAmount.value(), constants._0, FMT(prefix, ".realReduceFloatAmount"))
        {
            
        }
        void generate_r1cs_witness() 
        {
            LOG(LogDebug, "in CalculateBalanceDifGadget", "generate_r1cs_witness");
            before_leq_after.generate_r1cs_witness();

            increase.generate_r1cs_witness();
            reduce.generate_r1cs_witness();
            minBalance.generate_r1cs_witness();
            
            std::cout << "in CalculateBalanceDifGadget before increaseAmount minBalance:" << pb.val(minBalance.result()) << std::endl;
            LOG(LogDebug, "in CalculateBalanceDifGadget before reduce", pb.val(reduce.result()));
            increaseAmount.generate_r1cs_witness();
            LOG(LogDebug, "in CalculateBalanceDifGadget increaseAmount", pb.val(increaseAmount.result()));
            reduceAmount.generate_r1cs_witness();
            LOG(LogDebug, "in CalculateBalanceDifGadget reduceAmount", pb.val(reduceAmount.result()));

            fIncreaseAmount.generate_r1cs_witness(toFloat(pb.val(increaseAmount.result()), Float29Encoding));
            LOG(LogDebug, "in CalculateBalanceDifGadget fIncreaseAmount", pb.val(fIncreaseAmount.getFArrayValue()));
            LOG(LogDebug, "in CalculateBalanceDifGadget fIncreaseAmount value", pb.val(fIncreaseAmount.value()));
            fReduceAmount.generate_r1cs_witness(toFloat(pb.val(reduceAmount.result()), Float29Encoding));
            LOG(LogDebug, "in CalculateBalanceDifGadget fReduceAmount", pb.val(fReduceAmount.getFArrayValue()));
            LOG(LogDebug, "in CalculateBalanceDifGadget fReduceAmount value", pb.val(fReduceAmount.value()));

            reduceNegativeAmount.generate_r1cs_witness();
            LOG(LogDebug, "in CalculateBalanceDifGadget reduceNegativeAmount", pb.val(reduceNegativeAmount.result()));

            balanceDif.generate_r1cs_witness();
            LOG(LogDebug, "in CalculateBalanceDifGadget balanceDif", pb.val(balanceDif.result()));

            balanceDifBits.generate_r1cs_witness();
            // std::cout << "in CalculateBalanceDifGadget balanceDifBits:" << pb.val(balanceDifBits.value()) << std::endl;
            increaseAmountEqual.generate_r1cs_witness();
            reduceAmountEqual.generate_r1cs_witness();
            requireValidIncreaseAmount.generate_r1cs_witness();
            requireValidReduceAmount.generate_r1cs_witness();

            realIncreaseFloatAmount.generate_r1cs_witness();
            realReduceFloatAmount.generate_r1cs_witness();
        }
        void generate_r1cs_constraints() 
        {
            before_leq_after.generate_r1cs_constraints();

            increase.generate_r1cs_constraints();
            reduce.generate_r1cs_constraints();

            minBalance.generate_r1cs_constraints();
            
            increaseAmount.generate_r1cs_constraints();
            reduceAmount.generate_r1cs_constraints();

            fIncreaseAmount.generate_r1cs_constraints();
            fReduceAmount.generate_r1cs_constraints();

            reduceNegativeAmount.generate_r1cs_constraints();

            balanceDif.generate_r1cs_constraints();

            balanceDifBits.generate_r1cs_constraints();

            increaseAmountEqual.generate_r1cs_constraints();
            reduceAmountEqual.generate_r1cs_constraints();
            requireValidIncreaseAmount.generate_r1cs_constraints();
            requireValidReduceAmount.generate_r1cs_constraints();

            realIncreaseFloatAmount.generate_r1cs_constraints();
            realReduceFloatAmount.generate_r1cs_constraints();
        }
        const VariableArrayT &getBalanceDifFloatBits() const
        {
            return balanceDifBits.bits;
        }

        const VariableT &getBalanceDif() const
        {
            return balanceDif.result();
        }

        const VariableT &getIncreaseAmount() const 
        {
            return realIncreaseFloatAmount.result();
        }

        const VariableT &getReduceAmount() const 
        {
            return realReduceFloatAmount.result();
        }
};

// The token type calculation tool calculates the token type according to the order information and tokens information provided
// It is necessary to consider the case that the user is empty. At this time, the tokenid is 0
class BatchUserTokenTypeGadget: public GadgetT 
{
    public:
        EqualGadget tokenS_eq_firstToken;
        EqualGadget tokenS_eq_secondToken;
        EqualGadget tokenS_eq_thirdToken;

        EqualGadget tokenB_eq_firstToken;
        EqualGadget tokenB_eq_secondToken;
        EqualGadget tokenB_eq_thirdToken;

        // type = 0
        AndGadget typeZero_firstScene;
        AndGadget typeZero_secondScene;
        OrGadget typeZero;

        // type = 1
        AndGadget typeOne_firstScene;
        AndGadget typeOne_secondScene;
        OrGadget typeOne;

        // type = 2
        AndGadget typeTwo_firstScene;
        AndGadget typeTwo_secondScene;
        OrGadget typeTwo;

        SelectGadget tokenTypeSelect;

        TernaryGadget validTokenType;
        ToBitsGadget tokenType;

        BatchUserTokenTypeGadget(
            ProtoboardT &pb,
            const Constants &constants,
            const std::vector<VariableT> &tokens,
            // The first order of the user. If the first order isnoop = true, it means that the user is also NOOP
            const BatchOrderGadget &firstOrder,
            const std::string &prefix)
            : GadgetT(pb, prefix),
            tokenS_eq_firstToken(pb, firstOrder.order.tokenS.packed, tokens[0], FMT(prefix, ".tokenS_eq_firstToken")),
            tokenS_eq_secondToken(pb, firstOrder.order.tokenS.packed, tokens[1], FMT(prefix, ".tokenS_eq_secondToken")),
            tokenS_eq_thirdToken(pb, firstOrder.order.tokenS.packed, tokens[2], FMT(prefix, ".tokenS_eq_thirdToken")),

            tokenB_eq_firstToken(pb, firstOrder.order.tokenB.packed, tokens[0], FMT(prefix, ".tokenB_eq_firstToken")),
            tokenB_eq_secondToken(pb, firstOrder.order.tokenB.packed, tokens[1], FMT(prefix, ".tokenB_eq_secondToken")),
            tokenB_eq_thirdToken(pb, firstOrder.order.tokenB.packed, tokens[2], FMT(prefix, ".tokenB_eq_thirdToken")),

            // type = 0
            typeZero_firstScene(pb, {tokenS_eq_firstToken.result(), tokenB_eq_secondToken.result()}, FMT(prefix, ".typeZero_firstScene")),
            typeZero_secondScene(pb, {tokenB_eq_firstToken.result(), tokenS_eq_secondToken.result()}, FMT(prefix, ".typeZero_secondScene")),
            typeZero(pb, {typeZero_firstScene.result(), typeZero_secondScene.result()}, FMT(prefix, ".typeZero")),

            // type = 1
            typeOne_firstScene(pb, {tokenS_eq_firstToken.result(), tokenB_eq_thirdToken.result()}, FMT(prefix, ".typeOne_firstScene")),
            typeOne_secondScene(pb, {tokenB_eq_firstToken.result(), tokenS_eq_thirdToken.result()}, FMT(prefix, ".typeOne_secondScene")),
            typeOne(pb, {typeOne_firstScene.result(), typeOne_secondScene.result()}, FMT(prefix, ".typeOne")),

            // type = 2
            typeTwo_firstScene(pb, {tokenS_eq_secondToken.result(), tokenB_eq_thirdToken.result()}, FMT(prefix, ".typeTwo_firstScene")),
            typeTwo_secondScene(pb, {tokenB_eq_secondToken.result(), tokenS_eq_thirdToken.result()}, FMT(prefix, ".typeTwo_secondScene")),
            typeTwo(pb, {typeTwo_firstScene.result(), typeTwo_secondScene.result()}, FMT(prefix, ".typeTwo")),

            tokenTypeSelect(pb, constants, var_array({typeZero.result(), typeOne.result(), typeTwo.result()}), subVector(constants.values, 0, 3), FMT(prefix, ".tokenTypeSelect")),
            validTokenType(pb, firstOrder.isNoop.packed, constants._0, tokenTypeSelect.result(), FMT(prefix, ".validTokenType")),
            tokenType(pb, validTokenType.result(), NUM_BITS_BATCH_SPOTRADE_TOKEN_TYPE, FMT(prefix, ".tokenType"))
        {

        }

        void generate_r1cs_witness() 
        {
            tokenS_eq_firstToken.generate_r1cs_witness();
            tokenS_eq_secondToken.generate_r1cs_witness();
            tokenS_eq_thirdToken.generate_r1cs_witness();

            tokenB_eq_firstToken.generate_r1cs_witness();
            tokenB_eq_secondToken.generate_r1cs_witness();
            tokenB_eq_thirdToken.generate_r1cs_witness();

            // type = 0
            typeZero_firstScene.generate_r1cs_witness();
            typeZero_secondScene.generate_r1cs_witness();
            typeZero.generate_r1cs_witness();

            // type = 1
            typeOne_firstScene.generate_r1cs_witness();
            typeOne_secondScene.generate_r1cs_witness();
            typeOne.generate_r1cs_witness();

            // type = 2
            typeTwo_firstScene.generate_r1cs_witness();
            typeTwo_secondScene.generate_r1cs_witness();
            typeTwo.generate_r1cs_witness();

            tokenTypeSelect.generate_r1cs_witness();
            validTokenType.generate_r1cs_witness();
            tokenType.generate_r1cs_witness();

        }

        void generate_r1cs_constraints() 
        {
            tokenS_eq_firstToken.generate_r1cs_constraints();
            tokenS_eq_secondToken.generate_r1cs_constraints();
            tokenS_eq_thirdToken.generate_r1cs_constraints();

            tokenB_eq_firstToken.generate_r1cs_constraints();
            tokenB_eq_secondToken.generate_r1cs_constraints();
            tokenB_eq_thirdToken.generate_r1cs_constraints();

            // type = 0
            typeZero_firstScene.generate_r1cs_constraints();
            typeZero_secondScene.generate_r1cs_constraints();
            typeZero.generate_r1cs_constraints();

            // type = 1
            typeOne_firstScene.generate_r1cs_constraints();
            typeOne_secondScene.generate_r1cs_constraints();
            typeOne.generate_r1cs_constraints();

            // type = 2
            typeTwo_firstScene.generate_r1cs_constraints();
            typeTwo_secondScene.generate_r1cs_constraints();
            typeTwo.generate_r1cs_constraints();

            tokenTypeSelect.generate_r1cs_constraints();
            validTokenType.generate_r1cs_constraints();
            tokenType.generate_r1cs_constraints();
        }

        const ToBitsGadget &getTokenType() const
        {
            return tokenType;
        }
};

// Calculate the user's token changes. Except for the first user, there will be three token changes, and the subsequent users have only two changes
// tokenType = 0, 1; firstTokenDif = balanceOneDif
// tokenType = 2; firstTokenDif = balanceTwoDif
// tokenType = 0; secondTokenDif = balanceTwoDif
// tokenType = 1, 2; secondTokenDif = balanceThreeDif
// tokenType = 0; thirdToken = balanceThreeDif
// tokenType = 1; thirdToken = balanceTwoDif
// tokenType = 2; thirdToken = balanceOneDif
class BatchUserTokenAmountExchangeGadget: public GadgetT 
{
    public:
        EqualGadget tokenType_is_zero;
        EqualGadget tokenType_is_one;
        EqualGadget tokenType_is_two;

        ArrayTernaryGadget firstTokenDif;
        ArrayTernaryGadget secondTokenDif;

        SelectGadget thirdTokenDif;
        IfThenRequireEqualGadget requireValidTirdTokenDif;
        BatchUserTokenAmountExchangeGadget(
            ProtoboardT &pb,
            const Constants &constants,
            const VariableT &tokenType,
            const CalculateBalanceDifGadget &balanceOneDif,
            const CalculateBalanceDifGadget &balanceTwoDif,
            const CalculateBalanceDifGadget &balanceThreeDif,
            const VariableT &verifyThirdTokenDif,
            const std::string &prefix)
            : GadgetT(pb, prefix),
            tokenType_is_zero(pb, tokenType, constants._0, FMT(prefix, ".tokenType_is_zero")),
            tokenType_is_one(pb, tokenType, constants._1, FMT(prefix, ".tokenType_is_one")),
            tokenType_is_two(pb, tokenType, constants._2, FMT(prefix, ".tokenType_is_two")),

            firstTokenDif(pb, tokenType_is_two.result(), balanceTwoDif.getBalanceDifFloatBits(), balanceOneDif.getBalanceDifFloatBits(), FMT(prefix, ".firstTokenDif")),
            secondTokenDif(pb, tokenType_is_zero.result(), balanceTwoDif.getBalanceDifFloatBits(), balanceThreeDif.getBalanceDifFloatBits(), FMT(prefix, ".secondTokenDif")),

            thirdTokenDif(
                pb, 
                constants,
                var_array({tokenType_is_zero.result(), tokenType_is_one.result(), tokenType_is_two.result()}), 
                // subVector(constants.values, 0, 3),
                {balanceThreeDif.getBalanceDif(), balanceTwoDif.getBalanceDif(), balanceOneDif.getBalanceDif()},
                // threeVariable_to_vector(balanceThreeDif.getBalanceDif(), balanceTwoDif.getBalanceDif(), balanceOneDif.getBalanceDif()),
                FMT(prefix, ".tokenType_is_zero")),
            requireValidTirdTokenDif(pb, verifyThirdTokenDif, thirdTokenDif.result(), constants._0, FMT(prefix, ".requireValidTirdTokenDif"))
        {

        }
        
        void generate_r1cs_witness() 
        {
            tokenType_is_zero.generate_r1cs_witness();
            tokenType_is_one.generate_r1cs_witness();
            tokenType_is_two.generate_r1cs_witness();

            firstTokenDif.generate_r1cs_witness();
            secondTokenDif.generate_r1cs_witness();

            thirdTokenDif.generate_r1cs_witness();
            requireValidTirdTokenDif.generate_r1cs_witness();
        }

        void generate_r1cs_constraints() 
        {
            tokenType_is_zero.generate_r1cs_constraints();
            tokenType_is_one.generate_r1cs_constraints();
            tokenType_is_two.generate_r1cs_constraints();

            firstTokenDif.generate_r1cs_constraints();
            secondTokenDif.generate_r1cs_constraints();

            thirdTokenDif.generate_r1cs_constraints();
            requireValidTirdTokenDif.generate_r1cs_constraints();
        }

        const VariableArrayT &getFirstExchange() const
        {
            return firstTokenDif.result();
        }

        const VariableArrayT &getSecondExchange() const
        {
            return secondTokenDif.result();
        }
};

class BatchUserGadget: public GadgetT
{
    public:
        std::vector<VariableT> tokens;
        std::vector<StorageGadget> storageGadgets;
        ToBitsGadget firstToken;
        ToBitsGadget secondToken;
        ToBitsGadget thirdToken;
        DualVariableGadget accountID;
        DualVariableGadget isNoop;

        Constants constants;
        VariableT timestamp;
        VariableT blockExchange;
        VariableT maxTradingFeeBips;
        TransactionAccountState account;
        VariableT type;
        VariableT isBatchSpotTradeTx;
        // EqualGadget isBatchSpotTradeTx;
        // 4 - 2 - 1 - 1 - 1 - 1
        // add conditions for the order which size == 4，== 2 == 1 == 1 == 1 == 1
        std::vector<BatchOrderGadget> orders;
        // The calculation of the amount data under a single user is performed here in advance. 
        // It is not placed under batchspottradecircuit to eliminate the existence of two-dimensional arrays
        // Compared with order, amount B belongs to the forward direction and amount s belongs to the reverse direction. 
        // After adding the data of all users, the forward and reverse of the same token should be equal
        // The amount of the first token is accumulated, and the tool will distinguish between forward and reversemount
        std::unique_ptr<BatchTokenAmountSumGadget> tokenOneAmountsSum;
        // The amount of the second token is accumulated, and the tool will distinguish between forward and reversemount
        std::unique_ptr<BatchTokenAmountSumGadget> tokenTwoAmountsSum;
        // The amount of the thid token is accumulated, and the tool will distinguish between forward and reversemount
        std::unique_ptr<BatchTokenAmountSumGadget> tokenThreeAmountsSum;

        // Cumulative tradingfee, which is the sum of the difference between tokenamount and tokeneexchangeamount of all orders
        // Why is the sum of this difference: for an order, if a token is not buytoken, the number of tokenamount and tokeneexchangeamount will be the same
        std::vector<AddGadget> tokenOneTradingFeeAmount;
        std::vector<AddGadget> tokenTwoTradingFeeAmount;
        std::vector<AddGadget> tokenThreeTradingFeeAmount;

        // Accumulate gasfee. Because gasfee is collected separately, it does not need to calculate whether the overall balance is calculated. 
        // Therefore, it is sufficient to directly accumulate gasfee of all orders. However, tokenid needs to be distinguished
        std::vector<AddGadget> tokenOneGasFeeAmount;
        std::vector<AddGadget> tokenTwoGasFeeAmount;
        std::vector<AddGadget> tokenThreeGasFeeAmount;

        // std::unique_ptr<EqualGadget> firstToken_eq_thirdToken;
        // std::unique_ptr<EqualGadget> secondToken_eq_thirdToken;
        // std::unique_ptr<OrGadget> existSameToken;

        std::unique_ptr<DynamicBalanceGadget> balanceOne;
        std::unique_ptr<DynamicBalanceGadget> balanceTwo;
        std::unique_ptr<DynamicBalanceGadget> balanceThree;

        std::unique_ptr<DynamicBalanceGadget> balanceOneBefore;
        std::unique_ptr<DynamicBalanceGadget> balanceTwoBefore;
        std::unique_ptr<DynamicBalanceGadget> balanceThreeBefore;

        // Perform currency exchange calculation
        std::unique_ptr<BalanceExchangeGadget> balanceOneExchange;
        std::unique_ptr<BalanceExchangeGadget> balanceTwoExchange;
        std::unique_ptr<BalanceExchangeGadget> balanceThreeExchange;

        // reduce TradingFee
        std::unique_ptr<BalanceReduceGadget> balanceOneReduceTradingFee;
        std::unique_ptr<BalanceReduceGadget> balanceTwoReduceTradingFee;
        std::unique_ptr<BalanceReduceGadget> balanceThreeReduceTradingFee;

        // reduce GasFee
        std::unique_ptr<BalanceReduceGadget> balanceOneReduceGasFee;
        std::unique_ptr<BalanceReduceGadget> balanceTwoReduceGasFee;
        std::unique_ptr<BalanceReduceGadget> balanceThreeReduceGasFee;

        std::unique_ptr<CalculateBalanceDifGadget> balanceOneDif;
        std::unique_ptr<CalculateBalanceDifGadget> balanceTwoDif;
        std::unique_ptr<CalculateBalanceDifGadget> balanceThreeDif;

        std::unique_ptr<BalanceExchangeGadget> balanceOneRealExchange;
        std::unique_ptr<BalanceExchangeGadget> balanceTwoRealExchange;
        std::unique_ptr<BalanceExchangeGadget> balanceThreeRealExchange;

        // BatchSpotTrade Token Type
        std::unique_ptr<BatchUserTokenTypeGadget> tokenType;

        std::unique_ptr<BatchUserTokenAmountExchangeGadget> amountExchange;
        
        VariableArrayT hashArray;
        VariableArrayT requireSignatureArray;
        VariableArrayT publicXArray;
        VariableArrayT publicYArray;
        BatchUserGadget( //
            ProtoboardT &pb,
            const Constants &_constants,
            const VariableT &_timestamp,
            const VariableT &_blockExchange,
            const VariableT &_maxTradingFeeBips,
            const std::vector<VariableT> &_tokens,
            const std::vector<StorageGadget> &_storageGadgets,
            const TransactionAccountState &_account,
            const VariableT &_type,
            const VariableT &_isBatchSpotTradeTx,
            const VariableT &verifyThirdTokenDif,
            unsigned int orderSize,
            const std::string &prefix)
                : GadgetT(pb, prefix),
                timestamp(_timestamp),
                tokens(_tokens),
                storageGadgets(_storageGadgets),
                account(_account),
                type(_type),
                isBatchSpotTradeTx(_isBatchSpotTradeTx),
                firstToken(pb, tokens[0], NUM_BITS_TOKEN, FMT(prefix, ".firstToken")),
                secondToken(pb, tokens[1], NUM_BITS_TOKEN, FMT(prefix, ".secondToken")),
                thirdToken(pb, tokens[2], NUM_BITS_TOKEN, FMT(prefix, ".thirdToken")),
                accountID(pb, NUM_BITS_ACCOUNT, FMT(prefix, ".accountID")),
                isNoop(pb, NUM_BITS_BIT, FMT(prefix, ".isNoop")),

                constants(_constants),
                blockExchange(_blockExchange),
                maxTradingFeeBips(_maxTradingFeeBips)
        {
            LOG(LogDebug, "in BatchUserGadget", "");
            balanceOne.reset(new DynamicBalanceGadget(pb, account.balanceS.balance, FMT(prefix, ".balanceOne")));
            balanceTwo.reset(new DynamicBalanceGadget(pb, account.balanceB.balance, FMT(prefix, ".balanceTwo")));
            balanceThree.reset(new DynamicBalanceGadget(pb, account.balanceFee.balance, FMT(prefix, ".balanceThree")));

            balanceOneBefore.reset(new DynamicBalanceGadget(pb, account.balanceS.balance, FMT(prefix, ".balanceOneBefore")));
            balanceTwoBefore.reset(new DynamicBalanceGadget(pb, account.balanceB.balance, FMT(prefix, ".balanceTwoBefore")));
            balanceThreeBefore.reset(new DynamicBalanceGadget(pb, account.balanceFee.balance, FMT(prefix, ".balanceThreeBefore")));

            std::vector<VariableT> tokenOneAmounts;
            std::vector<VariableT> tokenTwoAmounts;
            std::vector<VariableT> tokenThreeAmounts;

            std::vector<VariableT> tokenOneSigns;
            std::vector<VariableT> tokenTwoSigns;
            std::vector<VariableT> tokenThreeSigns;
            for (unsigned int i = 0; i < orderSize; i++) 
            {
                LOG(LogDebug, "in BatchUserGadget i", std::to_string(i));
                
                // orders.emplace_back(pb, constants, timestamp, blockExchange, storageGadgets[i], type, maxTradingFeeBips, tokens, account, isBatchSpotTradeTx, prefix + std::string(".ordersize:") + std::to_string(orderSize) + std::string(".BatchUserGadget order_") + std::to_string(i));
                orders.emplace_back(pb, constants, timestamp, blockExchange, storageGadgets[i], maxTradingFeeBips, tokens, account, isBatchSpotTradeTx, prefix + std::string(".ordersize:") + std::to_string(orderSize) + std::string(".BatchUserGadget order_") + std::to_string(i));

                LOG(LogDebug, "in BatchUserGadget before hashArray", "");
                // set signature information
                hashArray.emplace_back(orders[i].hash());
                LOG(LogDebug, "in BatchUserGadget before hashArray", "");
                requireSignatureArray.emplace_back(orders[i].requireSignature());
                LOG(LogDebug, "in BatchUserGadget before publicXArray", "");
                publicXArray.emplace_back(orders[i].getResolvedAuthorX());
                publicYArray.emplace_back(orders[i].getResolvedAuthorY());

                LOG(LogDebug, "in BatchUserGadget before tokenOneAmounts", "");
                tokenOneAmounts.emplace_back(orders[i].getSelectTokenOneAmount());
                tokenOneSigns.emplace_back(orders[i].getSelectTokenOneSign());

                LOG(LogDebug, "in BatchUserGadget before tokenTwoAmounts", "");
                tokenTwoAmounts.emplace_back(orders[i].getSelectTokenTwoAmount());
                tokenTwoSigns.emplace_back(orders[i].getSelectTokenTwoSign());

                LOG(LogDebug, "in BatchUserGadget before tokenThreeAmounts", "");
                tokenThreeAmounts.emplace_back(orders[i].getSelectTokenThreeAmount());
                tokenThreeSigns.emplace_back(orders[i].getSelectTokenThreeSign());

                LOG(LogDebug, "in BatchUserGadget before tokenOneDiffrence", "");
                tokenOneTradingFeeAmount.emplace_back(
                    pb,
                    (i == 0) ? constants._0 : tokenOneTradingFeeAmount.back().result(),
                    orders[i].getSelectTokenOneTradingFee(),
                    NUM_BITS_AMOUNT,
                    std::string(".tokenOneTradingFeeAmount_") + std::to_string(i));
                
                tokenTwoTradingFeeAmount.emplace_back(
                    pb,
                    (i == 0) ? constants._0 : tokenTwoTradingFeeAmount.back().result(),
                    orders[i].getSelectTokenTwoTradingFee(),
                    NUM_BITS_AMOUNT,
                    std::string(".tokenTwoTradingFeeAmount_") + std::to_string(i));

                tokenThreeTradingFeeAmount.emplace_back(
                    pb,
                    (i == 0) ? constants._0 : tokenThreeTradingFeeAmount.back().result(),
                    // tokenThreeDiffrence.back().result(),
                    orders[i].getSelectTokenThreeTradingFee(),
                    NUM_BITS_AMOUNT,
                    std::string(".tokenThreeTradingFeeAmount_") + std::to_string(i));

                LOG(LogDebug, "in BatchUserGadget before tokenOneGasFeeAmount", "");
                // add up GasFee
                tokenOneGasFeeAmount.emplace_back(
                    pb,
                    (i == 0) ? constants._0 : tokenOneGasFeeAmount.back().result(),
                    orders[i].getSelectTokenOneGasFee(),
                    NUM_BITS_AMOUNT,
                    std::string(".tokenOneGasFeeAmount_") + std::to_string(i));
                tokenTwoGasFeeAmount.emplace_back(
                    pb,
                    (i == 0) ? constants._0 : tokenTwoGasFeeAmount.back().result(),
                    orders[i].getSelectTokenTwoGasFee(),
                    NUM_BITS_AMOUNT,
                    std::string(".tokenTwoGasFeeAmount_") + std::to_string(i));
                tokenThreeGasFeeAmount.emplace_back(
                    pb,
                    (i == 0) ? constants._0 : tokenThreeGasFeeAmount.back().result(),
                    orders[i].getSelectTokenThreeGasFee(),
                    NUM_BITS_AMOUNT,
                    std::string(".tokenThreeGasFeeAmount_") + std::to_string(i));
                LOG(LogDebug, "in BatchUserGadget after tokenThreeGasFeeAmount", "");
            }
            // firstToken_eq_thirdToken.reset(new EqualGadget(pb, firstToken.packed, thirdToken.packed, FMT(prefix, ".firstToken_eq_thirdToken")));
            // secondToken_eq_thirdToken.reset(new EqualGadget(pb, secondToken.packed, thirdToken.packed, FMT(prefix, ".secondToken_eq_thirdToken")));
            // existSameToken.reset(new OrGadget(pb, {firstToken_eq_thirdToken->result(), secondToken_eq_thirdToken->result()}, FMT(prefix, ".existSameToken")));
            
            LOG(LogDebug, "in BatchUserGadget before tokenOneAmountsSum", "");
            // Execute the accumulation operation according to the sign. Sign = = 1 is added to forward, sign = = 2 is added to reverse, 
            // and sign = = 0 does not execute the accumulation
            tokenOneAmountsSum.reset(new BatchTokenAmountSumGadget(
                pb,
                constants,
                tokenOneAmounts,
                tokenOneSigns,
                NUM_BITS_AMOUNT,
                ".tokenOneAmountsSum"
            ));

            tokenTwoAmountsSum.reset(new BatchTokenAmountSumGadget(
                pb,
                constants,
                tokenTwoAmounts,
                tokenTwoSigns,
                NUM_BITS_AMOUNT,
                ".tokenTwoAmountsSum"
            ));

            tokenThreeAmountsSum.reset(new BatchTokenAmountSumGadget(
                pb,
                constants,
                tokenThreeAmounts,
                tokenThreeSigns,
                NUM_BITS_AMOUNT,
                ".tokenThreeAmountsSum"
            ));

            LOG(LogDebug, "in BatchUserGadget before balanceOneExchange", "");
            balanceOneExchange.reset(new BalanceExchangeGadget(
                pb,
                *balanceOne,
                tokenOneAmountsSum->getForwardAmount(),
                tokenOneAmountsSum->getReverseAmount(),
                ".balanceOneExchange"));
            balanceTwoExchange.reset(new BalanceExchangeGadget(
                pb,
                *balanceTwo,
                tokenTwoAmountsSum->getForwardAmount(),
                tokenTwoAmountsSum->getReverseAmount(),
                ".balanceTwoExchange"));
            balanceThreeExchange.reset(new BalanceExchangeGadget(
                pb,
                *balanceThree,
                tokenThreeAmountsSum->getForwardAmount(),
                tokenThreeAmountsSum->getReverseAmount(),
                ".balanceThreeExchange"));

            LOG(LogDebug, "in BatchUserGadget before balanceOneReduceTradingFee", "");
            balanceOneReduceTradingFee.reset(new BalanceReduceGadget(pb, *balanceOne, tokenOneTradingFeeAmount.back().result(), ".balanceOneReduceTradingFee"));
            balanceTwoReduceTradingFee.reset(new BalanceReduceGadget(pb, *balanceTwo, tokenTwoTradingFeeAmount.back().result(), ".balanceTwoReduceTradingFee"));
            balanceThreeReduceTradingFee.reset(new BalanceReduceGadget(pb, *balanceThree, tokenThreeTradingFeeAmount.back().result(), ".balanceThreeReduceTradingFee"));

            LOG(LogDebug, "in BatchUserGadget before balanceOneReduceGasFee", "");
            balanceOneReduceGasFee.reset(new BalanceReduceGadget(pb, *balanceOne, tokenOneGasFeeAmount.back().result(), ".balanceOneReduceTradingFee"));
            balanceTwoReduceGasFee.reset(new BalanceReduceGadget(pb, *balanceTwo, tokenTwoGasFeeAmount.back().result(), ".balanceTwoReduceTradingFee"));
            balanceThreeReduceGasFee.reset(new BalanceReduceGadget(pb, *balanceThree, tokenThreeGasFeeAmount.back().result(), ".balanceThreeReduceTradingFee"));

            LOG(LogDebug, "in BatchUserGadget before balanceOneDif", " ");
            balanceOneDif.reset(new CalculateBalanceDifGadget(pb, constants, *balanceOneBefore, *balanceOne, FMT(prefix, ".balanceOneDif")));
            balanceTwoDif.reset(new CalculateBalanceDifGadget(pb, constants, *balanceTwoBefore, *balanceTwo, FMT(prefix, ".balanceTwoDif")));
            balanceThreeDif.reset(new CalculateBalanceDifGadget(pb, constants, *balanceThreeBefore, *balanceThree, FMT(prefix, ".balanceThreeDif")));

            balanceOneRealExchange.reset(new BalanceExchangeGadget(
                pb,
                *balanceOneBefore,
                balanceOneDif->getIncreaseAmount(),
                balanceOneDif->getReduceAmount(),
                ".balanceOneRealExchange"));
            balanceTwoRealExchange.reset(new BalanceExchangeGadget(
                pb,
                *balanceTwoBefore,
                balanceTwoDif->getIncreaseAmount(),
                balanceTwoDif->getReduceAmount(),
                ".balanceTwoRealExchange"));
            balanceThreeRealExchange.reset(new BalanceExchangeGadget(
                pb,
                *balanceThreeBefore,
                balanceThreeDif->getIncreaseAmount(),
                balanceThreeDif->getReduceAmount(),
                ".balanceThreeRealExchange"));

            LOG(LogDebug, "in BatchUserGadget before tokenType", "");
            tokenType.reset(new BatchUserTokenTypeGadget(pb, constants, tokens, orders[0], FMT(prefix, ".BatchUserTokenTypeGadget")));

            LOG(LogDebug, "in BatchUserGadget before amountExchange", "");
            amountExchange.reset(new BatchUserTokenAmountExchangeGadget(
                pb, constants, tokenType->getTokenType().packed, *balanceOneDif, 
                *balanceTwoDif, *balanceThreeDif, verifyThirdTokenDif, FMT(prefix, ".amountExchange")));
            LOG(LogDebug, "in BatchUserGadget after amountExchange", "");
        }

        void generate_r1cs_witness(const BatchSpotTradeUser &user)
        {
            LOG(LogDebug, "in BatchUserGadget account.balanceS:", pb.val(account.balanceS.balance));
            firstToken.generate_r1cs_witness();
            secondToken.generate_r1cs_witness();
            thirdToken.generate_r1cs_witness();
            accountID.generate_r1cs_witness(pb, user.accountID);
            isNoop.generate_r1cs_witness(pb, user.isNoop);

            balanceOne->generate_r1cs_witness();
            balanceTwo->generate_r1cs_witness();
            balanceThree->generate_r1cs_witness();

            balanceOneBefore->generate_r1cs_witness();
            balanceTwoBefore->generate_r1cs_witness();
            balanceThreeBefore->generate_r1cs_witness();

            LOG(LogDebug, "in BatchUserGadget before balanceOneRealExchange balanceOne", pb.val(balanceOne->balance()));
            LOG(LogDebug, "in BatchUserGadget before balanceOneRealExchange balanceOneBefore", pb.val(balanceOneBefore->balance()));

            for (size_t i = 0; i < user.orders.size(); i++) 
            {
                LOG(LogDebug, "in BatchUserGadget generate_r1cs_witness i", std::to_string(i));
                // orders.emplace_back(pb, constants, timestamp, blockExchange, account, type, maxTradingFeeBips, tokens, std::string(".BatchUserGadget order_") + std::to_string(i));
                orders[i].generate_r1cs_witness(user.orders[i]);

                tokenOneTradingFeeAmount[i].generate_r1cs_witness();
                tokenTwoTradingFeeAmount[i].generate_r1cs_witness();
                tokenThreeTradingFeeAmount[i].generate_r1cs_witness();

                // add up GasFee
                tokenOneGasFeeAmount[i].generate_r1cs_witness();
                tokenTwoGasFeeAmount[i].generate_r1cs_witness();
                tokenThreeGasFeeAmount[i].generate_r1cs_witness();
            }
            // firstToken_eq_thirdToken->generate_r1cs_witness();
            // secondToken_eq_thirdToken->generate_r1cs_witness();
            // existSameToken->generate_r1cs_witness();

            // Execute the accumulation operation according to the sign. Sign = = 1 is added to forward, sign = = 2 is added to reverse, 
            // and sign = = 0 does not execute the accumulation
            tokenOneAmountsSum->generate_r1cs_witness();

            tokenTwoAmountsSum->generate_r1cs_witness();

            tokenThreeAmountsSum->generate_r1cs_witness();

            LOG(LogDebug, "in BatchUserGadget tokenOneAmountsSum forward", pb.val(tokenOneAmountsSum->getForwardAmount()));
            LOG(LogDebug, "in BatchUserGadget tokenOneAmountsSum reverse", pb.val(tokenOneAmountsSum->getReverseAmount()));

            LOG(LogDebug, "in BatchUserGadget tokenTwoAmountsSum forward", pb.val(tokenTwoAmountsSum->getForwardAmount()));
            LOG(LogDebug, "in BatchUserGadget tokenTwoAmountsSum reverse", pb.val(tokenTwoAmountsSum->getReverseAmount()));

            LOG(LogDebug, "in BatchUserGadget tokenThreeAmountsSum forward", pb.val(tokenThreeAmountsSum->getForwardAmount()));
            LOG(LogDebug, "in BatchUserGadget tokenThreeAmountsSum reverse", pb.val(tokenThreeAmountsSum->getReverseAmount()));

            balanceOneExchange->generate_r1cs_witness();
            balanceTwoExchange->generate_r1cs_witness();
            balanceThreeExchange->generate_r1cs_witness();

            balanceOneReduceTradingFee->generate_r1cs_witness();

            balanceTwoReduceTradingFee->generate_r1cs_witness();

            balanceThreeReduceTradingFee->generate_r1cs_witness();

            LOG(LogDebug, "in BatchUserGadget balanceOneReduceGasFee tokenOneGasFeeAmount", pb.val(tokenOneGasFeeAmount.back().result()));
            LOG(LogDebug, "in BatchUserGadget balanceOneReduceGasFee tokenTwoGasFeeAmount", pb.val(tokenTwoGasFeeAmount.back().result()));
            LOG(LogDebug, "in BatchUserGadget balanceOneReduceGasFee tokenThreeGasFeeAmount", pb.val(tokenThreeGasFeeAmount.back().result()));
            balanceOneReduceGasFee->generate_r1cs_witness();
            balanceTwoReduceGasFee->generate_r1cs_witness();
            balanceThreeReduceGasFee->generate_r1cs_witness();

            balanceOneDif->generate_r1cs_witness();
            balanceTwoDif->generate_r1cs_witness();
            balanceThreeDif->generate_r1cs_witness();

            LOG(LogDebug, "in BatchUserGadget balanceOneRealExchange balanceOne", pb.val(balanceOne->balance()));
            LOG(LogDebug, "in BatchUserGadget balanceOneRealExchange balanceOneBefore", pb.val(balanceOneBefore->balance()));
            LOG(LogDebug, "in BatchUserGadget balanceOneRealExchange balanceOneDif getIncreaseAmount", pb.val(balanceOneDif->getIncreaseAmount()));
            LOG(LogDebug, "in BatchUserGadget balanceOneRealExchange balanceOneDif getReduceAmount", pb.val(balanceOneDif->getReduceAmount()));
            balanceOneRealExchange->generate_r1cs_witness();
            balanceTwoRealExchange->generate_r1cs_witness();
            balanceThreeRealExchange->generate_r1cs_witness();

            tokenType->generate_r1cs_witness();

            amountExchange->generate_r1cs_witness();

            LOG(LogDebug, "in BatchUserGadget tokenOneTradingFeeAmount", pb.val(tokenOneTradingFeeAmount.back().result()));
            LOG(LogDebug, "in BatchUserGadget tokenTwoTradingFeeAmount", pb.val(tokenTwoTradingFeeAmount.back().result()));
            LOG(LogDebug, "in BatchUserGadget tokenThreeTradingFeeAmount", pb.val(tokenThreeTradingFeeAmount.back().result()));
        }
        void generate_r1cs_constraints() 
        {
            LOG(LogDebug, "in BatchUserGadget", "generate_r1cs_constraints");
            firstToken.generate_r1cs_constraints();
            secondToken.generate_r1cs_constraints();
            thirdToken.generate_r1cs_constraints();
            accountID.generate_r1cs_constraints();
            isNoop.generate_r1cs_constraints();

            balanceOne->generate_r1cs_constraints();
            balanceTwo->generate_r1cs_constraints();
            balanceThree->generate_r1cs_constraints();

            balanceOneBefore->generate_r1cs_constraints();
            balanceTwoBefore->generate_r1cs_constraints();
            balanceThreeBefore->generate_r1cs_constraints();

            for (unsigned int i = 0; i < orders.size(); i++) 
            {
                orders[i].generate_r1cs_constraints();

                tokenOneTradingFeeAmount[i].generate_r1cs_constraints();
                tokenTwoTradingFeeAmount[i].generate_r1cs_constraints();
                tokenThreeTradingFeeAmount[i].generate_r1cs_constraints();

                tokenOneGasFeeAmount[i].generate_r1cs_constraints();
                tokenTwoGasFeeAmount[i].generate_r1cs_constraints();
                tokenThreeGasFeeAmount[i].generate_r1cs_constraints();
            }

            // std::cout << "OrderGadget before firstToken_eq_thirdToken:" << pb.num_constraints() << std::endl;
            // firstToken_eq_thirdToken->generate_r1cs_constraints();
            // secondToken_eq_thirdToken->generate_r1cs_constraints();
            // existSameToken->generate_r1cs_constraints();
            // std::cout << "OrderGadget after existSameToken:" << pb.num_constraints() << std::endl;

            tokenOneAmountsSum->generate_r1cs_constraints();
            tokenTwoAmountsSum->generate_r1cs_constraints();
            tokenThreeAmountsSum->generate_r1cs_constraints();

            balanceOneExchange->generate_r1cs_constraints();
            balanceTwoExchange->generate_r1cs_constraints();
            balanceThreeExchange->generate_r1cs_constraints();

            balanceOneReduceTradingFee->generate_r1cs_constraints();
            balanceTwoReduceTradingFee->generate_r1cs_constraints();
            balanceThreeReduceTradingFee->generate_r1cs_constraints();

            balanceOneReduceGasFee->generate_r1cs_constraints();
            balanceTwoReduceGasFee->generate_r1cs_constraints();
            balanceThreeReduceGasFee->generate_r1cs_constraints();

            balanceOneDif->generate_r1cs_constraints();
            balanceTwoDif->generate_r1cs_constraints();
            balanceThreeDif->generate_r1cs_constraints();

            balanceOneRealExchange->generate_r1cs_constraints();
            balanceTwoRealExchange->generate_r1cs_constraints();
            balanceThreeRealExchange->generate_r1cs_constraints();

            tokenType->generate_r1cs_constraints();

            amountExchange->generate_r1cs_constraints();
        }

        const VariableT &getTokenOneForwardAmount() const
        {
            return tokenOneAmountsSum->getForwardAmount();
        }

        const VariableT &getTokenOneReverseAmount() const
        {
            return tokenOneAmountsSum->getReverseAmount();
        }

        const VariableT &getTokenTwoForwardAmount() const
        {
            return tokenTwoAmountsSum->getForwardAmount();
        }

        const VariableT &getTokenTwoReverseAmount() const
        {
            return tokenTwoAmountsSum->getReverseAmount();
        }

        const VariableT &getTokenThreeForwardAmount() const
        {
            return tokenThreeAmountsSum->getForwardAmount();
        }

        const VariableT &getTokenThreeReverseAmount() const
        {
            return tokenThreeAmountsSum->getReverseAmount();
        }

        const VariableT &getTokenOneTradingFee() const
        {
            // return tokenOneTradingFeeAmount.back().result();
            // return tokenOneTradingFeeTogether->result();
            return tokenOneTradingFeeAmount.back().result();
        }

        const VariableT &getTokenTwoTradingFee() const
        {
            // return tokenTwoTradingFeeAmount.back().result();
            // return tokenTwoTradingFeeTogether->result();
            return tokenTwoTradingFeeAmount.back().result();
        }

        const VariableT &getTokenThreeTradingFee() const
        {
            // return tokenThreeTradingFeeAmount.back().result();
            // return tokenThreeTradingFeeTogether->result();
            return tokenThreeTradingFeeAmount.back().result();
        }

        const VariableT &getTokenOneGasFee() const
        {
            // return tokenOneGasFeeAmount.back().result();
            // return tokenOneGasFeeTogether->result();
            return tokenOneGasFeeAmount.back().result();
        }

        const VariableT &getTokenTwoGasFee() const
        {
            // return tokenTwoGasFeeAmount.back().result();
            // return tokenTwoGasFeeTogether->result();
            return tokenTwoGasFeeAmount.back().result();
        }

        const VariableT &getTokenThreeGasFee() const
        {
            return tokenThreeGasFeeAmount.back().result();
        }

        const ToBitsGadget &getBatchSpotTradeTokenType() const
        {
            return tokenType->getTokenType();
        }

        const VariableArrayT &getFirstTokenAmountExchange() const
        {
            return amountExchange->getFirstExchange();
        }

        const VariableArrayT &getSecondTokenAmountExchange() const
        {
            return amountExchange->getSecondExchange();
        }

        const VariableT &getTokenOneFloatIncrease() const
        {
            return balanceOneDif->getIncreaseAmount();
        }

        const VariableT &getTokenOneFloatReduce() const
        {
            return balanceOneDif->getReduceAmount();
        }

        const VariableT &getTokenTwoFloatIncrease() const
        {
            return balanceTwoDif->getIncreaseAmount();
        }

        const VariableT &getTokenTwoFloatReduce() const
        {
            return balanceTwoDif->getReduceAmount();
        }

        const VariableT &getTokenThreeFloatIncrease() const
        {
            return balanceThreeDif->getIncreaseAmount();
        }

        const VariableT &getTokenThreeFloatReduce() const
        {
            return balanceThreeDif->getReduceAmount();
        }
        
        
};

// The three tokens are different. Bindtoken is the third token
class ValidTokensGadget : public GadgetT 
{
    public:
        EqualGadget tokenOne_eq_tokenTwo;
        EqualGadget tokenOne_eq_tokenThree;
        EqualGadget tokenTwo_eq_tokenThree;

        OrGadget hadEqualToken;

        // The three tokens are different
        IfThenRequireEqualGadget requireValidTokens;

        // Bindtoken is the third token
        IfThenRequireEqualGadget requireValidBindToken;
        ValidTokensGadget(
            ProtoboardT &pb,
            const Constants &constants,
            const std::vector<VariableT> &tokens,
            const VariableT &bindedToken,
            const VariableT &isBatchSpotTrade,
            const std::string &prefix)
                : GadgetT(pb, prefix),
                tokenOne_eq_tokenTwo(pb, tokens[0], tokens[1], FMT(prefix, ".tokenOne_eq_tokenTwo")),
                tokenOne_eq_tokenThree(pb, tokens[0], tokens[2], FMT(prefix, ".tokenOne_eq_tokenThree")),
                tokenTwo_eq_tokenThree(pb, tokens[1], tokens[2], FMT(prefix, ".tokenTwo_eq_tokenThree")),

                hadEqualToken(pb, {tokenOne_eq_tokenTwo.result(), tokenOne_eq_tokenThree.result(), tokenTwo_eq_tokenThree.result()}, FMT(prefix, ".hadEqualToken")),
                requireValidTokens(pb, isBatchSpotTrade, hadEqualToken.result(), constants._0, FMT(prefix, ".requireValidTokens")),

                requireValidBindToken(pb, isBatchSpotTrade, tokens[2], bindedToken, FMT(prefix, ".requireValidBindToken"))
        {

        }

        void generate_r1cs_witness() 
        {
            tokenOne_eq_tokenTwo.generate_r1cs_witness();
            tokenOne_eq_tokenThree.generate_r1cs_witness();
            tokenTwo_eq_tokenThree.generate_r1cs_witness();

            hadEqualToken.generate_r1cs_witness();
            requireValidTokens.generate_r1cs_witness();

            requireValidBindToken.generate_r1cs_witness();
        }

        void generate_r1cs_constraints() 
        {
            tokenOne_eq_tokenTwo.generate_r1cs_constraints();
            tokenOne_eq_tokenThree.generate_r1cs_constraints();
            tokenTwo_eq_tokenThree.generate_r1cs_constraints();

            hadEqualToken.generate_r1cs_constraints();
            requireValidTokens.generate_r1cs_constraints();
            
            requireValidBindToken.generate_r1cs_constraints();
        }
};

} // namespace Loopring



#endif
