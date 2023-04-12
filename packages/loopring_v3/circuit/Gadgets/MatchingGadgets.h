// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
#ifndef _MATCHINGGADGETS_H_
#define _MATCHINGGADGETS_H_

#include "../Utils/Constants.h"
#include "../Utils/Data.h"
#include "MathGadgets.h"
#include "OrderGadgets.h"
#include "StorageGadgets.h"

#include "ethsnarks.hpp"
#include "utils.hpp"
#include "gadgets/subadd.hpp"

using namespace ethsnarks;

namespace Loopring
{

// Checks if the fill rate <= 0.1% worse than the target rate
// (fillAmountS/fillAmountB) * 1000 <= (amountS/amountB) * 1001
// (fillAmountS * amountB * 1000) <= (fillAmountB * amountS * 1001)
// Also checks that not just a single fill is non-zero.
class RequireFillRateGadget : public GadgetT
{
  public:
    UnsafeMulGadget fillAmountS_mul_amountB;
    UnsafeMulGadget fillAmountS_mul_amountB_mul_1000;
    UnsafeMulGadget fillAmountB_mul_amountS;
    UnsafeMulGadget fillAmountB_mul_amountS_mul_1001;
    // RequireLeqGadget validRate;
    IfThenRequireLeqGadget validRate;

    IsNonZero isNonZeroFillAmountS;
    IsNonZero isNonZeroFillAmountB;
    AndGadget fillsNonZero;
    NotGadget isZeroFillAmountS;
    NotGadget isZeroFillAmountB;
    AndGadget fillsZero;
    OrGadget fillsValid;
    // RequireEqualGadget requireFillsValid;
    IfThenRequireEqualGadget requireFillsValid;

    RequireFillRateGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &amountS,
      const VariableT &amountB,
      const VariableT &fillAmountS,
      const VariableT &fillAmountB,
      unsigned int n,
      const VariableT &verify,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          fillAmountS_mul_amountB( //
            pb,
            fillAmountS,
            amountB,
            FMT(prefix, ".fillAmountS_mul_amountB")),
          fillAmountS_mul_amountB_mul_1000(
            pb,
            fillAmountS_mul_amountB.result(),
            constants._1000,
            FMT(prefix, ".fillAmountS_mul_amountB_mul_1000")),
          fillAmountB_mul_amountS(pb, fillAmountB, amountS, FMT(prefix, ".fillAmountB_mul_amountS")),
          fillAmountB_mul_amountS_mul_1001(
            pb,
            fillAmountB_mul_amountS.result(),
            constants._1001,
            FMT(prefix, ".fillAmountB_mul_amountS_mul_1001")),
          validRate(
            pb,
            constants,
            verify,
            fillAmountS_mul_amountB_mul_1000.result(),
            fillAmountB_mul_amountS_mul_1001.result(),
            n * 2 + 10 /*=ceil(log2(1000))*/,
            FMT(prefix, ".validRate")),

          // Also enforce that either both fill amounts are zero or both are
          // non-zero.
          isNonZeroFillAmountS(pb, fillAmountS, FMT(prefix, "isNonZeroFillAmountS")),
          isNonZeroFillAmountB(pb, fillAmountB, FMT(prefix, "isNonZeroFillAmountB")),
          fillsNonZero(pb, {isNonZeroFillAmountS.result(), isNonZeroFillAmountB.result()}, FMT(prefix, "fillsNonZero")),
          isZeroFillAmountS(pb, isNonZeroFillAmountS.result(), FMT(prefix, "isZeroFillAmountS")),
          isZeroFillAmountB(pb, isNonZeroFillAmountB.result(), FMT(prefix, "isZeroFillAmountB")),
          fillsZero(pb, {isZeroFillAmountS.result(), isZeroFillAmountB.result()}, FMT(prefix, "fillsZero")),
          fillsValid(pb, {fillsNonZero.result(), fillsZero.result()}, FMT(prefix, "fillsValid")),
          requireFillsValid(pb, verify, fillsValid.result(), constants._1, FMT(prefix, "requireFillsValid"))
    {
    }

