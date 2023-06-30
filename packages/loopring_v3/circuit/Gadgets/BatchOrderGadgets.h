// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 DeGate DAO
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

// Each order contains two tokens.
// For a specifying tokenIDs(tokenA), find out the assignation by comparison between tokenIDs(tokenX, tokenY, tokenZ).
// Each token amount(amountA) is bound to a specific tokenID.
// For the final output of amountX, amountY, amountZ, 2 of the amounts will have the value of 0 and 1 amount with a value for token A.
// /For the final output of signX, signY, signZ, 2 of the signs will have the value 0 and 1 sign with a value for token A. 
// sign: 0: default; 1: amount increased for the user; 2: amount reduced for the user.
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

        // tokenA must be one of tokenX, tokenY and tokenZ
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

            // 0: no sign, belongs to fee or does not match; 1: amount increase;2: amount reduce
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

// If the order is a Noop, then some amount related fields need to be required to be 0
class RequireValidNoopOrderGadget : public GadgetT 
{
    public:
        IfThenRequireEqualGadget validDeltaFilledS;
        IfThenRequireEqualGadget validDeltaFilledB;
        IfThenRequireEqualGadget validAppointTradingFee;
        IfThenRequireEqualGadget validGasFee;
        IfThenRequireEqualGadget validAmountS;
        IfThenRequireEqualGadget validAmountB;
        RequireValidNoopOrderGadget(
            ProtoboardT &pb,
            const Constants &constants,
            const VariableT &deltaFilledS,
            const VariableT &deltaFilledB,
            const VariableT &appointTradingFee,
            const VariableT &gasFee,
            const VariableT &amountS,
            const VariableT &amountB,
            const VariableT &isNoop,
            const std::string &prefix)
                : GadgetT(pb, prefix),
                validDeltaFilledS(pb, isNoop, deltaFilledS, constants._0, FMT(prefix, ".validDeltaFilledS")),
                validDeltaFilledB(pb, isNoop, deltaFilledB, constants._0, FMT(prefix, ".validDeltaFilledB")),
                validAppointTradingFee(pb, isNoop, appointTradingFee, constants._0, FMT(prefix, ".validAppointTradingFee")),
                validGasFee(pb, isNoop, gasFee, constants._0, FMT(prefix, ".validGasFee")),
                validAmountS(pb, isNoop, amountS, constants._0, FMT(prefix, ".validAmountS")),
                validAmountB(pb, isNoop, amountB, constants._0, FMT(prefix, ".validAmountB"))
        {
            
        }

        void generate_r1cs_witness()
        {
            validDeltaFilledS.generate_r1cs_witness();
            validDeltaFilledB.generate_r1cs_witness();
            validAppointTradingFee.generate_r1cs_witness();
            validGasFee.generate_r1cs_witness();
            validAmountS.generate_r1cs_witness();
            validAmountB.generate_r1cs_witness();
        }
        void generate_r1cs_constraints()
        {
            validDeltaFilledS.generate_r1cs_constraints();
            validDeltaFilledB.generate_r1cs_constraints();
            validAppointTradingFee.generate_r1cs_constraints();
            validGasFee.generate_r1cs_constraints();
            validAmountS.generate_r1cs_constraints();
            validAmountB.generate_r1cs_constraints();
        }
};

// BatchOrderGadget will include OrderGadget, and add aggregation constraints on the basis of the original OrderGadget
// "deltaFilledS" and "deltaFilledB" are unique fields of BatchOrder. They are mainly used to tell BatchSpotTrade the number of orders 
//   matched at present. The difference from SpotTrade is that BatchOrder has no counterparty for matching. 
// It only needs to keep the balance between "deltaFilledS" and "deltaFilledB" of all orders at the end to consider a valid match
// BatchOrderGadget will calculate the corresponding changes in the number of token matching, 
//   and changes in tradingFee and gasFee for BatchUserGadget according to the determined order of the three tokens.
// Therefore, the output seen will be tokenOneAmount, tokenTwoAmount, tokenThreeAmount, and tokenOneTradingFee, tokenTwoTradingFee, tokenThreeTradingFee, 
//   gasFee in tokenFeeSelect, represented by amountX, amountY, amountZ respectively
class BatchOrderGadget : public GadgetT
{
  public:
    Constants constants;
    std::vector<VariableT> tokens;
    OrderGadget order;

