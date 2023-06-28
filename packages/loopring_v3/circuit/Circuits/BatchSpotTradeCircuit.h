// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 DeGate DAO
#ifndef _BATCHSPOTTRADECIRCUIT_H_
#define _BATCHSPOTTRADECIRCUIT_H_

#include "Circuit.h"
#include "../Utils/Constants.h"
#include "../Utils/Data.h"
#include "../Gadgets/BatchOrderGadgets.h"

#include "ethsnarks.hpp"
#include "utils.hpp"

using namespace ethsnarks;

namespace Loopring
{
// Matched orders from a group of users are batched up.
// The circuit ensures that the expenditure and income of every order from all users comply with the order price limit,
// It also ensures and maintains the valid cumulative balance of revenue and expenditure of all currencies for all users.
// For a single order of a single user, don't need to know which order has been matched with, but only need to know whether the matching result of collective orders is correct. 
// In order to improve the aggregation efficiency of aggregating the number of orders by users, the supported token types have been adjusted.
// Currently, 6 users are supported. The number of orders users 1 to 6 can have is 4, 2, 1, 1, 1, 1 respectively. Orders from user 1 can have up to 3 different tokens.
// Orders from users 2 to 6 can only deal up to 2 out of the 3 tokens user 1 has. The combination of those two tokens is distinguished by using TokenType(00, 01, 10).
class BatchSpotTradeCircuit : public BaseTransactionCircuit
{
  public:
    Constants constants;
    TransactionState state;
    ToBitsGadget typeTx;
    DualVariableGadget bindTokenID;
    VariableT blockExchange;
    VariableT maxTradingFeeBips;

    IsBatchSpotTradeGadget isBatchSpotTradeTx;

    std::vector<DualVariableGadget> tokensDual;
    std::vector<VariableT> tokens;
    std::unique_ptr<ValidTokensGadget> validTokens;

    std::vector<BatchUserGadget> users;

    // both userA and userB must have an order and the first order must not be a noop
    std::unique_ptr<IfThenRequireEqualGadget> requireUserAOrderNotNoop;
    std::unique_ptr<IfThenRequireEqualGadget> requireUserBOrderNotNoop;

    // forward: amount buy
    // reverse: amount sell
    // forward sum must == reverse sum, 
    // require forwardOneAmounts.back().result() == reverseOneAmounts.back().result()
    // Cumulatively calculate the income and expenditure of all users' three tokens(excluding the change in the number of tradingFee and gasFee)
    std::vector<AddGadget> forwardOneAmounts;
    std::vector<AddGadget> reverseOneAmounts;
    std::vector<AddGadget> forwardTwoAmounts;
    std::vector<AddGadget> reverseTwoAmounts;
    std::vector<AddGadget> forwardThreeAmounts;
    std::vector<AddGadget> reverseThreeAmounts;

    // Accumulate the spending amount of the three tokens of all users, the amount is calculated according to the value of float
    std::vector<AddGadget> tokenOneFloatReverse;
    std::vector<AddGadget> tokenTwoFloatReverse;
    std::vector<AddGadget> tokenThreeFloatReverse;

    // Accumulate the income amount of the three tokens of all users, and the amount is calculated according to the value of float
    std::vector<AddGadget> tokenOneFloatForward;
    std::vector<AddGadget> tokenTwoFloatForward;
    std::vector<AddGadget> tokenThreeFloatForward;

    // Calculate the income of the operator account according to the accumulated income and expenditure of the three tokens of all users
    // real reverse - real forward = operator fee
    std::unique_ptr<SubGadget> firstTokenFeeSum;
    std::unique_ptr<SubGadget> secondTokenFeeSum;
    std::unique_ptr<SubGadget> thirdTokenFeeSum;

    // forward sum must == reverse sum
    // require forwardOneAmounts.back().result() == reverseOneAmounts.back().result()
    // BatchSpotTrade is different from SpotTrade. The user of the order does not need to know who the counterparty is, but only needs to ensure that the change 
    //   in the number of three tokens of the user can reach a balance of payments (excluding the change in the number of tradingFee and gasFee)
    std::unique_ptr<RequireEqualGadget> tokenOneMatch;
    std::unique_ptr<RequireEqualGadget> tokenTwoMatch;
    std::unique_ptr<RequireEqualGadget> tokenThreeMatch;

    // Used for storage update, if the order is noop, then use the before data in account update
    std::vector<ToBitsGadget> userAStorageIDBits;
    std::vector<ArrayTernaryGadget> userAStorageAddress;
    std::vector<TernaryGadget> userAStorageTokenSID;
    std::vector<TernaryGadget> userAStorageTokenBID;
    std::vector<TernaryGadget> userAStorageData;
    std::vector<TernaryGadget> userAStorageStorageID;
    std::vector<TernaryGadget> userAStorageCancelled;
    std::vector<TernaryGadget> userAStorageGasFee;
    std::vector<TernaryGadget> userAStorageForward;

    // Used for storage update, if the order is noop, then use the before data in account update
    std::vector<ToBitsGadget> userBStorageIDBits;
    std::vector<ArrayTernaryGadget> userBStorageAddress;
    std::vector<TernaryGadget> userBStorageTokenSID;
    std::vector<TernaryGadget> userBStorageTokenBID;
    std::vector<TernaryGadget> userBStorageData;
    std::vector<TernaryGadget> userBStorageStorageID;
    std::vector<TernaryGadget> userBStorageCancelled;
    std::vector<TernaryGadget> userBStorageGasFee;
    std::vector<TernaryGadget> userBStorageForward;

    // Used for storage update, if the order is noop, then use the before data in account update
    std::vector<ToBitsGadget> userCStorageIDBits;
    std::vector<ArrayTernaryGadget> userCStorageAddress;
    std::vector<TernaryGadget> userCStorageTokenSID;
    std::vector<TernaryGadget> userCStorageTokenBID;
    std::vector<TernaryGadget> userCStorageData;
    std::vector<TernaryGadget> userCStorageStorageID;
    std::vector<TernaryGadget> userCStorageCancelled;
    std::vector<TernaryGadget> userCStorageGasFee;
    std::vector<TernaryGadget> userCStorageForward;

    // Used for storage update, if the order is noop, then use the before data in account update
    std::vector<ToBitsGadget> userDStorageIDBits;
    std::vector<ArrayTernaryGadget> userDStorageAddress;
    std::vector<TernaryGadget> userDStorageTokenSID;
    std::vector<TernaryGadget> userDStorageTokenBID;
    std::vector<TernaryGadget> userDStorageData;
    std::vector<TernaryGadget> userDStorageStorageID;
    std::vector<TernaryGadget> userDStorageCancelled;
    std::vector<TernaryGadget> userDStorageGasFee;
    std::vector<TernaryGadget> userDStorageForward;

    // Used for storage update, if the order is noop, then use the before data in account update
    std::vector<ToBitsGadget> userEStorageIDBits;
    std::vector<ArrayTernaryGadget> userEStorageAddress;
    std::vector<TernaryGadget> userEStorageTokenSID;
    std::vector<TernaryGadget> userEStorageTokenBID;
    std::vector<TernaryGadget> userEStorageData;
    std::vector<TernaryGadget> userEStorageStorageID;
    std::vector<TernaryGadget> userEStorageCancelled;
    std::vector<TernaryGadget> userEStorageGasFee;
    std::vector<TernaryGadget> userEStorageForward;

    // Used for storage update, if the order is noop, then use the before data in account update
    std::vector<ToBitsGadget> userFStorageIDBits;
    std::vector<ArrayTernaryGadget> userFStorageAddress;
    std::vector<TernaryGadget> userFStorageTokenSID;
    std::vector<TernaryGadget> userFStorageTokenBID;
    std::vector<TernaryGadget> userFStorageData;
    std::vector<TernaryGadget> userFStorageStorageID;
    std::vector<TernaryGadget> userFStorageCancelled;
    std::vector<TernaryGadget> userFStorageGasFee;
    std::vector<TernaryGadget> userFStorageForward;

    // The operator balance change needs to be calculated to calculate the posting of the user's real balance change
    // The operator's revenue is the total outgoing of all users minus incoming of all users
    std::unique_ptr<DynamicBalanceGadget> balanceC_O;
    std::unique_ptr<DynamicBalanceGadget> balanceB_O;
    std::unique_ptr<DynamicBalanceGadget> balanceA_O;

    std::unique_ptr<BalanceIncreaseGadget> balanceC_O_Increase;
    std::unique_ptr<BalanceIncreaseGadget> balanceB_O_Increase;
    std::unique_ptr<BalanceIncreaseGadget> balanceA_O_Increase;

