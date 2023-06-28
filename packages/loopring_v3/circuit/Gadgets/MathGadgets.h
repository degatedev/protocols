// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
// Modified by DeGate DAO, 2022
#ifndef _MATHGADGETS_H_
#define _MATHGADGETS_H_

#include "../Utils/Constants.h"
#include "../Utils/Data.h"

#include "ethsnarks.hpp"
#include "utils.hpp"
#include "jubjub/point.hpp"
#include "jubjub/eddsa.hpp"
#include "gadgets/subadd.hpp"
#include "gadgets/poseidon.hpp"

using namespace ethsnarks;
using namespace jubjub;

namespace Loopring
{

// All Poseidon permutations used
using Poseidon_2 = Poseidon_gadget_T<3, 1, 6, 51, 2, 1>;
template <unsigned n_outputs> using Poseidon_4_ = Poseidon_gadget_T<5, 1, 6, 52, n_outputs, 1>;
using Poseidon_4 = Poseidon_4_<4>;
using Poseidon_5 = Poseidon_gadget_T<6, 1, 6, 52, 5, 1>;
using Poseidon_6 = Poseidon_gadget_T<7, 1, 6, 52, 6, 1>;
using Poseidon_7 = Poseidon_gadget_T<8, 1, 6, 53, 7, 1>;
using Poseidon_8 = Poseidon_gadget_T<9, 1, 6, 53, 8, 1>;
using Poseidon_9 = Poseidon_gadget_T<10, 1, 6, 53, 9, 1>;
using Poseidon_10 = Poseidon_gadget_T<11, 1, 6, 53, 10, 1>;
using Poseidon_11 = Poseidon_gadget_T<12, 1, 6, 53, 11, 1>;
using Poseidon_12 = Poseidon_gadget_T<13, 1, 6, 53, 12, 1>;
using Poseidon_13 = Poseidon_gadget_T<14, 1, 6, 53, 13, 1>;
using Poseidon_14 = Poseidon_gadget_T<15, 1, 6, 53, 14, 1>;
using Poseidon_15 = Poseidon_gadget_T<16, 1, 6, 53, 15, 1>;
using Poseidon_16 = Poseidon_gadget_T<17, 1, 6, 53, 16, 1>;
// using Poseidon_17 = Poseidon_gadget_T<18, 1, 6, 53, 17, 1>;
using Poseidon_18 = Poseidon_gadget_T<19, 1, 6, 53, 18, 1>;

// require(A == B)
static void requireEqual( //
  ProtoboardT &pb,
  const VariableT &A,
  const VariableT &B,
  const std::string &annotation_prefix)
{
    pb.add_r1cs_constraint( //
      ConstraintT(A, FieldT::one(), B),
      FMT(annotation_prefix, ".requireEqual"));
}

// Constants stored in a VariableT for ease of use
class Constants : public GadgetT
{
  public:
    const VariableT _0;
    const VariableT _1;
    const VariableT _2;
    const VariableT _3;
    const VariableT _4;
    const VariableT _5;
    const VariableT _6;
    const VariableT _7;
    const VariableT _8;
    const VariableT _9;
    const VariableT _10;
    const VariableT _11;
    const VariableT _12;
    const VariableT _13;
    const VariableT _14;
    const VariableT _15;
    const VariableT _16;
    const VariableT _17;

    const VariableT _100;
    const VariableT _1000;
    const VariableT _1001;
    const VariableT _10000;
    const VariableT _100000;
    const VariableT _2Pow30;
    const VariableT fixedBase;
    const VariableT emptyStorage;
    const VariableT maxAmount;
    const VariableT numStorageSlots;
    const VariableT txTypeSpotTrade;
    const VariableT txTypeBatchSpotTrade;
    const VariableT txTypeTransfer;
    const VariableT txTypeWithdrawal;
    const VariableT txTypeOrderCancel;
    const VariableT txTypeAppKeyUpdate;
    const VariableT feeMultiplier;

    const VariableT depositType;
    const VariableT accountUpdateType;
    const VariableT withdrawType;

    const VariableArrayT zeroAccount;

    std::vector<VariableT> values;

    Constants( //
      ProtoboardT &pb,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          _0(make_variable(pb, FieldT::zero(), FMT(prefix, ".zero"))),
          _1(make_variable(pb, FieldT::one(), FMT(prefix, ".one"))),
          _2(make_variable(pb, ethsnarks::FieldT(2), FMT(prefix, ".two"))),
          _3(make_variable(pb, ethsnarks::FieldT(3), FMT(prefix, ".three"))),
          _4(make_variable(pb, ethsnarks::FieldT(4), FMT(prefix, ".four"))),
          _5(make_variable(pb, ethsnarks::FieldT(5), FMT(prefix, ".five"))),
          _6(make_variable(pb, ethsnarks::FieldT(6), FMT(prefix, ".six"))),
          _7(make_variable(pb, ethsnarks::FieldT(7), FMT(prefix, ".seven"))),
          _8(make_variable(pb, ethsnarks::FieldT(8), FMT(prefix, ".eight"))),
          _9(make_variable(pb, ethsnarks::FieldT(9), FMT(prefix, ".nine"))),
          _10(make_variable(pb, ethsnarks::FieldT(10), FMT(prefix, ".ten"))),
          _11(make_variable(pb, ethsnarks::FieldT(11), FMT(prefix, ".11"))),
          _12(make_variable(pb, ethsnarks::FieldT(12), FMT(prefix, ".12"))),
          _13(make_variable(pb, ethsnarks::FieldT(13), FMT(prefix, ".13"))),
          _14(make_variable(pb, ethsnarks::FieldT(14), FMT(prefix, ".14"))),
          _15(make_variable(pb, ethsnarks::FieldT(15), FMT(prefix, ".15"))),
          _16(make_variable(pb, ethsnarks::FieldT(16), FMT(prefix, ".16"))),
          _17(make_variable(pb, ethsnarks::FieldT(17), FMT(prefix, ".17"))),

          _100(make_variable(pb, ethsnarks::FieldT(100), FMT(prefix, "._100"))),
          _1000(make_variable(pb, ethsnarks::FieldT(1000), FMT(prefix, "._1000"))),
          _1001(make_variable(pb, ethsnarks::FieldT(1001), FMT(prefix, "._1001"))),
          _10000(make_variable(pb, ethsnarks::FieldT(10000), FMT(prefix, "._10000"))),
          _100000(make_variable(pb, ethsnarks::FieldT(100000), FMT(prefix, "._100000"))),
          _2Pow30(make_variable(pb, ethsnarks::FieldT(1073741824), FMT(prefix, "._2Pow30"))),
          fixedBase(make_variable(pb, ethsnarks::FieldT(FIXED_BASE), FMT(prefix, ".fixedBase"))),
          emptyStorage(make_variable(pb, ethsnarks::FieldT(EMPTY_TRADE_HISTORY), FMT(prefix, ".emptyStorage"))),
          maxAmount(make_variable(pb, ethsnarks::FieldT(MAX_AMOUNT), FMT(prefix, ".maxAmount"))),
          numStorageSlots(make_variable(pb, ethsnarks::FieldT(NUM_STORAGE_SLOTS), FMT(prefix, ".numStorageSlots"))),
          txTypeSpotTrade(
            make_variable(pb, ethsnarks::FieldT(int(TransactionType::SpotTrade)), FMT(prefix, ".txTypeSpotTrade"))),
          txTypeBatchSpotTrade(
            make_variable(pb, ethsnarks::FieldT(int(TransactionType::BatchSpotTrade)), FMT(prefix, ".txTypeBatchSpotTrade"))),
          
          txTypeTransfer(
            make_variable(pb, ethsnarks::FieldT(int(TransactionType::Transfer)), FMT(prefix, ".txTypeTransfer"))),
          txTypeWithdrawal(
            make_variable(pb, ethsnarks::FieldT(int(TransactionType::Withdrawal)), FMT(prefix, ".txTypeWithdrawal"))),
          txTypeOrderCancel(
            make_variable(pb, ethsnarks::FieldT(int(TransactionType::OrderCancel)), FMT(prefix, ".txTypeOrderCancel"))),
          txTypeAppKeyUpdate(
            make_variable(pb, ethsnarks::FieldT(int(TransactionType::AppKeyUpdate)), FMT(prefix, ".txTypeAppKeyUpdate"))),
          feeMultiplier(make_variable(pb, ethsnarks::FieldT(FEE_MULTIPLIER), FMT(prefix, ".feeMultiplier"))),

          depositType(make_variable(pb, ethsnarks::FieldT(6), FMT(prefix, ".depositType"))),
          accountUpdateType(make_variable(pb, ethsnarks::FieldT(7), FMT(prefix, ".accountUpdateType"))),
          withdrawType(make_variable(pb, ethsnarks::FieldT(8), FMT(prefix, ".withdrawType"))),

          zeroAccount(NUM_BITS_ACCOUNT, _0)
    {
        assert(NUM_BITS_MAX_VALUE == FieldT::size_in_bits());
        assert(NUM_BITS_FIELD_CAPACITY == FieldT::capacity());

        values.push_back(_0);
        values.push_back(_1);
        values.push_back(_2);
        values.push_back(_3);
        values.push_back(_4);
        values.push_back(_5);
        values.push_back(_6);
        values.push_back(_7);
        values.push_back(_8);
        values.push_back(_9);
        values.push_back(_10);
        values.push_back(_11);
        values.push_back(_12);
        values.push_back(_13);
        values.push_back(_14);
        values.push_back(_15);
        values.push_back(_16);
        values.push_back(_17);
    }

    void generate_r1cs_witness()
    {
    }

    void generate_r1cs_constraints()
    {
        pb.add_r1cs_constraint(ConstraintT(_0, FieldT::one(), FieldT::zero()), ".zero");
        pb.add_r1cs_constraint(ConstraintT(_1, FieldT::one(), FieldT::one()), ".one");
        pb.add_r1cs_constraint(ConstraintT(_2, FieldT::one(), FieldT(2)), ".two");
        pb.add_r1cs_constraint(ConstraintT(_3, FieldT::one(), FieldT(3)), ".three");
        pb.add_r1cs_constraint(ConstraintT(_4, FieldT::one(), FieldT(4)), ".four");
        pb.add_r1cs_constraint(ConstraintT(_5, FieldT::one(), FieldT(5)), ".five");
        pb.add_r1cs_constraint(ConstraintT(_6, FieldT::one(), FieldT(6)), ".six");
        pb.add_r1cs_constraint(ConstraintT(_7, FieldT::one(), FieldT(7)), ".seven");
        pb.add_r1cs_constraint(ConstraintT(_8, FieldT::one(), FieldT(8)), ".eight");
        pb.add_r1cs_constraint(ConstraintT(_9, FieldT::one(), FieldT(9)), ".nine");
        pb.add_r1cs_constraint(ConstraintT(_10, FieldT::one(), FieldT(10)), ".ten");
        pb.add_r1cs_constraint(ConstraintT(_11, FieldT::one(), FieldT(11)), ".11");
        pb.add_r1cs_constraint(ConstraintT(_12, FieldT::one(), FieldT(12)), ".12");
        pb.add_r1cs_constraint(ConstraintT(_13, FieldT::one(), FieldT(13)), ".13");
        pb.add_r1cs_constraint(ConstraintT(_14, FieldT::one(), FieldT(14)), ".14");
        pb.add_r1cs_constraint(ConstraintT(_15, FieldT::one(), FieldT(15)), ".15");
        pb.add_r1cs_constraint(ConstraintT(_16, FieldT::one(), FieldT(16)), ".16");
        pb.add_r1cs_constraint(ConstraintT(_17, FieldT::one(), FieldT(17)), ".17");

        pb.add_r1cs_constraint(ConstraintT(_100, FieldT::one(), ethsnarks::FieldT(100)), "._100");
        pb.add_r1cs_constraint(ConstraintT(_1000, FieldT::one(), ethsnarks::FieldT(1000)), "._1000");
        pb.add_r1cs_constraint(ConstraintT(_1001, FieldT::one(), ethsnarks::FieldT(1001)), "._1001");
        pb.add_r1cs_constraint(ConstraintT(_10000, FieldT::one(), ethsnarks::FieldT(10000)), "._10000");
        pb.add_r1cs_constraint(ConstraintT(_100000, FieldT::one(), ethsnarks::FieldT(100000)), "._100000");
        pb.add_r1cs_constraint(ConstraintT(_2Pow30, FieldT::one(), ethsnarks::FieldT(1073741824)), "._2Pow30");
        pb.add_r1cs_constraint(ConstraintT(fixedBase, FieldT::one(), ethsnarks::FieldT(FIXED_BASE)), ".fixedBase");
        pb.add_r1cs_constraint(
          ConstraintT(emptyStorage, FieldT::one(), ethsnarks::FieldT(EMPTY_TRADE_HISTORY)), ".emptyStorage");
        pb.add_r1cs_constraint(ConstraintT(maxAmount, FieldT::one(), ethsnarks::FieldT(MAX_AMOUNT)), ".maxAmount");
        pb.add_r1cs_constraint(
          ConstraintT(numStorageSlots, FieldT::one(), ethsnarks::FieldT(NUM_STORAGE_SLOTS)), ".numStorageSlots");
        pb.add_r1cs_constraint(
          ConstraintT(txTypeSpotTrade, FieldT::one(), ethsnarks::FieldT(int(TransactionType::SpotTrade))),
          ".txTypeSpotTrade");
        pb.add_r1cs_constraint(ConstraintT(txTypeBatchSpotTrade, FieldT::one(), ethsnarks::FieldT(int(TransactionType::BatchSpotTrade))), ".txTypeBatchSpotTrade");
        pb.add_r1cs_constraint(
          ConstraintT(txTypeTransfer, FieldT::one(), ethsnarks::FieldT(int(TransactionType::Transfer))),
          ".txTypeTransfer");
        pb.add_r1cs_constraint(
          ConstraintT(txTypeWithdrawal, FieldT::one(), ethsnarks::FieldT(int(TransactionType::Withdrawal))),
          ".txTypeWithdrawal");
        pb.add_r1cs_constraint(
          ConstraintT(txTypeOrderCancel, FieldT::one(), ethsnarks::FieldT(int(TransactionType::OrderCancel))),
          ".txTypeOrderCancel");
        pb.add_r1cs_constraint(
          ConstraintT(txTypeAppKeyUpdate, FieldT::one(), ethsnarks::FieldT(int(TransactionType::AppKeyUpdate))),
          ".txTypeAppKeyUpdate");
          
        pb.add_r1cs_constraint(
          ConstraintT(feeMultiplier, FieldT::one(), ethsnarks::FieldT(FEE_MULTIPLIER)), ".feeMultiplier");

        pb.add_r1cs_constraint(ConstraintT(depositType, FieldT::one(), FieldT(6)), ".depositType");
        pb.add_r1cs_constraint(ConstraintT(accountUpdateType, FieldT::one(), FieldT(7)), ".accountUpdateType");
        pb.add_r1cs_constraint(ConstraintT(withdrawType, FieldT::one(), FieldT(8)), ".withdrawType");
        
    }
};

class DualVariableGadget : public libsnark::dual_variable_gadget<FieldT>
{
  public:
    DualVariableGadget( //
      ProtoboardT &pb,
      const size_t width,
      const std::string &prefix)
        : libsnark::dual_variable_gadget<FieldT>(pb, width, prefix)
    {
    }