    DualVariableGadget isNoop;
    NotGadget isNotNoop;

    // isBatchSpotTradeTx && isNotNoop, then this order is valid BatchOrder
    AndGadget needCheckOrderValid;
    
    // read history data on storage tree
    StorageReaderGadget tradeHistory;
    AutoMarketOrderCheck autoMarketOrderCheck;

    // auto-market order may change the direct of match, the data and gasFee fields are reset once the direction is changed
    // so, need this StorageReaderForAutoMarketGadget
    StorageReaderForAutoMarketGadget tradeHistoryWithAutoMarket;
    DualVariableGadget deltaFilledS;
    DualVariableGadget deltaFilledB;

    // calculate deltaFilledS, deltaFilledB legitimacy
    // the old logic about order matching needs to be put into batch spot order
    BatchOrderMatchingGadget batchOrderMatching;

    FeeCalculatorGadget tradingFeeCalculator;

    // tradingFee appointed by operator, but must less then calculeted with feeBips
    // Because the user's aggregate transaction token changes must conform to float29(float30 in calldata, 1bit is used to represent positive and negative numbers), 
    // it is necessary to allow modification of tradingFee and gasFee
    DualVariableGadget appointTradingFee;
    // Check whether tradingFee and gasFee has met the requirements
    // appointTradingFee <= tradingFeeCalculator(deltaAmountB * feeBips / 10000); gasFee <= maxFee
    GasFeeMatchingGadget gasFeeMatch;
    // important here
    // If the order is noop, need to ensure that the deltaAmountS, deltaFilledB, appointTradingFee, gasFee, amountS, amountB corresponding to the order is 0
    // If not, there is the possibility that the operator is doing something malicious
    RequireValidNoopOrderGadget validNoopOrder;

    // tokenBExchange = deltaFilledB - appointTradingFee; 
    // tradingFee must be charged on amountB, so it can be deducted directly from deltaFiledB
    // But gasFee is different, need to see the value of feeTokenID
    SubGadget tokenBExchange;

    // Determine which of the three tokens TokenS is, and provide the amount change of TokenS, output amountX, amountY, amountZ, signX, signY, signZ
    // if TokenS is the first token, then amountX == deltaFilledS, amountY == 0, amountZ == 0 and signX == 2, signY == 0, signZ == 0
    // if TokenS is the second token, then amountX == 0, amountY == deltaFilledS, amountZ == 0 and signX == 0, signY == 2, signZ == 0
    SelectOneTokenAmountGadget tokenSSelect;
    // Determine which of the three tokens TokenB is, and provide the amount change of TokenB, output amountX, amountY, amountZ, signX, signY, signZ
    // if TokenS is the first token, then amountX == deltaFilledB, amountY == 0, amountZ == 0 and signX == 1, signY == 0, signZ == 0
    // if TokenS is the second token, then amountX == 0, amountY == deltaFilledB, amountZ == 0 and signX == 0, signY == 1, signZ == 0
    SelectOneTokenAmountGadget tokenBSelect;

    // Calculate the quantity changes of the three tokens on the matching data respectively
    // tokenOneAmount = tokenSSelect.getAmountX() + tokenBSelect.getAmountX()
    AddGadget tokenOneAmount;
    // The sign is special, because the default value is 0 and other valid values ​​are greater than 0(1: amount increase for user; 2: amount reduce for user), 
    // so it can be directly added to get the increase and decrease attribute value of a token
    // tokenOneSign = tokenSSelect.getSignX() + tokenBSelect.getSignX()
    AddGadget tokenOneSign;
    // tokenTwoAmount = tokenSSelect.getAmountY() + tokenBSelect.getAmountY()
    AddGadget tokenTwoAmount;
    // tokenTwoSign = tokenSSelect.getSignY() + tokenBSelect.getSignY()
    AddGadget tokenTwoSign;
    // tokenThreeAmount = tokenSSelect.getAmountZ() + tokenBSelect.getAmountZ()
    AddGadget tokenThreeAmount;
    // tokenThreeSign = tokenSSelect.getSignZ() + tokenBSelect.getSignZ()
    AddGadget tokenThreeSign;

