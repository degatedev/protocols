// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
#ifndef _STORAGEGADGETS_H_
#define _STORAGEGADGETS_H_

#include "../Utils/Constants.h"
#include "../Utils/Data.h"

#include "MerkleTree.h"

#include "ethsnarks.hpp"
#include "utils.hpp"

using namespace ethsnarks;

namespace Loopring
{

struct StorageState
{
    VariableT tokenSID;
    VariableT tokenBID;
    VariableT data;
    VariableT storageID;
    VariableT gasFee;
    VariableT cancelled;
    VariableT forward;
};

static void printStorage(const ProtoboardT &pb, const StorageState &state)
{
    std::cout << "- tokenSID: " << pb.val(state.tokenSID) << std::endl;
    std::cout << "- tokenBID: " << pb.val(state.tokenBID) << std::endl;
    std::cout << "- data: " << pb.val(state.data) << std::endl;
    std::cout << "- storageID: " << pb.val(state.storageID) << std::endl;
    std::cout << "- gasFee: " << pb.val(state.gasFee) << std::endl;
    std::cout << "- cancelled: " << pb.val(state.cancelled) << std::endl;
    std::cout << "- forward: " << pb.val(state.forward) << std::endl;
}

class StorageGadget : public GadgetT
{
  public:
    VariableT tokenSID;
    VariableT tokenBID;
    VariableT data;
    VariableT storageID;
    VariableT gasFee;
    VariableT cancelled;
    VariableT forward;

    StorageGadget( //
      ProtoboardT &pb,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          tokenSID(make_variable(pb, FMT(prefix, ".tokenSID"))),
          tokenBID(make_variable(pb, FMT(prefix, ".tokenBID"))),
          data(make_variable(pb, FMT(prefix, ".data"))),
          storageID(make_variable(pb, FMT(prefix, ".storageID"))),
          gasFee(make_variable(pb, FMT(prefix, ".gasFee"))),
          cancelled(make_variable(pb, FMT(prefix, ".cancelled"))),
          forward(make_variable(pb, FMT(prefix, ".forward")))
    {
    }

    void generate_r1cs_witness(const StorageLeaf &storageLeaf)
    {
        pb.val(tokenSID) = storageLeaf.tokenSID;
        pb.val(tokenBID) = storageLeaf.tokenBID;
        pb.val(data) = storageLeaf.data;
        pb.val(storageID) = storageLeaf.storageID;
        pb.val(gasFee) = storageLeaf.gasFee;
        pb.val(cancelled) = storageLeaf.cancelled;
        pb.val(forward) = storageLeaf.forward;
    }
};

class UpdateStorageGadget : public GadgetT
{
  public:
    HashStorageLeaf leafBefore;
    HashStorageLeaf leafAfter;

    StorageState valuesBefore;
    StorageState valuesAfter;

    const VariableArrayT proof;
    MerklePathCheckT proofVerifierBefore;
    MerklePathT rootCalculatorAfter;