    void generate_r1cs_witness()
    {
        fillAmountS_mul_amountB.generate_r1cs_witness();
        fillAmountS_mul_amountB_mul_1000.generate_r1cs_witness();
        fillAmountB_mul_amountS.generate_r1cs_witness();
        fillAmountB_mul_amountS_mul_1001.generate_r1cs_witness();
        validRate.generate_r1cs_witness();

        isNonZeroFillAmountS.generate_r1cs_witness();
        isNonZeroFillAmountB.generate_r1cs_witness();
        fillsNonZero.generate_r1cs_witness();
        isZeroFillAmountS.generate_r1cs_witness();
        isZeroFillAmountB.generate_r1cs_witness();
        fillsZero.generate_r1cs_witness();
        fillsValid.generate_r1cs_witness();
        requireFillsValid.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        fillAmountS_mul_amountB.generate_r1cs_constraints();
        fillAmountS_mul_amountB_mul_1000.generate_r1cs_constraints();
        fillAmountB_mul_amountS.generate_r1cs_constraints();
        fillAmountB_mul_amountS_mul_1001.generate_r1cs_constraints();
        validRate.generate_r1cs_constraints();

        isNonZeroFillAmountS.generate_r1cs_constraints();
        isNonZeroFillAmountB.generate_r1cs_constraints();
        fillsNonZero.generate_r1cs_constraints();
        isZeroFillAmountS.generate_r1cs_constraints();
        isZeroFillAmountB.generate_r1cs_constraints();
        fillsZero.generate_r1cs_constraints();
        fillsValid.generate_r1cs_constraints();
        requireFillsValid.generate_r1cs_constraints();
    }
};

// Check if an order is filled correctly
class RequireValidOrderGadget : public GadgetT
{
  public:
    // RequireLtGadget requireValidUntil;
    IfThenRequireLtGadget requireValidUntil;
    // DEG-146 order cancel
    // RequireEqualGadget notCancelled;
    IfThenRequireEqualGadget notCancelled;

    RequireValidOrderGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &timestamp,
      // DEG-146 order cancel
      const VariableT &cancelled,
      const OrderGadget &order,
      const VariableT &verify,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          requireValidUntil(
            pb,
            constants,
            verify,
            timestamp,
            order.validUntil.packed,
            NUM_BITS_TIMESTAMP,
            FMT(prefix, ".requireValidUntil")),
          // DEG-146 order cancel
          notCancelled(pb, verify, cancelled, constants._0, FMT(prefix, ".notCancelled"))
    {
    }

    void generate_r1cs_witness()
    {
        requireValidUntil.generate_r1cs_witness();
        // DEG-146 order cancel
        notCancelled.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        requireValidUntil.generate_r1cs_constraints();
        // DEG-146 order cancel
        notCancelled.generate_r1cs_constraints();
    }
};

// Calculates the fees for an order
// split trading fee and gas fee, now, just need calculate trading fee
class FeeCalculatorGadget : public GadgetT
{
  public:
    // split trading fee and gas fee
    // MulDivGadget protocolFee;
    MulDivGadget fee;

    FeeCalculatorGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &amountB,
      // const VariableT &protocolFeeBips,
      const VariableT &feeBips,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          // split trading fee and gas fee
          // protocolFee(
          //   pb,
          //   constants,
          //   amountB,
          //   protocolFeeBips,
          //   constants._100000,
          //   NUM_BITS_AMOUNT,
          //   NUM_BITS_PROTOCOL_FEE_BIPS,
          //   17 /*=ceil(log2(100000))*/,
          //   FMT(prefix, ".protocolFee")),
          fee(
            pb,
            constants,
            amountB,
            feeBips,
            constants._10000,
            NUM_BITS_AMOUNT,
            NUM_BITS_BIPS,
            14 /*=ceil(log2(10000))*/,
            FMT(prefix, ".fee"))
    {
    }

    void generate_r1cs_witness()
    {
        // split trading fee and gas fee
        // protocolFee.generate_r1cs_witness();
        fee.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        // split trading fee and gas fee
        // protocolFee.generate_r1cs_constraints();
        fee.generate_r1cs_constraints();
    }

    const VariableT &getFee() const
    {
        return fee.result();
    }
};

// Checks if the order isn't filled too much
class RequireFillLimitGadget : public GadgetT
{
  public:
    TernaryGadget fillAmount;
    TernaryGadget fillLimit;
    AddGadget filledAfter;
    // RequireLeqGadget filledAfter_leq_fillLimit;
    IfThenRequireLeqGadget filledAfter_leq_fillLimit;

    RequireFillLimitGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const OrderGadget &order,
      const VariableT &filled,
      const VariableT &fillS,
      const VariableT &fillB,
      const VariableT &verify,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          fillAmount(pb, order.fillAmountBorS.packed, fillB, fillS, FMT(prefix, ".fillAmount")),
          fillLimit(
            pb,
            order.fillAmountBorS.packed,
            order.amountB.packed,
            order.amountS.packed,
            FMT(prefix, ".fillLimit")),
          filledAfter(pb, filled, fillAmount.result(), NUM_BITS_AMOUNT, FMT(prefix, ".filledAfter")),
          filledAfter_leq_fillLimit(
            pb,
            constants,
            verify,
            filledAfter.result(),
            fillLimit.result(),
            NUM_BITS_AMOUNT,
            FMT(prefix, ".filledAfter_leq_fillLimit"))
    {
    }

    void generate_r1cs_witness()
    {
        fillAmount.generate_r1cs_witness();
        fillLimit.generate_r1cs_witness();
        filledAfter.generate_r1cs_witness();
        filledAfter_leq_fillLimit.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        fillAmount.generate_r1cs_constraints();
        fillLimit.generate_r1cs_constraints();
        filledAfter.generate_r1cs_constraints();
        filledAfter_leq_fillLimit.generate_r1cs_constraints();
    }

    const VariableT &getFilledAfter() const
    {
        return filledAfter.result();
    }
};

// Checks if the order requirements are fulfilled with the given fill amounts
class RequireOrderFillsGadget : public GadgetT
{
  public:
    // Check rate
    // Check whether the filling proportion is normal, that is, whether the filling price is normal
    RequireFillRateGadget requireFillRate;
    // Check fill limit
    // Check whether the filled currency quantity is less than or equal to the currency quantity
    RequireFillLimitGadget requireFillLimit;

    RequireOrderFillsGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const OrderGadget &order,
      const VariableT &filled,
      const VariableT &fillS,
      const VariableT &fillB,
      const VariableT &verify,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          // Check rate
          requireFillRate(
            pb,
            constants,
            order.amountS.packed,
            order.amountB.packed,
            fillS,
            fillB,
            NUM_BITS_AMOUNT,
            verify,
            FMT(prefix, ".requireFillRate")),
          // Check fill limit
          requireFillLimit(pb, constants, order, filled, fillS, fillB, verify, FMT(prefix, ".requireFillLimit"))
    {
    }

    void generate_r1cs_witness()
    {
        requireFillRate.generate_r1cs_witness();
        requireFillLimit.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        requireFillRate.generate_r1cs_constraints();
        requireFillLimit.generate_r1cs_constraints();
    }

    const VariableT &getFilledAfter() const
    {
        return requireFillLimit.getFilledAfter();
    }
};

// Checks if the order requirements are fulfilled with the given fill amounts
class RequireValidTakerGadget : public GadgetT
{
  public:
    EqualGadget takerMatches;
    EqualGadget takerOpen;
    OrGadget valid;
    RequireEqualGadget requireValid;

    RequireValidTakerGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &taker,
      const VariableT &expectedTaker,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          takerMatches(pb, taker, expectedTaker, FMT(prefix, ".takerMatches")),
          takerOpen(pb, constants._0, expectedTaker, FMT(prefix, ".takerOpen")),
          valid(pb, {takerMatches.result(), takerOpen.result()}, FMT(prefix, ".valid")),
          requireValid(pb, valid.result(), constants._1, FMT(prefix, ".requireValid"))
    {
    }

    void generate_r1cs_witness()
    {
        takerMatches.generate_r1cs_witness();
        takerOpen.generate_r1cs_witness();
        valid.generate_r1cs_witness();
        requireValid.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        takerMatches.generate_r1cs_constraints();
        takerOpen.generate_r1cs_constraints();
        valid.generate_r1cs_constraints();
        requireValid.generate_r1cs_constraints();
    }
};

// Matches 2 orders
class OrderMatchingGadget : public GadgetT
{
  public:
    // Verify the order fills
    RequireOrderFillsGadget requireOrderFillsA;
    RequireOrderFillsGadget requireOrderFillsB;

    // Check if tokenS/tokenB match
    RequireEqualGadget orderA_tokenS_eq_orderB_tokenB;
    RequireEqualGadget orderA_tokenB_eq_orderB_tokenS;

    // Check if the takers match
    RequireValidTakerGadget validateTakerA;
    RequireValidTakerGadget validateTakerB;