    // calculate token B exchange, this amount will reduce trading fee
    // Calculate the change in quantity after removing tradingFee
    SelectOneTokenAmountGadget tokenBExchangeSelect;
    // tokenOneAmount = tokenSSelect.getAmountX() + tokenBExchangeSelect.getAmountX()
    AddGadget tokenOneExchangeAmount;
    // tokenOneSign = tokenSSelect.getSignX() + tokenBExchangeSelect.getSignX()
    AddGadget tokenOneExchangeSign;
    // tokenTwoAmount = tokenSSelect.getAmountY() + tokenBExchangeSelect.getAmountY()
    AddGadget tokenTwoExchangeAmount;
    // tokenTwoSign = tokenSSelect.getSignY() + tokenBExchangeSelect.getSignY()
    AddGadget tokenTwoExchangeSign;
    // tokenThreeAmount = tokenSSelect.getAmountZ() + tokenBExchangeSelect.getAmountZ()
    AddGadget tokenThreeExchangeAmount;
    // tokenThreeSign = tokenSSelect.getSignZ() + tokenBExchangeSelect.getSignZ()
    AddGadget tokenThreeExchangeSign;

    // Determine which of the three tokens feeTokenID is, and provide the amount change of feeToken, output amountX, amountY, amountZ, signX, signY, signZ
    // gasFee is the payout so sign is 1
    SelectOneTokenAmountGadget tokenFeeSelect;

    // Determine which of the three tokens tradingFee is charged on
    // If tokenBID is first token, then tokenOneTradingFee = appointTradingFee, and tokenTwoTradingFee = tokenThreeTradingFee == 0
    // If tokenBID is second token, then tokenTwoTradingFee = appointTradingFee, and tokenOneTradingFee = tokenThreeTradingFee == 0
    // If tokenBID is third token, then tokenThreeTradingFee = appointTradingFee, and tokenOneTradingFee = tokenTwoTradingFee == 0
    // tokenOneTradingFee = tokenOneAmount - tokenOneExchangeAmount
    SubGadget tokenOneTradingFee;
    // tokenTwoTradingFee = tokenTwoAmount - tokenTwoExchangeAmount
    SubGadget tokenTwoTradingFee;
    // tokenThreeTradingFee = tokenThreeAmount - tokenThreeExchangeAmount
    SubGadget tokenThreeTradingFee;

    // select public key; appkey or asset key
    // SpotTrade allow the use of appKey, choose which key to use to verify the signature
    TernaryGadget resolvedAuthorX;
    TernaryGadget resolvedAuthorY;