    BatchSpotTradeCircuit( //
      ProtoboardT &pb,
      const TransactionState &_state,
      const std::string &prefix)
        : BaseTransactionCircuit(pb, _state, prefix),
        constants(_state.constants),
        state(_state),
        typeTx(pb, state.constants.txTypeBatchSpotTrade, NUM_BITS_TX_TYPE, FMT(prefix, ".typeTx")),
        bindTokenID(pb, NUM_BITS_BIND_TOKEN_ID_SIZE, FMT(prefix, ".bindTokenID")),
        blockExchange(_state.exchange),
        maxTradingFeeBips(_state.protocolFeeBips),
        isBatchSpotTradeTx(pb, _state.constants, state.type, typeTx.packed, FMT(prefix, ".isBatchSpotTradeTx"))
    {
        LOG(LogDebug, "in BatchSpotTradeCircuit", "");

        for (size_t i = 0; i < 3; i++) 
        {
            tokensDual.emplace_back(pb, NUM_BITS_TOKEN, ".tokensDual" + std::to_string(i));
            tokens.emplace_back(tokensDual.back().packed);
        }
        validTokens.reset(new ValidTokensGadget(pb, constants, tokens, bindTokenID.packed, isBatchSpotTradeTx.result(), FMT(prefix, ".validTokens")));

        for (unsigned int i = 0; i < BATCH_SPOT_TRADE_MAX_USER; i++) 
        {
            LOG(LogDebug, "in BatchSpotTradeCircuit i", std::to_string(i));
            std::vector<StorageGadget> storageGadgets;
            if (i == 0) {
                storageGadgets.emplace_back(state.accountA.storage);
                for (unsigned int j = 0; j < state.accountA.storageArray.size(); j++) {
                    storageGadgets.emplace_back(state.accountA.storageArray[j]);
                }
                users.emplace_back(pb, constants, state.timestamp, blockExchange, maxTradingFeeBips, tokens, storageGadgets, state.accountA, state.type, isBatchSpotTradeTx.result(), constants._0, ORDER_SIZE_USER_A, prefix + std::string("user_") + std::to_string(i));
            } else if (i == 1) {
                storageGadgets.emplace_back(state.accountB.storage);
                for (unsigned int j = 0; j < state.accountB.storageArray.size(); j++) {
                    storageGadgets.emplace_back(state.accountB.storageArray[j]);
                }
                users.emplace_back(pb, constants, state.timestamp, blockExchange, maxTradingFeeBips, tokens, storageGadgets, state.accountB, state.type, isBatchSpotTradeTx.result(), constants._1, ORDER_SIZE_USER_B, prefix + std::string("user_") + std::to_string(i));
            } else if (i == 2) {
                for (unsigned int j = 0; j < state.accountC.storageArray.size(); j++) {
                    storageGadgets.emplace_back(state.accountC.storageArray[j]);
                }
                users.emplace_back(pb, constants, state.timestamp, blockExchange, maxTradingFeeBips, tokens, storageGadgets, state.accountC, state.type, isBatchSpotTradeTx.result(), constants._1, ORDER_SIZE_USER_C, prefix + std::string("user_") + std::to_string(i));
            } else if (i == 3) {
                for (unsigned int j = 0; j < state.accountD.storageArray.size(); j++) {
                    storageGadgets.emplace_back(state.accountD.storageArray[j]);
                }
                users.emplace_back(pb, constants, state.timestamp, blockExchange, maxTradingFeeBips, tokens, storageGadgets, state.accountD, state.type, isBatchSpotTradeTx.result(), constants._1, ORDER_SIZE_USER_D, prefix + std::string("user_") + std::to_string(i));
            } else if (i == 4) {
                for (unsigned int j = 0; j < state.accountE.storageArray.size(); j++) {
                    storageGadgets.emplace_back(state.accountE.storageArray[j]);
                }
                users.emplace_back(pb, constants, state.timestamp, blockExchange, maxTradingFeeBips, tokens, storageGadgets, state.accountE, state.type, isBatchSpotTradeTx.result(), constants._1, ORDER_SIZE_USER_E, prefix + std::string("user_") + std::to_string(i));
            } else if (i == 5) {
                for (unsigned int j = 0; j < state.accountF.storageArray.size(); j++) {
                    storageGadgets.emplace_back(state.accountF.storageArray[j]);
                }
                users.emplace_back(pb, constants, state.timestamp, blockExchange, maxTradingFeeBips, tokens, storageGadgets, state.accountF, state.type, isBatchSpotTradeTx.result(), constants._1, ORDER_SIZE_USER_F, prefix + std::string("user_") + std::to_string(i));
            }

            forwardOneAmounts.emplace_back(pb, (i == 0) ? constants._0 : forwardOneAmounts.back().result(), users.back().getTokenOneForwardAmount(), NUM_BITS_AMOUNT, std::string(".forwardOneAmounts_") + std::to_string(i));
            forwardTwoAmounts.emplace_back(pb, (i == 0) ? constants._0 : forwardTwoAmounts.back().result(), users.back().getTokenTwoForwardAmount(), NUM_BITS_AMOUNT, std::string(".forwardTwoAmounts_") + std::to_string(i));
            forwardThreeAmounts.emplace_back(pb, (i == 0) ? constants._0 : forwardThreeAmounts.back().result(), users.back().getTokenThreeForwardAmount(), NUM_BITS_AMOUNT, std::string(".forwardThreeAmounts_") + std::to_string(i));
            
            reverseOneAmounts.emplace_back(pb, (i == 0) ? constants._0 : reverseOneAmounts.back().result(), users.back().getTokenOneReverseAmount(), NUM_BITS_AMOUNT, std::string(".reverseOneAmounts_") + std::to_string(i));
            reverseTwoAmounts.emplace_back(pb, (i == 0) ? constants._0 : reverseTwoAmounts.back().result(), users.back().getTokenTwoReverseAmount(), NUM_BITS_AMOUNT, std::string(".reverseTwoAmounts_") + std::to_string(i));
            reverseThreeAmounts.emplace_back(pb, (i == 0) ? constants._0 : reverseThreeAmounts.back().result(), users.back().getTokenThreeReverseAmount(), NUM_BITS_AMOUNT, std::string(".reverseThreeAmounts_") + std::to_string(i));

            tokenOneFloatForward.emplace_back(pb, (i == 0) ? constants._0 : tokenOneFloatForward.back().result(), users.back().getTokenOneFloatIncrease(), NUM_BITS_AMOUNT, std::string(".tokenOneFloatForward_") + std::to_string(i));
            tokenTwoFloatForward.emplace_back(pb, (i == 0) ? constants._0 : tokenTwoFloatForward.back().result(), users.back().getTokenTwoFloatIncrease(), NUM_BITS_AMOUNT, std::string(".tokenTwoFloatForward_") + std::to_string(i));
            tokenThreeFloatForward.emplace_back(pb, (i == 0) ? constants._0 : tokenThreeFloatForward.back().result(), users.back().getTokenThreeFloatIncrease(), NUM_BITS_AMOUNT, std::string(".tokenThreeFloatForward_") + std::to_string(i));

            tokenOneFloatReverse.emplace_back(pb, (i == 0) ? constants._0 : tokenOneFloatReverse.back().result(), users.back().getTokenOneFloatReduce(), NUM_BITS_AMOUNT, std::string(".tokenOneFloatReduce_") + std::to_string(i));
            tokenTwoFloatReverse.emplace_back(pb, (i == 0) ? constants._0 : tokenTwoFloatReverse.back().result(), users.back().getTokenTwoFloatReduce(), NUM_BITS_AMOUNT, std::string(".tokenTwoFloatReduce_") + std::to_string(i));
            tokenThreeFloatReverse.emplace_back(pb, (i == 0) ? constants._0 : tokenThreeFloatReverse.back().result(), users.back().getTokenThreeFloatReduce(), NUM_BITS_AMOUNT, std::string(".tokenThreeFloatReduce_") + std::to_string(i));
        }

        requireUserAOrderNotNoop.reset(new IfThenRequireEqualGadget(pb, isBatchSpotTradeTx.result(), users[0].orders[0].isNoop.packed, constants._0, FMT(prefix, ".requireUserAOrderNotNoop")));
        requireUserBOrderNotNoop.reset(new IfThenRequireEqualGadget(pb, isBatchSpotTradeTx.result(), users[1].orders[0].isNoop.packed, constants._0, FMT(prefix, ".requireUserBOrderNotNoop")));
        
        firstTokenFeeSum.reset(new SubGadget(pb, tokenOneFloatReverse.back().result(), tokenOneFloatForward.back().result(), NUM_BITS_AMOUNT, FMT(prefix, ".firstTokenFeeSum")));
        secondTokenFeeSum.reset(new SubGadget(pb, tokenTwoFloatReverse.back().result(), tokenTwoFloatForward.back().result(), NUM_BITS_AMOUNT, FMT(prefix, ".secondTokenFeeSum")));
        thirdTokenFeeSum.reset(new SubGadget(pb, tokenThreeFloatReverse.back().result(), tokenThreeFloatForward.back().result(), NUM_BITS_AMOUNT, FMT(prefix, ".thirdTokenFeeSum")));

        LOG(LogDebug, "in BatchUserGadget before one storage", "");
        for (unsigned int j = 1; j < ORDER_SIZE_USER_A; j++) 
        {
            userAStorageIDBits.emplace_back(pb, state.accountA.storageArray[j - 1].storageID, NUM_BITS_STORAGEID, FMT(prefix, ".userAStorageIDBits"));
            userAStorageAddress.emplace_back(
                pb, 
                users[0].orders[j].isNoop.packed, 
                subArray(userAStorageIDBits.back().bits, 0, NUM_BITS_STORAGE_ADDRESS),
                subArray(users[0].orders[j].order.storageID.bits, 0, NUM_BITS_STORAGE_ADDRESS),
                FMT(prefix, ".userAStorageAddress"));
            
            userAStorageTokenSID.emplace_back(
                pb,
                users[0].orders[j].isNoop.packed, 
                state.accountA.storageArray[j - 1].tokenSID,
                users[0].orders[j].autoMarketOrderCheck.getTokenSIDForStorageUpdate(),
                FMT(prefix, ".userAStorageTokenSID"));
            userAStorageTokenBID.emplace_back(
                pb,
                users[0].orders[j].isNoop.packed, 
                state.accountA.storageArray[j - 1].tokenBID,
                users[0].orders[j].autoMarketOrderCheck.getTokenBIDForStorageUpdate(),
                FMT(prefix, ".userAStorageTokenBID"));
            userAStorageData.emplace_back(
                pb,
                users[0].orders[j].isNoop.packed, 
                state.accountA.storageArray[j - 1].data,
                users[0].orders[j].batchOrderMatching.getFilledAfter(),
                FMT(prefix, ".userAStorageData"));
            userAStorageStorageID.emplace_back(
                pb,
                users[0].orders[j].isNoop.packed, 
                state.accountA.storageArray[j - 1].storageID,
                users[0].orders[j].order.storageID.packed,
                FMT(prefix, ".userAStorageStorageID"));
            userAStorageCancelled.emplace_back(
                pb,
                users[0].orders[j].isNoop.packed, 
                state.accountA.storageArray[j - 1].cancelled,
                users[0].orders[j].tradeHistory.getCancelled(),
                FMT(prefix, ".userAStorageCancelled"));
            userAStorageGasFee.emplace_back(
                pb,
                users[0].orders[j].isNoop.packed, 
                state.accountA.storageArray[j - 1].gasFee,
                users[0].orders[j].gasFeeMatch.getFeeSum(),
                FMT(prefix, ".userAStorageGasFee"));
            userAStorageForward.emplace_back(
                pb,
                users[0].orders[j].isNoop.packed, 
                state.accountA.storageArray[j - 1].forward,
                users[0].orders[j].autoMarketOrderCheck.getNewForwardForStorageUpdate(),
                FMT(prefix, ".userAStorageForward"));
        }

        LOG(LogDebug, "in BatchUserGadget before two storage", "");
        for (unsigned int j = 1; j < ORDER_SIZE_USER_B; j++) 
        {
            userBStorageIDBits.emplace_back(pb, state.accountB.storageArray[j - 1].storageID, NUM_BITS_STORAGEID, FMT(prefix, ".userBStorageIDBits"));
            userBStorageAddress.emplace_back(
                pb, 
                users[1].orders[j].isNoop.packed, 
                subArray(userBStorageIDBits.back().bits, 0, NUM_BITS_STORAGE_ADDRESS),
                subArray(users[1].orders[j].order.storageID.bits, 0, NUM_BITS_STORAGE_ADDRESS),
                FMT(prefix, ".userBStorageAddress"));
            
            userBStorageTokenSID.emplace_back(
                pb,
                users[1].orders[j].isNoop.packed, 
                state.accountB.storageArray[j - 1].tokenSID,
                users[1].orders[j].autoMarketOrderCheck.getTokenSIDForStorageUpdate(),
                FMT(prefix, ".userBStorageTokenSID"));
            userBStorageTokenBID.emplace_back(
                pb,
                users[1].orders[j].isNoop.packed, 
                state.accountB.storageArray[j - 1].tokenBID,
                users[1].orders[j].autoMarketOrderCheck.getTokenBIDForStorageUpdate(),
                FMT(prefix, ".userBStorageTokenBID"));
            userBStorageData.emplace_back(
                pb,
                users[1].orders[j].isNoop.packed, 
                state.accountB.storageArray[j - 1].data,
                users[1].orders[j].batchOrderMatching.getFilledAfter(),
                FMT(prefix, ".userBStorageData"));
            userBStorageStorageID.emplace_back(
                pb,
                users[1].orders[j].isNoop.packed, 
                state.accountB.storageArray[j - 1].storageID,
                users[1].orders[j].order.storageID.packed,
                FMT(prefix, ".userBStorageStorageID"));
            userBStorageCancelled.emplace_back(
                pb,
                users[1].orders[j].isNoop.packed, 
                state.accountB.storageArray[j - 1].cancelled,
                users[1].orders[j].tradeHistory.getCancelled(),
                FMT(prefix, ".userBStorageCancelled"));
            userBStorageGasFee.emplace_back(
                pb,
                users[1].orders[j].isNoop.packed, 
                state.accountB.storageArray[j - 1].gasFee,
                users[1].orders[j].gasFeeMatch.getFeeSum(),
                FMT(prefix, ".userBStorageGasFee"));
            userBStorageForward.emplace_back(
                pb,
                users[1].orders[j].isNoop.packed, 
                state.accountB.storageArray[j - 1].forward,
                users[1].orders[j].autoMarketOrderCheck.getNewForwardForStorageUpdate(),
                FMT(prefix, ".userBStorageForward"));
        }
        
        LOG(LogDebug, "in BatchUserGadget before three storage", "");
        for (unsigned int j = 0; j < ORDER_SIZE_USER_C; j++) 
        {
            userCStorageIDBits.emplace_back(pb, state.accountC.storageArray[j].storageID, NUM_BITS_STORAGEID, FMT(prefix, ".userCStorageIDBits"));
            userCStorageAddress.emplace_back(
                pb, 
                users[2].orders[j].isNoop.packed, 
                subArray(userCStorageIDBits.back().bits, 0, NUM_BITS_STORAGE_ADDRESS),
                subArray(users[2].orders[j].order.storageID.bits, 0, NUM_BITS_STORAGE_ADDRESS),
                FMT(prefix, ".userCStorageAddress"));
            
            userCStorageTokenSID.emplace_back(
                pb,
                users[2].orders[j].isNoop.packed, 
                state.accountC.storageArray[j].tokenSID,
                users[2].orders[j].autoMarketOrderCheck.getTokenSIDForStorageUpdate(),
                FMT(prefix, ".userCStorageTokenSID"));
            userCStorageTokenBID.emplace_back(
                pb,
                users[2].orders[j].isNoop.packed, 
                state.accountC.storageArray[j].tokenBID,
                users[2].orders[j].autoMarketOrderCheck.getTokenBIDForStorageUpdate(),
                FMT(prefix, ".userCStorageTokenBID"));
            userCStorageData.emplace_back(
                pb,
                users[2].orders[j].isNoop.packed, 
                state.accountC.storageArray[j].data,
                users[2].orders[j].batchOrderMatching.getFilledAfter(),
                FMT(prefix, ".userCStorageData"));
            userCStorageStorageID.emplace_back(
                pb,
                users[2].orders[j].isNoop.packed, 
                state.accountC.storageArray[j].storageID,
                users[2].orders[j].order.storageID.packed,
                FMT(prefix, ".userCStorageStorageID"));
            userCStorageCancelled.emplace_back(
                pb,
                users[2].orders[j].isNoop.packed, 
                state.accountC.storageArray[j].cancelled,
                users[2].orders[j].tradeHistory.getCancelled(),
                FMT(prefix, ".userCStorageCancelled"));
            userCStorageGasFee.emplace_back(
                pb,
                users[2].orders[j].isNoop.packed, 
                state.accountC.storageArray[j].gasFee,
                users[2].orders[j].gasFeeMatch.getFeeSum(),
                FMT(prefix, ".userCStorageGasFee"));
            userCStorageForward.emplace_back(
                pb,
                users[2].orders[j].isNoop.packed, 
                state.accountC.storageArray[j].forward,
                users[2].orders[j].autoMarketOrderCheck.getNewForwardForStorageUpdate(),
                FMT(prefix, ".userCStorageForward"));
        }
        
        LOG(LogDebug, "in BatchUserGadget before four storage", "");
        for (unsigned int j = 0; j < ORDER_SIZE_USER_D; j++) 
        {
            userDStorageIDBits.emplace_back(pb, state.accountD.storageArray[j].storageID, NUM_BITS_STORAGEID, FMT(prefix, ".userDStorageIDBits"));
            userDStorageAddress.emplace_back(
                pb, 
                users[3].orders[j].isNoop.packed, 
                subArray(userDStorageIDBits.back().bits, 0, NUM_BITS_STORAGE_ADDRESS),
                subArray(users[3].orders[j].order.storageID.bits, 0, NUM_BITS_STORAGE_ADDRESS),
                FMT(prefix, ".userDStorageAddress"));
            
            userDStorageTokenSID.emplace_back(
                pb,
                users[3].orders[j].isNoop.packed, 
                state.accountD.storageArray[j].tokenSID,
                users[3].orders[j].autoMarketOrderCheck.getTokenSIDForStorageUpdate(),
                FMT(prefix, ".userDStorageTokenSID"));
            userDStorageTokenBID.emplace_back(
                pb,
                users[3].orders[j].isNoop.packed, 
                state.accountD.storageArray[j].tokenBID,
                users[3].orders[j].autoMarketOrderCheck.getTokenBIDForStorageUpdate(),
                FMT(prefix, ".userDStorageTokenBID"));
            userDStorageData.emplace_back(
                pb,
                users[3].orders[j].isNoop.packed, 
                state.accountD.storageArray[j].data,
                users[3].orders[j].batchOrderMatching.getFilledAfter(),
                FMT(prefix, ".userDStorageData"));
            userDStorageStorageID.emplace_back(
                pb,
                users[3].orders[j].isNoop.packed, 
                state.accountD.storageArray[j].storageID,
                users[3].orders[j].order.storageID.packed,
                FMT(prefix, ".userDStorageStorageID"));
            userDStorageCancelled.emplace_back(
                pb,
                users[3].orders[j].isNoop.packed, 
                state.accountD.storageArray[j].cancelled,
                users[3].orders[j].tradeHistory.getCancelled(),
                FMT(prefix, ".userDStorageCancelled"));
            userDStorageGasFee.emplace_back(
                pb,
                users[3].orders[j].isNoop.packed, 
                state.accountD.storageArray[j].gasFee,
                users[3].orders[j].gasFeeMatch.getFeeSum(),
                FMT(prefix, ".userDStorageGasFee"));
            userDStorageForward.emplace_back(
                pb,
                users[3].orders[j].isNoop.packed, 
                state.accountD.storageArray[j].forward,
                users[3].orders[j].autoMarketOrderCheck.getNewForwardForStorageUpdate(),
                FMT(prefix, ".userDStorageForward"));
        }
        
        LOG(LogDebug, "in BatchUserGadget before five storage", "");
        for (unsigned int j = 0; j < ORDER_SIZE_USER_E; j++) 
        {
            userEStorageIDBits.emplace_back(pb, state.accountE.storageArray[j].storageID, NUM_BITS_STORAGEID, FMT(prefix, ".userEStorageIDBits"));
            userEStorageAddress.emplace_back(
                pb, 
                users[4].orders[j].isNoop.packed, 
                subArray(userEStorageIDBits.back().bits, 0, NUM_BITS_STORAGE_ADDRESS),
                subArray(users[4].orders[j].order.storageID.bits, 0, NUM_BITS_STORAGE_ADDRESS),
                FMT(prefix, ".userEStorageAddress"));
            
            userEStorageTokenSID.emplace_back(
                pb,
                users[4].orders[j].isNoop.packed, 
                state.accountE.storageArray[j].tokenSID,
                users[4].orders[j].autoMarketOrderCheck.getTokenSIDForStorageUpdate(),
                FMT(prefix, ".userEStorageTokenSID"));
            userEStorageTokenBID.emplace_back(
                pb,
                users[4].orders[j].isNoop.packed, 
                state.accountE.storageArray[j].tokenBID,
                users[4].orders[j].autoMarketOrderCheck.getTokenBIDForStorageUpdate(),
                FMT(prefix, ".userEStorageTokenBID"));
            userEStorageData.emplace_back(
                pb,
                users[4].orders[j].isNoop.packed, 
                state.accountE.storageArray[j].data,
                users[4].orders[j].batchOrderMatching.getFilledAfter(),
                FMT(prefix, ".userEStorageData"));
            userEStorageStorageID.emplace_back(
                pb,
                users[4].orders[j].isNoop.packed, 
                state.accountE.storageArray[j].storageID,
                users[4].orders[j].order.storageID.packed,
                FMT(prefix, ".userEStorageStorageID"));
            userEStorageCancelled.emplace_back(
                pb,
                users[4].orders[j].isNoop.packed, 
                state.accountE.storageArray[j].cancelled,
                users[4].orders[j].tradeHistory.getCancelled(),
                FMT(prefix, ".userEStorageCancelled"));
            userEStorageGasFee.emplace_back(
                pb,
                users[4].orders[j].isNoop.packed, 
                state.accountE.storageArray[j].gasFee,
                users[4].orders[j].gasFeeMatch.getFeeSum(),
                FMT(prefix, ".userEStorageGasFee"));
            userEStorageForward.emplace_back(
                pb,
                users[4].orders[j].isNoop.packed, 
                state.accountE.storageArray[j].forward,
                users[4].orders[j].autoMarketOrderCheck.getNewForwardForStorageUpdate(),
                FMT(prefix, ".userEStorageForward"));
        }
        
        LOG(LogDebug, "in BatchUserGadget before six storage", "");
        for (unsigned int j = 0; j < ORDER_SIZE_USER_F; j++) 
        {
            userFStorageIDBits.emplace_back(pb, state.accountF.storageArray[j].storageID, NUM_BITS_STORAGEID, FMT(prefix, ".userFStorageIDBits"));
            userFStorageAddress.emplace_back(
                pb, 
                users[5].orders[j].isNoop.packed, 
                subArray(userFStorageIDBits.back().bits, 0, NUM_BITS_STORAGE_ADDRESS),
                subArray(users[5].orders[j].order.storageID.bits, 0, NUM_BITS_STORAGE_ADDRESS),
                FMT(prefix, ".userFStorageAddress"));
            
            userFStorageTokenSID.emplace_back(
                pb,
                users[5].orders[j].isNoop.packed, 
                state.accountF.storageArray[j].tokenSID,
                users[5].orders[j].autoMarketOrderCheck.getTokenSIDForStorageUpdate(),
                FMT(prefix, ".userFStorageTokenSID"));
            userFStorageTokenBID.emplace_back(
                pb,
                users[5].orders[j].isNoop.packed, 
                state.accountF.storageArray[j].tokenBID,
                users[5].orders[j].autoMarketOrderCheck.getTokenBIDForStorageUpdate(),
                FMT(prefix, ".userFStorageTokenBID"));
            userFStorageData.emplace_back(
                pb,
                users[5].orders[j].isNoop.packed, 
                state.accountF.storageArray[j].data,
                users[5].orders[j].batchOrderMatching.getFilledAfter(),
                FMT(prefix, ".userFStorageData"));
            userFStorageStorageID.emplace_back(
                pb,
                users[5].orders[j].isNoop.packed, 
                state.accountF.storageArray[j].storageID,
                users[5].orders[j].order.storageID.packed,
                FMT(prefix, ".userFStorageStorageID"));
            userFStorageCancelled.emplace_back(
                pb,
                users[5].orders[j].isNoop.packed, 
                state.accountF.storageArray[j].cancelled,
                users[5].orders[j].tradeHistory.getCancelled(),
                FMT(prefix, ".userFStorageCancelled"));
            userFStorageGasFee.emplace_back(
                pb,
                users[5].orders[j].isNoop.packed, 
                state.accountF.storageArray[j].gasFee,
                users[5].orders[j].gasFeeMatch.getFeeSum(),
                FMT(prefix, ".userFStorageGasFee"));
            userFStorageForward.emplace_back(
                pb,
                users[5].orders[j].isNoop.packed, 
                state.accountF.storageArray[j].forward,
                users[5].orders[j].autoMarketOrderCheck.getNewForwardForStorageUpdate(),
                FMT(prefix, ".userFStorageForward"));
        }
        
        LOG(LogDebug, "in BatchUserGadget before balanceC_O", "");
        balanceC_O.reset(new DynamicBalanceGadget(
            pb, state.oper.balanceC, FMT(prefix, ".balanceC_O")
        ));
        balanceB_O.reset(new DynamicBalanceGadget(
            pb, state.oper.balanceB, FMT(prefix, ".balanceB_O")
        ));
        balanceA_O.reset(new DynamicBalanceGadget(
            pb, state.oper.balanceA, FMT(prefix, ".balanceA_O")
        ));

        LOG(LogDebug, "in BatchUserGadget before balanceC_O_Increase", "");
        balanceC_O_Increase.reset(new BalanceIncreaseGadget(
            pb,
            *balanceC_O,
            firstTokenFeeSum->result(),
            FMT(prefix, ".balanceC_O_Increase")
        ));
        balanceB_O_Increase.reset(new BalanceIncreaseGadget(
            pb,
            *balanceB_O,
            secondTokenFeeSum->result(),
            FMT(prefix, ".balanceB_O_Increase")
        ));
        balanceA_O_Increase.reset(new BalanceIncreaseGadget(
            pb,
            *balanceA_O,
            thirdTokenFeeSum->result(),
            FMT(prefix, ".balanceA_O_Increase")
        ));
        LOG(LogDebug, "in BatchUserGadget before tokenOneMatch", "");
        tokenOneMatch.reset(new RequireEqualGadget(pb, forwardOneAmounts.back().result(), reverseOneAmounts.back().result(), prefix + std::string(".tokenOneMatch")));
        tokenTwoMatch.reset(new RequireEqualGadget(pb, forwardTwoAmounts.back().result(), reverseTwoAmounts.back().result(), prefix + std::string(".tokenTwoMatch")));
        tokenThreeMatch.reset(new RequireEqualGadget(pb, forwardThreeAmounts.back().result(), reverseThreeAmounts.back().result(), prefix + std::string(".tokenThreeMatch")));

        LOG(LogDebug, "in BatchUserGadget before setUserAData", "");
        setUserAData(users[0]);
        setUserBData(users[1]);
        setUserCData(users[2]);
        setUserDData(users[3]);
        setUserEData(users[4]);
        setUserFData(users[5]);
        LOG(LogDebug, "in BatchUserGadget before setOpetratorData", "");
        setOpetratorData(users[0]);

        setSignature();
    }
    void generate_r1cs_witness(const BatchSpotTrade &batchSpotTrade)
    {
        LOG(LogDebug, "in BatchSpotTradeCircuit", "generate_r1cs_witness");
        typeTx.generate_r1cs_witness();
        bindTokenID.generate_r1cs_witness(pb, batchSpotTrade.bindTokenID);
        isBatchSpotTradeTx.generate_r1cs_witness();
        for (size_t i = 0; i < 3; i++) 
        {
            tokensDual[i].generate_r1cs_witness(pb, batchSpotTrade.tokens[i]);
        }
        validTokens->generate_r1cs_witness();

        balanceC_O->generate_r1cs_witness();
        balanceB_O->generate_r1cs_witness();
        balanceA_O->generate_r1cs_witness();

        for (size_t i = 0; i < BATCH_SPOT_TRADE_MAX_USER; i++) 
        {
            LOG(LogDebug, "in BatchSpotTradeCircuit generate_r1cs_witness i:", std::to_string(i));
            users[i].generate_r1cs_witness(batchSpotTrade.users[i]);
            
            forwardOneAmounts[i].generate_r1cs_witness();
            forwardTwoAmounts[i].generate_r1cs_witness();
            forwardThreeAmounts[i].generate_r1cs_witness();

            reverseOneAmounts[i].generate_r1cs_witness();
            reverseTwoAmounts[i].generate_r1cs_witness();
            reverseThreeAmounts[i].generate_r1cs_witness();

            tokenOneFloatForward[i].generate_r1cs_witness();
            tokenTwoFloatForward[i].generate_r1cs_witness();
            tokenThreeFloatForward[i].generate_r1cs_witness();

            tokenOneFloatReverse[i].generate_r1cs_witness();
            tokenTwoFloatReverse[i].generate_r1cs_witness();
            tokenThreeFloatReverse[i].generate_r1cs_witness();
        }

        requireUserAOrderNotNoop->generate_r1cs_witness();
        requireUserBOrderNotNoop->generate_r1cs_witness();
            
        LOG(LogDebug, "in BatchSpotTradeCircuit generate_r1cs_witness before firstTokenFeeSum", "");
        
        firstTokenFeeSum->generate_r1cs_witness();
        secondTokenFeeSum->generate_r1cs_witness();
        thirdTokenFeeSum->generate_r1cs_witness();

        balanceC_O_Increase->generate_r1cs_witness();
        balanceB_O_Increase->generate_r1cs_witness();
        balanceA_O_Increase->generate_r1cs_witness();
        LOG(LogDebug, "in BatchSpotTradeCircuit generate_r1cs_witness before userAStorageIDBits", "");
        for (unsigned int j = 0; j < userAStorageIDBits.size(); j++) 
        {
            userAStorageIDBits[j].generate_r1cs_witness();
            userAStorageAddress[j].generate_r1cs_witness();

            userAStorageTokenSID[j].generate_r1cs_witness();
            userAStorageTokenBID[j].generate_r1cs_witness();
            userAStorageData[j].generate_r1cs_witness();
            userAStorageStorageID[j].generate_r1cs_witness();
            userAStorageCancelled[j].generate_r1cs_witness();
            userAStorageGasFee[j].generate_r1cs_witness();
            userAStorageForward[j].generate_r1cs_witness();
        }

        LOG(LogDebug, "in BatchSpotTradeCircuit generate_r1cs_witness before userBStorageIDBits", "");
        for (unsigned int j = 0; j < userBStorageIDBits.size(); j++) 
        {
            userBStorageIDBits[j].generate_r1cs_witness();
            userBStorageAddress[j].generate_r1cs_witness();

            userBStorageTokenSID[j].generate_r1cs_witness();
            userBStorageTokenBID[j].generate_r1cs_witness();
            userBStorageData[j].generate_r1cs_witness();
            userBStorageStorageID[j].generate_r1cs_witness();
            userBStorageCancelled[j].generate_r1cs_witness();
            userBStorageGasFee[j].generate_r1cs_witness();
            userBStorageForward[j].generate_r1cs_witness();
        }

        LOG(LogDebug, "in BatchSpotTradeCircuit generate_r1cs_witness before userCStorageIDBits", "");
        for (unsigned int j = 0; j < userCStorageIDBits.size(); j++) 
        {
            userCStorageIDBits[j].generate_r1cs_witness();
            userCStorageAddress[j].generate_r1cs_witness();

            userCStorageTokenSID[j].generate_r1cs_witness();
            userCStorageTokenBID[j].generate_r1cs_witness();
            userCStorageData[j].generate_r1cs_witness();
            userCStorageStorageID[j].generate_r1cs_witness();
            userCStorageCancelled[j].generate_r1cs_witness();
            userCStorageGasFee[j].generate_r1cs_witness();
            userCStorageForward[j].generate_r1cs_witness();
        }

        LOG(LogDebug, "in BatchSpotTradeCircuit generate_r1cs_witness before userDStorageIDBits", "");
        for (unsigned int j = 0; j < userDStorageIDBits.size(); j++) 
        {
            userDStorageIDBits[j].generate_r1cs_witness();
            userDStorageAddress[j].generate_r1cs_witness();

            userDStorageTokenSID[j].generate_r1cs_witness();
            userDStorageTokenBID[j].generate_r1cs_witness();
            userDStorageData[j].generate_r1cs_witness();
            userDStorageStorageID[j].generate_r1cs_witness();
            userDStorageCancelled[j].generate_r1cs_witness();
            userDStorageGasFee[j].generate_r1cs_witness();
            userDStorageForward[j].generate_r1cs_witness();
        }

        LOG(LogDebug, "in BatchSpotTradeCircuit generate_r1cs_witness before userEStorageIDBits", "");
        for (unsigned int j = 0; j < userEStorageIDBits.size(); j++) 
        {
            userEStorageIDBits[j].generate_r1cs_witness();
            userEStorageAddress[j].generate_r1cs_witness();

            userEStorageTokenSID[j].generate_r1cs_witness();
            userEStorageTokenBID[j].generate_r1cs_witness();
            userEStorageData[j].generate_r1cs_witness();
            userEStorageStorageID[j].generate_r1cs_witness();
            userEStorageCancelled[j].generate_r1cs_witness();
            userEStorageGasFee[j].generate_r1cs_witness();
            userEStorageForward[j].generate_r1cs_witness();
        }

        LOG(LogDebug, "in BatchSpotTradeCircuit generate_r1cs_witness before userFStorageIDBits", "");
        for (unsigned int j = 0; j < userFStorageIDBits.size(); j++) 
        {
            userFStorageIDBits[j].generate_r1cs_witness();
            userFStorageAddress[j].generate_r1cs_witness();

            userFStorageTokenSID[j].generate_r1cs_witness();
            userFStorageTokenBID[j].generate_r1cs_witness();
            userFStorageData[j].generate_r1cs_witness();
            userFStorageStorageID[j].generate_r1cs_witness();
            userFStorageCancelled[j].generate_r1cs_witness();
            userFStorageGasFee[j].generate_r1cs_witness();
            userFStorageForward[j].generate_r1cs_witness();
        }
        LOG(LogDebug, "in BatchSpotTradeCircuit generate_r1cs_witness before tokenOneMatch", "");
        tokenOneMatch->generate_r1cs_witness();
        tokenTwoMatch->generate_r1cs_witness();
        tokenThreeMatch->generate_r1cs_witness();
    }
    void generate_r1cs_constraints()
    {
        LOG(LogDebug, "in BatchSpotTradeCircuit", "generate_r1cs_constraints");
        typeTx.generate_r1cs_constraints();
        bindTokenID.generate_r1cs_constraints(true);
        isBatchSpotTradeTx.generate_r1cs_constraints();
        for (size_t i = 0; i < 3; i++) 
        {
            tokensDual[i].generate_r1cs_constraints();
        }
        validTokens->generate_r1cs_constraints();

        balanceC_O->generate_r1cs_constraints();
        balanceB_O->generate_r1cs_constraints();
        balanceA_O->generate_r1cs_constraints();
        
        for (unsigned int i = 0; i < BATCH_SPOT_TRADE_MAX_USER; i++) 
        {
            LOG(LogDebug, "in BatchSpotTradeCircuit generate_r1cs_constraints: i:", std::to_string(i));
            users[i].generate_r1cs_constraints();

            forwardOneAmounts[i].generate_r1cs_constraints();
            forwardTwoAmounts[i].generate_r1cs_constraints();
            forwardThreeAmounts[i].generate_r1cs_constraints();

            reverseOneAmounts[i].generate_r1cs_constraints();
            reverseTwoAmounts[i].generate_r1cs_constraints();
            reverseThreeAmounts[i].generate_r1cs_constraints();

            tokenOneFloatForward[i].generate_r1cs_constraints();
            tokenTwoFloatForward[i].generate_r1cs_constraints();
            tokenThreeFloatForward[i].generate_r1cs_constraints();

            tokenOneFloatReverse[i].generate_r1cs_constraints();
            tokenTwoFloatReverse[i].generate_r1cs_constraints();
            tokenThreeFloatReverse[i].generate_r1cs_constraints();
        }

        requireUserAOrderNotNoop->generate_r1cs_constraints();
        requireUserBOrderNotNoop->generate_r1cs_constraints();

        firstTokenFeeSum->generate_r1cs_constraints();
        secondTokenFeeSum->generate_r1cs_constraints();
        thirdTokenFeeSum->generate_r1cs_constraints();
        
        balanceC_O_Increase->generate_r1cs_constraints();
        balanceB_O_Increase->generate_r1cs_constraints();
        balanceA_O_Increase->generate_r1cs_constraints();

        for (unsigned int j = 0; j < userAStorageIDBits.size(); j++) 
        {
            userAStorageIDBits[j].generate_r1cs_constraints();
            userAStorageAddress[j].generate_r1cs_constraints();

            userAStorageTokenSID[j].generate_r1cs_constraints();
            userAStorageTokenBID[j].generate_r1cs_constraints();
            userAStorageData[j].generate_r1cs_constraints();
            userAStorageStorageID[j].generate_r1cs_constraints();
            userAStorageCancelled[j].generate_r1cs_constraints();
            userAStorageGasFee[j].generate_r1cs_constraints();
            userAStorageForward[j].generate_r1cs_constraints();
        }

        for (unsigned int j = 0; j < userBStorageIDBits.size(); j++) 
        {
            userBStorageIDBits[j].generate_r1cs_constraints();
            userBStorageAddress[j].generate_r1cs_constraints();

            userBStorageTokenSID[j].generate_r1cs_constraints();
            userBStorageTokenBID[j].generate_r1cs_constraints();
            userBStorageData[j].generate_r1cs_constraints();
            userBStorageStorageID[j].generate_r1cs_constraints();
            userBStorageCancelled[j].generate_r1cs_constraints();
            userBStorageGasFee[j].generate_r1cs_constraints();
            userBStorageForward[j].generate_r1cs_constraints();
        }

        for (unsigned int j = 0; j < userCStorageIDBits.size(); j++) 
        {
            userCStorageIDBits[j].generate_r1cs_constraints();
            userCStorageAddress[j].generate_r1cs_constraints();

            userCStorageTokenSID[j].generate_r1cs_constraints();
            userCStorageTokenBID[j].generate_r1cs_constraints();
            userCStorageData[j].generate_r1cs_constraints();
            userCStorageStorageID[j].generate_r1cs_constraints();
            userCStorageCancelled[j].generate_r1cs_constraints();
            userCStorageGasFee[j].generate_r1cs_constraints();
            userCStorageForward[j].generate_r1cs_constraints();
        }

        for (unsigned int j = 0; j < userDStorageIDBits.size(); j++) 
        {
            userDStorageIDBits[j].generate_r1cs_constraints();
            userDStorageAddress[j].generate_r1cs_constraints();

            userDStorageTokenSID[j].generate_r1cs_constraints();
            userDStorageTokenBID[j].generate_r1cs_constraints();
            userDStorageData[j].generate_r1cs_constraints();
            userDStorageStorageID[j].generate_r1cs_constraints();
            userDStorageCancelled[j].generate_r1cs_constraints();
            userDStorageGasFee[j].generate_r1cs_constraints();
            userDStorageForward[j].generate_r1cs_constraints();
        }

        for (unsigned int j = 0; j < userEStorageIDBits.size(); j++) 
        {
            userEStorageIDBits[j].generate_r1cs_constraints();
            userEStorageAddress[j].generate_r1cs_constraints();

            userEStorageTokenSID[j].generate_r1cs_constraints();
            userEStorageTokenBID[j].generate_r1cs_constraints();
            userEStorageData[j].generate_r1cs_constraints();
            userEStorageStorageID[j].generate_r1cs_constraints();
            userEStorageCancelled[j].generate_r1cs_constraints();
            userEStorageGasFee[j].generate_r1cs_constraints();
            userEStorageForward[j].generate_r1cs_constraints();
        }

        for (unsigned int j = 0; j < userFStorageIDBits.size(); j++) 
        {
            userFStorageIDBits[j].generate_r1cs_constraints();
            userFStorageAddress[j].generate_r1cs_constraints();

            userFStorageTokenSID[j].generate_r1cs_constraints();
            userFStorageTokenBID[j].generate_r1cs_constraints();
            userFStorageData[j].generate_r1cs_constraints();
            userFStorageStorageID[j].generate_r1cs_constraints();
            userFStorageCancelled[j].generate_r1cs_constraints();
            userFStorageGasFee[j].generate_r1cs_constraints();
            userFStorageForward[j].generate_r1cs_constraints();
        }
        tokenOneMatch->generate_r1cs_constraints();
        tokenTwoMatch->generate_r1cs_constraints();
        tokenThreeMatch->generate_r1cs_constraints();
    }