    // Check if the orders are valid
    RequireValidOrderGadget requireValidA;
    RequireValidOrderGadget requireValidB;

    OrderMatchingGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &timestamp,
      const OrderGadget &orderA,
      const OrderGadget &orderB,
      const VariableT &ownerA,
      const VariableT &ownerB,
      const VariableT &filledA,
      const VariableT &filledB,
      // DEG-146 order cancel
      const VariableT &cancelledA,
      const VariableT &cancelledB,
      const VariableT &fillS_A,
      const VariableT &fillS_B,
      // It only applies to the canceled judgment and has no effect on other judgments
      const VariableT &verify,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          // Check if the fills are valid for the orders
          requireOrderFillsA(pb, constants, orderA, filledA, fillS_A, fillS_B, constants._1, FMT(prefix, ".requireOrderFillsA")),
          requireOrderFillsB(pb, constants, orderB, filledB, fillS_B, fillS_A, constants._1, FMT(prefix, ".requireOrderFillsB")),

          // Check if tokenS/tokenB match
          orderA_tokenS_eq_orderB_tokenB(
            pb,
            orderA.tokenS.packed,
            orderB.tokenB.packed,
            FMT(prefix, ".orderA_tokenS_eq_orderB_tokenB")),
          orderA_tokenB_eq_orderB_tokenS(
            pb,
            orderA.tokenB.packed,
            orderB.tokenS.packed,
            FMT(prefix, ".orderA_tokenB_eq_orderB_tokenS")),

          // Check if the takers match
          validateTakerA(pb, constants, ownerB, orderA.taker, FMT(prefix, ".validateTakerA")),
          validateTakerB(pb, constants, ownerA, orderB.taker, FMT(prefix, ".validateTakerB")),

          // Check if the orders in the settlement are correctly filled
          // DEG-146 order cancel
          requireValidA(pb, constants, timestamp, cancelledA, orderA, verify, FMT(prefix, ".checkValidA")),
          requireValidB(pb, constants, timestamp, cancelledB, orderB, verify, FMT(prefix, ".checkValidB"))
    {
    }

    void generate_r1cs_witness()
    {
        // Check if the fills are valid for the orders
        LOG(LogDebug, "in OrderMatchingGadget before requireOrderFillsA", "");
        requireOrderFillsA.generate_r1cs_witness();
        LOG(LogDebug, "in OrderMatchingGadget before requireOrderFillsB", "");
        requireOrderFillsB.generate_r1cs_witness();

        // Check if tokenS/tokenB match
        orderA_tokenS_eq_orderB_tokenB.generate_r1cs_witness();
        orderA_tokenB_eq_orderB_tokenS.generate_r1cs_witness();

        // Check if the takers match
        validateTakerA.generate_r1cs_witness();
        validateTakerB.generate_r1cs_witness();

        // Check if the orders in the settlement are correctly filled
        requireValidA.generate_r1cs_witness();
        requireValidB.generate_r1cs_witness();
        LOG(LogDebug, "in OrderMatchingGadget end", "");
    }

    void generate_r1cs_constraints()
    {
        // Check if the fills are valid for the orders
        requireOrderFillsA.generate_r1cs_constraints();
        requireOrderFillsB.generate_r1cs_constraints();

        // Check if tokenS/tokenB match
        orderA_tokenS_eq_orderB_tokenB.generate_r1cs_constraints();
        orderA_tokenB_eq_orderB_tokenS.generate_r1cs_constraints();

        // Check if the takers match
        validateTakerA.generate_r1cs_constraints();
        validateTakerB.generate_r1cs_constraints();

        // Check if the orders in the settlement are correctly filled
        requireValidA.generate_r1cs_constraints();
        requireValidB.generate_r1cs_constraints();
    }

    const VariableT &getFilledAfter_A() const
    {
        return requireOrderFillsA.getFilledAfter();
    }

    const VariableT &getFilledAfter_B() const
    {
        return requireOrderFillsB.getFilledAfter();
    }
};
// TODO Why does NOOP not need these tests? Theoretically, if it is NOOP, it should fill in the default data, 
//      which is the same as the performance of spottradecircuit in other transactions
// Answer: because a new storage will be allocated to ordinary transactions, the data will be in the status of 0, 
//    but the aggregated transaction is not. The storage allocation of the aggregated transaction is in the operator. 
//    It is not known which storageid is empty, so storageid = 0 will be allocated. This location has a high probability of value
//1: check whether it is NOOP. If it is NOOP, these tests are not required
//2: order filled test: can you eat so many orders
//3: detect talker, which must be 0
//4: check whether it has been cancelled or timed out
class BatchOrderMatchingGadget: public GadgetT
{
  public:
    // Verify the order fills
    RequireOrderFillsGadget requireOrderFills;

