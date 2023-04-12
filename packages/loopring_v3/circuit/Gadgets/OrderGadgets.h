// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
#ifndef _ORDERGADGETS_H_
#define _ORDERGADGETS_H_

#include "../Utils/Constants.h"
#include "../Utils/Data.h"
#include "StorageGadgets.h"
#include "AccountGadgets.h"

#include "ethsnarks.hpp"
#include "gadgets/poseidon.hpp"
#include "utils.hpp"

using namespace ethsnarks;

namespace Loopring
{

class OrderGadget : public GadgetT
{
  public:
    // Inputs
    DualVariableGadget storageID;
    DualVariableGadget accountID;
    DualVariableGadget tokenS;
    DualVariableGadget tokenB;
    DualVariableGadget amountS;
    DualVariableGadget amountB;
    // split trading fee and gas fee
    DualVariableGadget feeTokenID;
    DualVariableGadget fee;
    DualVariableGadget maxFee;
    DualVariableGadget useAppKey;
    ToBitsGadget disableAppKey;

    DualVariableGadget validUntil;
    // split TradingFee and GasFee - ProtocolFeeBips as the max TradingFee
    // DualVariableGadget maxFeeBips;
    DualVariableGadget fillAmountBorS;

    // split trading fee and gas fee
    FloatGadget fFee;
    RequireAccuracyGadget requireAccuracyFee;

    VariableT taker;

    DualVariableGadget feeBips;

    DualVariableGadget feeBipsMultiplierFlag;
    DualVariableGadget feeBipsData;

    // DualVariableGadget deltaFilledS;
    // DualVariableGadget deltaFilledB;
    // DualVariableGadget isNoop;

    // Checks
    RequireLeqGadget feeBips_leq_maxFeeBips;
    // split trading fee and gas fee
    RequireLeqGadget fee_leq_maxFee;
    // RequireNotEqualGadget tokenS_neq_tokenB;
    IfThenRequireNotEqualGadget tokenS_neq_tokenB;
    IfThenRequireNotEqualGadget amountS_notZero;
    IfThenRequireNotEqualGadget amountB_notZero;
    // Fee checks
    TernaryGadget feeMultiplier;
    UnsafeMulGadget decodedFeeBips;
    RequireEqualGadget feeBipsEqualsDecodedFeeBips;
    
    // Poseidon_15 hash;
    // DEG-265 auto market
    // AutoMarket
    DualVariableGadget type;
    // DualVariableGadget autoMarketID;
    DualVariableGadget level;
    DualVariableGadget gridOffset;
    DualVariableGadget orderOffset;
    DualVariableGadget maxLevel;

    // check appKey author
    IfThenRequireEqualGadget ifUseAppKey_then_require_enable_switch;

    // Signature
    Poseidon_17 hash;