    void setUserAData(BatchUserGadget &user) 
    {
        LOG(LogDebug, "in BatchSpotTradeCircuit", "setUserAData");
        // Set tokens
        setArrayOutput(TXV_BALANCE_A_S_ADDRESS, user.firstToken.bits);
        setArrayOutput(TXV_BALANCE_A_B_ADDRESS, user.secondToken.bits);

        // Update account A
        setArrayOutput(TXV_STORAGE_A_ADDRESS, subArray(user.orders[0].order.storageID.bits, 0, NUM_BITS_STORAGE_ADDRESS));
        setOutput(TXV_STORAGE_A_TOKENSID, user.orders[0].autoMarketOrderCheck.getTokenSIDForStorageUpdate());
        setOutput(TXV_STORAGE_A_TOKENBID, user.orders[0].autoMarketOrderCheck.getTokenBIDForStorageUpdate());
        setOutput(TXV_STORAGE_A_DATA, user.orders[0].batchOrderMatching.getFilledAfter());
        setOutput(TXV_STORAGE_A_STORAGEID, user.orders[0].order.storageID.packed);
        setOutput(TXV_STORAGE_A_CANCELLED, user.orders[0].tradeHistory.getCancelled());
        setOutput(TXV_STORAGE_A_GASFEE, user.orders[0].gasFeeMatch.getFeeSum());
        setOutput(TXV_STORAGE_A_FORWARD, user.orders[0].autoMarketOrderCheck.getNewForwardForStorageUpdate());

        setArrayOutput(TXV_STORAGE_A_ADDRESS_ARRAY_0, userAStorageAddress[0].result());
        setOutput(TXV_STORAGE_A_TOKENSID_ARRAY_0, userAStorageTokenSID[0].result());
        setOutput(TXV_STORAGE_A_TOKENBID_ARRAY_0, userAStorageTokenBID[0].result());
        setOutput(TXV_STORAGE_A_DATA_ARRAY_0, userAStorageData[0].result());
        setOutput(TXV_STORAGE_A_STORAGEID_ARRAY_0, userAStorageStorageID[0].result());
        setOutput(TXV_STORAGE_A_CANCELLED_ARRAY_0, userAStorageCancelled[0].result());
        setOutput(TXV_STORAGE_A_GASFEE_ARRAY_0, userAStorageGasFee[0].result());
        setOutput(TXV_STORAGE_A_FORWARD_ARRAY_0, userAStorageForward[0].result());

        setArrayOutput(TXV_STORAGE_A_ADDRESS_ARRAY_1, userAStorageAddress[1].result());
        setOutput(TXV_STORAGE_A_TOKENSID_ARRAY_1, userAStorageTokenSID[1].result());
        setOutput(TXV_STORAGE_A_TOKENBID_ARRAY_1, userAStorageTokenBID[1].result());
        setOutput(TXV_STORAGE_A_DATA_ARRAY_1, userAStorageData[1].result());
        setOutput(TXV_STORAGE_A_STORAGEID_ARRAY_1, userAStorageStorageID[1].result());
        setOutput(TXV_STORAGE_A_CANCELLED_ARRAY_1, userAStorageCancelled[1].result());
        setOutput(TXV_STORAGE_A_GASFEE_ARRAY_1, userAStorageGasFee[1].result());
        setOutput(TXV_STORAGE_A_FORWARD_ARRAY_1, userAStorageForward[1].result());

        setArrayOutput(TXV_STORAGE_A_ADDRESS_ARRAY_2, userAStorageAddress[2].result());
        setOutput(TXV_STORAGE_A_TOKENSID_ARRAY_2, userAStorageTokenSID[2].result());
        setOutput(TXV_STORAGE_A_TOKENBID_ARRAY_2, userAStorageTokenBID[2].result());
        setOutput(TXV_STORAGE_A_DATA_ARRAY_2, userAStorageData[2].result());
        setOutput(TXV_STORAGE_A_STORAGEID_ARRAY_2, userAStorageStorageID[2].result());
        setOutput(TXV_STORAGE_A_CANCELLED_ARRAY_2, userAStorageCancelled[2].result());
        setOutput(TXV_STORAGE_A_GASFEE_ARRAY_2, userAStorageGasFee[2].result());
        setOutput(TXV_STORAGE_A_FORWARD_ARRAY_2, userAStorageForward[2].result());

        setOutput(TXV_BALANCE_A_S_BALANCE, user.balanceOneBefore->balance());
        setOutput(TXV_BALANCE_A_B_BALANCE, user.balanceTwoBefore->balance());

        setArrayOutput(TXV_BALANCE_A_FEE_Address, user.thirdToken.bits);
        setOutput(TXV_BALANCE_A_FEE_BALANCE, user.balanceThreeBefore->balance());

        setArrayOutput(TXV_ACCOUNT_A_ADDRESS, user.accountID.bits);
    }