    void generate_r1cs_witness( //
      ProtoboardT &pb,
      const FieldT &value)
    {
        pb.val(packed) = value;
        generate_r1cs_witness_from_packed();
    }

    void generate_r1cs_witness( //
      ProtoboardT &pb,
      const LimbT &value)
    {
        assert(value.max_bits() == 256);
        for (unsigned int i = 0; i < 256; i++)
        {
            pb.val(bits[255 - i]) = value.test_bit(i);
        }
        generate_r1cs_witness_from_bits();
    }

    void generate_r1cs_constraints(bool enforce = true)
    {
        libsnark::dual_variable_gadget<FieldT>::generate_r1cs_constraints(enforce);
    }
};

class ToBitsGadget : public libsnark::dual_variable_gadget<FieldT>
{
  public:
    ToBitsGadget( //
      ProtoboardT &pb,
      const VariableT &value,
      const size_t width,
      const std::string &prefix)
        : libsnark::dual_variable_gadget<FieldT>(pb, value, width, prefix)
    {
    }

    void generate_r1cs_witness()
    {
        generate_r1cs_witness_from_packed();
    }

    void generate_r1cs_constraints()
    {
        libsnark::dual_variable_gadget<FieldT>::generate_r1cs_constraints(true);
    }
};

typedef ToBitsGadget RangeCheckGadget;

class FromBitsGadget : public libsnark::dual_variable_gadget<FieldT>
{
  public:
    FromBitsGadget( //
      ProtoboardT &pb,
      const VariableArrayT &bits,
      const std::string &prefix)
        : libsnark::dual_variable_gadget<FieldT>(pb, bits, prefix)
    {
    }

    void generate_r1cs_witness()
    {
        generate_r1cs_witness_from_bits();
    }

    void generate_r1cs_constraints(bool enforce = true)
    {
        libsnark::dual_variable_gadget<FieldT>::generate_r1cs_constraints(enforce);
    }
};

// Helper function that contains the history of all the values of a variable
class DynamicVariableGadget : public GadgetT
{
  public:
    std::vector<VariableT> variables;

    DynamicVariableGadget( //
      ProtoboardT &pb,
      const VariableT &initialValue,
      const std::string &prefix)
        : GadgetT(pb, prefix)
    {
        add(initialValue);
    }

    const VariableT &front() const
    {
        return variables.front();
    }

    const VariableT &back() const
    {
        return variables.back();
    }

    void add(const VariableT &variable)
    {
        variables.push_back(variable);
    }
};

// A - B
class UnsafeSubGadget : public GadgetT
{
  public:
    VariableT value;
    VariableT sub;
    VariableT sum;

    UnsafeSubGadget( //
      ProtoboardT &pb,
      const VariableT &_value,
      const VariableT &_sub,
      const std::string &prefix)
        : GadgetT( //
            pb,
            prefix),
          value(_value),
          sub(_sub),
          sum(make_variable(pb, FMT(prefix, ".sum")))
    {
    }

    const VariableT &result() const
    {
        return sum;
    }

    void generate_r1cs_witness()
    {
        pb.val(sum) = pb.val(value) - pb.val(sub);
    }

    void generate_r1cs_constraints()
    {
        pb.add_r1cs_constraint(
          ConstraintT( //
            value - sub,
            FieldT::one(),
            sum),
          FMT(annotation_prefix, ".value - sub = sum"));
    }
};

// A + B
class UnsafeAddGadget : public GadgetT
{
  public:
    VariableT value;
    VariableT add;
    VariableT sum;

    UnsafeAddGadget( //
      ProtoboardT &pb,
      const VariableT &_value,
      const VariableT &_add,
      const std::string &prefix)
        : GadgetT( //
            pb,
            prefix),
          value(_value),
          add(_add),
          sum(make_variable(pb, FMT(prefix, ".sum")))
    {
    }

    const VariableT &result() const
    {
        return sum;
    }

    void generate_r1cs_witness()
    {
        pb.val(sum) = pb.val(value) + pb.val(add);
    }

    void generate_r1cs_constraints()
    {
        pb.add_r1cs_constraint(
          ConstraintT( //
            value + add,
            FieldT::one(),
            sum),
          FMT(annotation_prefix, ".value + add = sum"));
    }
};

// A * B
class UnsafeMulGadget : public GadgetT
{
  public:
    VariableT valueA;
    VariableT valueB;
    VariableT product;

    UnsafeMulGadget( //
      ProtoboardT &pb,
      const VariableT &_valueA,
      const VariableT &_valueB,
      const std::string &prefix)
        : GadgetT( //
            pb,
            prefix),
          valueA(_valueA),
          valueB(_valueB),
          product(make_variable(pb, FMT(prefix, ".product")))
    {
    }

    const VariableT &result() const
    {
        return product;
    }

    void generate_r1cs_witness()
    {
        pb.val(product) = pb.val(valueA) * pb.val(valueB);
    }

    void generate_r1cs_constraints()
    {
        pb.add_r1cs_constraint( //
          ConstraintT(valueA, valueB, product),
          ".valueA * valueB = product");
    }
};

// A * B
class SafeMulGadget : public GadgetT
{
  public:
    UnsafeMulGadget unsafeMul;
    RangeCheckGadget rangeCheck;

    SafeMulGadget( //
      ProtoboardT &pb,
      const VariableT &_valueA,
      const VariableT &_valueB,
      unsigned int n,
      const std::string &prefix)
        : GadgetT( //
            pb,
            prefix),
          unsafeMul(pb, _valueA, _valueB, FMT(prefix, ".unsafeMul")),
          rangeCheck(pb, unsafeMul.product, n, FMT(prefix, ".rangeCheck"))
    {
    }

    const VariableT &result() const
    {
        return unsafeMul.product;
    }

    void generate_r1cs_witness()
    {
        unsafeMul.generate_r1cs_witness();
        rangeCheck.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        unsafeMul.generate_r1cs_constraints();
        rangeCheck.generate_r1cs_constraints();
    }
};

// A + B = sum with A, B and sum < 2^n
//
// This gadget is not designed to handle inputs of more than a couple of
// variables. Therefore, we have not optimized the constraints as suggested
// in https://github.com/daira/r1cs/blob/master/zkproofs.pdf.
class AddGadget : public GadgetT
{
  public:
    UnsafeAddGadget unsafeAdd;
    RangeCheckGadget rangeCheck;

    AddGadget( //
      ProtoboardT &pb,
      const VariableT &A,
      const VariableT &B,
      unsigned int n,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          unsafeAdd(pb, A, B, FMT(prefix, ".unsafeAdd")),
          rangeCheck(pb, unsafeAdd.result(), n, FMT(prefix, ".rangeCheck"))
    {
        assert(n + 1 <= NUM_BITS_FIELD_CAPACITY);
    }

    void generate_r1cs_witness()
    {
        unsafeAdd.generate_r1cs_witness();
        rangeCheck.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        unsafeAdd.generate_r1cs_constraints();
        rangeCheck.generate_r1cs_constraints();
    }

    const VariableT &result() const
    {
        return unsafeAdd.result();
    }
};

// A - B = sub with A, B and sub >= 0
class SubGadget : public GadgetT
{
  public:
    UnsafeSubGadget unsafeSub;
    RangeCheckGadget rangeCheck;

    SubGadget( //
      ProtoboardT &pb,
      const VariableT &A,
      const VariableT &B,
      unsigned int n,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          unsafeSub(pb, A, B, FMT(prefix, ".unsafeAdd")),
          rangeCheck(pb, unsafeSub.result(), n, FMT(prefix, ".rangeCheck"))
    {
        assert(n + 1 <= NUM_BITS_FIELD_CAPACITY);
    }

    void generate_r1cs_witness()
    {
        unsafeSub.generate_r1cs_witness();
        rangeCheck.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        unsafeSub.generate_r1cs_constraints();
        rangeCheck.generate_r1cs_constraints();
    }

    const VariableT &result() const
    {
        return unsafeSub.result();
    }
};

// Helper function to do transfers
class TransferGadget : public GadgetT
{
  public:
    SubGadget sub;
    AddGadget add;

    TransferGadget(
      ProtoboardT &pb,
      DynamicVariableGadget &from,
      DynamicVariableGadget &to,
      const VariableT &value,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          sub(pb, from.back(), value, NUM_BITS_AMOUNT_MAX, FMT(prefix, ".sub")),
          add(pb, to.back(), value, NUM_BITS_AMOUNT_MAX, FMT(prefix, ".add"))
    {
        from.add(sub.result());
        to.add(add.result());
    }

    void generate_r1cs_witness()
    {
        sub.generate_r1cs_witness();
        add.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        sub.generate_r1cs_constraints();
        add.generate_r1cs_constraints();
    }
};
// Balance changes, new additions and subtractions
class BalanceExchangeGadget : public GadgetT
{
  public:
    AddGadget add;
    SubGadget sub;

    BalanceExchangeGadget(
      ProtoboardT &pb,
      DynamicVariableGadget &from,
      const VariableT &increase,
      const VariableT &reduce,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          add(pb, from.back(), increase, NUM_BITS_AMOUNT_MAX, FMT(prefix, ".add")),
          sub(pb, add.result(), reduce, NUM_BITS_AMOUNT_MAX, FMT(prefix, ".sub"))
    {
        // Keep the final result
        from.add(sub.result());
    }

    void generate_r1cs_witness()
    {
        add.generate_r1cs_witness();
        sub.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        add.generate_r1cs_constraints();
        sub.generate_r1cs_constraints();
    }
};
class BalanceReduceGadget : public GadgetT
{
  public:
    SubGadget sub;

