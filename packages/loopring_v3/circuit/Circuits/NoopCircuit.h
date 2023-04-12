// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
#ifndef _NOOPCIRCUIT_H_
#define _NOOPCIRCUIT_H_

#include "Circuit.h"
#include "../Utils/Constants.h"
#include "../Utils/Data.h"

#include "ethsnarks.hpp"
#include "utils.hpp"

using namespace ethsnarks;

namespace Loopring
{

class NoopCircuit : public BaseTransactionCircuit
{
  public:
    DualVariableGadget typeTx;
    // type is 3bits, need add 1bit, then 4bits can convert to 1 hex char
    DualVariableGadget typeTxPad;
    NoopCircuit( //
      ProtoboardT &pb,
      const TransactionState &state,
      const std::string &prefix)
        : BaseTransactionCircuit(pb, state, prefix),
        typeTx(pb, NUM_BITS_TX_TYPE, FMT(prefix, ".typeTx")),
          typeTxPad(pb, NUM_BITS_BIT, FMT(prefix, ".typeTxPad"))
    {
        LOG(LogDebug, "in NoopCircuit", "");
        // No signatures needed
        setOutput(TXV_SIGNATURE_REQUIRED_A, state.constants._0);
        setOutput(TXV_SIGNATURE_REQUIRED_B, state.constants._0);
    }

    void generate_r1cs_witness()
    {
        LOG(LogDebug, "in NoopCircuit", "generate_r1cs_witness");
        typeTx.generate_r1cs_witness(pb, ethsnarks::FieldT(int(Loopring::TransactionType::Noop)));
        typeTxPad.generate_r1cs_witness(pb, ethsnarks::FieldT(0));
    }

    void generate_r1cs_constraints()
    {
        LOG(LogDebug, "in NoopCircuit", "generate_r1cs_constraints");
        typeTx.generate_r1cs_constraints(true);
        typeTxPad.generate_r1cs_constraints(true);
    }

    const VariableArrayT getPublicData() const
    {
        // return VariableArrayT(0, state.constants._0);
        return flattenReverse({
            typeTx.bits,
            typeTxPad.bits
        });
    }
};

} // namespace Loopring

#endif