    UpdateStorageGadget(
      ProtoboardT &pb,
      const VariableT &merkleRoot,
      const VariableArrayT &slotID,
      const StorageState &before,
      const StorageState &after,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          valuesBefore(before),
          valuesAfter(after),

          leafBefore(pb, 
          var_array({before.tokenSID, before.tokenBID, before.data, before.storageID, before.gasFee, before.cancelled, before.forward}), 
          FMT(prefix, ".leafBefore")),
          leafAfter(pb, 
          var_array({after.tokenSID, after.tokenBID, after.data, after.storageID, after.gasFee, after.cancelled, after.forward}), 
          FMT(prefix, ".leafAfter")),

          proof(make_var_array(pb, TREE_DEPTH_STORAGE * 3, FMT(prefix, ".proof"))),
          proofVerifierBefore(
            pb,
            TREE_DEPTH_STORAGE,
            slotID,
            leafBefore.result(),
            merkleRoot,
            proof,
            FMT(prefix, ".pathBefore")),
          rootCalculatorAfter(pb, TREE_DEPTH_STORAGE, slotID, leafAfter.result(), proof, FMT(prefix, ".pathAfter"))
    {
    }

    void generate_r1cs_witness(const StorageUpdate &update)
    {
        // std::cout << "in storage update: json data before:" << std::endl;
        // std::cout << "in storage update: == storageID:" << update.before.storageID << std::endl;
        // std::cout << "in storage update: == TokenSID:" << update.before.tokenSID << std::endl;
        // std::cout << "in storage update: == tokenBID:" << update.before.tokenBID << std::endl;
        // std::cout << "in storage update: == data:" << update.before.data << std::endl;
        // std::cout << "in storage update: == gasFee:" << update.before.gasFee << std::endl;
        // std::cout << "in storage update: == cancelled:" << update.before.cancelled << std::endl;
        // std::cout << "in storage update: == forward:" << update.before.forward << std::endl;

        // std::cout << "in storage update: json data after:" << std::endl;
        // std::cout << "in storage update: == storageID:" << update.after.storageID << std::endl;
        // std::cout << "in storage update: == TokenSID:" << update.after.tokenSID << std::endl;
        // std::cout << "in storage update: == tokenBID:" << update.after.tokenBID << std::endl;
        // std::cout << "in storage update: == data:" << update.after.data << std::endl;
        // std::cout << "in storage update: == gasFee:" << update.after.gasFee << std::endl;
        // std::cout << "in storage update: == cancelled:" << update.after.cancelled << std::endl;
        // std::cout << "in storage update: == forward:" << update.after.forward << std::endl;
        leafBefore.generate_r1cs_witness();
        leafAfter.generate_r1cs_witness();

        proof.fill_with_field_elements(pb, update.proof.data);
        proofVerifierBefore.generate_r1cs_witness();
        rootCalculatorAfter.generate_r1cs_witness();

        ASSERT(pb.val(proofVerifierBefore.m_expected_root) == update.rootBefore, annotation_prefix);
        if (pb.val(rootCalculatorAfter.result()) != update.rootAfter)
        {
            printStorage(pb, valuesBefore);
            printStorage(pb, valuesAfter);
            ASSERT(pb.val(rootCalculatorAfter.result()) == update.rootAfter, annotation_prefix);
        }
    }

    void generate_r1cs_constraints()
    {
        leafBefore.generate_r1cs_constraints();
        leafAfter.generate_r1cs_constraints();

        proofVerifierBefore.generate_r1cs_constraints();
        rootCalculatorAfter.generate_r1cs_constraints();
    }

    const VariableT &result() const
    {
        return rootCalculatorAfter.result();
    }
};

class StorageReaderGadget : public GadgetT
{
    LeqGadget storageID_leq_leafStorageID;
    IfThenRequireGadget requireValidStorageID;

    TernaryGadget tokenSID;
    TernaryGadget tokenBID;
    TernaryGadget data;
    TernaryGadget gasFee;
    TernaryGadget cancelled;
    TernaryGadget forward;

  public:
    StorageReaderGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const StorageGadget &storage,
      const DualVariableGadget &storageID,
      const VariableT &verify,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          storageID_leq_leafStorageID(
            pb,
            storageID.packed,
            storage.storageID,
            NUM_BITS_STORAGEID,
            FMT(prefix, ".storageID_leq_leafStorageID")),
          requireValidStorageID(pb, verify, storageID_leq_leafStorageID.gte(), FMT(prefix, ".requireValidStorageID")),

          tokenSID(pb, storageID_leq_leafStorageID.eq(), storage.tokenSID, constants._0, FMT(prefix, ".tokenSID")),
          tokenBID(pb, storageID_leq_leafStorageID.eq(), storage.tokenBID, constants._0, FMT(prefix, ".tokenBID")),
          data(pb, storageID_leq_leafStorageID.eq(), storage.data, constants._0, FMT(prefix, ".data")),
          gasFee(pb, storageID_leq_leafStorageID.eq(), storage.gasFee, constants._0, FMT(prefix, ".gasFee")),
          cancelled(pb, storageID_leq_leafStorageID.eq(), storage.cancelled, constants._0, FMT(prefix, ".cancelled")),

          forward(pb, storageID_leq_leafStorageID.eq(), storage.forward, constants._1, FMT(prefix, ".forward"))
    {
    }

    void generate_r1cs_witness()
    {
        storageID_leq_leafStorageID.generate_r1cs_witness();
        requireValidStorageID.generate_r1cs_witness();

        tokenSID.generate_r1cs_witness();
        tokenBID.generate_r1cs_witness();
        data.generate_r1cs_witness();
        gasFee.generate_r1cs_witness();
        cancelled.generate_r1cs_witness();
        forward.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        storageID_leq_leafStorageID.generate_r1cs_constraints();
        requireValidStorageID.generate_r1cs_constraints();

        tokenSID.generate_r1cs_constraints();
        tokenBID.generate_r1cs_constraints();
        data.generate_r1cs_constraints();
        gasFee.generate_r1cs_constraints();
        cancelled.generate_r1cs_constraints();
        forward.generate_r1cs_constraints();
    }

    const VariableT &getTokenSID() const
    {
        return tokenSID.result();
    }

    const VariableT &getTokenBID() const
    {
        return tokenBID.result();
    }

    const VariableT &getData() const
    {
        return data.result();
    }

    const VariableT &getGasFee() const
    {
        return gasFee.result();
    }