    BalanceReduceGadget(
      ProtoboardT &pb,
      DynamicVariableGadget &from,
      const VariableT &reduce,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          sub(pb, from.back(), reduce, NUM_BITS_AMOUNT_MAX, FMT(prefix, ".sub"))
    {
        // Keep the final result
        from.add(sub.result());
    }

    void generate_r1cs_witness()
    {
        sub.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        sub.generate_r1cs_constraints();
    }
};
class BalanceIncreaseGadget : public GadgetT
{
  public:
    AddGadget add;

    BalanceIncreaseGadget(
      ProtoboardT &pb,
      DynamicVariableGadget &from,
      const VariableT &increase,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          add(pb, from.back(), increase, NUM_BITS_AMOUNT_MAX, FMT(prefix, ".add"))
    {
        // Keep the final result
        from.add(add.result());
    }

    void generate_r1cs_witness()
    {
        add.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        add.generate_r1cs_constraints();
    }

    const VariableT &result() const
    {
        return add.result();
    }
};
// b ? A : B
class TernaryGadget : public GadgetT
{
  public:
    VariableT b;
    VariableT x;
    VariableT y;

    VariableT selected;

    TernaryGadget(
      ProtoboardT &pb,
      const VariableT &_b,
      const VariableT &_x,
      const VariableT &_y,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          b(_b),
          x(_x),
          y(_y),

          selected(make_variable(pb, FMT(prefix, ".selected")))
    {
    }

    const VariableT &result() const
    {
        return selected;
    }

    void generate_r1cs_witness()
    {
        pb.val(selected) = (pb.val(b) == FieldT::one()) ? pb.val(x) : pb.val(y);
    }

    void generate_r1cs_constraints(bool enforceBitness = true)
    {
        if (enforceBitness)
        {
            libsnark::generate_boolean_r1cs_constraint<ethsnarks::FieldT>(pb, b, FMT(annotation_prefix, ".bitness"));
        }
        pb.add_r1cs_constraint(
          ConstraintT(b, y - x, y - selected), FMT(annotation_prefix, ".b * (y - x) == (y - selected)"));
    }
};

// b ? A[] : B[]
class ArrayTernaryGadget : public GadgetT
{
  public:
    VariableT b;
    std::vector<TernaryGadget> results;
    VariableArrayT res;

    ArrayTernaryGadget(
      ProtoboardT &pb,
      const VariableT &_b,
      const VariableArrayT &x,
      const VariableArrayT &y,
      const std::string &prefix)
        : GadgetT(pb, prefix), b(_b)
    {
        assert(x.size() == y.size());
        results.reserve(x.size());
        for (unsigned int i = 0; i < x.size(); i++)
        {
            results.emplace_back(pb, b, x[i], y[i], FMT(prefix, ".results"));
            res.emplace_back(results.back().result());
        }
    }

    void generate_r1cs_witness()
    {
        for (unsigned int i = 0; i < results.size(); i++)
        {
            results[i].generate_r1cs_witness();
        }
    }

    void generate_r1cs_constraints(bool enforceBitness = true)
    {
        if (enforceBitness)
        {
            libsnark::generate_boolean_r1cs_constraint<ethsnarks::FieldT>(pb, b, FMT(annotation_prefix, ".bitness"));
        }
        for (unsigned int i = 0; i < results.size(); i++)
        {
            results[i].generate_r1cs_constraints(false);
        }
    }

    const VariableArrayT &result() const
    {
        return res;
    }
};

// b ? A[][] : B[][]
class VectorArrayTernaryGadget : public GadgetT
{
  public:
    VariableT b;
    std::vector<ArrayTernaryGadget> results;
    std::vector<VariableArrayT> res;

    VectorArrayTernaryGadget(
      ProtoboardT &pb,
      const VariableT &_b,
      const std::vector<VariableArrayT> &x,
      const std::vector<VariableArrayT> &y,
      const std::string &prefix)
        : GadgetT(pb, prefix), b(_b)
    {
        assert(x.size() == y.size());
        results.reserve(x.size());
        for (unsigned int i = 0; i < x.size(); i++)
        {
            results.emplace_back(pb, b, x[i], y[i], FMT(prefix, ".results"));
            res.emplace_back(results.back().result());
        }
    }

    void generate_r1cs_witness()
    {
        for (unsigned int i = 0; i < results.size(); i++)
        {
            results[i].generate_r1cs_witness();
        }
    }

    void generate_r1cs_constraints(bool enforceBitness = true)
    {
        if (enforceBitness)
        {
            libsnark::generate_boolean_r1cs_constraint<ethsnarks::FieldT>(pb, b, FMT(annotation_prefix, ".bitness"));
        }
        for (unsigned int i = 0; i < results.size(); i++)
        {
            results[i].generate_r1cs_constraints(false);
        }
    }

    const std::vector<VariableArrayT> &result() const
    {
        return res;
    }
};

// (input[0] && input[1] && ...) (all inputs need to be boolean)
class AndGadget : public GadgetT
{
  public:
    std::vector<VariableT> inputs;
    std::vector<VariableT> results;

    AndGadget( //
      ProtoboardT &pb,
      const std::vector<VariableT> &_inputs,
      const std::string &prefix)
        : GadgetT(pb, prefix), inputs(_inputs)
    {
        assert(inputs.size() > 1);
        for (unsigned int i = 1; i < inputs.size(); i++)
        {
            results.emplace_back(make_variable(pb, FMT(prefix, ".results")));
        }
    }

    const VariableT &result() const
    {
        return results.back();
    }

    void generate_r1cs_witness()
    {
        pb.val(results[0]) = pb.val(inputs[0]) * pb.val(inputs[1]);
        for (unsigned int i = 2; i < inputs.size(); i++)
        {
            pb.val(results[i - 1]) = pb.val(results[i - 2]) * pb.val(inputs[i]);
        }
    }

    void generate_r1cs_constraints()
    {
        // This can be done more efficiently but since we never have any long inputs, it is not needed
        if (inputs.size() > 3)
        {
            std::cout << "[AndGadget] unexpected input length " << inputs.size() << std::endl;
        }
        pb.add_r1cs_constraint(ConstraintT(inputs[0], inputs[1], results[0]), FMT(annotation_prefix, ".A && B"));
        for (unsigned int i = 2; i < inputs.size(); i++)
        {
            pb.add_r1cs_constraint(
              ConstraintT(inputs[i], results[i - 2], results[i - 1]), FMT(annotation_prefix, ".A && B"));
        }
    }
};
class AndThreeGadget : public GadgetT
{
  public:
    std::vector<VariableT> inputs;
    std::vector<VariableT> results;

    AndThreeGadget( //
      ProtoboardT &pb,
      const VariableT &first,
      const VariableT &second,
      const VariableT &three,
      const std::string &prefix)
        : GadgetT(pb, prefix)
    {   
        inputs.emplace_back(first);
        inputs.emplace_back(second);
        inputs.emplace_back(three);
        for (unsigned int i = 1; i < inputs.size(); i++)
        {
            results.emplace_back(make_variable(pb, FMT(prefix, ".results")));
        }
    }

    const VariableT &result() const
    {
        return results.back();
    }

    void generate_r1cs_witness()
    {
        pb.val(results[0]) = pb.val(inputs[0]) * pb.val(inputs[1]);
        for (unsigned int i = 2; i < inputs.size(); i++)
        {
            pb.val(results[i - 1]) = pb.val(results[i - 2]) * pb.val(inputs[i]);
        }
    }

    void generate_r1cs_constraints()
    {
        // This can be done more efficiently but since we never have any long inputs, it is not needed
        if (inputs.size() > 3)
        {
            std::cout << "[AndGadget] unexpected input length " << inputs.size() << std::endl;
        }
        pb.add_r1cs_constraint(ConstraintT(inputs[0], inputs[1], results[0]), FMT(annotation_prefix, ".A && B"));
        for (unsigned int i = 2; i < inputs.size(); i++)
        {
            pb.add_r1cs_constraint(
              ConstraintT(inputs[i], results[i - 2], results[i - 1]), FMT(annotation_prefix, ".A && B"));
        }
    }
};
class AndTwoGadget : public GadgetT
{
  public:
    std::vector<VariableT> inputs;
    std::vector<VariableT> results;

    AndTwoGadget( //
      ProtoboardT &pb,
      const VariableT &first,
      const VariableT &second,
      const std::string &prefix)
        : GadgetT(pb, prefix)
    {
        inputs.emplace_back(first);
        inputs.emplace_back(second);
        for (unsigned int i = 1; i < inputs.size(); i++)
        {
            results.emplace_back(make_variable(pb, FMT(prefix, ".results")));
        }
    }

    const VariableT &result() const
    {
        return results.back();
    }

    void generate_r1cs_witness()
    {
        pb.val(results[0]) = pb.val(inputs[0]) * pb.val(inputs[1]);
        for (unsigned int i = 2; i < inputs.size(); i++)
        {
            pb.val(results[i - 1]) = pb.val(results[i - 2]) * pb.val(inputs[i]);
        }
    }

    void generate_r1cs_constraints()
    {
        // This can be done more efficiently but since we never have any long inputs, it is not needed
        if (inputs.size() > 3)
        {
            std::cout << "[AndGadget] unexpected input length " << inputs.size() << std::endl;
        }
        pb.add_r1cs_constraint(ConstraintT(inputs[0], inputs[1], results[0]), FMT(annotation_prefix, ".A && B"));
        for (unsigned int i = 2; i < inputs.size(); i++)
        {
            pb.add_r1cs_constraint(
              ConstraintT(inputs[i], results[i - 2], results[i - 1]), FMT(annotation_prefix, ".A && B"));
        }
    }
};

// (input[0] || input[1] || ...) (all inputs need to be boolean)
class OrGadget : public GadgetT
{
  public:
    std::vector<VariableT> inputs;
    std::vector<VariableT> results;

    OrGadget( //
      ProtoboardT &pb,
      const std::vector<VariableT> &_inputs,
      const std::string &prefix)
        : GadgetT(pb, prefix), inputs(_inputs)
    {
        assert(inputs.size() > 1);
        for (unsigned int i = 1; i < inputs.size(); i++)
        {
            results.emplace_back(make_variable(pb, FMT(prefix, ".results")));
        }
    }

    const VariableT &result() const
    {
        return results.back();
    }

    void generate_r1cs_witness()
    {
        pb.val(results[0]) = FieldT::one() - (FieldT::one() - pb.val(inputs[0])) * (FieldT::one() - pb.val(inputs[1]));
        for (unsigned int i = 2; i < inputs.size(); i++)
        {
            pb.val(results[i - 1]) =
              FieldT::one() - (FieldT::one() - pb.val(results[i - 2])) * (FieldT::one() - pb.val(inputs[i]));
        }
    }

    void generate_r1cs_constraints()
    {
        if (inputs.size() > 3)
        {
            std::cout << "[OrGadget] unexpected input length " << inputs.size() << std::endl;
        }

        pb.add_r1cs_constraint(
          ConstraintT(FieldT::one() - inputs[0], FieldT::one() - inputs[1], FieldT::one() - results[0]),
          FMT(annotation_prefix, ".A || B == _or"));
        for (unsigned int i = 2; i < inputs.size(); i++)
        {
            pb.add_r1cs_constraint(
              ConstraintT(FieldT::one() - inputs[i], FieldT::one() - results[i - 2], FieldT::one() - results[i - 1]),
              FMT(annotation_prefix, ".A || B == _or"));
        }
    }
};

class OrThreeGadget : public GadgetT
{
  public:
    std::vector<VariableT> inputs;
    std::vector<VariableT> results;

    OrThreeGadget( //
      ProtoboardT &pb,
      const VariableT &first,
      const VariableT &second,
      const VariableT &third,
      const std::string &prefix)
        : GadgetT(pb, prefix)
    {
        inputs.emplace_back(first);
        inputs.emplace_back(second);
        inputs.emplace_back(third);
        for (unsigned int i = 1; i < inputs.size(); i++)
        {
            results.emplace_back(make_variable(pb, FMT(prefix, ".results")));
        }
    }

    const VariableT &result() const
    {
        return results.back();
    }

    void generate_r1cs_witness()
    {
        pb.val(results[0]) = FieldT::one() - (FieldT::one() - pb.val(inputs[0])) * (FieldT::one() - pb.val(inputs[1]));
        for (unsigned int i = 2; i < inputs.size(); i++)
        {
            pb.val(results[i - 1]) =
              FieldT::one() - (FieldT::one() - pb.val(results[i - 2])) * (FieldT::one() - pb.val(inputs[i]));
        }
    }