    OrderGadget( //
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &blockExchange,
      // split TradingFee and GasFee - ProtocolFeeBips as the max TradingFee
      const VariableT &maxFeeBips,
      const VariableT &isNormalOrder,
      const VariableT &_disableAppKey,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          // Inputs
          storageID(pb, NUM_BITS_STORAGEID, FMT(prefix, ".storageID")),
          accountID(pb, NUM_BITS_ACCOUNT, FMT(prefix, ".accountID")),
          tokenS(pb, NUM_BITS_TOKEN, FMT(prefix, ".tokenS")),
          tokenB(pb, NUM_BITS_TOKEN, FMT(prefix, ".tokenB")),
          amountS(pb, NUM_BITS_AMOUNT, FMT(prefix, ".amountS")),
          amountB(pb, NUM_BITS_AMOUNT, FMT(prefix, ".amountB")),
          validUntil(pb, NUM_BITS_TIMESTAMP, FMT(prefix, ".validUntil")),
          // split TradingFee and GasFee - ProtocolFeeBips as the max TradingFee
          // maxFeeBips(pb, NUM_BITS_BIPS, FMT(prefix, ".maxFeeBips")),
          // split trading fee and gas fee
          feeTokenID(pb, NUM_BITS_TOKEN, FMT(prefix, ".feeTokenID")),
          fee(pb, NUM_BITS_AMOUNT, FMT(prefix, ".fee")),
          maxFee(pb, NUM_BITS_AMOUNT, FMT(prefix, ".maxFee")),
          useAppKey(pb, NUM_BITS_BYTE, FMT(prefix, ".useAppKey")),
          disableAppKey(pb, _disableAppKey, 1, FMT(prefix, ".disableAppKey")),
          fillAmountBorS(pb, 1, FMT(prefix, ".fillAmountBorS")),
          taker(make_variable(pb, FMT(prefix, ".taker"))),

          feeBips(pb, NUM_BITS_BIPS, FMT(prefix, ".feeBips")),

          feeBipsMultiplierFlag(pb, 1, FMT(prefix, ".feeBipsMultiplierFlag")),
          feeBipsData(pb, NUM_BITS_BIPS_DA, FMT(prefix, ".feeBipsData")),
          
          // deltaFilledS(pb, NUM_BITS_AMOUNT, FMT(prefix, ".deltaFilledS")),
          // deltaFilledB(pb, NUM_BITS_AMOUNT, FMT(prefix, ".deltaFilledB")),
          // isNoop(pb, NUM_BITS_BIT, FMT(prefix, ".isNoop")),

          // AutoMarket
          type(pb, NUM_BITS_TYPE, FMT(prefix, ".type")),
          // autoMarketID(pb, NUM_BITS_AUTOMARKETID, FMT(prefix, ".autoMarketID")),
          // autoMarketLevel(pb, NUM_BITS_TYPE, FMT(prefix, ".autoMarketLevel")),
          level(pb, NUM_BITS_AUTOMARKET_LEVEL, FMT(prefix, ".level")),
          gridOffset(pb, NUM_BITS_AMOUNT, FMT(prefix, ".gridOffset")),
          orderOffset(pb, NUM_BITS_AMOUNT, FMT(prefix, ".orderOffset")),
          maxLevel(pb, NUM_BITS_AUTOMARKET_LEVEL, FMT(prefix, ".maxLevel")),

          // Checks
          feeBips_leq_maxFeeBips(
            pb,
            feeBips.packed,
            // split TradingFee and GasFee - ProtocolFeeBips as the max TradingFee
            maxFeeBips,
            NUM_BITS_BIPS,
            FMT(prefix, ".feeBips <= maxFeeBips")),
          // TODO need check logic, batch spot trade cancel this condition
          tokenS_neq_tokenB(pb, isNormalOrder, tokenS.packed, tokenB.packed, FMT(prefix, ".tokenS != tokenB")),
          // TODO need check logic, can not be zero change to not equal 0
          amountS_notZero(pb, isNormalOrder, amountS.packed, constants._0, FMT(prefix, ".amountS != 0")),
          amountB_notZero(pb, isNormalOrder, amountB.packed, constants._0, FMT(prefix, ".amountB != 0")),
          // Fee checks
          // If feebips exceeds 64, the specific value of feemultiplier = 50, otherwise it is 1
          feeMultiplier(
            pb,
            feeBipsMultiplierFlag.packed,
            constants.feeMultiplier,
            constants._1,
            FMT(prefix, ".feeMultiplier")),
          // Feebipsdata is multiplied by feemultipler. If feebips > = 64, then feebipsdata = feebips / 50
          // Only the original feebips value is restored here
          decodedFeeBips(pb, feeBipsData.packed, feeMultiplier.result(), FMT(prefix, ".decodedFeeBips")),
          // There is a function implied here. If feebips needs to be modified, it must be a multiple of 50, 
          //   otherwise the circuit verification will not pass
          feeBipsEqualsDecodedFeeBips(
            pb,
            feeBips.packed,
            decodedFeeBips.result(),
            FMT(prefix, ".feeBipsEqualsDecodedFeeBips")),

          // split trading fee and gas fee
          // 1: fee <= maxFee
          fee_leq_maxFee(pb, fee.packed, maxFee.packed, NUM_BITS_AMOUNT, FMT(prefix, ".fee <= maxFee")),
          fFee(pb, constants, Float16Encoding, FMT(prefix, ".fFee")),
          requireAccuracyFee(
            pb,
            fFee.value(),
            fee.packed,
            Float16Accuracy,
            NUM_BITS_AMOUNT,
            FMT(prefix, ".requireAccuracyFee")),

          ifUseAppKey_then_require_enable_switch(
            pb,
            useAppKey.packed,
            disableAppKey.packed,
            constants._0,
            FMT(prefix, ".ifUseAppKey_then_require_enable_switch")),
          
          // Signature
          hash(
            pb,
            var_array(
              {blockExchange,
               storageID.packed,
               accountID.packed,
               tokenS.packed,
               tokenB.packed,
               amountS.packed,
               amountB.packed,
               validUntil.packed,
               // split TradingFee and GasFee - ProtocolFeeBips as the max TradingFee
               //  maxFeeBips.packed,
               fillAmountBorS.packed,
               taker,
                // split trading fee and gas fee
               feeTokenID.packed,
               maxFee.packed,
               type.packed,
              //  autoMarketID.packed
               gridOffset.packed,
               orderOffset.packed,
               maxLevel.packed,
               useAppKey.packed
               }),
            FMT(this->annotation_prefix, ".hash"))
    //  fee.packed, The signature does not contain the specific number of fee, only maxfee is required
    {
    }

    void generate_r1cs_witness(const Order &order)
    {
        LOG(LogDebug, "in OrderGadgets", "generate_r1cs_witness");
        // Inputs
        storageID.generate_r1cs_witness(pb, order.storageID);
        accountID.generate_r1cs_witness(pb, order.accountID);
        tokenS.generate_r1cs_witness(pb, order.tokenS);
        tokenB.generate_r1cs_witness(pb, order.tokenB);
        amountS.generate_r1cs_witness(pb, order.amountS);
        amountB.generate_r1cs_witness(pb, order.amountB);
        validUntil.generate_r1cs_witness(pb, order.validUntil);
        // split TradingFee and GasFee - ProtocolFeeBips as the max TradingFee
        // maxFeeBips.generate_r1cs_witness(pb, order.maxFeeBips);
        fillAmountBorS.generate_r1cs_witness(pb, order.fillAmountBorS);
        pb.val(taker) = order.taker;

        LOG(LogDebug, "in OrderGadgets", "feeBips");
        feeBips.generate_r1cs_witness(pb, order.feeBips);

        // Use the fee multiplier if necessary
        if (toBigInt(order.feeBips) >= 64 /*2**NUM_BITS_BIPS_DA*/)
        {
            feeBipsMultiplierFlag.generate_r1cs_witness(pb, ethsnarks::FieldT(1));
            feeBipsData.generate_r1cs_witness(
              pb, ethsnarks::FieldT((toBigInt(order.feeBips) / FEE_MULTIPLIER).to_int()));
        }
        else
        {
            feeBipsMultiplierFlag.generate_r1cs_witness(pb, ethsnarks::FieldT(0));
            feeBipsData.generate_r1cs_witness(pb, order.feeBips);
        }

        // std::cout << "in OrderGadgets before deltaFilledS" << std::endl;
        // deltaFilledS.generate_r1cs_witness(pb, order.deltaFilledS);
        // deltaFilledB.generate_r1cs_witness(pb, order.deltaFilledB);
        // std::cout << "in OrderGadgets before noop" << std::endl;
        // isNoop.generate_r1cs_witness(pb, order.isNoop);

        // Checks
        LOG(LogDebug, "in OrderGadgets", "feeBips_leq_maxFeeBips");
        feeBips_leq_maxFeeBips.generate_r1cs_witness();
        LOG(LogDebug, "in OrderGadgets", "tokenS_neq_tokenB");
        tokenS_neq_tokenB.generate_r1cs_witness();
        LOG(LogDebug, "in OrderGadgets", "amountS_notZero");
        amountS_notZero.generate_r1cs_witness();
        LOG(LogDebug, "in OrderGadgets", "amountB_notZero");
        amountB_notZero.generate_r1cs_witness();
        LOG(LogDebug, "in OrderGadgets", "feeMultiplier");
        // Fee checks
        feeMultiplier.generate_r1cs_witness();
        LOG(LogDebug, "in OrderGadgets", "decodedFeeBips");
        decodedFeeBips.generate_r1cs_witness();
        LOG(LogDebug, "in OrderGadgets", "feeBipsEqualsDecodedFeeBips");
        feeBipsEqualsDecodedFeeBips.generate_r1cs_witness();

        // split trading fee and gas fee
        LOG(LogDebug, "in OrderGadgets", "feeTokenID");
        feeTokenID.generate_r1cs_witness(pb, order.feeTokenID);
        fee.generate_r1cs_witness(pb, order.fee);
        LOG(LogDebug, "in OrderGadgets", "maxFee");
        maxFee.generate_r1cs_witness(pb, order.maxFee);
        LOG(LogDebug, "in OrderGadgets", "fee_leq_maxFee");
        fee_leq_maxFee.generate_r1cs_witness();
        LOG(LogDebug, "in OrderGadgets", "fFee");
        fFee.generate_r1cs_witness(toFloat(order.fee, Float16Encoding));
        LOG(LogDebug, "in OrderGadgets", "requireAccuracyFee");
        requireAccuracyFee.generate_r1cs_witness();

        LOG(LogDebug, "in OrderGadgets", "useAppKey");
        useAppKey.generate_r1cs_witness(pb, order.useAppKey);
        LOG(LogDebug, "in OrderGadgets", "disableAppKey");
        disableAppKey.generate_r1cs_witness();


        // AutoMarket
        LOG(LogDebug, "in OrderGadgets", "type");
        type.generate_r1cs_witness(pb, order.type);
        LOG(LogDebug, "in OrderGadgets", "level");
        // std::cout << "in OrderGadgets: before autoMarketID" << std::endl;
        // autoMarketID.generate_r1cs_witness(pb, order.autoMarketID);
        level.generate_r1cs_witness(pb, order.level);
        LOG(LogDebug, "in OrderGadgets", "gridOffset");
        gridOffset.generate_r1cs_witness(pb, order.gridOffset);
        LOG(LogDebug, "in OrderGadgets", "orderOffset");
        orderOffset.generate_r1cs_witness(pb, order.orderOffset);
        LOG(LogDebug, "in OrderGadgets", "maxLevel");
        maxLevel.generate_r1cs_witness(pb, order.maxLevel);
        LOG(LogDebug, "in OrderGadgets", "ifUseAppKey_then_require_enable_switch");

        ifUseAppKey_then_require_enable_switch.generate_r1cs_witness();
        LOG(LogDebug, "in OrderGadgets", "hash");

        // Signature
        hash.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        LOG(LogDebug, "in OrderGadgets", "generate_r1cs_constraints");
        // Inputs
        storageID.generate_r1cs_constraints(true);
        accountID.generate_r1cs_constraints(true);
        tokenS.generate_r1cs_constraints(true);
        tokenB.generate_r1cs_constraints(true);
        amountS.generate_r1cs_constraints(true);
        amountB.generate_r1cs_constraints(true);
        validUntil.generate_r1cs_constraints(true);
        // split TradingFee and GasFee - ProtocolFeeBips as the max TradingFee
        // maxFeeBips.generate_r1cs_constraints(true);
        fillAmountBorS.generate_r1cs_constraints(true);

        feeBips.generate_r1cs_constraints(true);

        feeBipsMultiplierFlag.generate_r1cs_constraints(true);
        feeBipsData.generate_r1cs_constraints(true);

        // deltaFilledS.generate_r1cs_constraints(true);
        // deltaFilledB.generate_r1cs_constraints(true);
        // isNoop.generate_r1cs_constraints(true);

        // Checks
        feeBips_leq_maxFeeBips.generate_r1cs_constraints();
        tokenS_neq_tokenB.generate_r1cs_constraints();
        amountS_notZero.generate_r1cs_constraints();
        amountB_notZero.generate_r1cs_constraints();
        // Fee checks
        feeMultiplier.generate_r1cs_constraints();
        decodedFeeBips.generate_r1cs_constraints();
        feeBipsEqualsDecodedFeeBips.generate_r1cs_constraints();

        // split trading fee and gas fee
        feeTokenID.generate_r1cs_constraints(true);
        fee.generate_r1cs_constraints(true);
        maxFee.generate_r1cs_constraints(true);
        fee_leq_maxFee.generate_r1cs_constraints();
        fFee.generate_r1cs_constraints();
        requireAccuracyFee.generate_r1cs_constraints();

        useAppKey.generate_r1cs_constraints(true);
        disableAppKey.generate_r1cs_constraints();

        // AutoMarket
        type.generate_r1cs_constraints(true);
        // autoMarketID.generate_r1cs_constraints(true);
        level.generate_r1cs_constraints(true);
        gridOffset.generate_r1cs_constraints(true);
        orderOffset.generate_r1cs_constraints(true);
        maxLevel.generate_r1cs_constraints(true);

        ifUseAppKey_then_require_enable_switch.generate_r1cs_constraints();

        // Signature
        hash.generate_r1cs_constraints();
    }
};

// If the current order is a new one, check whether the last order is completed
// New order: the forward stored in automarketconfig is contrary to the information provided by the current order, indicating that it is a new order.
// config. forward = 1, order. Forward = 1, no verification required
// config. forward = 1, order. Forward = 0, need verification
// config. forward = 0, order. Forward = 1, need verification
// config. forward = 0, order. Forward = 0, no verification required
// Complete or not: storage stores filled data = = filled currency data of startorder (filled currency is specified by fillamountbors field)
// If it is a new order, you need to update the forward under storage, provide the isneworder field, or provide newforward
class PreOrderCompleteCheckGadget : public GadgetT 
{
  public:
    NotEqualGadget forwardStatusNotMatch;
    TernaryGadget isNewOrder;
    TernaryGadget firstOrderFillAmount;

    // AndGadget needCheck;

    IfThenRequireAccuracyGadget requireAccuracyAmount;
    // IfThenRequireEqualGadget preOrderCompleted;

    PreOrderCompleteCheckGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &isAutoMarketOrder,
      const VariableT &isForward,
      const VariableT &firstOrderAmountB,
      const VariableT &firstOrderAmountS,
      const VariableT &startOrderFillAmountBorS,
      // const AutoMarketStorageReaderGadget &autoMarketStorage,
      const StorageReaderGadget &storage,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          forwardStatusNotMatch(pb, storage.getForward(), isForward, FMT(prefix, ".isNewOrder")),
          isNewOrder(pb, isAutoMarketOrder, forwardStatusNotMatch.result(), constants._0, FMT(prefix, ".isNewOrder")),
          // Depending on the currency in which the initial order of the order chain is fixed, the quantity in this currency will be returned here, 
          // and then the forward and reverse orders. No matter how forward and reverse, the fixed currency will always be the same currency
          firstOrderFillAmount(pb, startOrderFillAmountBorS, firstOrderAmountB, firstOrderAmountS, FMT(prefix, ".firstOrderFillAmount")),
          // needCheck(pb, {isNewOrder.result(), verify}, FMT(prefix, ".needCheck")),
          // preOrderCompleted(pb, isNewOrder.result(), storage.getData(), firstOrderFillAmount.result(), FMT(prefix, ".preOrderCompleted"))
          requireAccuracyAmount(
            pb,
            constants,
            isNewOrder.result(),
            storage.getData(),
            firstOrderFillAmount.result(),
            AutoMarketCompleteAccuracy,
            NUM_BITS_AMOUNT,
            FMT(prefix, ".requireAccuracyAmount")
          )
    {
      
    }

    void generate_r1cs_witness() 
    {
      forwardStatusNotMatch.generate_r1cs_witness();
      LOG(LogDebug, "in PreOrderCompleteCheckGadget after forwardStatusNotMatch", pb.val(forwardStatusNotMatch.result()));
      isNewOrder.generate_r1cs_witness();
      LOG(LogDebug, "in PreOrderCompleteCheckGadget after isNewOrder", pb.val(isNewOrder.result()));
      firstOrderFillAmount.generate_r1cs_witness();
      LOG(LogDebug, "in PreOrderCompleteCheckGadget after firstOrderFillAmount", pb.val(firstOrderFillAmount.result()));
      // needCheck.generate_r1cs_witness();
      // preOrderCompleted.generate_r1cs_witness();
      LOG(LogDebug, "in PreOrderCompleteCheckGadget before requireAccuracyAmount", "");
      requireAccuracyAmount.generate_r1cs_witness();
      LOG(LogDebug, "in PreOrderCompleteCheckGadget after requireAccuracyAmount", "");
    }
    void generate_r1cs_constraints() 
    {
      forwardStatusNotMatch.generate_r1cs_constraints();
      isNewOrder.generate_r1cs_constraints();
      firstOrderFillAmount.generate_r1cs_constraints();
      // needCheck.generate_r1cs_constraints();
      // preOrderCompleted.generate_r1cs_constraints();
      requireAccuracyAmount.generate_r1cs_constraints();
    }

    const VariableT &getIsNewOrder() const
    {
        return isNewOrder.result();
    }
};

// Calculate the current latest forward value. If it is a new order, update the stored forward field. 
// If it is not a new order, return the original value directly
class NextForwardGadget : public GadgetT 
{
  public:
    NotGadget notForward;
    TernaryGadget nextForward;
    NextForwardGadget(
      ProtoboardT &pb,
      const VariableT &forward,
      const VariableT &isNewOrder,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          notForward(pb, forward, FMT(prefix, ".notForward")),
          nextForward(pb, isNewOrder, notForward.result(), forward, FMT(prefix, ".nextForward"))
    {

    }

    void generate_r1cs_witness() 
    {
      notForward.generate_r1cs_witness();
      nextForward.generate_r1cs_witness();
      LOG(LogDebug, "in NextForwardGadget nextForward", pb.val(nextForward.result()));
    }

    void generate_r1cs_constraints() 
    {
      notForward.generate_r1cs_constraints();
      nextForward.generate_r1cs_constraints();
    }

    const VariableT &result() const
    {
        return nextForward.result();
    }
};

// Judge whether the current order is a forward order or a reverse order
// If it is a forward order, the amounts and amounts B of the order should be consistent with the firstorder
class ForwardOrderAmountCheckGadget : public GadgetT 
{
  public:
    // EqualGadget amountSEqual;
    // EqualGadget amountBEqual;
    BothAccuracyGadget amountSEqual;
    BothAccuracyGadget amountBEqual;
    AndGadget forwardOrderAmountOK;
    ForwardOrderAmountCheckGadget(
      ProtoboardT &pb,
      const VariableT &firstOrderAmountS,
      const VariableT &firstOrderAmountB,
      const OrderGadget &orderGadget,
      const std::string &prefix
    ) : GadgetT(pb, prefix),
        amountSEqual(pb, firstOrderAmountS, orderGadget.amountS.packed, AutoMarketAmountAccuracy, NUM_BITS_AMOUNT, FMT(prefix, ".amountSEqual")),
        amountBEqual(pb, firstOrderAmountB, orderGadget.amountB.packed, AutoMarketAmountAccuracy, NUM_BITS_AMOUNT, FMT(prefix, ".amountBEqual")),
        forwardOrderAmountOK(pb, {amountSEqual.result(), amountBEqual.result()}, FMT(prefix, ".isForwardOrder"))
    {

    }

    void generate_r1cs_witness() 
    {
      amountSEqual.generate_r1cs_witness();
      amountBEqual.generate_r1cs_witness();
      forwardOrderAmountOK.generate_r1cs_witness();
    }
    void generate_r1cs_constraints() 
    {
      amountSEqual.generate_r1cs_constraints();
      amountBEqual.generate_r1cs_constraints();
      forwardOrderAmountOK.generate_r1cs_constraints();
    }

    const VariableT &forwardAmountOK() const
    {
        return forwardOrderAmountOK.result();
    }
};
// If it is a reverse order and the initial order fillamountbors is 0, it is currently a reverse order with type = 6, 
//   that is, the reverse order of the sales order
// fillAmountBorS = 0, fillAmountS; Reverse single amounts = forward single amounts B - orderoffset, 
//   reverse single amounts B = = forward single amounts
class ReserveOrderFilleAmountSAmountCheckGadget : public GadgetT 
{
  public:
    // EqualGadget orderAmountB_eq_firstOrderAmountS;
    BothAccuracyGadget orderAmountB_eq_firstOrderAmountS;
    UnsafeSubGadget calculateReverseAmountS;
    // EqualGadget orderAmountS_eq_calculateReverseAmountS;
    BothAccuracyGadget orderAmountS_eq_calculateReverseAmountS;
    AndGadget amountOK;
    IfThenRequireLeqGadget requireValidOrderOffset;
    ReserveOrderFilleAmountSAmountCheckGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &firstOrderAmountS,
      const VariableT &firstOrderAmountB,
      const OrderGadget &orderGadget,
      const VariableT &orderOffset,
      const std::string &prefix
    ) : GadgetT(pb, prefix),
        orderAmountB_eq_firstOrderAmountS(pb, firstOrderAmountS, orderGadget.amountB.packed, AutoMarketAmountAccuracy, NUM_BITS_AMOUNT, FMT(prefix, ".orderAmountB_eq_firstOrderAmountS")),
        calculateReverseAmountS(pb, firstOrderAmountB, orderOffset, FMT(prefix, ".calculateReverseAmountS")),
        orderAmountS_eq_calculateReverseAmountS(pb, calculateReverseAmountS.result(), orderGadget.amountS.packed, AutoMarketAmountAccuracy, NUM_BITS_AMOUNT, FMT(prefix, ".orderAmountS_eq_calculateReverseAmountS")),
        amountOK(pb, {orderAmountB_eq_firstOrderAmountS.result(), orderAmountS_eq_calculateReverseAmountS.result()}, FMT(prefix, ".amountOK")),
        requireValidOrderOffset(pb, constants, amountOK.result(), orderOffset, firstOrderAmountB, NUM_BITS_AMOUNT, FMT(prefix, ".requireValidOrderOffset"))
    {

    }