    BatchOrderGadget(
      ProtoboardT &pb,
      const Constants &_constants,
      const VariableT &timestamp,
      const VariableT &blockExchange,
      const StorageGadget &storageGadget,
      // split TradingFee and GasFee - ProtocolFeeBips as the max TradingFee
      const VariableT &maxFeeBips,
      const std::vector<VariableT> &_tokens,
      const BaseTransactionAccountState &account,
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
        validNoopOrder(
            pb,
            constants,
            deltaFilledS.packed,
            deltaFilledB.packed,
            appointTradingFee.packed,
            order.fee.packed,
            order.amountS.packed,
            order.amountB.packed,
            isNoop.packed,
            FMT(prefix, ".validNoopOrder")
        ),

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

        deltaFilledS.generate_r1cs_witness(pb, orderEntity.deltaFilledS);
        deltaFilledB.generate_r1cs_witness(pb, orderEntity.deltaFilledB);

        batchOrderMatching.generate_r1cs_witness();
        tradingFeeCalculator.generate_r1cs_witness();
        appointTradingFee.generate_r1cs_witness(pb, orderEntity.tradingFee);
        gasFeeMatch.generate_r1cs_witness();
        validNoopOrder.generate_r1cs_witness();

        tokenBExchange.generate_r1cs_witness();

        tokenSSelect.generate_r1cs_witness();
        tokenBSelect.generate_r1cs_witness();
        tokenOneAmount.generate_r1cs_witness();
        tokenOneSign.generate_r1cs_witness();
        tokenTwoAmount.generate_r1cs_witness();
        tokenTwoSign.generate_r1cs_witness();
        tokenThreeAmount.generate_r1cs_witness();
        tokenThreeSign.generate_r1cs_witness();

        tokenBExchangeSelect.generate_r1cs_witness();
        tokenOneExchangeAmount.generate_r1cs_witness();
        tokenOneExchangeSign.generate_r1cs_witness();
        tokenTwoExchangeAmount.generate_r1cs_witness();
        tokenTwoExchangeSign.generate_r1cs_witness();
        tokenThreeExchangeAmount.generate_r1cs_witness();
        tokenThreeExchangeSign.generate_r1cs_witness();

        tokenFeeSelect.generate_r1cs_witness();

        tokenOneTradingFee.generate_r1cs_witness();
        tokenTwoTradingFee.generate_r1cs_witness();
        tokenThreeTradingFee.generate_r1cs_witness();

        resolvedAuthorX.generate_r1cs_witness();
        resolvedAuthorY.generate_r1cs_witness();
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
        validNoopOrder.generate_r1cs_constraints();

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

    // GasFee, the gas fee may belong one token
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

// According to the provided tokenAmounts and tokenSigns, accumulate the data to forwardAmount and reverseAmount respectively
// if sign[i] == 1, then  forwardAmount = forwardAmount + tokenAmounts[i], else forwardAmount = forwardAmount + 0
// if sign[i] == 2, then  reverseAmount = reverseAmount + tokenAmounts[i], else reverseAmount = reverseAmount + 0
class BatchTokenAmountSumGadget : public GadgetT
{
    public:
        // Count the increase amount of a token
        std::vector<EqualGadget> forwardSignsSelect;
        std::vector<TernaryGadget> forwardAmountsSelect;
        std::vector<AddGadget> forwardAmounts;

        // Count the reduce amount of a token
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
// The return value is the transform value of float30, the length is 30bits. Among them, 29bits represent the value, and 1bit represents the positive and negative numbers.
// There is a mandatory requirement here that the value after conversion to float29 must be the same as the original value without losing precision.
// The data here will be uploaded to SmartContract as calldata.
class CalculateBalanceDifGadget : public GadgetT 
{
    public:
        // if increase, then balanceBefore <= balanceAfter
        // if reduce, the balanceBefore > balanceAfter
        LeqGadget before_leq_after;
        EqualGadget increase;
        EqualGadget reduce;
        // minBalance = min(balanceBefore, balanceAfter)
        // The minimum value calculation method is used here for the following more convenient calculation
        // Because of the particularity of the circuit, whether it is increase or reduce, both increaseAmount and reduceAmount will be calculated, 
        // If minBalance is not used, it will cause negative numbers to appear, which will exceed the calculation range.
        MinGadget minBalance;

        // if increase, then increaseAmount = balanceAfter - minBalance
        IfThenSubGadget increaseAmount;
        // if reduce, then increaseAmount = balanceBefore - minBalance
        IfThenSubGadget reduceAmount;
        // Convert to float form
        FloatGadget fIncreaseAmount;
        // Convert to float form
        FloatGadget fReduceAmount;

        // Negative value: use 2 to the 30th power minus fReduceAmount.floatValue
        SubGadget reduceNegativeAmount;
        // select floatValue: if increase, return fIncreaseAmount.getFArrayValue(), else return reduceNegativeAmount
        TernaryGadget balanceDif;
        // to bits
        ToBitsGadget balanceDifBits;

        // The data is required to be equal. If it is an increase, the increased data is equal to the data after the float. 
        // If it is a reduce, the reduce data is equal to the data after the float
        EqualGadget increaseAmountEqual;
        EqualGadget reduceAmountEqual;
        IfThenRequireGadget requireValidIncreaseAmount;
        IfThenRequireGadget requireValidReduceAmount;

        // The change of user balance uses the float value, and the increase or reduce after the float transformation is calculated here
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
                
                fIncreaseAmount(pb, constants, Float29Encoding, FMT(prefix, ".fIncreaseAmount")),
                fReduceAmount(pb, constants, Float29Encoding, FMT(prefix, ".fReduceAmount")),
                // _ 2pow30 is at the length of byte31, so byte31 is used as the constraint. but it becomes byte30 as long as any non-zero value is subtracted
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
            
            increaseAmount.generate_r1cs_witness();
            reduceAmount.generate_r1cs_witness();

            fIncreaseAmount.generate_r1cs_witness(toFloat(pb.val(increaseAmount.result()), Float29Encoding));
            fReduceAmount.generate_r1cs_witness(toFloat(pb.val(reduceAmount.result()), Float29Encoding));

            reduceNegativeAmount.generate_r1cs_witness();

            balanceDif.generate_r1cs_witness();

            balanceDifBits.generate_r1cs_witness();
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

// Important: The three tokens will definitely not have the same item
// The token type calculation tool calculates the token type according to the order information and tokens information provided
// It is mainly used for UserB to UserF. These users can only have two tokens and need to choose from three tokens. The tokenType indicates which two tokens are selected. 
// UserA has three tokens, no tokenType required
// It is necessary to consider the case that the user is Noop or Order is Noop. At this time, the tokenID is 0, amount is 0, so it does matter what the tokenType is, it's normally 0
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
        // tokenS_eq_firstToken == true and tokenB_eq_secondToken == true
        AndGadget typeZero_firstScene;
        // tokenB_eq_firstToken == true and tokenS_eq_secondToken == true
        AndGadget typeZero_secondScene;
        // typeZero = tokenS_eq_firstToken or typeZero_secondScene
        OrGadget typeZero;

        // type = 1
        // tokenS_eq_firstToken == true and tokenB_eq_thirdToken == true
        AndGadget typeOne_firstScene;
        // tokenB_eq_firstToken == true and tokenS_eq_thirdToken == true
        AndGadget typeOne_secondScene;
        // tokenS_eq_firstToken or tokenB_eq_firstToken
        OrGadget typeOne;

        // type = 2
        // tokenS_eq_secondToken == true and tokenB_eq_thirdToken == true
        AndGadget typeTwo_firstScene;
        // tokenB_eq_secondToken == true and tokenS_eq_thirdToken == true
        AndGadget typeTwo_secondScene;
        // tokenS_eq_secondToken or typeTwo_secondScene
        OrGadget typeTwo;

        // (typeZero, typeOne, typeTwo), (0, 1, 2)
        // Only one will be true at a time, because three tokens will definitely not have the same item
        // if typeZero == true, then tokenTypeSelect.result() = 0
        // if typeOne == true, then tokenTypeSelect.result() = 1
        // if typeTwo == true, then tokenTypeSelect.result() = 2
        SelectGadget tokenTypeSelect;

        // If the user's first order is Noop, then tokenType returns 0 directly. 
        // At present, in addition to UserA having multiple orders, only UserB has two orders, but UserB's first order must be non-Noop
        TernaryGadget validTokenType;
        // to bits
        ToBitsGadget tokenType;

        BatchUserTokenTypeGadget(
            ProtoboardT &pb,
            const Constants &constants,
            const std::vector<VariableT> &tokens,
            // The first order of the user. If the first order isNoop = true, it means that the user is also NOOP
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
// so, so, if tokenType == 2, then firstTokenDif = balanceTwoDif, else firstTokenDif = balanceOneDif
// tokenType = 0; secondTokenDif = balanceTwoDif
// tokenType = 1, 2; secondTokenDif = balanceThreeDif
// so, if tokenType == 0, then secondTokenDif = balanceTwoDif, else secondTokenDif = balanceThreeDif
// tokenType = 0; thirdToken = balanceThreeDif
// tokenType = 1; thirdToken = balanceTwoDif
// tokenType = 2; thirdToken = balanceOneDif
// so, (tokenType_is_zero, tokenType_is_one, tokenType_is_two) * (balanceThreeDif, balanceTwoDif, balanceOneDif)
class BatchUserTokenAmountExchangeGadget: public GadgetT 
{
    public:
        EqualGadget tokenType_is_zero;
        EqualGadget tokenType_is_one;
        EqualGadget tokenType_is_two;

        // if tokenType == 2, then firstTokenDif = balanceTwoDif, else firstTokenDif = balanceOneDif
        ArrayTernaryGadget firstTokenDif;
        // if tokenType == 0, then secondTokenDif = balanceTwoDif, else secondTokenDif = balanceThreeDif
        ArrayTernaryGadget secondTokenDif;

        // (tokenType_is_zero, tokenType_is_one, tokenType_is_two) * (balanceThreeDif, balanceTwoDif, balanceOneDif)
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
                {balanceThreeDif.getBalanceDif(), balanceTwoDif.getBalanceDif(), balanceOneDif.getBalanceDif()},
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

// The logical processing of users in BatchSpotTrade, this tool will process one or more order data of the user, and finally return the user's balance change, gasFee change, and tradingFee change on three given tokens.
// UserA contains the balance changes of three tokens, and UserB to F will only have balance changes in two of the three tokens.
class BatchUserGadget: public GadgetT
{
    public:
        std::vector<VariableT> tokens;
        std::vector<StorageGadget> storageGadgets;
        ToBitsGadget firstToken;
        ToBitsGadget secondToken;
        ToBitsGadget thirdToken;
        DualVariableGadget accountID;
        // DualVariableGadget isNoop;

        Constants constants;
        VariableT timestamp;
        VariableT blockExchange;
        VariableT maxTradingFeeBips;
        BaseTransactionAccountState account;
        VariableT type;
        VariableT isBatchSpotTradeTx;
        // EqualGadget isBatchSpotTradeTx;
        // 4 - 2 - 1 - 1 - 1 - 1
        // add conditions for the order which size == 4，== 2 == 1 == 1 == 1 == 1
        std::vector<BatchOrderGadget> orders;
        // The amount data under a single user is pre-calculated here, and it is not placed under BatchSpotTradeCircuit to prevent the existence of two-dimensional arrays
        // Compared with Order, AmountB belongs to the forward direction, and AmountS belongs to the reverse direction. After adding all User data, the forward and reverse of the same token should be equal
        // The amount of the first token is accumulated, and the tool will distinguish between forward and reverseAmount
        std::unique_ptr<BatchTokenAmountSumGadget> tokenOneAmountsSum;
        // The amount of the second token is accumulated, and the tool will distinguish between forward and reverseAmount
        std::unique_ptr<BatchTokenAmountSumGadget> tokenTwoAmountsSum;
        // The amount of the third token is accumulated, and the tool will distinguish between forward and reverseAmount
        std::unique_ptr<BatchTokenAmountSumGadget> tokenThreeAmountsSum;

        // Accumulate TradingFee, TradingFee is the sum of the difference between the respective tokenAmount and tokenExchangeAmount of all orders
        // Why is this difference sum: For an order, if a certain token is not a buytoken, then the amount of tokenAmount and tokenExchangeAmount will be the same
        std::vector<AddGadget> tokenOneTradingFeeAmount;
        std::vector<AddGadget> tokenTwoTradingFeeAmount;
        std::vector<AddGadget> tokenThreeTradingFeeAmount;

        // require accountID equal
        std::vector<IfThenRequireEqualGadget> requireAccountsEqual;

        // Accumulate GasFee, because GasFee is charged separately, there is no need to additionally calculate the overall balance, 
        // so you can directly accumulate the GasFee of all orders, but you need to distinguish the TokenID
        std::vector<AddGadget> tokenOneGasFeeAmount;
        std::vector<AddGadget> tokenTwoGasFeeAmount;
        std::vector<AddGadget> tokenThreeGasFeeAmount;

        // The user's banalceS, balanceB, and balanceFee are listed in the order of tokens
        std::unique_ptr<DynamicBalanceGadget> balanceOne;
        std::unique_ptr<DynamicBalanceGadget> balanceTwo;
        std::unique_ptr<DynamicBalanceGadget> balanceThree;
        // Before balance is used to calculate the difference with the calculated balance
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

        // The balance change of the first token, which may be positive or negative, is represented by float30
        std::unique_ptr<CalculateBalanceDifGadget> balanceOneDif;
        std::unique_ptr<CalculateBalanceDifGadget> balanceTwoDif;
        std::unique_ptr<CalculateBalanceDifGadget> balanceThreeDif;

        // If the number of tokens increases, the decrease will be 0. If the number of tokens decreases, the increase will be 0
        std::unique_ptr<BalanceExchangeGadget> balanceOneRealExchange;
        std::unique_ptr<BalanceExchangeGadget> balanceTwoRealExchange;
        std::unique_ptr<BalanceExchangeGadget> balanceThreeRealExchange;

        // BatchSpotTrade Token Type
        std::unique_ptr<BatchUserTokenTypeGadget> tokenType;

        // Mainly used for UserB to UserF, these users have only two tokens, and the number of these two tokens needs to be changed according to the tokenType. 
        // It is relatively simple for UserA to directly return the changes in the number of three tokens.
        std::unique_ptr<BatchUserTokenAmountExchangeGadget> amountExchange;
        
        // Accumulate the hash and signature data of all orders
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
            const BaseTransactionAccountState &_account,
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
                orders.emplace_back(pb, constants, timestamp, blockExchange, storageGadgets[i], maxTradingFeeBips, tokens, account, isBatchSpotTradeTx, prefix + std::string(".ordersize:") + std::to_string(orderSize) + std::string(".BatchUserGadget order_") + std::to_string(i));

                // set signature information
                hashArray.emplace_back(orders[i].hash());
                requireSignatureArray.emplace_back(orders[i].requireSignature());
                publicXArray.emplace_back(orders[i].getResolvedAuthorX());
                publicYArray.emplace_back(orders[i].getResolvedAuthorY());

                tokenOneAmounts.emplace_back(orders[i].getSelectTokenOneAmount());
                tokenOneSigns.emplace_back(orders[i].getSelectTokenOneSign());

                tokenTwoAmounts.emplace_back(orders[i].getSelectTokenTwoAmount());
                tokenTwoSigns.emplace_back(orders[i].getSelectTokenTwoSign());

                tokenThreeAmounts.emplace_back(orders[i].getSelectTokenThreeAmount());
                tokenThreeSigns.emplace_back(orders[i].getSelectTokenThreeSign());

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
                    orders[i].getSelectTokenThreeTradingFee(),
                    NUM_BITS_AMOUNT,
                    std::string(".tokenThreeTradingFeeAmount_") + std::to_string(i));

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

                requireAccountsEqual.emplace_back(
                    pb,
                    orders.back().isNotNoop.result(),
                    accountID.packed,
                    orders.back().order.accountID.packed,
                    std::string(".requireAccountsEqual_") + std::to_string(i));
            }
            
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

            balanceOneReduceTradingFee.reset(new BalanceReduceGadget(pb, *balanceOne, tokenOneTradingFeeAmount.back().result(), ".balanceOneReduceTradingFee"));
            balanceTwoReduceTradingFee.reset(new BalanceReduceGadget(pb, *balanceTwo, tokenTwoTradingFeeAmount.back().result(), ".balanceTwoReduceTradingFee"));
            balanceThreeReduceTradingFee.reset(new BalanceReduceGadget(pb, *balanceThree, tokenThreeTradingFeeAmount.back().result(), ".balanceThreeReduceTradingFee"));

            balanceOneReduceGasFee.reset(new BalanceReduceGadget(pb, *balanceOne, tokenOneGasFeeAmount.back().result(), ".balanceOneReduceTradingFee"));
            balanceTwoReduceGasFee.reset(new BalanceReduceGadget(pb, *balanceTwo, tokenTwoGasFeeAmount.back().result(), ".balanceTwoReduceTradingFee"));
            balanceThreeReduceGasFee.reset(new BalanceReduceGadget(pb, *balanceThree, tokenThreeGasFeeAmount.back().result(), ".balanceThreeReduceTradingFee"));

            balanceOneDif.reset(new CalculateBalanceDifGadget(pb, constants, *balanceOneBefore, *balanceOne, FMT(prefix, ".balanceOneDif")));
            balanceTwoDif.reset(new CalculateBalanceDifGadget(pb, constants, *balanceTwoBefore, *balanceTwo, FMT(prefix, ".balanceTwoDif")));
            balanceThreeDif.reset(new CalculateBalanceDifGadget(pb, constants, *balanceThreeBefore, *balanceThree, FMT(prefix, ".balanceThreeDif")));

            // Calculate the real value. The increase or decrease of user balance must be the float value
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

            tokenType.reset(new BatchUserTokenTypeGadget(pb, constants, tokens, orders[0], FMT(prefix, ".BatchUserTokenTypeGadget")));

            amountExchange.reset(new BatchUserTokenAmountExchangeGadget(
                pb, constants, tokenType->getTokenType().packed, *balanceOneDif, 
                *balanceTwoDif, *balanceThreeDif, verifyThirdTokenDif, FMT(prefix, ".amountExchange")));
        }

        void generate_r1cs_witness(const BatchSpotTradeUser &user)
        {
            LOG(LogDebug, "in BatchUserGadget generate_r1cs_witness", "");
            firstToken.generate_r1cs_witness();
            secondToken.generate_r1cs_witness();
            thirdToken.generate_r1cs_witness();
            accountID.generate_r1cs_witness(pb, user.accountID);

            balanceOne->generate_r1cs_witness();
            balanceTwo->generate_r1cs_witness();
            balanceThree->generate_r1cs_witness();

            balanceOneBefore->generate_r1cs_witness();
            balanceTwoBefore->generate_r1cs_witness();
            balanceThreeBefore->generate_r1cs_witness();

            for (size_t i = 0; i < user.orders.size(); i++) 
            {
                LOG(LogDebug, "in BatchUserGadget generate_r1cs_witness i", std::to_string(i));
                orders[i].generate_r1cs_witness(user.orders[i]);

                tokenOneTradingFeeAmount[i].generate_r1cs_witness();
                tokenTwoTradingFeeAmount[i].generate_r1cs_witness();
                tokenThreeTradingFeeAmount[i].generate_r1cs_witness();

                // add up GasFee
                tokenOneGasFeeAmount[i].generate_r1cs_witness();
                tokenTwoGasFeeAmount[i].generate_r1cs_witness();
                tokenThreeGasFeeAmount[i].generate_r1cs_witness();

                requireAccountsEqual[i].generate_r1cs_witness();
            }

            // Execute the accumulation operation according to the sign. Sign = = 1 is added to forward, sign = = 2 is added to reverse, 
            // and sign = = 0 does not execute the accumulation
            tokenOneAmountsSum->generate_r1cs_witness();

            tokenTwoAmountsSum->generate_r1cs_witness();

            tokenThreeAmountsSum->generate_r1cs_witness();

            balanceOneExchange->generate_r1cs_witness();
            balanceTwoExchange->generate_r1cs_witness();
            balanceThreeExchange->generate_r1cs_witness();

            balanceOneReduceTradingFee->generate_r1cs_witness();

            balanceTwoReduceTradingFee->generate_r1cs_witness();

            balanceThreeReduceTradingFee->generate_r1cs_witness();

            balanceOneReduceGasFee->generate_r1cs_witness();
            balanceTwoReduceGasFee->generate_r1cs_witness();
            balanceThreeReduceGasFee->generate_r1cs_witness();

            balanceOneDif->generate_r1cs_witness();
            balanceTwoDif->generate_r1cs_witness();
            balanceThreeDif->generate_r1cs_witness();

            balanceOneRealExchange->generate_r1cs_witness();
            balanceTwoRealExchange->generate_r1cs_witness();
            balanceThreeRealExchange->generate_r1cs_witness();

            tokenType->generate_r1cs_witness();

            amountExchange->generate_r1cs_witness();
        }
        void generate_r1cs_constraints() 
        {
            LOG(LogDebug, "in BatchUserGadget", "generate_r1cs_constraints");
            firstToken.generate_r1cs_constraints();
            secondToken.generate_r1cs_constraints();
            thirdToken.generate_r1cs_constraints();
            accountID.generate_r1cs_constraints();

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

                requireAccountsEqual[i].generate_r1cs_constraints();
            }

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
            return tokenOneTradingFeeAmount.back().result();
        }

        const VariableT &getTokenTwoTradingFee() const
        {
            return tokenTwoTradingFeeAmount.back().result();
        }

        const VariableT &getTokenThreeTradingFee() const
        {
            return tokenThreeTradingFeeAmount.back().result();
        }

        const VariableT &getTokenOneGasFee() const
        {
            return tokenOneGasFeeAmount.back().result();
        }

        const VariableT &getTokenTwoGasFee() const
        {
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

// The three tokens are different. Bindtoken must be the third token
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