    void setUserBData(BatchUserGadget &user) 
    {
        LOG(LogDebug, "in BatchSpotTradeCircuit", "setUserBData");

        setArrayOutput(TXV_BALANCE_B_S_ADDRESS, user.firstToken.bits);
        setArrayOutput(TXV_BALANCE_B_B_ADDRESS, user.secondToken.bits);
        // Update account B
        setArrayOutput(TXV_STORAGE_B_ADDRESS, subArray(user.orders[0].order.storageID.bits, 0, NUM_BITS_STORAGE_ADDRESS));
        setOutput(TXV_STORAGE_B_TOKENSID, user.orders[0].autoMarketOrderCheck.getTokenSIDForStorageUpdate());
        setOutput(TXV_STORAGE_B_TOKENBID, user.orders[0].autoMarketOrderCheck.getTokenBIDForStorageUpdate());
        setOutput(TXV_STORAGE_B_DATA, user.orders[0].batchOrderMatching.getFilledAfter());
        setOutput(TXV_STORAGE_B_STORAGEID, user.orders[0].order.storageID.packed);
        setOutput(TXV_STORAGE_B_CANCELLED, user.orders[0].tradeHistory.getCancelled());
        setOutput(TXV_STORAGE_B_GASFEE, user.orders[0].gasFeeMatch.getFeeSum());
        setOutput(TXV_STORAGE_B_FORWARD, user.orders[0].autoMarketOrderCheck.getNewForwardForStorageUpdate());

        setArrayOutput(TXV_STORAGE_B_ADDRESS_ARRAY_0, userBStorageAddress[0].result());
        setOutput(TXV_STORAGE_B_TOKENSID_ARRAY_0, userBStorageTokenSID[0].result());
        setOutput(TXV_STORAGE_B_TOKENBID_ARRAY_0, userBStorageTokenBID[0].result());
        setOutput(TXV_STORAGE_B_DATA_ARRAY_0, userBStorageData[0].result());
        setOutput(TXV_STORAGE_B_STORAGEID_ARRAY_0, userBStorageStorageID[0].result());
        setOutput(TXV_STORAGE_B_CANCELLED_ARRAY_0, userBStorageCancelled[0].result());
        setOutput(TXV_STORAGE_B_GASFEE_ARRAY_0, userBStorageGasFee[0].result());
        setOutput(TXV_STORAGE_B_FORWARD_ARRAY_0, userBStorageForward[0].result());

        setOutput(TXV_BALANCE_B_S_BALANCE, user.balanceOneBefore->balance());
        setOutput(TXV_BALANCE_B_B_BALANCE, user.balanceTwoBefore->balance());

        setArrayOutput(TXV_BALANCE_B_FEE_Address, user.thirdToken.bits);
        setOutput(TXV_BALANCE_B_FEE_BALANCE, user.balanceThreeBefore->balance());

        setArrayOutput(TXV_ACCOUNT_B_ADDRESS, user.accountID.bits);
    }
    void setUserCData(BatchUserGadget &user) 
    {
        LOG(LogDebug, "in BatchSpotTradeCircuit", "setUserCData");
        setArrayOutput(TXV_BALANCE_C_S_ADDRESS, user.firstToken.bits);
        setArrayOutput(TXV_BALANCE_C_B_ADDRESS, user.secondToken.bits);

        setArrayOutput(TXV_STORAGE_C_ADDRESS_ARRAY_0, userCStorageAddress[0].result());
        setOutput(TXV_STORAGE_C_TOKENSID_ARRAY_0, userCStorageTokenSID[0].result());
        setOutput(TXV_STORAGE_C_TOKENBID_ARRAY_0, userCStorageTokenBID[0].result());
        setOutput(TXV_STORAGE_C_DATA_ARRAY_0, userCStorageData[0].result());
        setOutput(TXV_STORAGE_C_STORAGEID_ARRAY_0, userCStorageStorageID[0].result());
        setOutput(TXV_STORAGE_C_CANCELLED_ARRAY_0, userCStorageCancelled[0].result());
        setOutput(TXV_STORAGE_C_GASFEE_ARRAY_0, userCStorageGasFee[0].result());
        setOutput(TXV_STORAGE_C_FORWARD_ARRAY_0, userCStorageForward[0].result());

        setOutput(TXV_BALANCE_C_S_BALANCE, user.balanceOneBefore->balance());
        setOutput(TXV_BALANCE_C_B_BALANCE, user.balanceTwoBefore->balance());

        setArrayOutput(TXV_BALANCE_C_FEE_Address, user.thirdToken.bits);
        setOutput(TXV_BALANCE_C_FEE_BALANCE, user.balanceThreeBefore->balance());

        setArrayOutput(TXV_ACCOUNT_C_ADDRESS, user.accountID.bits);
    }
    void setUserDData(BatchUserGadget &user) 
    {
        LOG(LogDebug, "in BatchSpotTradeCircuit", "setUserDData");
        setArrayOutput(TXV_BALANCE_D_S_ADDRESS, user.firstToken.bits);
        setArrayOutput(TXV_BALANCE_D_B_ADDRESS, user.secondToken.bits);

        setArrayOutput(TXV_STORAGE_D_ADDRESS_ARRAY_0, userDStorageAddress[0].result());
        setOutput(TXV_STORAGE_D_TOKENSID_ARRAY_0, userDStorageTokenSID[0].result());
        setOutput(TXV_STORAGE_D_TOKENBID_ARRAY_0, userDStorageTokenBID[0].result());
        setOutput(TXV_STORAGE_D_DATA_ARRAY_0, userDStorageData[0].result());
        setOutput(TXV_STORAGE_D_STORAGEID_ARRAY_0, userDStorageStorageID[0].result());
        setOutput(TXV_STORAGE_D_CANCELLED_ARRAY_0, userDStorageCancelled[0].result());
        setOutput(TXV_STORAGE_D_GASFEE_ARRAY_0, userDStorageGasFee[0].result());
        setOutput(TXV_STORAGE_D_FORWARD_ARRAY_0, userDStorageForward[0].result());

        setOutput(TXV_BALANCE_D_S_BALANCE, user.balanceOneBefore->balance());
        setOutput(TXV_BALANCE_D_B_BALANCE, user.balanceTwoBefore->balance());

        setArrayOutput(TXV_BALANCE_D_FEE_Address, user.thirdToken.bits);
        setOutput(TXV_BALANCE_D_FEE_BALANCE, user.balanceThreeBefore->balance());

        setArrayOutput(TXV_ACCOUNT_D_ADDRESS, user.accountID.bits);
    }
    void setUserEData(BatchUserGadget &user) 
    {
        LOG(LogDebug, "in BatchSpotTradeCircuit", "setUserEData");
        setArrayOutput(TXV_BALANCE_E_S_ADDRESS, user.firstToken.bits);
        setArrayOutput(TXV_BALANCE_E_B_ADDRESS, user.secondToken.bits);

        setArrayOutput(TXV_STORAGE_E_ADDRESS_ARRAY_0, userEStorageAddress[0].result());
        setOutput(TXV_STORAGE_E_TOKENSID_ARRAY_0, userEStorageTokenSID[0].result());
        setOutput(TXV_STORAGE_E_TOKENBID_ARRAY_0, userEStorageTokenBID[0].result());
        setOutput(TXV_STORAGE_E_DATA_ARRAY_0, userEStorageData[0].result());
        setOutput(TXV_STORAGE_E_STORAGEID_ARRAY_0, userEStorageStorageID[0].result());
        setOutput(TXV_STORAGE_E_CANCELLED_ARRAY_0, userEStorageCancelled[0].result());
        setOutput(TXV_STORAGE_E_GASFEE_ARRAY_0, userEStorageGasFee[0].result());
        setOutput(TXV_STORAGE_E_FORWARD_ARRAY_0, userEStorageForward[0].result());

        setOutput(TXV_BALANCE_E_S_BALANCE, user.balanceOneBefore->balance());
        setOutput(TXV_BALANCE_E_B_BALANCE, user.balanceTwoBefore->balance());

        setArrayOutput(TXV_BALANCE_E_FEE_Address, user.thirdToken.bits);
        setOutput(TXV_BALANCE_E_FEE_BALANCE, user.balanceThreeBefore->balance());

        setArrayOutput(TXV_ACCOUNT_E_ADDRESS, user.accountID.bits);
    }
    void setUserFData(BatchUserGadget &user) 
    {
        LOG(LogDebug, "in BatchSpotTradeCircuit", "setUserFData");
        setArrayOutput(TXV_BALANCE_F_S_ADDRESS, user.firstToken.bits);
        setArrayOutput(TXV_BALANCE_F_B_ADDRESS, user.secondToken.bits);

        setArrayOutput(TXV_STORAGE_F_ADDRESS_ARRAY_0, userFStorageAddress[0].result());
        setOutput(TXV_STORAGE_F_TOKENSID_ARRAY_0, userFStorageTokenSID[0].result());
        setOutput(TXV_STORAGE_F_TOKENBID_ARRAY_0, userFStorageTokenBID[0].result());
        setOutput(TXV_STORAGE_F_DATA_ARRAY_0, userFStorageData[0].result());
        setOutput(TXV_STORAGE_F_STORAGEID_ARRAY_0, userFStorageStorageID[0].result());
        setOutput(TXV_STORAGE_F_CANCELLED_ARRAY_0, userFStorageCancelled[0].result());
        setOutput(TXV_STORAGE_F_GASFEE_ARRAY_0, userFStorageGasFee[0].result());
        setOutput(TXV_STORAGE_F_FORWARD_ARRAY_0, userFStorageForward[0].result());

        setOutput(TXV_BALANCE_F_S_BALANCE, user.balanceOneBefore->balance());
        setOutput(TXV_BALANCE_F_B_BALANCE, user.balanceTwoBefore->balance());

        setArrayOutput(TXV_BALANCE_F_FEE_Address, user.thirdToken.bits);
        setOutput(TXV_BALANCE_F_FEE_BALANCE, user.balanceThreeBefore->balance());

        setArrayOutput(TXV_ACCOUNT_F_ADDRESS, user.accountID.bits);
    }
    void setOpetratorData(BatchUserGadget &user) 
    {
        setArrayOutput(TXV_BALANCE_O_C_Address, user.firstToken.bits);
        setArrayOutput(TXV_BALANCE_O_B_Address, user.secondToken.bits);
        setArrayOutput(TXV_BALANCE_O_A_Address, user.thirdToken.bits);

        setOutput(TXV_BALANCE_O_C_BALANCE, balanceC_O->balance());
        setOutput(TXV_BALANCE_O_B_BALANCE, balanceB_O->balance());
        setOutput(TXV_BALANCE_O_A_BALANCE, balanceA_O->balance());
        
    }