    void generate_r1cs_constraints()
    {
        if (inputs.size() > 3)
        {
            std::cout << "[OrGadget] unexpected input length " << inputs.size() << std::endl;
        }

        pb.add_r1cs_constraint(
          ConstraintT(FieldT::one() - inputs[0], FieldT::one() - inputs[1], FieldT::one() - results[0]),
          FMT(annotation_prefix, ".A || B == _or"));
        for (unsigned int i = 2; i < inputs.size(); i++)
        {
            pb.add_r1cs_constraint(
              ConstraintT(FieldT::one() - inputs[i], FieldT::one() - results[i - 2], FieldT::one() - results[i - 1]),
              FMT(annotation_prefix, ".A || B == _or"));
        }
    }
};

// !A (A needs to be boolean)
class NotGadget : public GadgetT
{
  public:
    VariableT A;
    VariableT _not;

    NotGadget( //
      ProtoboardT &pb,
      const VariableT &_A,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          A(_A),
          _not(make_variable(pb, FMT(prefix, "._not")))
    {
    }

    const VariableT &result() const
    {
        return _not;
    }

    void generate_r1cs_witness()
    {
        pb.val(_not) = FieldT::one() - pb.val(A);
    }

    void generate_r1cs_constraints(bool enforceBitness = true)
    {
        if (enforceBitness)
        {
            libsnark::generate_boolean_r1cs_constraint<ethsnarks::FieldT>(pb, A, FMT(annotation_prefix, ".bitness"));
        }
        pb.add_r1cs_constraint(
          ConstraintT(FieldT::one() - A, FieldT::one(), _not), FMT(annotation_prefix, ".!A == _not"));
    }
};

// A[i] ^ B[i]
class XorArrayGadget : public GadgetT
{
  public:
    VariableArrayT A;
    VariableArrayT B;
    VariableArrayT C;

    XorArrayGadget( //
      ProtoboardT &pb,
      VariableArrayT _A,
      VariableArrayT _B,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          A(_A),
          B(_B),

          C(make_var_array(pb, A.size(), FMT(prefix, ".C")))
    {
        assert(A.size() == B.size());
    }

    const VariableArrayT &result() const
    {
        return C;
    }

    void generate_r1cs_witness()
    {
        for (unsigned int i = 0; i < C.size(); i++)
        {
            pb.val(C[i]) =
              pb.val(A[i]) + pb.val(B[i]) - ((pb.val(A[i]) == FieldT::one() && pb.val(B[i]) == FieldT::one()) ? 2 : 0);
        }
    }

    void generate_r1cs_constraints()
    {
        for (unsigned int i = 0; i < C.size(); i++)
        {
            pb.add_r1cs_constraint(
              ConstraintT(2 * A[i], B[i], A[i] + B[i] - C[i]), FMT(annotation_prefix, ".A ^ B == C"));
        }
    }
};

// (A == B)
class EqualGadget : public GadgetT
{
  public:
    UnsafeSubGadget difference;
    IsNonZero isNonZeroDifference;
    NotGadget isZeroDifference;

    EqualGadget( //
      ProtoboardT &pb,
      const VariableT &A,
      const VariableT &B,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          difference(pb, A, B, FMT(prefix, ".difference")),
          isNonZeroDifference(pb, difference.result(), FMT(prefix, ".isNonZeroDifference")),
          isZeroDifference(pb, isNonZeroDifference.result(), FMT(prefix, ".isZeroDifference"))
    {
    }

    const VariableT &result() const
    {
        return isZeroDifference.result();
    }

    void generate_r1cs_witness()
    {
        difference.generate_r1cs_witness();
        isNonZeroDifference.generate_r1cs_witness();
        isZeroDifference.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        difference.generate_r1cs_constraints();
        isNonZeroDifference.generate_r1cs_constraints();
        isZeroDifference.generate_r1cs_constraints();
    }
};

// require(A == B)
class RequireEqualGadget : public GadgetT
{
  public:
    VariableT A;
    VariableT B;

    RequireEqualGadget( //
      ProtoboardT &pb,
      const VariableT &_A,
      const VariableT &_B,
      const std::string &prefix)
        : GadgetT(pb, prefix), A(_A), B(_B)
    {
    }

    void generate_r1cs_witness()
    {

        // ASSERT(pb.val(A) == pb.val(B), annotation_prefix);
    }

    void generate_r1cs_constraints()
    {
        requireEqual(pb, A, B, FMT(annotation_prefix, ".requireEqual"));
    }
};
// (A != B)
class NotEqualGadget : public GadgetT
{
  public:
    EqualGadget equal;
    NotGadget notEqual;

    NotEqualGadget( //
      ProtoboardT &pb,
      const VariableT &A,
      const VariableT &B,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          equal(pb, A, B, FMT(prefix, ".equal")),
          notEqual(pb, equal.result(), FMT(prefix, ".notEqual"))
    {
    }

    const VariableT &result() const
    {
        return notEqual.result();
    }

    void generate_r1cs_witness()
    {
        equal.generate_r1cs_witness();
        notEqual.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        equal.generate_r1cs_constraints();
        notEqual.generate_r1cs_constraints();
    }
};

// require(A == 0 || B == 0)
class RequireZeroAorBGadget : public GadgetT
{
  public:
    VariableT A;
    VariableT B;

    RequireZeroAorBGadget( //
      ProtoboardT &pb,
      const VariableT &_A,
      const VariableT &_B,
      const std::string &prefix)
        : GadgetT(pb, prefix), A(_A), B(_B)
    {
    }

    void generate_r1cs_witness()
    {

        // ASSERT(pb.val(A) == FieldT::zero() || pb.val(B) == FieldT::zero(), annotation_prefix);
    }

    void generate_r1cs_constraints()
    {
        pb.add_r1cs_constraint(ConstraintT(A, B, FieldT::zero()), FMT(annotation_prefix, ".A == 0 || B == 0"));
    }
};

// require(A != 0)
class RequireNotZeroGadget : public GadgetT
{
  public:
    VariableT A;
    VariableT A_inv;

    RequireNotZeroGadget( //
      ProtoboardT &pb,
      const VariableT &_A,
      const std::string &prefix)
        : GadgetT(pb, prefix), A(_A), A_inv(make_variable(pb, FMT(prefix, ".A_inv")))
    {
    }

    void generate_r1cs_witness()
    {
        pb.val(A_inv) = pb.val(A).inverse();
        // ASSERT(pb.val(A) != FieldT::one(), annotation_prefix);
    }

    void generate_r1cs_constraints()
    {
        pb.add_r1cs_constraint(ConstraintT(A, A_inv, FieldT::one()), FMT(annotation_prefix, ".A * A_inv == 1"));
    }
};

// require(A != B)
class RequireNotEqualGadget : public GadgetT
{
  public:
    VariableT A;
    VariableT B;

    VariableT difference;
    RequireNotZeroGadget notZero;

    RequireNotEqualGadget( //
      ProtoboardT &pb,
      const VariableT &_A,
      const VariableT &_B,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          A(_A),
          B(_B),
          difference(make_variable(pb, FMT(prefix, ".difference"))),
          notZero(pb, difference, FMT(prefix, ".difference != 0"))
    {
    }

    void generate_r1cs_witness()
    {
        pb.val(difference) = pb.val(A) - pb.val(B);
        notZero.generate_r1cs_witness();
        // ASSERT(pb.val(A) != pb.val(B), annotation_prefix);
    }

    void generate_r1cs_constraints()
    {
        pb.add_r1cs_constraint(
          ConstraintT(A - B, FieldT::one(), difference), FMT(annotation_prefix, ".A - B == difference"));
        notZero.generate_r1cs_constraints();
    }
};

// (A <(=) B)
class LeqGadget : public GadgetT
{
  public:
    VariableT _lt;
    VariableT _leq;
    libsnark::comparison_gadget<ethsnarks::FieldT> comparison;
    NotGadget _gt;
    NotGadget _gte;
    AndGadget _eq;

    LeqGadget( //
      ProtoboardT &pb,
      const VariableT &A,
      const VariableT &B,
      const size_t n,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          _lt(make_variable(pb, 1, FMT(prefix, ".lt"))),
          _leq(make_variable(pb, 1, FMT(prefix, ".leq"))),
          comparison(pb, n, A, B, _lt, _leq, FMT(prefix, ".A <(=) B")),
          _gt(pb, _leq, FMT(prefix, ".gt")),
          _gte(pb, _lt, FMT(prefix, ".gte")),
          _eq(pb, {_leq, _gte.result()}, FMT(prefix, ".eq"))
    {
        // The comparison gadget is only guaranteed to work correctly on values in
        // the field capacity - 1
        // assert(n <= NUM_BITS_FIELD_CAPACITY - 1);

        ASSERT(n <= NUM_BITS_FIELD_CAPACITY - 1, prefix);
    }

    const VariableT &lt() const
    {
        return _lt;
    }

    const VariableT &leq() const
    {
        return _leq;
    }

    const VariableT &eq() const
    {
        return _eq.result();
    }

    const VariableT &gte() const
    {
        return _gte.result();
    }

    const VariableT &gt() const
    {
        return _gt.result();
    }

    void generate_r1cs_witness()
    {
        comparison.generate_r1cs_witness();
        _gt.generate_r1cs_witness();
        _gte.generate_r1cs_witness();
        _eq.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        comparison.generate_r1cs_constraints();
        _gt.generate_r1cs_constraints();
        _gte.generate_r1cs_constraints();
        _eq.generate_r1cs_constraints();
    }
};

// (A < B)
class LtFieldGadget : public GadgetT
{
  public:
    field2bits_strict Abits;
    field2bits_strict Bbits;
    FromBitsGadget Alo;
    FromBitsGadget Ahi;
    FromBitsGadget Blo;
    FromBitsGadget Bhi;
    LeqGadget partLo;
    LeqGadget partHi;
    TernaryGadget res;

    LtFieldGadget( //
      ProtoboardT &pb,
      const VariableT &A,
      const VariableT &B,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          Abits(pb, A, FMT(prefix, ".Abits")),
          Bbits(pb, B, FMT(prefix, ".Bbits")),

          Alo(pb, subArray(Abits.result(), 0, 254 / 2), FMT(prefix, ".Alo")),
          Ahi(pb, subArray(Abits.result(), 254 / 2, 254 / 2), FMT(prefix, ".Ahi")),
          Blo(pb, subArray(Bbits.result(), 0, 254 / 2), FMT(prefix, ".Blo")),
          Bhi(pb, subArray(Bbits.result(), 254 / 2, 254 / 2), FMT(prefix, ".Bhi")),
          partLo(pb, Alo.packed, Blo.packed, 254 / 2, FMT(prefix, ".partLo")),
          partHi(pb, Ahi.packed, Bhi.packed, 254 / 2, FMT(prefix, ".partHi")),
          res(pb, partHi.eq(), partLo.lt(), partHi.lt(), FMT(prefix, ".res"))
    {
    }

    const VariableT &lt() const
    {
        return res.result();
    }

    void generate_r1cs_witness()
    {
        Abits.generate_r1cs_witness();
        Bbits.generate_r1cs_witness();
        Alo.generate_r1cs_witness();
        Ahi.generate_r1cs_witness();
        Blo.generate_r1cs_witness();
        Bhi.generate_r1cs_witness();
        partLo.generate_r1cs_witness();
        partHi.generate_r1cs_witness();
        res.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        Abits.generate_r1cs_constraints();
        Bbits.generate_r1cs_constraints();
        Alo.generate_r1cs_constraints(false);
        Ahi.generate_r1cs_constraints(false);
        Blo.generate_r1cs_constraints(false);
        Bhi.generate_r1cs_constraints(false);
        partLo.generate_r1cs_constraints();
        partHi.generate_r1cs_constraints();
        res.generate_r1cs_constraints();
    }
};

// min(A, B)
class MinGadget : public GadgetT
{
  public:
    LeqGadget A_lt_B;
    TernaryGadget minimum;

    MinGadget( //
      ProtoboardT &pb,
      const VariableT &A,
      const VariableT &B,
      const size_t n,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          A_lt_B(pb, A, B, n, FMT(prefix, ".(A < B)")),
          minimum(pb, A_lt_B.lt(), A, B, FMT(prefix, ".minimum = (A < B) ? A : B"))
    {
    }

    const VariableT &result() const
    {
        return minimum.result();
    }

    void generate_r1cs_witness()
    {
        A_lt_B.generate_r1cs_witness();
        minimum.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        A_lt_B.generate_r1cs_constraints();
        minimum.generate_r1cs_constraints();
    }
};

// max(A, B)
class MaxGadget : public GadgetT
{
  public:
    LeqGadget A_lt_B;
    TernaryGadget maximum;