    void generate_r1cs_witness() 
    {
      LOG(LogDebug, "in ReserveOrderFilleAmountSAmountCheckGadget", "orderAmountB_eq_firstOrderAmountS");
      orderAmountB_eq_firstOrderAmountS.generate_r1cs_witness();
      LOG(LogDebug, "in ReserveOrderFilleAmountSAmountCheckGadget", "calculateReverseAmountS");
      calculateReverseAmountS.generate_r1cs_witness();
      LOG(LogDebug, "in ReserveOrderFilleAmountSAmountCheckGadget", "orderAmountS_eq_calculateReverseAmountS");
      orderAmountS_eq_calculateReverseAmountS.generate_r1cs_witness();
      LOG(LogDebug, "in ReserveOrderFilleAmountSAmountCheckGadget", "amountOK");
      amountOK.generate_r1cs_witness();
      LOG(LogDebug, "in ReserveOrderFilleAmountSAmountCheckGadget", "end");
      requireValidOrderOffset.generate_r1cs_witness();
    }
    void generate_r1cs_constraints() 
    {
      orderAmountB_eq_firstOrderAmountS.generate_r1cs_constraints();
      calculateReverseAmountS.generate_r1cs_constraints();
      orderAmountS_eq_calculateReverseAmountS.generate_r1cs_constraints();
      amountOK.generate_r1cs_constraints();
      requireValidOrderOffset.generate_r1cs_constraints();
    }
    const VariableT &reserveAmountOK() const
    {
        return amountOK.result();
    }
};
// If the initial order fillamountbors is 1, then the current reverse order is type = 7, that is, the reverse order of the purchase
// Fillamountbors = 1, fillamountb: reverse order: reverse order amounts = = forward order amounts B, 
//    reverse order amounts B = forward order amounts + orderoffset
class ReserveOrderFilleAmountBAmountCheckGadget : public GadgetT 
{
  public:
    // EqualGadget orderAmountS_eq_firstOrderAmountB;
    BothAccuracyGadget orderAmountS_eq_firstOrderAmountB;
    AddGadget calculateReverseAmountB;
    // EqualGadget orderAmountB_eq_calculateReverseAmountB;
    BothAccuracyGadget orderAmountB_eq_calculateReverseAmountB;
    AndGadget amountOK;
    ReserveOrderFilleAmountBAmountCheckGadget(
      ProtoboardT &pb,
      const VariableT &firstOrderAmountS,
      const VariableT &firstOrderAmountB,
      const OrderGadget &orderGadget,
      const VariableT &orderOffset,
      const std::string &prefix
    ) : GadgetT(pb, prefix),
        orderAmountS_eq_firstOrderAmountB(pb, firstOrderAmountB, orderGadget.amountS.packed, AutoMarketAmountAccuracy, NUM_BITS_AMOUNT, FMT(prefix, ".orderAmountS_eq_firstOrderAmountB")),
        calculateReverseAmountB(pb, firstOrderAmountS, orderOffset, NUM_BITS_AMOUNT, FMT(prefix, ".calculateReverseAmountB")),
        orderAmountB_eq_calculateReverseAmountB(pb, calculateReverseAmountB.result(), orderGadget.amountB.packed, AutoMarketAmountAccuracy, NUM_BITS_AMOUNT, FMT(prefix, ".orderAmountB_eq_calculateReverseAmountB")),
        amountOK(pb, {orderAmountS_eq_firstOrderAmountB.result(), orderAmountB_eq_calculateReverseAmountB.result()}, FMT(prefix, ".amountOK"))
    {

    }

    void generate_r1cs_witness() 
    {
      orderAmountS_eq_firstOrderAmountB.generate_r1cs_witness();
      LOG(LogDebug, "in ReserveOrderFilleAmountBAmountCheckGadget after orderAmountS_eq_firstOrderAmountB", pb.val(orderAmountS_eq_firstOrderAmountB.result()));
      calculateReverseAmountB.generate_r1cs_witness();
      LOG(LogDebug, "in ReserveOrderFilleAmountBAmountCheckGadget after calculateReverseAmountB", pb.val(calculateReverseAmountB.result()));
      orderAmountB_eq_calculateReverseAmountB.generate_r1cs_witness();
      LOG(LogDebug, "in ReserveOrderFilleAmountBAmountCheckGadget after orderAmountB_eq_calculateReverseAmountB", pb.val(orderAmountB_eq_calculateReverseAmountB.result()));
      amountOK.generate_r1cs_witness();
      LOG(LogDebug, "in ReserveOrderFilleAmountBAmountCheckGadget after amountOK", pb.val(amountOK.result()));
    }
    void generate_r1cs_constraints() 
    {
      orderAmountS_eq_firstOrderAmountB.generate_r1cs_constraints();
      calculateReverseAmountB.generate_r1cs_constraints();
      orderAmountB_eq_calculateReverseAmountB.generate_r1cs_constraints();
      amountOK.generate_r1cs_constraints();
    }
    const VariableT &reserveAmountOK() const
    {
        return amountOK.result();
    }
};

// Calculate the data of the initial order
// Type = 6 (sales order): fixed amounts, modified amounts B
// Type = 7 (pay): fixed amount B, modified amount s
class FirstOrderGadget : public GadgetT 
{
  public:
    EqualGadget isFixedAmountS;
    EqualGadget isFixedAmountB;

    // calculation from both sides to the middle belongs to reverse push. 
    // The value added or subtracted is not directly multiplied by level with gridoffset, but MAXLEVEL - level
    // SubGadget gridOffsetMultiple;
    SafeMulGadget amountOffset;
    // Calculate the amount. Either amount is fixed or amount B is fixed. If amount B is fixed, then amounts = startorderamounts - amount offset
    // IfThenAddGadget firstOrderAmountS;
    IfThenSubGadget firstOrderAmountS;
    // Calculate the quantity of amountb. Either amounts is fixed or amountb is fixed. If amounts is fixed, then amountb = startorderamountb + amountoffset
    // IfThenSubGadget firstOrderAmountB;
    IfThenAddGadget firstOrderAmountB;
    
    FirstOrderGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const OrderGadget &orderGadget,
      const VariableT &startOrderAmountS,
      const VariableT &startOrderAmountB,
      // const AutoMarketReaderGadget &autoMarketConfig,
      const std::string &prefix
    ) : GadgetT(pb, prefix),
        // gridOffsetMultiple(pb, orderGadget.maxLevel.packed, orderGadget.level.packed, NUM_BITS_AUTOMARKET_LEVEL, FMT(prefix, ".gridOffsetMultiple")),
        amountOffset(pb, orderGadget.gridOffset.packed, orderGadget.level.packed, NUM_BITS_AMOUNT, FMT(prefix, ".amountOffset")),
        // amountOffset(pb, orderGadget.gridOffset.packed, gridOffsetMultiple.result(), NUM_BITS_AMOUNT, FMT(prefix, ".amountOffset")),
        // type = 6: fixed amounts
        // type = 7: fixed amountb
        isFixedAmountS(pb, orderGadget.type.packed, constants._6, FMT(prefix, ".isFixedAmountS")),
        isFixedAmountB(pb, orderGadget.type.packed, constants._7, FMT(prefix, ".isFixedAmountB")),
        // If it is fixed amountb, subtract it
        firstOrderAmountS(pb, constants, isFixedAmountB.result(), startOrderAmountS, amountOffset.result(), NUM_BITS_AMOUNT, FMT(prefix, ".firstOrderAmountS")),
        // If the amount is fixed, add it
        firstOrderAmountB(pb, isFixedAmountS.result(), startOrderAmountB, amountOffset.result(), NUM_BITS_AMOUNT, FMT(prefix, ".firstOrderAmountB"))
    {
      
    }

    void generate_r1cs_witness() 
    {
      // gridOffsetMultiple.generate_r1cs_witness();
      LOG(LogDebug, "in FirstOrderGadget", "amountOffset");
      amountOffset.generate_r1cs_witness();
      LOG(LogDebug, "in FirstOrderGadget", "isFixedAmountS");
      isFixedAmountS.generate_r1cs_witness();
      LOG(LogDebug, "in FirstOrderGadget", "isFixedAmountB");
      isFixedAmountB.generate_r1cs_witness();
      LOG(LogDebug, "in FirstOrderGadget isFixedAmountB", pb.val(isFixedAmountB.result()));
      LOG(LogDebug, "in FirstOrderGadget amountOffset", pb.val(amountOffset.result()));
      LOG(LogDebug, "in FirstOrderGadget", "firstOrderAmountS");
      firstOrderAmountS.generate_r1cs_witness();
      LOG(LogDebug, "in FirstOrderGadget firstOrderAmountS", pb.val(firstOrderAmountS.result()));
      LOG(LogDebug, "in FirstOrderGadget", "firstOrderAmountB");
      firstOrderAmountB.generate_r1cs_witness();
      LOG(LogDebug, "in FirstOrderGadget firstOrderAmountB", pb.val(firstOrderAmountB.result()));
      LOG(LogDebug, "in FirstOrderGadget", "end");
    }
    void generate_r1cs_constraints() 
    {
      // gridOffsetMultiple.generate_r1cs_constraints();
      amountOffset.generate_r1cs_constraints();
      isFixedAmountS.generate_r1cs_constraints();
      isFixedAmountB.generate_r1cs_constraints();
      firstOrderAmountS.generate_r1cs_constraints();
      firstOrderAmountB.generate_r1cs_constraints();
    }

    const VariableT &getAmountS() const
    {
        return firstOrderAmountS.result();
    }

    const VariableT &getAmountB() const
    {
        return firstOrderAmountB.result();
    }

};
// Forward doc data verification: for a forward doc, you only need to verify whether the quantities of two currencies are equal
class GridOrderForwardCheckGadget : public GadgetT 
{
  public:
    ForwardOrderAmountCheckGadget forwardOrderCheckGadget;

    // If it is a forward order, check whether the amount is correct. Otherwise, it will be passed directly
    AndGadget needCheckForwardOrderAmount;
    IfThenRequireGadget requireValidForwardAmount;

    // If it is a bill, type = 7, and it is a positive bill, then the fillamountbors of order must be 1
    EqualGadget isBuy;
    AndGadget isBuy_and_isForward;
    IfThenRequireEqualGadget requireBuyOrderFillAmountBorSValid;

    // If it is a sales order with type = 6 and a positive order, the fillamountbors of the order must be 0
    EqualGadget isSell;
    AndGadget isSell_and_isForward;
    IfThenRequireEqualGadget requireSellOrderFillAmountBorSValid;

    GridOrderForwardCheckGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &isAutoMarketOrder,
      const VariableT &isForward,
      const OrderGadget &orderGadget,
      const FirstOrderGadget &firstOrderGadget,
      const VariableT &orderType,
      const std::string &prefix
    ) : GadgetT(pb, prefix),
      // Returns whether the current order is a forward order or a reverse order
      forwardOrderCheckGadget(pb, firstOrderGadget.getAmountS(), firstOrderGadget.getAmountB(), orderGadget, FMT(prefix, ".forwardOrderCheckGadget")),
      
      // Determine whether the current order should be a forward order or a reverse order according to other information. 
      // If the external incoming isforward and isautomarketorder are established at the same time, 
      // it is required that the forwardordercheckgadget must be true
      needCheckForwardOrderAmount(pb, {isForward, isAutoMarketOrder}, FMT(prefix, ".needCheckForwardOrderAmount")),
      // If you need to verify a forward doc, you need to verify it
      requireValidForwardAmount(pb, needCheckForwardOrderAmount.result(), forwardOrderCheckGadget.forwardAmountOK(), FMT(prefix, ".requireValidForwardAmount")),

      isBuy(pb, orderType, constants._7, FMT(prefix, ".isBuy")),
      isBuy_and_isForward(pb, {isBuy.result(), isForward}, FMT(prefix, ".isBuy_and_isForward")),
      requireBuyOrderFillAmountBorSValid(pb, isBuy_and_isForward.result(), orderGadget.fillAmountBorS.packed, constants._1, FMT(prefix, ".requireBuyOrderFillAmountBorSValid")),

      isSell(pb, orderType, constants._6, FMT(prefix, ".isSell")),
      isSell_and_isForward(pb, {isSell.result(), isForward}, FMT(prefix, ".isSell_and_isForward")),
      requireSellOrderFillAmountBorSValid(pb, isSell_and_isForward.result(), orderGadget.fillAmountBorS.packed, constants._0, FMT(prefix, ".requireSellOrderFillAmountBorSValid"))
    {

    }

    void generate_r1cs_witness() 
    {
      forwardOrderCheckGadget.generate_r1cs_witness();
      needCheckForwardOrderAmount.generate_r1cs_witness();
      requireValidForwardAmount.generate_r1cs_witness();

      isBuy.generate_r1cs_witness();
      isBuy_and_isForward.generate_r1cs_witness();
      requireBuyOrderFillAmountBorSValid.generate_r1cs_witness();

      isSell.generate_r1cs_witness();
      isSell_and_isForward.generate_r1cs_witness();
      requireSellOrderFillAmountBorSValid.generate_r1cs_witness();
    }

    void generate_r1cs_constraints() 
    {
      forwardOrderCheckGadget.generate_r1cs_constraints();
      needCheckForwardOrderAmount.generate_r1cs_constraints();
      requireValidForwardAmount.generate_r1cs_constraints();

      isBuy.generate_r1cs_constraints();
      isBuy_and_isForward.generate_r1cs_constraints();
      requireBuyOrderFillAmountBorSValid.generate_r1cs_constraints();

      isSell.generate_r1cs_constraints();
      isSell_and_isForward.generate_r1cs_constraints();
      requireSellOrderFillAmountBorSValid.generate_r1cs_constraints();
    }
};

// Reverse bill data verification, type = 7
class GridOrderReserveBuyCheckGadget : public GadgetT 
{
  public:
    EqualGadget orderFillAmoutB;
    AndGadget fillAmountB_and_reserve_order;

    ReserveOrderFilleAmountBAmountCheckGadget reserveOrderFilleAmountBAmountCheck;
    AndGadget needCheckReverseFillAmountBAmount;
    IfThenRequireGadget requireValidReverseFillAmountBAmount;

    // If it is a purchase order and a reverse order, the fillamountbors of the order must be 0
    EqualGadget isBuy;
    AndGadget isBuy_and_isReserve;
    IfThenRequireEqualGadget requireOrderFillAmountBorSValid;
    GridOrderReserveBuyCheckGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &isAutoMarketOrder,
      const VariableT &isReverse,
      const OrderGadget &orderGadget,
      const FirstOrderGadget &firstOrderGadget,
      const VariableT &startOrderFillAmountBorS,
      const VariableT &orderType,
      const std::string &prefix
    ) : GadgetT(pb, prefix), 
      // The order is in the fixed currency of amountb, and amountb can buy all the orders after they are finished
      orderFillAmoutB(pb, startOrderFillAmountBorS, constants._1, FMT(prefix, ".orderFillAmoutB")),
      // It is also a reverse order. The initial order is a fixed amount B. at present, it appears in the reverse order with type = 7
      fillAmountB_and_reserve_order(pb, {orderFillAmoutB.result(), isReverse}, FMT(prefix, ".fillAmountB_and_reserve_order")),

      reserveOrderFilleAmountBAmountCheck(pb, firstOrderGadget.getAmountS(), firstOrderGadget.getAmountB(), orderGadget, orderGadget.orderOffset.packed, FMT(prefix, ".reserveOrderFilleAmountBAmountCheck")),
      needCheckReverseFillAmountBAmount(pb, {fillAmountB_and_reserve_order.result(), isAutoMarketOrder}, FMT(prefix, ".needCheckReverseFillAmountBAmount")),
      requireValidReverseFillAmountBAmount(pb, needCheckReverseFillAmountBAmount.result(), reserveOrderFilleAmountBAmountCheck.reserveAmountOK(), FMT(prefix, ".requireValidReverseFillAmountBAmount")),

      isBuy(pb, orderType, constants._7, FMT(prefix, ".isBuy")),
      isBuy_and_isReserve(pb, {isBuy.result(), isReverse}, FMT(prefix, ".isBuy_and_isReserve")),
      requireOrderFillAmountBorSValid(pb, isBuy_and_isReserve.result(), orderGadget.fillAmountBorS.packed, constants._0, FMT(prefix, ".requireOrderFillAmountBorSValid"))
    {

    }

    void generate_r1cs_witness() 
    {
      orderFillAmoutB.generate_r1cs_witness();
      fillAmountB_and_reserve_order.generate_r1cs_witness();

      reserveOrderFilleAmountBAmountCheck.generate_r1cs_witness();
      needCheckReverseFillAmountBAmount.generate_r1cs_witness();
      requireValidReverseFillAmountBAmount.generate_r1cs_witness();

      isBuy.generate_r1cs_witness();
      isBuy_and_isReserve.generate_r1cs_witness();
      requireOrderFillAmountBorSValid.generate_r1cs_witness();
    }

    void generate_r1cs_constraints() 
    {
      orderFillAmoutB.generate_r1cs_constraints();
      fillAmountB_and_reserve_order.generate_r1cs_constraints();

      reserveOrderFilleAmountBAmountCheck.generate_r1cs_constraints();
      needCheckReverseFillAmountBAmount.generate_r1cs_constraints();
      requireValidReverseFillAmountBAmount.generate_r1cs_constraints();

      isBuy.generate_r1cs_constraints();
      isBuy_and_isReserve.generate_r1cs_constraints();
      requireOrderFillAmountBorSValid.generate_r1cs_constraints();
    }

};
// Reverse sales order data verification, type = 6
class GridOrderReserveSellCheckGadget : public GadgetT 
{
  public:
    EqualGadget orderFillAmoutS;
    AndGadget fillAmountS_and_reserve_order;

    ReserveOrderFilleAmountSAmountCheckGadget reserveOrderFilleAmountSAmountCheck;
    AndGadget needCheckReverseFillAmountSAmount;
    IfThenRequireGadget requireValidReverseFillAmountSAmount;