    void setSignature() {
        LOG(LogDebug, "in BatchSpotTradeCircuit", "setSignature");
        setOutput(TXV_HASH_A, users[0].hashArray[0]);
        setOutput(TXV_HASH_B, users[1].hashArray[0]);
        setArrayOutput(TXV_HASH_A_ARRAY, subArray(users[0].hashArray, 1, ORDER_SIZE_USER_A - 1));
        setArrayOutput(TXV_HASH_B_ARRAY, subArray(users[1].hashArray, 1, ORDER_SIZE_USER_B - 1));
        setArrayOutput(TXV_HASH_C_ARRAY, users[2].hashArray);
        setArrayOutput(TXV_HASH_D_ARRAY, users[3].hashArray);
        setArrayOutput(TXV_HASH_E_ARRAY, users[4].hashArray);
        setArrayOutput(TXV_HASH_F_ARRAY, users[5].hashArray);

        setOutput(TXV_PUBKEY_X_A, users[0].publicXArray[0]);
        setOutput(TXV_PUBKEY_Y_A, users[0].publicYArray[0]);
        setArrayOutput(TXV_PUBKEY_X_A_ARRAY, subArray(users[0].publicXArray, 1, ORDER_SIZE_USER_A - 1));
        setArrayOutput(TXV_PUBKEY_Y_A_ARRAY, subArray(users[0].publicYArray, 1, ORDER_SIZE_USER_A - 1));

        setOutput(TXV_PUBKEY_X_B, users[1].publicXArray[0]);
        setOutput(TXV_PUBKEY_Y_B, users[1].publicYArray[0]);
        setArrayOutput(TXV_PUBKEY_X_B_ARRAY, subArray(users[1].publicXArray, 1, ORDER_SIZE_USER_B - 1));
        setArrayOutput(TXV_PUBKEY_Y_B_ARRAY, subArray(users[1].publicYArray, 1, ORDER_SIZE_USER_B - 1));

        setArrayOutput(TXV_PUBKEY_X_C_ARRAY, users[2].publicXArray);
        setArrayOutput(TXV_PUBKEY_Y_C_ARRAY, users[2].publicYArray);

        setArrayOutput(TXV_PUBKEY_X_D_ARRAY, users[3].publicXArray);
        setArrayOutput(TXV_PUBKEY_Y_D_ARRAY, users[3].publicYArray);

        setArrayOutput(TXV_PUBKEY_X_E_ARRAY, users[4].publicXArray);
        setArrayOutput(TXV_PUBKEY_Y_E_ARRAY, users[4].publicYArray);

        setArrayOutput(TXV_PUBKEY_X_F_ARRAY, users[5].publicXArray);
        setArrayOutput(TXV_PUBKEY_Y_F_ARRAY, users[5].publicYArray);

        setOutput(TXV_SIGNATURE_REQUIRED_A, users[0].requireSignatureArray[0]);
        setOutput(TXV_SIGNATURE_REQUIRED_B, users[1].requireSignatureArray[0]);
        setArrayOutput(TXV_SIGNATURE_REQUIRED_A_ARRAY, subArray(users[0].requireSignatureArray, 1, ORDER_SIZE_USER_A - 1));
        setArrayOutput(TXV_SIGNATURE_REQUIRED_B_ARRAY, subArray(users[1].requireSignatureArray, 1, ORDER_SIZE_USER_B - 1));
        setArrayOutput(TXV_SIGNATURE_REQUIRED_C_ARRAY, users[2].requireSignatureArray);
        setArrayOutput(TXV_SIGNATURE_REQUIRED_D_ARRAY, users[3].requireSignatureArray);
        setArrayOutput(TXV_SIGNATURE_REQUIRED_E_ARRAY, users[4].requireSignatureArray);
        setArrayOutput(TXV_SIGNATURE_REQUIRED_F_ARRAY, users[5].requireSignatureArray);
    }