    MaxGadget( //
      ProtoboardT &pb,
      const VariableT &A,
      const VariableT &B,
      const size_t n,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          A_lt_B(pb, A, B, n, FMT(prefix, ".(A < B)")),
          maximum(pb, A_lt_B.lt(), B, A, FMT(prefix, ".maximum = (A < B) ? B : A"))
    {
    }

    const VariableT &result() const
    {
        return maximum.result();
    }

    void generate_r1cs_witness()
    {
        A_lt_B.generate_r1cs_witness();
        maximum.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        A_lt_B.generate_r1cs_constraints();
        maximum.generate_r1cs_constraints();
    }
};

// require(A <= B)
class RequireLeqGadget : public GadgetT
{
  public:
    LeqGadget leqGadget;

    RequireLeqGadget( //
      ProtoboardT &pb,
      const VariableT &A,
      const VariableT &B,
      const size_t n,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          leqGadget(pb, A, B, n, FMT(prefix, ".leq"))
    {
    }

    void generate_r1cs_witness()
    {
        leqGadget.generate_r1cs_witness();
        // ASSERT(pb.val(leqGadget.leq()) == FieldT::one(), annotation_prefix);
    }

    void generate_r1cs_constraints()
    {
        leqGadget.generate_r1cs_constraints();
        pb.add_r1cs_constraint(
          ConstraintT(leqGadget.leq(), FieldT::one(), FieldT::one()), FMT(annotation_prefix, ".leq == 1"));
    }

    const VariableT &result() const
    {
        return leqGadget.leq();
    }
};

// require(A < B)
class RequireLtGadget : public GadgetT
{
  public:
    LeqGadget leqGadget;

    RequireLtGadget( //
      ProtoboardT &pb,
      const VariableT &A,
      const VariableT &B,
      const size_t n,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          leqGadget(pb, A, B, n, FMT(prefix, ".leq"))
    {
    }

    void generate_r1cs_witness()
    {
        leqGadget.generate_r1cs_witness();
        // ASSERT(pb.val(leqGadget.lt()) == FieldT::one(), annotation_prefix);
    }

    void generate_r1cs_constraints()
    {
        leqGadget.generate_r1cs_constraints();
        pb.add_r1cs_constraint(
          ConstraintT(leqGadget.lt(), FieldT::one(), FieldT::one()), FMT(annotation_prefix, ".lt == 1"));
    }
};

// if (C) then require(A), i.e.,
// require(!C || A)
class IfThenRequireGadget : public GadgetT
{
  public:
    NotGadget notC;
    OrGadget res;

    IfThenRequireGadget( //
      ProtoboardT &pb,
      const VariableT &C,
      const VariableT &A,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          notC(pb, C, FMT(prefix, ".notC")),
          res(pb, {notC.result(), A}, FMT(prefix, ".res"))
    {
    }

    void generate_r1cs_witness()
    {
        notC.generate_r1cs_witness();
        res.generate_r1cs_witness();
        ASSERT(pb.val(res.result()) == FieldT::one(), annotation_prefix);
    }

    void generate_r1cs_constraints()
    {
        notC.generate_r1cs_constraints();
        res.generate_r1cs_constraints();
        pb.add_r1cs_constraint(
          ConstraintT(res.result(), FieldT::one(), FieldT::one()), FMT(annotation_prefix, ".valid"));
    }
};

// if (C) then require(A == B), i.e.,
// require(!C || A == B)
class IfThenRequireEqualGadget : public GadgetT
{
  public:
    EqualGadget eq;
    IfThenRequireGadget res;

    IfThenRequireEqualGadget(
      ProtoboardT &pb,
      const VariableT &C,
      const VariableT &A,
      const VariableT &B,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          eq(pb, A, B, FMT(prefix, ".eq")),
          res(pb, C, eq.result(), FMT(prefix, ".res"))
    {
    }

    void generate_r1cs_witness()
    {
        eq.generate_r1cs_witness();
        res.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        eq.generate_r1cs_constraints();
        res.generate_r1cs_constraints();
    }
};

// if (C) then require(A <= B), i.e.,
// require(!C || A <= B)
class IfThenRequireLeqGadget : public GadgetT
{
  public:
    LeqGadget leq;
    IfThenRequireEqualGadget res;

    IfThenRequireLeqGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &C,
      const VariableT &A,
      const VariableT &B,
      const size_t n,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          leq(pb, A, B, n, FMT(prefix, ".eq")),
          res(pb, C, leq.leq(), constants._1, FMT(prefix, ".res"))
    {
    }

    void generate_r1cs_witness()
    {
        leq.generate_r1cs_witness();
        res.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        leq.generate_r1cs_constraints();
        res.generate_r1cs_constraints();
        // pb.add_r1cs_constraint(
        //   ConstraintT(leq.leq(), FieldT::one(), FieldT::one()), FMT(annotation_prefix, ".valid"));
    }

    const VariableT &result() const
    {
        return leq.leq();
    }

};

// if (C) then require(A <= B), i.e.,
// require(!C || A <= B)
class IfThenRequireLtGadget : public GadgetT
{
  public:
    LeqGadget leq;
    IfThenRequireEqualGadget res;

    IfThenRequireLtGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &C,
      const VariableT &A,
      const VariableT &B,
      const size_t n,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          leq(pb, A, B, n, FMT(prefix, ".eq")),
          res(pb, C, leq.lt(), constants._1, FMT(prefix, ".res"))
    {
    }

    void generate_r1cs_witness()
    {
        leq.generate_r1cs_witness();
        res.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        leq.generate_r1cs_constraints();
        res.generate_r1cs_constraints();
        // pb.add_r1cs_constraint(
        //   ConstraintT(leq.lt(), FieldT::one(), FieldT::one()), FMT(annotation_prefix, ".valid"));
    }
};

// if (C) then require(A != B), i.e.,
// require(!C || A != B)
class IfThenRequireNotEqualGadget : public GadgetT
{
  public:
    EqualGadget eq;
    NotGadget notEq;
    IfThenRequireGadget res;

    IfThenRequireNotEqualGadget(
      ProtoboardT &pb,
      const VariableT &C,
      const VariableT &A,
      const VariableT &B,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          eq(pb, A, B, FMT(prefix, ".eq")),
          notEq(pb, eq.result(), FMT(prefix, ".notEq")),
          res(pb, C, notEq.result(), FMT(prefix, ".res"))
    {
    }

    void generate_r1cs_witness()
    {
        eq.generate_r1cs_witness();
        notEq.generate_r1cs_witness();
        res.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        eq.generate_r1cs_constraints();
        notEq.generate_r1cs_constraints();
        res.generate_r1cs_constraints();
    }
};

// if (C) then return A + B; else return A
class IfThenAddGadget : public GadgetT
{
  public:
    AddGadget aAddB;
    TernaryGadget ifC_A_Add_B_else_A;

    IfThenAddGadget(
      ProtoboardT &pb,
      const VariableT &C,
      const VariableT &A,
      const VariableT &B,
      unsigned int n,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          aAddB(pb, A, B, n, FMT(prefix, ".aAddB")),
          ifC_A_Add_B_else_A(pb, C, aAddB.result(), A, FMT(prefix, ".ifC_A_Add_B_else_A"))
    {
    }

    void generate_r1cs_witness()
    {
        aAddB.generate_r1cs_witness();
        ifC_A_Add_B_else_A.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        aAddB.generate_r1cs_constraints();
        ifC_A_Add_B_else_A.generate_r1cs_constraints();
    }

    const VariableT &result() const
    {
        return ifC_A_Add_B_else_A.result();
    }
};

// if (C) then return A - B; else return A
class IfThenSubGadget : public GadgetT
{
  public:
    MinGadget min;
    MaxGadget max;
    SubGadget aSubB;
    TernaryGadget ifC_A_Sub_B_else_A;
    IfThenRequireLeqGadget ifC_B_leq_A;

    IfThenSubGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &C,
      const VariableT &A,
      const VariableT &B,
      unsigned int n,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          min(pb, A, B, n, FMT(prefix, ".min")),
          max(pb, A, B, n, FMT(prefix, ".max")),
          aSubB(pb, max.result(), min.result(), n, FMT(prefix, ".aSubB")),
          ifC_A_Sub_B_else_A(pb, C, aSubB.result(), A, FMT(prefix, ".ifC_A_Sub_B_else_A")),
          ifC_B_leq_A(pb, constants, C, B, A, n, FMT(prefix, ".ifC_B_leq_A"))
    {
    }

    void generate_r1cs_witness()
    {
        min.generate_r1cs_witness();
        max.generate_r1cs_witness();
        aSubB.generate_r1cs_witness();
        ifC_A_Sub_B_else_A.generate_r1cs_witness();
        ifC_B_leq_A.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        min.generate_r1cs_constraints();
        max.generate_r1cs_constraints();
        aSubB.generate_r1cs_constraints();
        ifC_A_Sub_B_else_A.generate_r1cs_constraints();
        ifC_B_leq_A.generate_r1cs_constraints();
    }

    const VariableT &result() const
    {
        return ifC_A_Sub_B_else_A.result();
    }
};

// (value * numerator) = product = denominator * quotient + remainder
// product / denominator = quotient
// product % denominator = remainder
class MulDivGadget : public GadgetT
{
  public:
    const VariableT value;
    const VariableT numerator;
    const VariableT denominator;

    const VariableT quotient;

    RequireNotZeroGadget denominator_notZero;
    UnsafeMulGadget product;
    DualVariableGadget remainder;
    RequireLtGadget remainder_lt_denominator;

    MulDivGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &_value,
      const VariableT &_numerator,
      const VariableT &_denominator,
      unsigned int numBitsValue,
      unsigned int numBitsNumerator,
      unsigned int numBitsDenominator,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          value(_value),
          numerator(_numerator),
          denominator(_denominator),

          quotient(make_variable(pb, FMT(prefix, ".quotient"))),

          denominator_notZero(pb, denominator, FMT(prefix, ".denominator_notZero")),
          product(pb, value, numerator, FMT(prefix, ".product")),
          // Range limit the remainder. The comparison below is not guaranteed to
          // work for very large values.
          remainder(pb, numBitsDenominator, FMT(prefix, ".remainder")),
          remainder_lt_denominator(
            pb,
            remainder.packed,
            denominator,
            numBitsDenominator,
            FMT(prefix, ".remainder < denominator"))
    {
        assert(numBitsValue + numBitsNumerator <= NUM_BITS_FIELD_CAPACITY);
    }

    void generate_r1cs_witness()
    {
        denominator_notZero.generate_r1cs_witness();
        product.generate_r1cs_witness();
        if (pb.val(denominator) != FieldT::zero())
        {
            pb.val(quotient) = ethsnarks::FieldT(
              (toBigInt(pb.val(product.result())) / toBigInt(pb.val(denominator))).to_string().c_str());
        }
        else
        {
            pb.val(quotient) = FieldT::zero();
        }
        remainder.generate_r1cs_witness(pb, pb.val(product.result()) - (pb.val(denominator) * pb.val(quotient)));
        remainder_lt_denominator.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        denominator_notZero.generate_r1cs_constraints();
        product.generate_r1cs_constraints();
        pb.add_r1cs_constraint(
          ConstraintT(denominator, quotient, product.result() - remainder.packed),
          FMT(annotation_prefix, ".quotient * denominator == product - remainder"));
        remainder.generate_r1cs_constraints(true);
        remainder_lt_denominator.generate_r1cs_constraints();
    }

    const VariableT &result() const
    {
        return quotient;
    }

    const VariableT &getRemainder() const
    {
        return remainder.packed;
    }

    const VariableT &getProduct() const
    {
        return product.result();
    }
};

// _accuracy.numerator / _accuracy.denominator <=  value / original
// original * _accuracy.numerator <= value * _accuracy.denominator
// We have to make sure there are no overflows and the value is <= the original
// value (so a user never spends more) so we also check:
// - value <= original
// - value < 2^maxNumBits
class RequireAccuracyGadget : public GadgetT
{
  public:
    libsnark::dual_variable_gadget<FieldT> value;
    VariableT original;
    Accuracy accuracy;

    RequireLeqGadget value_leq_original;