    // If it is a sales order and a reverse order, the fillamountbors of the order must be 1
    EqualGadget isSell;
    AndGadget isSell_and_isReserve;
    IfThenRequireEqualGadget requireOrderFillAmountBorSValid;
    GridOrderReserveSellCheckGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &isAutoMarketOrder,
      const VariableT &isReverse,
      const OrderGadget &orderGadget,
      const FirstOrderGadget &firstOrderGadget,
      const VariableT &startOrderFillAmountBorS,
      const VariableT &orderType,
      const std::string &prefix
    ) : GadgetT(pb, prefix), 
      // The order is in the fixed currency of amounts, and all amounts are sold after the order is finished
      orderFillAmoutS(pb, startOrderFillAmountBorS, constants._0, FMT(prefix, ".orderFillAmoutS")),
      // It is also a reverse order. The initial order is a fixed amount. At present, it appears in the reverse order with type = 6
      fillAmountS_and_reserve_order(pb, {orderFillAmoutS.result(), isReverse}, FMT(prefix, ".fillAmountS_and_reserve_order")),

      reserveOrderFilleAmountSAmountCheck(pb, constants, firstOrderGadget.getAmountS(), firstOrderGadget.getAmountB(), orderGadget, orderGadget.orderOffset.packed, FMT(prefix, ".reserveOrderFilleAmountSAmountCheck")),
      needCheckReverseFillAmountSAmount(pb, {fillAmountS_and_reserve_order.result(), isAutoMarketOrder}, FMT(prefix, ".needCheckReverseFillAmountSAmount")),
      requireValidReverseFillAmountSAmount(pb, needCheckReverseFillAmountSAmount.result(), reserveOrderFilleAmountSAmountCheck.reserveAmountOK(), FMT(prefix, ".requireValidReverseFillAmountSAmount")),

      isSell(pb, orderType, constants._6, FMT(prefix, ".isSell")),
      isSell_and_isReserve(pb, {isSell.result(), isReverse}, FMT(prefix, ".isSell_and_isReserve")),
      requireOrderFillAmountBorSValid(pb, isSell_and_isReserve.result(), orderGadget.fillAmountBorS.packed, constants._1, FMT(prefix, ".requireOrderFillAmountBorSValid"))
    {

    }

    void generate_r1cs_witness() 
    {
      LOG(LogDebug, "in GridOrderReserveSellCheckGadget", "orderFillAmoutS");
      orderFillAmoutS.generate_r1cs_witness();
      LOG(LogDebug, "in GridOrderReserveSellCheckGadget", "fillAmountS_and_reserve_order");
      fillAmountS_and_reserve_order.generate_r1cs_witness();

      LOG(LogDebug, "in GridOrderReserveSellCheckGadget", "reserveOrderFilleAmountSAmountCheck");
      reserveOrderFilleAmountSAmountCheck.generate_r1cs_witness();
      LOG(LogDebug, "in GridOrderReserveSellCheckGadget", "needCheckReverseFillAmountSAmount");
      needCheckReverseFillAmountSAmount.generate_r1cs_witness();
      LOG(LogDebug, "in GridOrderReserveSellCheckGadget", "requireValidReverseFillAmountSAmount");
      requireValidReverseFillAmountSAmount.generate_r1cs_witness();
      LOG(LogDebug, "in GridOrderReserveSellCheckGadget", "isSell");

      isSell.generate_r1cs_witness();
      LOG(LogDebug, "in GridOrderReserveSellCheckGadget", "isSell_and_isReserve");
      isSell_and_isReserve.generate_r1cs_witness();
      LOG(LogDebug, "in GridOrderReserveSellCheckGadget", "requireOrderFillAmountBorSValid");
      requireOrderFillAmountBorSValid.generate_r1cs_witness();
      LOG(LogDebug, "in GridOrderReserveSellCheckGadget", "end");
    }

    void generate_r1cs_constraints() 
    {
      orderFillAmoutS.generate_r1cs_constraints();
      fillAmountS_and_reserve_order.generate_r1cs_constraints();

      reserveOrderFilleAmountSAmountCheck.generate_r1cs_constraints();
      needCheckReverseFillAmountSAmount.generate_r1cs_constraints();
      requireValidReverseFillAmountSAmount.generate_r1cs_constraints();

      isSell.generate_r1cs_constraints();
      isSell_and_isReserve.generate_r1cs_constraints();
      requireOrderFillAmountBorSValid.generate_r1cs_constraints();
    }

};

// Verify that the order conforms to the grid data
// First calculate the initial order data according to the levels of startorder and order, and then verify the correctness of the order data according to the logic of forward and reverse orders
// Order chain logic:
// Fillamountbors = 1, fillamountb: forward order: amounts and amounts are equal respectively, reverse order: reverse order amounts = = forward order amounts B, reverse order amounts B = forward order amounts + orderoffset
// Fillamountbors = 0, fillamounts: forward order: amounts and amounts B are equal respectively, reverse order: reverse order amounts = forward order amounts B - orderoffset, reverse order amounts B = = forward order amounts
class GridOrderCheckGadget : public GadgetT 
{
  public:
    // Calculate the starting order of order chain
    FirstOrderGadget firstOrderGadget;

    // If it is a forward order, verify whether the data of the forward order is correct
    GridOrderForwardCheckGadget forwardCheck;
    // If the reverse order is paid, type = 7, check whether the reverse order data is correct
    GridOrderReserveBuyCheckGadget reserveBuyCheck;
    // If it is a reverse sales order with type = 6, check whether the reverse order data is correct
    GridOrderReserveSellCheckGadget reserveSellCheck;
    GridOrderCheckGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &startOrderAmountS,
      const VariableT &startOrderAmountB,
      const VariableT &isAutoMarketOrder,
      const VariableT &isForward,
      const VariableT &isReverse,
      const VariableT &startOrderFillAmountBorS,
      const VariableT &orderType,
      const OrderGadget &orderGadget,
      // const AutoMarketReaderGadget &autoMarketConfig,
      const std::string &prefix
    ) : GadgetT(pb, prefix),
        // firstOrderGadget(pb, constants, orderGadget, autoMarketConfig, FMT(prefix, ".firstOrderGadget")),
        firstOrderGadget(pb, constants, orderGadget, startOrderAmountS, startOrderAmountB, FMT(prefix, ".firstOrderGadget")),

        forwardCheck(pb, constants, isAutoMarketOrder, isForward, orderGadget, firstOrderGadget, orderType, FMT(prefix, ".forwardCheck")),

        reserveBuyCheck(pb, constants, isAutoMarketOrder, isReverse, orderGadget, firstOrderGadget, startOrderFillAmountBorS, orderType, FMT(prefix, ".reserveBuyCheck")),
        
        reserveSellCheck(pb, constants, isAutoMarketOrder, isReverse, orderGadget, firstOrderGadget, startOrderFillAmountBorS, orderType, FMT(prefix, ".reserveSellCheck"))
    {
      
    }
    void generate_r1cs_witness() 
    {
      LOG(LogDebug, "in GridOrderCheckGadget", "generate_r1cs_witness");
      firstOrderGadget.generate_r1cs_witness();
      LOG(LogDebug, "in GridOrderCheckGadget", "forwardCheck");
      forwardCheck.generate_r1cs_witness();
      LOG(LogDebug, "in GridOrderCheckGadget", "reserveBuyCheck");
      reserveBuyCheck.generate_r1cs_witness();
      LOG(LogDebug, "in GridOrderCheckGadget", "reserveSellCheck");
      reserveSellCheck.generate_r1cs_witness();
      LOG(LogDebug, "in GridOrderCheckGadget after firstOrderGadget amountS", pb.val(firstOrderGadget.getAmountS()));
      LOG(LogDebug, "in GridOrderCheckGadget after firstOrderGadget amountB", pb.val(firstOrderGadget.getAmountB()));
    }
    void generate_r1cs_constraints() 
    {
      firstOrderGadget.generate_r1cs_constraints();
      forwardCheck.generate_r1cs_constraints();
      reserveBuyCheck.generate_r1cs_constraints();
      reserveSellCheck.generate_r1cs_constraints();
    }

    const VariableT &getFirstOrderAmountS() const
    {
        return firstOrderGadget.getAmountS();
    }

    const VariableT &getFirstOrderAmountB() const
    {
        return firstOrderGadget.getAmountB();
    }

};

// check auto market order
// Relationship to be proved:
// 1: the order amount is equal. Combined with the forward field, the forward and reverse orders are distinguished. 
//    The last order matching is completed. If the last order has been completed, it will be a new reverse order, and the forward will be updated at the same time
// 2: the storageids are equal. Check whether the storageid is the tokenstorageid under the corresponding level of the automarket
// 3: automarketid is equal
// 4: automarketorder is 1
// 5: tokensid, tokenbid, combined with the forward field, are compared with the fields configured in automarket
// 6: validuntil equal
// 7: accountid is equal
// 8: fillamountbors equal
// 9: taker equal
// 10: feetokenid equal
// 11: maxfee equal
// 12: uireferid equal
// 13: price comparison, because there is only a gap in the quantity of a fixed currency at different levels according to the algorithm, 
//     and the other currency needs to be carried out in the form of startorder
// 14: check grid amount
// 15: verify the amount of subsequent orders. If it is a forward order, the quantity is the same as the initial order. 
//     If it is a reverse order, it needs to be calculated
class AutoMarketOrderCheck : public GadgetT 
{
  public:
    DualVariableGadget storageID;
    DualVariableGadget accountID;
    DualVariableGadget tokenS;
    DualVariableGadget tokenB;
    DualVariableGadget amountS;
    DualVariableGadget amountB;
    DualVariableGadget feeTokenID;
    DualVariableGadget maxFee;

    DualVariableGadget validUntil;
    DualVariableGadget fillAmountBorS;
    VariableT taker;

    DualVariableGadget orderType;
    // DualVariableGadget autoMarketID;
    DualVariableGadget gridOffset;
    DualVariableGadget orderOffset;
    DualVariableGadget maxLevel;
    DualVariableGadget useAppKey;

    EqualGadget isAutoMarketBuyOrder;
    EqualGadget isAutoMarketSellOrder;
    OrGadget isAutoMarketOrder;

    AddGadget autoMarketStorageID;
    TernaryGadget tokenSIDForStorageUpdate;
    TernaryGadget tokenBIDForStorageUpdate;

    IfThenRequireEqualGadget storageIDEqual;
    IfThenRequireEqualGadget accountIDEqual;
    IfThenRequireEqualGadget feeTokenIDEqual;
    IfThenRequireEqualGadget maxFeeEqual;
    IfThenRequireEqualGadget validUntilEqual;
    IfThenRequireEqualGadget takerEqual;
    IfThenRequireEqualGadget orderTypeEqual;
    IfThenRequireEqualGadget gridOffsetEqual;
    IfThenRequireEqualGadget orderOffsetEqual;
    IfThenRequireEqualGadget maxLevelEqual;
    IfThenRequireEqualGadget useAppKeyEqual;