    // // Check if tokenS/tokenB match
    // RequireEqualGadget orderA_tokenS_eq_orderB_token;

    // Aggregation transaction cannot determine who the taker is, so the taker of aggregation transaction must be empty
    // RequireEqualGadget validTaker;
    IfThenRequireEqualGadget validTaker;

    // Check if the orders are valid
    RequireValidOrderGadget requireValid;

    BatchOrderMatchingGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &timestamp,
      const OrderGadget &order,
      const VariableT &filled,
      const VariableT &cancelled,
      const VariableT &deltaFilledS,
      const VariableT &deltaFilledB,
      const VariableT &verify,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          // Check if the fills are valid for the orders
          requireOrderFills(pb, constants, order, filled, deltaFilledS, deltaFilledB, verify, FMT(prefix, ".requireOrderFills")),

          // Check taker must be 0
          validTaker(pb, verify, order.taker, constants._0, FMT(prefix, ".validTaker")),

          // Check if the orders in the settlement are correctly filled
          requireValid(pb, constants, timestamp, cancelled, order, verify, FMT(prefix, ".requireValid"))
    {
    }

    void generate_r1cs_witness()
    {
        // Check if the fills are valid for the orders
        requireOrderFills.generate_r1cs_witness();

        // // Check if tokenS/tokenB match
        // orderA_tokenS_eq_orderB_tokenB.generate_r1cs_witness();

        // Check if the takers match
        validTaker.generate_r1cs_witness();

        // Check if the orders in the settlement are correctly filled
        requireValid.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        // Check if the fills are valid for the orders
        requireOrderFills.generate_r1cs_constraints();

        // // Check if tokenS/tokenB match
        // orderA_tokenS_eq_orderB_tokenB.generate_r1cs_constraints();

        // Check if the takers match
        validTaker.generate_r1cs_constraints();

        // Check if the orders in the settlement are correctly filled
        requireValid.generate_r1cs_constraints();
    }


    const VariableT &getFilledAfter() const
    {
        return requireOrderFills.getFilledAfter();
    }

    // const VariableT &getFilledAfter_A() const
    // {
    //     return requireOrderFillsA.getFilledAfter();
    // }

    // const VariableT &getFilledAfter_B() const
    // {
    //     return requireOrderFillsB.getFilledAfter();
    // }
};

// split trading fee and gas fee - add up gas
// Note that maxfee needs to check the binary digits in advance
class GasFeeMatchingGadget: public GadgetT 
{
  public:
    // Sum first
    AddGadget feeSum;
    // RequireLeqGadget feeSum_leq_max;
    IfThenRequireLeqGadget feeSum_leq_max;
    IfThenRequireLeqGadget appointTradingFee_leq_calculate;
    GasFeeMatchingGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &fee,
      const VariableT &feeSumHistory,
      const VariableT &maxFee,
      const VariableT &calculateTradingFee,
      const VariableT &appointTradingFee,
      const VariableT &verify,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          feeSum(pb, fee, feeSumHistory, NUM_BITS_AMOUNT, FMT(prefix, ".fee + feeSumHistory")),
          feeSum_leq_max(pb, constants, verify, feeSum.result(), maxFee, NUM_BITS_AMOUNT, FMT(prefix, ".feeSum <= maxFee")),
          appointTradingFee_leq_calculate(pb, constants, verify, appointTradingFee, calculateTradingFee, NUM_BITS_AMOUNT, FMT(prefix, ".appointTradingFee <= calculate"))
    {
    }
    void generate_r1cs_witness()
    {
        feeSum.generate_r1cs_witness();
        feeSum_leq_max.generate_r1cs_witness();
        appointTradingFee_leq_calculate.generate_r1cs_witness();
    }
    void generate_r1cs_constraints()
    {
        feeSum.generate_r1cs_constraints();
        feeSum_leq_max.generate_r1cs_constraints();
        appointTradingFee_leq_calculate.generate_r1cs_constraints();
    }

    const VariableT &getFeeSum() const
    {
        return feeSum.result();
    }
};

} // namespace Loopring

#endif