    VariableT original_mul_accuracyN;
    VariableT value_mul_accuracyD;

    RequireLeqGadget original_mul_accuracyN_LEQ_value_mul_accuracyD;

    RequireAccuracyGadget(
      ProtoboardT &pb,
      const VariableT &_value,
      const VariableT &_original,
      const Accuracy &_accuracy,
      unsigned int maxNumBits,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          // Range limit the value. The comparison below is not guaranteed to work
          // for very large values.
          value(pb, _value, maxNumBits, FMT(prefix, ".value")),
          original(_original),
          accuracy(_accuracy),

          value_leq_original(pb, value.packed, original, maxNumBits, FMT(prefix, ".value_lt_original")),

          original_mul_accuracyN(make_variable(pb, FMT(prefix, ".original_mul_accuracyN"))),
          value_mul_accuracyD(make_variable(pb, FMT(prefix, ".value_mul_accuracyD"))),

          original_mul_accuracyN_LEQ_value_mul_accuracyD(
            pb,
            original_mul_accuracyN,
            value_mul_accuracyD,
            maxNumBits + 32,
            FMT(prefix, ".original_mul_accuracyN_LEQ_value_mul_accuracyD"))
    {
    }

    void generate_r1cs_witness()
    {
        value.generate_r1cs_witness_from_packed();

        value_leq_original.generate_r1cs_witness();

        pb.val(original_mul_accuracyN) = pb.val(original) * accuracy.numerator;
        pb.val(value_mul_accuracyD) = pb.val(value.packed) * accuracy.denominator;
        original_mul_accuracyN_LEQ_value_mul_accuracyD.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        value.generate_r1cs_constraints(true);

        value_leq_original.generate_r1cs_constraints();

        pb.add_r1cs_constraint(
          ConstraintT(original, accuracy.numerator, original_mul_accuracyN),
          FMT(annotation_prefix, ".original * accuracy.numerator == original_mul_accuracyN"));
        pb.add_r1cs_constraint(
          ConstraintT(value.packed, accuracy.denominator, value_mul_accuracyD),
          FMT(annotation_prefix, ".value * accuracy.denominator == value_mul_accuracyD"));
        original_mul_accuracyN_LEQ_value_mul_accuracyD.generate_r1cs_constraints();
    }
};

class AccuracyGadget : public GadgetT
{
  public:
    libsnark::dual_variable_gadget<FieldT> value;
    VariableT original;
    Accuracy accuracy;

    LeqGadget value_leq_original;

    VariableT original_mul_accuracyN;
    VariableT value_mul_accuracyD;

    LeqGadget original_mul_accuracyN_LEQ_value_mul_accuracyD;

    AndGadget valid;

    AccuracyGadget(
      ProtoboardT &pb,
      const VariableT &_value,
      const VariableT &_original,
      const Accuracy &_accuracy,
      unsigned int maxNumBits,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          // Range limit the value. The comparison below is not guaranteed to work
          // for very large values.
          value(pb, _value, maxNumBits, FMT(prefix, ".value")),
          original(_original),
          accuracy(_accuracy),

          value_leq_original(pb, value.packed, original, maxNumBits, FMT(prefix, ".value_lt_original")),

          original_mul_accuracyN(make_variable(pb, FMT(prefix, ".original_mul_accuracyN"))),
          value_mul_accuracyD(make_variable(pb, FMT(prefix, ".value_mul_accuracyD"))),

          original_mul_accuracyN_LEQ_value_mul_accuracyD(
            pb,
            original_mul_accuracyN,
            value_mul_accuracyD,
            maxNumBits + 32,
            FMT(prefix, ".original_mul_accuracyN_LEQ_value_mul_accuracyD")),
          
          valid(pb, {value_leq_original.leq(), original_mul_accuracyN_LEQ_value_mul_accuracyD.leq()}, FMT(prefix, ".valid"))
    {
    }

    void generate_r1cs_witness()
    {
        value.generate_r1cs_witness_from_packed();

        value_leq_original.generate_r1cs_witness();

        pb.val(original_mul_accuracyN) = pb.val(original) * accuracy.numerator;
        pb.val(value_mul_accuracyD) = pb.val(value.packed) * accuracy.denominator;
        original_mul_accuracyN_LEQ_value_mul_accuracyD.generate_r1cs_witness();

        valid.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        value.generate_r1cs_constraints(true);

        value_leq_original.generate_r1cs_constraints();

        pb.add_r1cs_constraint(
          ConstraintT(original, accuracy.numerator, original_mul_accuracyN),
          FMT(annotation_prefix, ".original * accuracy.numerator == original_mul_accuracyN"));
        pb.add_r1cs_constraint(
          ConstraintT(value.packed, accuracy.denominator, value_mul_accuracyD),
          FMT(annotation_prefix, ".value * accuracy.denominator == value_mul_accuracyD"));
        original_mul_accuracyN_LEQ_value_mul_accuracyD.generate_r1cs_constraints();

        valid.generate_r1cs_constraints();
    }

    const VariableT &result() const
    {
        return valid.result();
    }
};

class BothAccuracyGadget : public GadgetT 
{
  public:
    AccuracyGadget value_original;
    AccuracyGadget original_value;
    OrGadget valid;
    BothAccuracyGadget(
      ProtoboardT &pb,
      const VariableT &value,
      const VariableT &original,
      const Accuracy &accuracy,
      unsigned int maxNumBits,
      const std::string &prefix)
        : GadgetT(pb, prefix),
        value_original(pb, value, original, accuracy, maxNumBits, FMT(prefix, ".value_original")),
        original_value(pb, original, value, accuracy, maxNumBits, FMT(prefix, ".original_value")),
        valid(pb, {value_original.result(), original_value.result()}, FMT(prefix, ".valid"))
    {

    }

    void generate_r1cs_witness() 
    {
      value_original.generate_r1cs_witness();
      original_value.generate_r1cs_witness();
      valid.generate_r1cs_witness();
    }

    void generate_r1cs_constraints() 
    {
      value_original.generate_r1cs_constraints();
      original_value.generate_r1cs_constraints();
      valid.generate_r1cs_constraints();
    }

    const VariableT &result() const
    {
        return valid.result();
    }
};

class IfThenRequireAccuracyGadget : public GadgetT
{
  public:
    libsnark::dual_variable_gadget<FieldT> value;
    VariableT original;
    Accuracy accuracy;

    IfThenRequireLeqGadget value_leq_original;

    VariableT original_mul_accuracyN;
    VariableT value_mul_accuracyD;

    IfThenRequireLeqGadget original_mul_accuracyN_LEQ_value_mul_accuracyD;

    IfThenRequireAccuracyGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &verify,
      const VariableT &_value,
      const VariableT &_original,
      const Accuracy &_accuracy,
      unsigned int maxNumBits,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          // Range limit the value. The comparison below is not guaranteed to work
          // for very large values.
          value(pb, _value, maxNumBits, FMT(prefix, ".value")),
          original(_original),
          accuracy(_accuracy),

          value_leq_original(pb, constants, verify, value.packed, original, maxNumBits, FMT(prefix, ".value_lt_original")),

          original_mul_accuracyN(make_variable(pb, FMT(prefix, ".original_mul_accuracyN"))),
          value_mul_accuracyD(make_variable(pb, FMT(prefix, ".value_mul_accuracyD"))),

          original_mul_accuracyN_LEQ_value_mul_accuracyD(
            pb,
            constants, 
            verify,
            original_mul_accuracyN,
            value_mul_accuracyD,
            maxNumBits + 32,
            FMT(prefix, ".original_mul_accuracyN_LEQ_value_mul_accuracyD"))
    {
    }

    void generate_r1cs_witness()
    {
        value.generate_r1cs_witness_from_packed();

        value_leq_original.generate_r1cs_witness();

        pb.val(original_mul_accuracyN) = pb.val(original) * accuracy.numerator;
        pb.val(value_mul_accuracyD) = pb.val(value.packed) * accuracy.denominator;
        original_mul_accuracyN_LEQ_value_mul_accuracyD.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        value.generate_r1cs_constraints(true);

        value_leq_original.generate_r1cs_constraints();

        pb.add_r1cs_constraint(
          ConstraintT(original, accuracy.numerator, original_mul_accuracyN),
          FMT(annotation_prefix, ".original * accuracy.numerator == original_mul_accuracyN"));
        pb.add_r1cs_constraint(
          ConstraintT(value.packed, accuracy.denominator, value_mul_accuracyD),
          FMT(annotation_prefix, ".value * accuracy.denominator == value_mul_accuracyD"));
        original_mul_accuracyN_LEQ_value_mul_accuracyD.generate_r1cs_constraints();
    }
};

class IfThenRequireBothAccuracyGadget : public GadgetT 
{
  public:
    BothAccuracyGadget valueOriginalValid;
    IfThenRequireGadget requireValid;
    IfThenRequireBothAccuracyGadget(
      ProtoboardT &pb,
      const VariableT &verify,
      const VariableT &value,
      const VariableT &original,
      const Accuracy &accuracy,
      unsigned int maxNumBits,
      const std::string &prefix)
        : GadgetT(pb, prefix),
        valueOriginalValid(pb, value, original, accuracy, maxNumBits, FMT(prefix, ".valueOriginalValid")),
        requireValid(pb, verify, valueOriginalValid.result(), FMT(prefix, ".requireValid"))
    {

    }

    void generate_r1cs_witness() 
    {
      valueOriginalValid.generate_r1cs_witness();
      requireValid.generate_r1cs_witness();
    }

    void generate_r1cs_constraints() 
    {
      valueOriginalValid.generate_r1cs_constraints();
      requireValid.generate_r1cs_constraints();
    }

};

// Public data helper class.
// Will hash all public data with sha256 to a single public input of
// NUM_BITS_FIELD_CAPACITY bits
class PublicDataGadget : public GadgetT
{
  public:
    const VariableT publicInput;
    VariableArrayT publicDataBits;

    std::unique_ptr<sha256_many> hasher;
    std::unique_ptr<FromBitsGadget> calculatedHash;

    PublicDataGadget( //
      ProtoboardT &pb,
      const std::string &prefix)
        : GadgetT(pb, prefix), publicInput(make_variable(pb, FMT(prefix, ".publicInput")))
    {
        pb.set_input_sizes(1);
    }

    void add(const VariableArrayT &bits)
    {
        publicDataBits.insert(publicDataBits.end(), bits.rbegin(), bits.rend());
    }

    void transform(unsigned int start, unsigned int count, unsigned int size)
    {
        VariableArrayT transformedBits;
        transformedBits.reserve(publicDataBits.size());
        for (unsigned int i = 0; i < start; i++)
        {
            transformedBits.emplace_back(publicDataBits[i]);
        }
        // Publicdata data fragmentation
        // The biggest transaction is BatchSpotTrade, which requires 83bytes
        // At present, it is subject to the addition of BatchSpotTrade
        // There are six users in the BatchSpotTrade, and if there are only two tokens for the six users, it needs 80 bytes
        unsigned int sizePart1 = 80 * 8;
        unsigned int sizePart2 = 3 * 8;

        unsigned int startPart1 = start;
        unsigned int startPart2 = startPart1 + sizePart1 * count;

        // Part 1
        for (unsigned int i = 0; i < count; i++)
        {
            for (unsigned int j = 0; j < sizePart1; j++)
            {
                transformedBits.emplace_back(publicDataBits[start + i * size + j]);
            }
        }
        // Part 2
        for (unsigned int i = 0; i < count; i++)
        {
            for (unsigned int j = 0; j < sizePart2; j++)
            {
                transformedBits.emplace_back(publicDataBits[start + i * size + sizePart1 + j]);
            }
        }

        publicDataBits = transformedBits;
    }

    void generate_r1cs_witness()
    {
        // Calculate the hash
        hasher->generate_r1cs_witness();

        // Calculate the expected public input
        calculatedHash->generate_r1cs_witness_from_bits();
        pb.val(publicInput) = pb.val(calculatedHash->packed);

        printBits("[ZKS]publicData: 0x", publicDataBits.get_bits(pb), false);
        printBits("[ZKS]publicDataHash: 0x", hasher->result().bits.get_bits(pb));
        print(pb, "[ZKS]publicInput", calculatedHash->packed);
    }