    EqualGadget tokenSSEqual;
    EqualGadget tokenBBEqual;
    EqualGadget tokenSBEqual;
    EqualGadget tokenBSEqual;

    AndGadget isForward;
    AndGadget isReverse;
    // One of isforward and isreverse must be 1
    OrGadget tokenIdValid;
    IfThenRequireGadget requireTokenIDValid;

    GridOrderCheckGadget gridOrderCheck;
    PreOrderCompleteCheckGadget preOrderCompleteCheck;
    NextForwardGadget nextForward;

    // LeqGadget autoMarketConfigValidUntil;
    // IfThenRequireGadget requireValidUntil;

    // NotGadget autoMarketNotCancelled;
    // IfThenRequireGadget requireAutoMarketNotCancelled;

    RequireLeqGadget requireLevelValid;

    // Poseidon_16 hash;
    Poseidon_17 hash;

    // If it is an automarket, use the hash here. If not, use the hash under order
    TernaryGadget verifyHash;

    AutoMarketOrderCheck(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &timestamp,
      const VariableT &blockExchange,
      const OrderGadget &orderGadget,
      // const AutoMarketReaderGadget &autoMarketConfig,
      // const AutoMarketStorageReaderGadget &autoMarketStorage,
      const StorageReaderGadget &storage,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          storageID(pb, NUM_BITS_STORAGEID, FMT(prefix, ".storageID")),
          accountID(pb, NUM_BITS_ACCOUNT, FMT(prefix, ".accountID")),
          tokenS(pb, NUM_BITS_TOKEN, FMT(prefix, ".tokenS")),
          tokenB(pb, NUM_BITS_TOKEN, FMT(prefix, ".tokenB")),
          amountS(pb, NUM_BITS_AMOUNT, FMT(prefix, ".amountS")),
          amountB(pb, NUM_BITS_AMOUNT, FMT(prefix, ".amountB")),
          validUntil(pb, NUM_BITS_TIMESTAMP, FMT(prefix, ".validUntil")),
          feeTokenID(pb, NUM_BITS_TOKEN, FMT(prefix, ".feeTokenID")),
          maxFee(pb, NUM_BITS_AMOUNT, FMT(prefix, ".maxFee")),
          fillAmountBorS(pb, 1, FMT(prefix, ".fillAmountBorS")),
          taker(make_variable(pb, FMT(prefix, ".taker"))),

          orderType(pb, NUM_BITS_TYPE, FMT(prefix, ".orderType")),
          gridOffset(pb, NUM_BITS_AMOUNT, FMT(prefix, ".gridOffset")),
          orderOffset(pb, NUM_BITS_AMOUNT, FMT(prefix, ".orderOffset")),
          maxLevel(pb, NUM_BITS_AUTOMARKET_LEVEL, FMT(prefix, ".maxLevel")),
          useAppKey(pb, NUM_BITS_BYTE, FMT(prefix, ".useAppKey")),

          // type = 6: Sales Order
          // type = 7: pay the bill
          isAutoMarketSellOrder(pb, orderType.packed, constants._6, FMT(prefix, ".isAutoMarketBuyOrder")),
          isAutoMarketBuyOrder(pb, orderType.packed, constants._7, FMT(prefix, ".isAutoMarketBuyOrder")),
          isAutoMarketOrder(pb, {isAutoMarketBuyOrder.result(), isAutoMarketSellOrder.result()}, FMT(prefix, ".isAutoMarketOrder")),

          // Storageid logical description for update
          // If it is an automarket, the storageid used for updating needs to comply with the rule: startorder storageID + order. 
          // Level, that is, use the calculated storageid instead of the one provided by order
          // If it is not an automarket, you can directly return the storageid of the order
          autoMarketStorageID(pb, storageID.packed, orderGadget.level.packed, NUM_BITS_STORAGEID, FMT(prefix, ".autoMarketStorageID")),
          // storageIDForUpdate(pb, isAutoMarketOrder.result(), autoMarketStorageID.result(), orderGadget.storageID.packed, FMT(prefix, ".autoMarketStorageID")),

          tokenSIDForStorageUpdate(pb, isAutoMarketOrder.result(), tokenS.packed, orderGadget.tokenS.packed, FMT(prefix, ".tokenSIDForStorageUpdate")),
          tokenBIDForStorageUpdate(pb, isAutoMarketOrder.result(), tokenB.packed, orderGadget.tokenB.packed, FMT(prefix, ".tokenBIDForStorageUpdate")),

          // Storageid of order = = calculated storageid
          storageIDEqual(pb, isAutoMarketOrder.result(), autoMarketStorageID.result(), orderGadget.storageID.packed, FMT(prefix, ".storageIDEqual")),
          accountIDEqual(pb, isAutoMarketOrder.result(), accountID.packed, orderGadget.accountID.packed, FMT(prefix, ".accountIDEqual")),
          feeTokenIDEqual(pb, isAutoMarketOrder.result(), feeTokenID.packed, orderGadget.feeTokenID.packed, FMT(prefix, ".feeTokenIDEqual")),
          maxFeeEqual(pb, isAutoMarketOrder.result(), maxFee.packed, orderGadget.maxFee.packed, FMT(prefix, ".maxFeeEqual")),
          // uiReferIDEqual(pb, isAutoMarketOrder.result(), uiReferID.packed, orderGadget.uiReferID.packed, FMT(prefix, ".uiReferIDEqual")),
          validUntilEqual(pb, isAutoMarketOrder.result(), validUntil.packed, orderGadget.validUntil.packed, FMT(prefix, ".validUntilEqual")),
          takerEqual(pb, isAutoMarketOrder.result(), taker, orderGadget.taker, FMT(prefix, ".takerEqual")),
          orderTypeEqual(pb, isAutoMarketOrder.result(), orderType.packed, orderGadget.type.packed, FMT(prefix, ".orderTypeEqual")),
          gridOffsetEqual(pb, isAutoMarketOrder.result(), gridOffset.packed, orderGadget.gridOffset.packed, FMT(prefix, ".gridOffsetEqual")),
          orderOffsetEqual(pb, isAutoMarketOrder.result(), orderOffset.packed, orderGadget.orderOffset.packed, FMT(prefix, ".orderOffsetEqual")),
          maxLevelEqual(pb, isAutoMarketOrder.result(), maxLevel.packed, orderGadget.maxLevel.packed, FMT(prefix, ".maxLevelEqual")),
          useAppKeyEqual(pb, isAutoMarketOrder.result(), useAppKey.packed, orderGadget.useAppKey.packed, FMT(prefix, ".useAppKeyEqual")),
          // autoMarketIDEqual(pb, isAutoMarketOrder.result(), autoMarketID.packed, orderGadget.autoMarketID.packed, FMT(prefix, ".autoMarketIDEqual")),

          // tokenStorageID_eq_autoMarketStorageID(pb, isAutoMarketOrder.result(), orderGadget.storageID.packed, autoMarketStorage.getTokenStorageID(), FMT(prefix, ".tokenStorageID_eq_autoMarketStorageID")),

          tokenSSEqual(pb, tokenS.packed, orderGadget.tokenS.packed, FMT(prefix, ".tokenSSEqual")),
          tokenBBEqual(pb, tokenB.packed, orderGadget.tokenB.packed, FMT(prefix, ".tokenBBEqual")),
          tokenSBEqual(pb, tokenS.packed, orderGadget.tokenB.packed, FMT(prefix, ".tokenSBEqual")),
          tokenBSEqual(pb, tokenB.packed, orderGadget.tokenS.packed, FMT(prefix, ".tokenBSEqual")),

          isForward(pb, {tokenSSEqual.result(), tokenBBEqual.result()}, FMT(prefix, ".isForward")),
          isReverse(pb, {tokenSBEqual.result(), tokenBSEqual.result()}, FMT(prefix, ".isReverse")),

          tokenIdValid(pb, {isForward.result(), isReverse.result()}, FMT(prefix, ".tokenIdValid")),
          requireTokenIDValid(pb, isAutoMarketOrder.result(), tokenIdValid.result(), FMT(prefix, ".requireTokenIDValid")),

          // gridOrderCheck(pb, constants, isAutoMarketOrder.result(), isForward.result(), isReverse.result(), fillAmountBorS.packed, orderGadget, autoMarketConfig, FMT(prefix, ".gridOrderCheck")),
          gridOrderCheck(pb, constants, amountS.packed, amountB.packed, isAutoMarketOrder.result(), isForward.result(), isReverse.result(), fillAmountBorS.packed, orderType.packed, orderGadget, FMT(prefix, ".gridOrderCheck")),
          
          // preOrderCompleteCheck(pb, constants, isAutoMarketOrder.result(), isForward.result(), amountB.packed, amountS.packed, fillAmountBorS.packed, autoMarketStorage, FMT(prefix, ".preOrderCompleteCheck")),
          // preOrderCompleteCheck(pb, constants, isAutoMarketOrder.result(), isForward.result(), gridOrderCheck.getFirstOrderAmountB(), gridOrderCheck.getFirstOrderAmountS(), fillAmountBorS.packed, autoMarketStorage, FMT(prefix, ".preOrderCompleteCheck")),
          preOrderCompleteCheck(pb, constants, isAutoMarketOrder.result(), isForward.result(), gridOrderCheck.getFirstOrderAmountB(), gridOrderCheck.getFirstOrderAmountS(), fillAmountBorS.packed, storage, FMT(prefix, ".preOrderCompleteCheck")),
          // nextForward(pb, autoMarketStorage.getForward(), isAutoMarketOrder.result(), preOrderCompleteCheck.getIsNewOrder(), FMT(prefix, ".nextForward")),
          nextForward(pb, storage.getForward(), preOrderCompleteCheck.getIsNewOrder(), FMT(prefix, ".nextForward")),
          
          // autoMarketConfigValidUntil(
          //   pb,
          //   timestamp,
          //   autoMarketConfig.getValidUntil(),
          //   NUM_BITS_TIMESTAMP,
          //   FMT(prefix, ".autoMarketConfigValidUntil")),
          // requireValidUntil(pb, isAutoMarketOrder.result(), autoMarketConfigValidUntil.lt(), FMT(prefix, ".requireValidUntil")),

          // autoMarketNotCancelled(pb, autoMarketConfig.getCancelled(), FMT(prefix, ".autoMarketNotCancelled")),
          // requireAutoMarketNotCancelled(pb, isAutoMarketOrder.result(), autoMarketNotCancelled.result(), FMT(prefix, ".requireNotCancelled")),

          requireLevelValid(pb, orderGadget.level.packed, maxLevel.packed, NUM_BITS_AUTOMARKET_LEVEL, FMT(prefix, ".requireLevelValid")),

          hash(
            pb,
            var_array(
              {blockExchange,
               storageID.packed,
               accountID.packed,
               tokenS.packed,
               tokenB.packed,
               amountS.packed,
               amountB.packed,
               validUntil.packed,
               fillAmountBorS.packed,
               taker,
               feeTokenID.packed,
               maxFee.packed,
               orderType.packed,
               gridOffset.packed,
               orderOffset.packed,
               maxLevel.packed,
               useAppKey.packed
               }),
            FMT(this->annotation_prefix, ".hash")),
          verifyHash(pb, isAutoMarketOrder.result(), hash.result(), orderGadget.hash.result(), FMT(prefix, ".verifyHash"))
    {
      
    }
    void generate_r1cs_witness(const AutoMarketOrder &autoMarketOrder) 
    {
      LOG(LogDebug, "in AutoMarketOrderCheck", "generate_r1cs_witness");

      storageID.generate_r1cs_witness(pb, autoMarketOrder.storageID);
      accountID.generate_r1cs_witness(pb, autoMarketOrder.accountID);
      tokenS.generate_r1cs_witness(pb, autoMarketOrder.tokenS);
      tokenB.generate_r1cs_witness(pb, autoMarketOrder.tokenB);
      amountS.generate_r1cs_witness(pb, autoMarketOrder.amountS);
      amountB.generate_r1cs_witness(pb, autoMarketOrder.amountB);
      validUntil.generate_r1cs_witness(pb, autoMarketOrder.validUntil);
      feeTokenID.generate_r1cs_witness(pb, autoMarketOrder.feeTokenID);
      maxFee.generate_r1cs_witness(pb, autoMarketOrder.maxFee);
      fillAmountBorS.generate_r1cs_witness(pb, autoMarketOrder.fillAmountBorS);
      pb.val(taker) = autoMarketOrder.taker;
      orderType.generate_r1cs_witness(pb, autoMarketOrder.type);
      gridOffset.generate_r1cs_witness(pb, autoMarketOrder.gridOffset);
      orderOffset.generate_r1cs_witness(pb, autoMarketOrder.orderOffset);
      maxLevel.generate_r1cs_witness(pb, autoMarketOrder.maxLevel);
      useAppKey.generate_r1cs_witness(pb, autoMarketOrder.useAppKey);

      isAutoMarketBuyOrder.generate_r1cs_witness();
      isAutoMarketSellOrder.generate_r1cs_witness();
      isAutoMarketOrder.generate_r1cs_witness();

      autoMarketStorageID.generate_r1cs_witness();

      tokenSIDForStorageUpdate.generate_r1cs_witness();
      tokenBIDForStorageUpdate.generate_r1cs_witness();

      storageIDEqual.generate_r1cs_witness();
      accountIDEqual.generate_r1cs_witness();
      feeTokenIDEqual.generate_r1cs_witness();
      maxFeeEqual.generate_r1cs_witness();
      validUntilEqual.generate_r1cs_witness();
      takerEqual.generate_r1cs_witness();
      orderTypeEqual.generate_r1cs_witness();
      gridOffsetEqual.generate_r1cs_witness();
      orderOffsetEqual.generate_r1cs_witness();
      maxLevelEqual.generate_r1cs_witness();
      useAppKeyEqual.generate_r1cs_witness();

      tokenSSEqual.generate_r1cs_witness();
      tokenBBEqual.generate_r1cs_witness();
      tokenSBEqual.generate_r1cs_witness();
      tokenBSEqual.generate_r1cs_witness();

      isForward.generate_r1cs_witness();
      isReverse.generate_r1cs_witness();

      tokenIdValid.generate_r1cs_witness();
      requireTokenIDValid.generate_r1cs_witness();

      gridOrderCheck.generate_r1cs_witness();
      preOrderCompleteCheck.generate_r1cs_witness();
      nextForward.generate_r1cs_witness();

      requireLevelValid.generate_r1cs_witness();

      hash.generate_r1cs_witness();
      verifyHash.generate_r1cs_witness();
    }
    void generate_r1cs_constraints() 
    {

      storageID.generate_r1cs_constraints(true);
      accountID.generate_r1cs_constraints(true);
      tokenS.generate_r1cs_constraints(true);
      tokenB.generate_r1cs_constraints(true);
      amountS.generate_r1cs_constraints(true);
      amountB.generate_r1cs_constraints(true);
      validUntil.generate_r1cs_constraints(true);
      feeTokenID.generate_r1cs_constraints(true);
      maxFee.generate_r1cs_constraints(true);
      fillAmountBorS.generate_r1cs_constraints(true);
      orderType.generate_r1cs_constraints(true);
      gridOffset.generate_r1cs_constraints(true);
      orderOffset.generate_r1cs_constraints(true);
      maxLevel.generate_r1cs_constraints(true);
      useAppKey.generate_r1cs_constraints(true);

      isAutoMarketBuyOrder.generate_r1cs_constraints();
      isAutoMarketSellOrder.generate_r1cs_constraints();
      isAutoMarketOrder.generate_r1cs_constraints();

      autoMarketStorageID.generate_r1cs_constraints();

      tokenSIDForStorageUpdate.generate_r1cs_constraints();
      tokenBIDForStorageUpdate.generate_r1cs_constraints();

      storageIDEqual.generate_r1cs_constraints();
      accountIDEqual.generate_r1cs_constraints();
      feeTokenIDEqual.generate_r1cs_constraints();
      maxFeeEqual.generate_r1cs_constraints();
      validUntilEqual.generate_r1cs_constraints();
      takerEqual.generate_r1cs_constraints();
      orderTypeEqual.generate_r1cs_constraints();
      gridOffsetEqual.generate_r1cs_constraints();
      orderOffsetEqual.generate_r1cs_constraints();
      maxLevelEqual.generate_r1cs_constraints();
      useAppKeyEqual.generate_r1cs_constraints();

      tokenSSEqual.generate_r1cs_constraints();
      tokenBBEqual.generate_r1cs_constraints();
      tokenSBEqual.generate_r1cs_constraints();
      tokenBSEqual.generate_r1cs_constraints();

      isForward.generate_r1cs_constraints();
      isReverse.generate_r1cs_constraints();

      tokenIdValid.generate_r1cs_constraints();
      requireTokenIDValid.generate_r1cs_constraints();

      gridOrderCheck.generate_r1cs_constraints();
      preOrderCompleteCheck.generate_r1cs_constraints();
      nextForward.generate_r1cs_constraints();

      requireLevelValid.generate_r1cs_constraints();

      hash.generate_r1cs_constraints();
      verifyHash.generate_r1cs_constraints();
    }

    // const VariableT &getAutoMarketIDForStorageUpdate() const
    // {
    //     return autoMarketIDForStorageUpdate.result();
    // }

    // const VariableT &getStorageIDForStorageUpdate() const
    // {
    //     return storageIDForUpdate.result();
    // }

    const VariableT &getNewForwardForStorageUpdate() const
    {
        return nextForward.result();
    }

    const VariableT &getVerifyHash() const
    {
        return verifyHash.result();
    }

    const VariableT &isNewOrder() const
    {
        return preOrderCompleteCheck.getIsNewOrder();
    }

    const VariableT &getIsAutoMarketOrder() const
    {
        return isAutoMarketOrder.result();
    }

    const VariableT &getTokenSIDForStorageUpdate() const
    {
        return tokenSIDForStorageUpdate.result();
    }

    const VariableT &getTokenBIDForStorageUpdate() const
    {
        return tokenBIDForStorageUpdate.result();
    }
};



} // namespace Loopring



#endif