    const VariableArrayT getPublicData() const
    {
        VariableArrayT batchSpotTradeData = flattenReverse({
            // 3bits
            typeTx.bits,
            // 5bits
            bindTokenID.bits,

            tokensDual[0].bits,
            tokensDual[1].bits,

            // each user token type uses 2bits. as UserA contains three tokens, there is no need for tokenType
            users[1].getBatchSpotTradeTokenType().bits,
            users[2].getBatchSpotTradeTokenType().bits,
            users[3].getBatchSpotTradeTokenType().bits,
            users[4].getBatchSpotTradeTokenType().bits,
            users[5].getBatchSpotTradeTokenType().bits,

            users[1].accountID.bits,
            users[1].getFirstTokenAmountExchange(),
            users[1].getSecondTokenAmountExchange(),

            users[2].accountID.bits,
            users[2].getFirstTokenAmountExchange(),
            users[2].getSecondTokenAmountExchange(),

            users[3].accountID.bits,
            users[3].getFirstTokenAmountExchange(),
            users[3].getSecondTokenAmountExchange(),

            users[4].accountID.bits,
            users[4].getFirstTokenAmountExchange(),
            users[4].getSecondTokenAmountExchange(),

            users[5].accountID.bits,
            users[5].getFirstTokenAmountExchange(),
            users[5].getSecondTokenAmountExchange(),

            users[0].accountID.bits,
            users[0].balanceOneDif->getBalanceDifFloatBits(),
            users[0].balanceTwoDif->getBalanceDifFloatBits(),
            users[0].balanceThreeDif->getBalanceDifFloatBits(),

        });


        return batchSpotTradeData;
    }
};

} // namespace Loopring

#endif