    void generate_r1cs_constraints()
    {
        // Calculate the hash
        hasher.reset(new sha256_many(pb, publicDataBits, ".hasher"));
        hasher->generate_r1cs_constraints();

        // Check that the hash matches the public input
        calculatedHash.reset(new FromBitsGadget(
          pb, reverse(subArray(hasher->result().bits, 0, NUM_BITS_FIELD_CAPACITY)), ".packCalculatedHash"));
        calculatedHash->generate_r1cs_constraints(false);
        requireEqual(pb, calculatedHash->packed, publicInput, ".publicDataCheck");
    }
};

class OnChainDataHashGadget : public GadgetT 
{
  public:
    VariableArrayT publicDataBits;

    std::unique_ptr<sha256_many> hasher;
    std::unique_ptr<FromBitsGadget> calculatedHash;
    OnChainDataHashGadget( //
      ProtoboardT &pb,
      const std::string &prefix)
        : GadgetT(pb, prefix)
    {
    }

    void add(const VariableArrayT &bits)
    {
        publicDataBits.insert(publicDataBits.end(), bits.rbegin(), bits.rend());
    }

    void generate_r1cs_witness() 
    {
      // Calculate the hash
      hasher->generate_r1cs_witness();

      // Calculate the expected public input
      calculatedHash->generate_r1cs_witness_from_bits();
    }

    void generate_r1cs_constraints() 
    {
      // Calculate the hash
      hasher.reset(new sha256_many(pb, publicDataBits, ".hasher"));
      hasher->generate_r1cs_constraints();

      // Check that the hash matches the public input, take the top 40 hex
      calculatedHash.reset(new FromBitsGadget(
        pb, reverse(subArray(hasher->result().bits, 0, NUM_BITS_HASH)), ".packCalculatedHash"));
      calculatedHash->generate_r1cs_constraints(false);
    }

    const VariableT &result() const
    {
        return calculatedHash->packed;
    }
};

// Decodes a float with the specified encoding
class FloatGadget : public GadgetT
{
  public:
    const Constants &constants;

    const FloatEncoding &floatEncoding;

    VariableArrayT f;

    std::vector<VariableT> values;
    std::vector<VariableT> baseMultipliers;
    std::vector<TernaryGadget> multipliers;

    FromBitsGadget fArrayValue;

    FloatGadget(
      ProtoboardT &pb,
      const Constants &_constants,
      const FloatEncoding &_floatEncoding,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          constants(_constants),
          floatEncoding(_floatEncoding),

          f(make_var_array(pb, floatEncoding.numBitsExponent + floatEncoding.numBitsMantissa, FMT(prefix, ".f"))),
          fArrayValue(pb, f, FMT(prefix, ".fArrayValue"))
    {
        for (unsigned int i = 0; i < f.size(); i++)
        {
            values.emplace_back(make_variable(pb, FMT(prefix, ".FloatToUintGadgetVariable")));
        }

        for (unsigned int i = 0; i < floatEncoding.numBitsExponent; i++)
        {
            baseMultipliers.emplace_back(make_variable(pb, FMT(prefix, ".baseMultipliers")));
            multipliers.emplace_back(
              pb, f[floatEncoding.numBitsMantissa + i], baseMultipliers[i], constants._1, FMT(prefix, ".multipliers"));
        }
    }

    void generate_r1cs_witness(const ethsnarks::FieldT &floatValue)
    {
        f.fill_with_bits_of_field_element(pb, floatValue);

        // Decodes the mantissa
        for (unsigned int i = 0; i < floatEncoding.numBitsMantissa; i++)
        {
            unsigned j = floatEncoding.numBitsMantissa - 1 - i;
            pb.val(values[i]) = (i == 0) ? pb.val(f[j]) : (pb.val(values[i - 1]) * 2 + pb.val(f[j]));
        }

        // Decodes the exponent and shifts the mantissa
        for (unsigned int i = floatEncoding.numBitsMantissa; i < f.size(); i++)
        {
            // Decode the exponent
            unsigned int j = i - floatEncoding.numBitsMantissa;
            pb.val(baseMultipliers[j]) =
              (j == 0) ? floatEncoding.exponentBase : pb.val(baseMultipliers[j - 1]) * pb.val(baseMultipliers[j - 1]);
            multipliers[j].generate_r1cs_witness();

            // Shift the value with the partial exponent
            pb.val(values[i]) = pb.val(values[i - 1]) * pb.val(multipliers[j].result());
        }
        fArrayValue.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        // Make sure all the bits of the float or 0s and 1s
        for (unsigned int i = 0; i < f.size(); i++)
        {
            libsnark::generate_boolean_r1cs_constraint<ethsnarks::FieldT>(pb, f[i], FMT(annotation_prefix, ".bitness"));
        }

        // Decodes the mantissa
        for (unsigned int i = 0; i < floatEncoding.numBitsMantissa; i++)
        {
            unsigned j = floatEncoding.numBitsMantissa - 1 - i;
            if (i == 0)
            {
                pb.add_r1cs_constraint(
                  ConstraintT(f[j], FieldT::one(), values[i]),
                  FMT(annotation_prefix, (std::string(".value_") + std::to_string(i)).c_str()));
            }
            else
            {
                pb.add_r1cs_constraint(
                  ConstraintT(values[i - 1] * 2 + f[j], FieldT::one(), values[i]),
                  FMT(annotation_prefix, (std::string(".value_") + std::to_string(i)).c_str()));
            }
        }

        // Decodes the exponent and shifts the mantissa
        for (unsigned int i = floatEncoding.numBitsMantissa; i < f.size(); i++)
        {
            // Decode the exponent
            unsigned int j = i - floatEncoding.numBitsMantissa;
            if (j == 0)
            {
                pb.add_r1cs_constraint(
                  ConstraintT(floatEncoding.exponentBase, FieldT::one(), baseMultipliers[j]), ".baseMultipliers");
            }
            else
            {
                pb.add_r1cs_constraint(
                  ConstraintT(baseMultipliers[j - 1], baseMultipliers[j - 1], baseMultipliers[j]), ".baseMultipliers");
            }
            multipliers[j].generate_r1cs_constraints();

            // Shift the value with the partial exponent
            pb.add_r1cs_constraint(ConstraintT(values[i - 1], multipliers[j].result(), values[i]), ".valuesExp");
        }
        fArrayValue.generate_r1cs_constraints(true);
    }

    const VariableT &value() const
    {
        return values.back();
    }
    const VariableT &getFArrayValue() const
    {
        return fArrayValue.packed;
    }

    const VariableArrayT &bits() const
    {
        return f;
    }
};

class FloatFromBitsGadget : public GadgetT
{
  public:
    const Constants &constants;

    const FloatEncoding &floatEncoding;

    VariableArrayT f;

    std::vector<VariableT> values;
    std::vector<VariableT> baseMultipliers;
    std::vector<TernaryGadget> multipliers;

    FloatFromBitsGadget(
      ProtoboardT &pb,
      const Constants &_constants,
      const FloatEncoding &_floatEncoding,
      const VariableArrayT &fBits,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          constants(_constants),
          floatEncoding(_floatEncoding),

          f(fBits)
    {
        for (unsigned int i = 0; i < f.size(); i++)
        {
            values.emplace_back(make_variable(pb, FMT(prefix, ".FloatToUintGadgetVariable")));
        }

        for (unsigned int i = 0; i < floatEncoding.numBitsExponent; i++)
        {
            baseMultipliers.emplace_back(make_variable(pb, FMT(prefix, ".baseMultipliers")));
            multipliers.emplace_back(
              pb, f[floatEncoding.numBitsMantissa + i], baseMultipliers[i], constants._1, FMT(prefix, ".multipliers"));
        }
    }

    void generate_r1cs_witness()
    {
        // Decodes the mantissa
        for (unsigned int i = 0; i < floatEncoding.numBitsMantissa; i++)
        {
            unsigned j = floatEncoding.numBitsMantissa - 1 - i;
            pb.val(values[i]) = (i == 0) ? pb.val(f[j]) : (pb.val(values[i - 1]) * 2 + pb.val(f[j]));
        }

        // Decodes the exponent and shifts the mantissa
        for (unsigned int i = floatEncoding.numBitsMantissa; i < f.size(); i++)
        {
            // Decode the exponent
            unsigned int j = i - floatEncoding.numBitsMantissa;
            pb.val(baseMultipliers[j]) =
              (j == 0) ? floatEncoding.exponentBase : pb.val(baseMultipliers[j - 1]) * pb.val(baseMultipliers[j - 1]);
            multipliers[j].generate_r1cs_witness();

            // Shift the value with the partial exponent
            pb.val(values[i]) = pb.val(values[i - 1]) * pb.val(multipliers[j].result());
        }
    }

    void generate_r1cs_constraints()
    {
        // Make sure all the bits of the float or 0s and 1s
        for (unsigned int i = 0; i < f.size(); i++)
        {
            libsnark::generate_boolean_r1cs_constraint<ethsnarks::FieldT>(pb, f[i], FMT(annotation_prefix, ".bitness"));
        }

        // Decodes the mantissa
        for (unsigned int i = 0; i < floatEncoding.numBitsMantissa; i++)
        {
            unsigned j = floatEncoding.numBitsMantissa - 1 - i;
            if (i == 0)
            {
                pb.add_r1cs_constraint(
                  ConstraintT(f[j], FieldT::one(), values[i]),
                  FMT(annotation_prefix, (std::string(".value_") + std::to_string(i)).c_str()));
            }
            else
            {
                pb.add_r1cs_constraint(
                  ConstraintT(values[i - 1] * 2 + f[j], FieldT::one(), values[i]),
                  FMT(annotation_prefix, (std::string(".value_") + std::to_string(i)).c_str()));
            }
        }

        // Decodes the exponent and shifts the mantissa
        for (unsigned int i = floatEncoding.numBitsMantissa; i < f.size(); i++)
        {
            // Decode the exponent
            unsigned int j = i - floatEncoding.numBitsMantissa;
            if (j == 0)
            {
                pb.add_r1cs_constraint(
                  ConstraintT(floatEncoding.exponentBase, FieldT::one(), baseMultipliers[j]), ".baseMultipliers");
            }
            else
            {
                pb.add_r1cs_constraint(
                  ConstraintT(baseMultipliers[j - 1], baseMultipliers[j - 1], baseMultipliers[j]), ".baseMultipliers");
            }
            multipliers[j].generate_r1cs_constraints();

            // Shift the value with the partial exponent
            pb.add_r1cs_constraint(ConstraintT(values[i - 1], multipliers[j].result(), values[i]), ".valuesExp");
        }
    }

    const VariableT &value() const
    {
        return values.back();
    }

    const VariableArrayT &bits() const
    {
        return f;
    }
};
// Checks 'type' is one of Constants.values - [0 - 10]
struct SelectorGadget : public GadgetT
{
    const Constants &constants;

    std::vector<EqualGadget> bits;
    std::vector<UnsafeAddGadget> sum;

    VariableArrayT res;

    SelectorGadget(
      ProtoboardT &pb,
      const Constants &_constants,
      const VariableT &type,
      unsigned int maxBits,
      const std::string &prefix)
        : GadgetT(pb, prefix), constants(_constants)
    {

        std::cout << "in SelectorGadget: maxBits:" << maxBits << std::endl;
        std::cout << "in SelectorGadget: constants.values.size():" << constants.values.size() << std::endl;
        assert(maxBits <= constants.values.size());
        for (unsigned int i = 0; i < maxBits; i++)
        {
            bits.emplace_back(pb, type, constants.values[i], FMT(annotation_prefix, ".bits"));
            sum.emplace_back(
              pb, (i == 0) ? constants._0 : sum.back().result(), bits.back().result(), FMT(annotation_prefix, ".sum"));
            res.emplace_back(bits.back().result());
        }
    }

    void generate_r1cs_witness()
    {
        for (unsigned int i = 0; i < bits.size(); i++)
        {
            bits[i].generate_r1cs_witness();
            sum[i].generate_r1cs_witness();
        }
    }

    void generate_r1cs_constraints()
    {
        for (unsigned int i = 0; i < bits.size(); i++)
        {
            bits[i].generate_r1cs_constraints();
            sum[i].generate_r1cs_constraints();
        }
        // Sum needs to equal 1
        requireEqual(pb, sum.back().result(), constants._1, FMT(annotation_prefix, ".selector_sum_one"));
    }

    const VariableArrayT &result() const
    {
        return res;
    }
};

// if selector=[1,0,0] and values = [a,b,c], return a
// if selector=[0,1,0] and values = [a,b,c], return b
// if selector=[0,0,1] and values = [a,b,c], return c
// special case,
// if selector=[0,0,0] and values = [a,b,c], return 0 ？
// if selector=[1,1,0] and values = [a,b,c], return b
// if selector=[0,1,1] and values = [a,b,c], return c
// if selector=[1,0,1] and values = [a,b,c], return c
class SelectGadget : public GadgetT
{
  public:
    std::vector<TernaryGadget> results;