    const VariableT &getCancelled() const
    {
        return cancelled.result();
    }
    const VariableT &getForward() const
    {
        return forward.result();
    }
};
// If it is a new order from automarket, the data and gasfee need to be reset to 0, and the cancelled field remains unchanged. 
// If it is cancelled, it cannot be traded
class StorageReaderForAutoMarketGadget : public GadgetT 
{
  public:
    TernaryGadget data;
    TernaryGadget gasFee;
    StorageReaderForAutoMarketGadget(
      ProtoboardT &pb,
      const Constants &_constants,
      const StorageReaderGadget &storage,
      const VariableT &isNewOrder,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          data(pb, isNewOrder, _constants._0, storage.getData(), FMT(prefix, ".data")),
          gasFee(pb, isNewOrder, _constants._0, storage.getGasFee(), FMT(prefix, ".gasFee"))
    {

    }
    void generate_r1cs_witness()
    {
        data.generate_r1cs_witness();
        gasFee.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        data.generate_r1cs_constraints();
        gasFee.generate_r1cs_constraints();
    }
    const VariableT &getData() const
    {
        return data.result();
    }

    const VariableT &getGasFee() const
    {
        return gasFee.result();
    }
};

class NonceGadget : public GadgetT
{
    const Constants &constants;
    const DualVariableGadget &storageID;

    StorageReaderGadget storageReader;
    IfThenRequireEqualGadget requireTokenSIDZero;
    IfThenRequireEqualGadget requireTokenBIDZero;
    IfThenRequireEqualGadget requireDataZero;
    IfThenRequireEqualGadget requireGasFeeZero;
    IfThenRequireEqualGadget requireCancelledZero;
    IfThenRequireEqualGadget requireForwardOne;


  public:
    NonceGadget(
      ProtoboardT &pb,
      const Constants &_constants,
      const StorageGadget &storage,
      const DualVariableGadget &_storageID,
      const VariableT &verify,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          constants(_constants),
          storageID(_storageID),

          storageReader(pb, constants, storage, storageID, verify, FMT(prefix, ".storageReader")),
          requireTokenSIDZero(pb, verify, storageReader.getTokenSID(), constants._0, FMT(prefix, ".requireTokenSIDZero")),
          requireTokenBIDZero(pb, verify, storageReader.getTokenBID(), constants._0, FMT(prefix, ".requireTokenBIDZero")),
          requireGasFeeZero(pb, verify, storageReader.getGasFee(), constants._0, FMT(prefix, ".requireGasFeeZero")),
          requireCancelledZero(pb, verify, storageReader.getCancelled(), constants._0, FMT(prefix, ".requireCancelledZero")),
          requireDataZero(pb, verify, storageReader.getData(), constants._0, FMT(prefix, ".requireDataZero")),
          requireForwardOne(pb, verify, storageReader.getForward(), constants._1, FMT(prefix, ".requireForwardOne"))
    {
    }

    void generate_r1cs_witness()
    {
        storageReader.generate_r1cs_witness();
        requireTokenSIDZero.generate_r1cs_witness();
        requireTokenBIDZero.generate_r1cs_witness();
        requireGasFeeZero.generate_r1cs_witness();
        requireCancelledZero.generate_r1cs_witness();
        requireDataZero.generate_r1cs_witness();
        requireForwardOne.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        storageReader.generate_r1cs_constraints();
        requireTokenSIDZero.generate_r1cs_constraints();
        requireTokenBIDZero.generate_r1cs_constraints();
        requireGasFeeZero.generate_r1cs_constraints();
        requireCancelledZero.generate_r1cs_constraints();
        requireDataZero.generate_r1cs_constraints();
        requireForwardOne.generate_r1cs_constraints();
    }

    const VariableT &getData() const
    {
        return constants._1;
    }
};

class OrderCancelledNonceGadget : public GadgetT
{
    const Constants &constants;
    const DualVariableGadget &storageID;

    StorageReaderGadget storageReader;
    IfThenRequireEqualGadget requireCancelledZero;

  public:
    OrderCancelledNonceGadget(
      ProtoboardT &pb,
      const Constants &_constants,
      const StorageGadget &storage,
      const DualVariableGadget &_storageID,
      const VariableT &verify,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          constants(_constants),
          storageID(_storageID),

          storageReader(pb, constants, storage, storageID, verify, FMT(prefix, ".storageReader")),
          requireCancelledZero(
            pb,
            verify,
            storageReader.getCancelled(),
            constants._0,
            FMT(prefix, ".requireCancelledZero"))
    {
    }

    void generate_r1cs_witness()
    {
        storageReader.generate_r1cs_witness();
        requireCancelledZero.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        storageReader.generate_r1cs_constraints();
        requireCancelledZero.generate_r1cs_constraints();
    }

    const VariableT &getCancelled() const
    {
        return constants._1;
    }
};

} // namespace Loopring

#endif