    SelectGadget(
      ProtoboardT &pb,
      const Constants &_constants,
      const VariableArrayT &selector,
      const std::vector<VariableT> &values,
      const std::string &prefix)
        : GadgetT(pb, prefix)
    {
        assert(values.size() == selector.size());
        for (unsigned int i = 0; i < values.size(); i++)
        {
            results.emplace_back(
              pb, selector[i], values[i], (i == 0) ? _constants._0 : results.back().result(), FMT(prefix, ".results"));
        }
    }

    void generate_r1cs_witness()
    {
        for (unsigned int i = 0; i < results.size(); i++)
        {
            results[i].generate_r1cs_witness();
        }
    }

    void generate_r1cs_constraints()
    {
        for (unsigned int i = 0; i < results.size(); i++)
        {
            results[i].generate_r1cs_constraints(false);
        }
    }

    const VariableT &result() const
    {
        return results.back().result();
    }
};

// Select one out of many arrays based on mutiple boolean selector values.
// If all the selector values are false, then the last array is selected.
class ArraySelectGadget : public GadgetT
{
  public:
    std::vector<ArrayTernaryGadget> results;

    ArraySelectGadget(
      ProtoboardT &pb,
      const Constants &_constants,
      const VariableArrayT &selector,
      const std::vector<VariableArrayT> &values,
      const std::string &prefix)
        : GadgetT(pb, prefix)
    {
        assert(values.size() == selector.size());
        for (unsigned int i = 0; i < values.size(); i++)
        {
            results.emplace_back(
              pb,
              selector[i],
              values[i],
              (i == 0) ? VariableArrayT(values[0].size(), _constants._0) : results.back().result(),
              FMT(prefix, ".results"));
        }
    }

    void generate_r1cs_witness()
    {
        for (unsigned int i = 0; i < results.size(); i++)
        {
            results[i].generate_r1cs_witness();
        }
    }

    void generate_r1cs_constraints()
    {
        for (unsigned int i = 0; i < results.size(); i++)
        {
            results[i].generate_r1cs_constraints(false);
        }
    }

    const VariableArrayT &result() const
    {
        return results.back().result();
    }
};

class VectorArraySelectGadget : public GadgetT
{
  public:
    std::vector<VectorArrayTernaryGadget> results;

    VectorArraySelectGadget(
      ProtoboardT &pb,
      const Constants &_constants,
      const VariableArrayT &selector,
      const std::vector<std::vector<VariableArrayT>> &values,
      const std::string &prefix)
        : GadgetT(pb, prefix)
    {
        assert(values.size() == selector.size());
        std::vector<VariableArrayT> first;
        for (unsigned int i = 0; i < values[0].size(); i++) 
        {
            first.emplace_back(VariableArrayT(values[0][0].size(), _constants._0));
        }
        
        for (unsigned int i = 0; i < values.size(); i++)
        {
            results.emplace_back(
              pb,
              selector[i],
              values[i],
              (i == 0) ? first : results.back().result(),
              FMT(prefix, ".results"));
        }
    }

    void generate_r1cs_witness()
    {
        for (unsigned int i = 0; i < results.size(); i++)
        {
            results[i].generate_r1cs_witness();
        }
    }

    void generate_r1cs_constraints()
    {
        for (unsigned int i = 0; i < results.size(); i++)
        {
            results[i].generate_r1cs_constraints(false);
        }
    }

    const std::vector<VariableArrayT> &result() const
    {
        return results.back().result();
    }
};

// Checks 'type' is one of value array - [n - m]
// The return value exists in this case: 0, 1, 1, 1
// That is, there must be only one match
// It is not limited to finding data, that is, there can be 0, 0, 0
struct ExistSelectorGadget : public GadgetT
{
    const Constants &constants;

    std::vector<EqualGadget> bits;
    std::vector<UnsafeAddGadget> sum;


    VariableArrayT res;

    ExistSelectorGadget(
      ProtoboardT &pb,
      const Constants &_constants,
      // const VariableArrayT &values,
      const std::vector<VariableT> &values,
      const VariableT &type,
      unsigned int maxBits,
      const std::string &prefix)
        : GadgetT(pb, prefix), constants(_constants)
    {
        assert(maxBits <= values.size());
        for (unsigned int i = 0; i < maxBits; i++)
        {
            bits.emplace_back(pb, type, values[i], FMT(annotation_prefix, ".bits"));
            sum.emplace_back(
              pb, (i == 0) ? constants._0 : sum.back().result(), bits.back().result(), FMT(annotation_prefix, ".sum"));
            res.emplace_back(bits.back().result());
        }
    }

    void generate_r1cs_witness()
    {
        for (unsigned int i = 0; i < bits.size(); i++)
        {
            bits[i].generate_r1cs_witness();
            sum[i].generate_r1cs_witness();
        }
    }

    void generate_r1cs_constraints()
    {
        for (unsigned int i = 0; i < bits.size(); i++)
        {
            bits[i].generate_r1cs_constraints();
            sum[i].generate_r1cs_constraints();
        }
    }

    const VariableArrayT &result() const
    {
        return res;
    }
};

// Checks that the new ower equals the current onwer or the current ower is 0.
class OwnerValidGadget : public GadgetT
{
  public:
    EqualGadget newOwner_equal_oldOwner;
    EqualGadget no_oldOwner;
    OrGadget equal_owner_or_no_owner;
    RequireEqualGadget equal_owner_or_no_owner_eq_true;

    OwnerValidGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &oldOwner,
      const VariableT &newOwner,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          newOwner_equal_oldOwner(pb, newOwner, oldOwner, FMT(prefix, ".newOwner_equal_oldOwner")),
          no_oldOwner(pb, oldOwner, constants._0, FMT(prefix, ".no_oldOwner")),
          equal_owner_or_no_owner(
            pb,
            {newOwner_equal_oldOwner.result(), no_oldOwner.result()},
            FMT(prefix, ".equal_owner_or_no_owner")),
          equal_owner_or_no_owner_eq_true(
            pb,
            equal_owner_or_no_owner.result(),
            constants._1,
            FMT(prefix, ".equal_owner_or_no_owner_eq_true"))
    {
    }

    void generate_r1cs_witness()
    {
        newOwner_equal_oldOwner.generate_r1cs_witness();
        no_oldOwner.generate_r1cs_witness();
        equal_owner_or_no_owner.generate_r1cs_witness();
        equal_owner_or_no_owner_eq_true.generate_r1cs_witness();
        ASSERT(pb.val(equal_owner_or_no_owner.result()) == FieldT::one(), annotation_prefix);
    }

    void generate_r1cs_constraints()
    {
        newOwner_equal_oldOwner.generate_r1cs_constraints();
        no_oldOwner.generate_r1cs_constraints();
        equal_owner_or_no_owner.generate_r1cs_constraints();
        equal_owner_or_no_owner_eq_true.generate_r1cs_constraints();
    }

    const VariableT &isNewAccount() const
    {
        return no_oldOwner.result();
    }
};


// Signed variable:
// positive: sign == 1
// negative: sign == 0
// Zero can be either positive or negative
struct SignedVariableT
{
  public:
    VariableT sign;
    VariableT value;

    SignedVariableT()
    {
    }

    SignedVariableT( //
      ProtoboardT &pb,
      const std::string &prefix)
        : sign(make_variable(pb, FMT(prefix, ".sign"))), value(make_variable(pb, FMT(prefix, ".value")))
    {
    }

    SignedVariableT( //
      const VariableT &_sign,
      const VariableT &_value)
        : sign(_sign), value(_value)
    {
    }
};

// sA + sB = sSum with abs(A), abs(B) and abs(sum) < 2^n
class SignedAddGadget : public GadgetT
{
  public:
    SignedVariableT _A;
    SignedVariableT _B;

    UnsafeAddGadget a_add_b;
    UnsafeSubGadget b_sub_a;
    UnsafeSubGadget a_sub_b;

    LeqGadget a_leq_b;

    EqualGadget signsEqual;
    TernaryGadget temp;
    TernaryGadget value;

    AndGadget signB_and_a_leq_b;
    AndGadget signA_and_not_a_leq_b;
    OrGadget sign;
    EqualGadget isZero;
    TernaryGadget normalizedSign;

    RangeCheckGadget rangeCheck;

    SignedAddGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const SignedVariableT &A,
      const SignedVariableT &B,
      unsigned int n,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          _A(A),
          _B(B),

          a_add_b(pb, A.value, B.value, FMT(prefix, ".a_add_b")),
          b_sub_a(pb, B.value, A.value, FMT(prefix, ".b_sub_a")),
          a_sub_b(pb, A.value, B.value, FMT(prefix, ".a_sub_b")),

          a_leq_b(pb, A.value, B.value, n, FMT(prefix, ".a_leq_b")),

          signsEqual(pb, A.sign, B.sign, FMT(prefix, ".signsEqual")),
          temp(pb, a_leq_b.lt(), b_sub_a.result(), a_sub_b.result(), FMT(prefix, ".temp")),
          value(pb, signsEqual.result(), a_add_b.result(), temp.result(), FMT(prefix, ".value")),

          signB_and_a_leq_b(pb, {B.sign, a_leq_b.leq()}, FMT(prefix, ".signB_and_a_leq_b")),
          signA_and_not_a_leq_b(pb, {A.sign, a_leq_b.gt()}, FMT(prefix, ".signA_and_not_a_leq_b")),
          sign(pb, {signB_and_a_leq_b.result(), signA_and_not_a_leq_b.result()}, FMT(prefix, ".sign")),
          isZero(pb, value.result(), constants._0, FMT(prefix, ".isZero")),
          normalizedSign(pb, isZero.result(), constants._0, sign.result(), FMT(prefix, ".sign")),

          rangeCheck(pb, value.result(), n, FMT(prefix, ".rangeCheck"))
    {
        assert(n + 1 <= NUM_BITS_FIELD_CAPACITY);
    }

    void generate_r1cs_witness()
    {
        a_add_b.generate_r1cs_witness();
        b_sub_a.generate_r1cs_witness();
        a_sub_b.generate_r1cs_witness();

        a_leq_b.generate_r1cs_witness();

        signsEqual.generate_r1cs_witness();
        temp.generate_r1cs_witness();
        value.generate_r1cs_witness();

        signB_and_a_leq_b.generate_r1cs_witness();
        signA_and_not_a_leq_b.generate_r1cs_witness();
        sign.generate_r1cs_witness();
        isZero.generate_r1cs_witness();
        normalizedSign.generate_r1cs_witness();

        rangeCheck.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        a_add_b.generate_r1cs_constraints();
        b_sub_a.generate_r1cs_constraints();
        a_sub_b.generate_r1cs_constraints();

        a_leq_b.generate_r1cs_constraints();

        signsEqual.generate_r1cs_constraints();
        temp.generate_r1cs_constraints();
        value.generate_r1cs_constraints();

        signB_and_a_leq_b.generate_r1cs_constraints();
        signA_and_not_a_leq_b.generate_r1cs_constraints();
        sign.generate_r1cs_constraints();
        isZero.generate_r1cs_constraints();
        normalizedSign.generate_r1cs_constraints();

        rangeCheck.generate_r1cs_constraints();
    }

    const SignedVariableT result() const
    {
        return SignedVariableT(normalizedSign.result(), value.result());
    }
};

// sA + (-sB) = sSum with abs(A), abs(B) and abs(sum) < 2^n
class SignedSubGadget : public GadgetT
{
  public:
    NotGadget notSignB;
    SignedAddGadget signedAddGadget;

    SignedSubGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const SignedVariableT &A,
      const SignedVariableT &B,
      unsigned int n,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          notSignB(pb, B.sign, FMT(prefix, ".notSignB")),
          signedAddGadget(
            pb,
            constants,
            A,
            SignedVariableT(notSignB.result(), B.value),
            n,
            FMT(prefix, ".signedAddGadget"))
    {
    }

    void generate_r1cs_witness()
    {
        notSignB.generate_r1cs_witness();
        signedAddGadget.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        notSignB.generate_r1cs_constraints();
        signedAddGadget.generate_r1cs_constraints();
    }

    const SignedVariableT result() const
    {
        return signedAddGadget.result();
    }
};

} // namespace Loopring

#endif
