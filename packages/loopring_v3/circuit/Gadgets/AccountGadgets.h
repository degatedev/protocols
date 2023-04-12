// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
#ifndef _ACCOUNTGADGETS_H_
#define _ACCOUNTGADGETS_H_

#include "../Utils/Constants.h"
#include "../Utils/Data.h"

#include "MerkleTree.h"

#include "ethsnarks.hpp"
#include "utils.hpp"
#include "gadgets/merkle_tree.hpp"
#include "gadgets/poseidon.hpp"

using namespace ethsnarks;

namespace Loopring
{

struct AccountState
{
    VariableT owner;
    VariableT publicKeyX;
    VariableT publicKeyY;
    VariableT appKeyPublicKeyX;
    VariableT appKeyPublicKeyY;
    VariableT nonce;
    VariableT disableAppKeySpotTrade;
    VariableT disableAppKeyWithdraw;
    VariableT disableAppKeyTransferToOther;
    VariableT balancesRoot;
    VariableT storageRoot;
};

static void printAccount(const ProtoboardT &pb, const AccountState &state)
{
    std::cout << "- owner: " << pb.val(state.owner) << std::endl;
    std::cout << "- publicKeyX: " << pb.val(state.publicKeyX) << std::endl;
    std::cout << "- publicKeyY: " << pb.val(state.publicKeyY) << std::endl;
    std::cout << "- appKeyPublicKeyX: " << pb.val(state.appKeyPublicKeyX) << std::endl;
    std::cout << "- appKeyPublicKeyY: " << pb.val(state.appKeyPublicKeyY) << std::endl;
    std::cout << "- nonce: " << pb.val(state.nonce) << std::endl;
    std::cout << "- disableAppKeySpotTrade: " << pb.val(state.disableAppKeySpotTrade) << std::endl;
    std::cout << "- disableAppKeyWithdraw: " << pb.val(state.disableAppKeyWithdraw) << std::endl;
    std::cout << "- disableAppKeyTransferToOther: " << pb.val(state.disableAppKeyTransferToOther) << std::endl;
    std::cout << "- balancesRoot: " << pb.val(state.balancesRoot) << std::endl;
    std::cout << "- storageRoot: " << pb.val(state.storageRoot) << std::endl;

}

class AccountGadget : public GadgetT
{
  public:
    VariableT owner;
    const jubjub::VariablePointT publicKey;
    const jubjub::VariablePointT appKeyPublicKey;
    VariableT nonce;
    VariableT disableAppKeySpotTrade;
    VariableT disableAppKeyWithdraw;
    VariableT disableAppKeyTransferToOther;
    VariableT balancesRoot;
    VariableT storageRoot;

    AccountGadget( //
      ProtoboardT &pb,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          owner(make_variable(pb, FMT(prefix, ".owner"))),
          publicKey(pb, FMT(prefix, ".publicKey")),
          appKeyPublicKey(pb, FMT(prefix, ".appKeyPublicKey")),
          nonce(make_variable(pb, FMT(prefix, ".nonce"))),
          disableAppKeySpotTrade(make_variable(pb, FMT(prefix, ".disableAppKeySpotTrade"))),
          disableAppKeyWithdraw(make_variable(pb, FMT(prefix, ".disableAppKeyWithdraw"))),
          disableAppKeyTransferToOther(make_variable(pb, FMT(prefix, ".disableAppKeyTransferToOther"))),
          balancesRoot(make_variable(pb, FMT(prefix, ".balancesRoot"))),
          storageRoot(make_variable(pb, FMT(prefix, ".storageRoot")))

    {
        LOG(LogDebug, "in AccountGadget", "");
    }

    void generate_r1cs_witness(const AccountLeaf &account)
    {
        LOG(LogDebug, "in AccountGadget", "generate_r1cs_witness");
        pb.val(owner) = account.owner;
        pb.val(publicKey.x) = account.publicKey.x;
        pb.val(publicKey.y) = account.publicKey.y;
        pb.val(appKeyPublicKey.x) = account.appKeyPublicKey.x;
        pb.val(appKeyPublicKey.y) = account.appKeyPublicKey.y;
        pb.val(nonce) = account.nonce;

        pb.val(disableAppKeySpotTrade) = account.disableAppKeySpotTrade;
        pb.val(disableAppKeyWithdraw) = account.disableAppKeyWithdraw;
        pb.val(disableAppKeyTransferToOther) = account.disableAppKeyTransferToOther;
        pb.val(balancesRoot) = account.balancesRoot;
        pb.val(storageRoot) = account.storageRoot;

    }
};

class UpdateAccountGadget : public GadgetT
{
  public:
    HashAccountLeaf leafBefore;
    HashAccountLeaf leafAfter;

    HashAssetAccountLeaf assetLeafBefore;
    HashAssetAccountLeaf assetLeafAfter;

    AccountState valuesBefore;
    AccountState valuesAfter;

    const VariableArrayT proof;
    MerklePathCheckT proofVerifierBefore;
    MerklePathT rootCalculatorAfter;

    const VariableArrayT assetProof;
    MerklePathCheckT assetProofVerifierBefore;
    MerklePathT assetRootCalculatorAfter;

    UpdateAccountGadget(
      ProtoboardT &pb,
      const VariableT &merkleRoot,
      const VariableT &merkleAssetRoot,
      const VariableArrayT &address,
      const AccountState &before,
      const AccountState &after,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          valuesBefore(before),
          valuesAfter(after),

          leafBefore(
            pb,
            var_array(
              {before.owner,
               before.publicKeyX,
               before.publicKeyY,
               before.appKeyPublicKeyX,
               before.appKeyPublicKeyY,
               before.nonce,
               before.disableAppKeySpotTrade,
               before.disableAppKeyWithdraw,
               before.disableAppKeyTransferToOther,
               before.balancesRoot,
               before.storageRoot}),
            FMT(prefix, ".leafBefore")),
          leafAfter(
            pb,
            var_array(
              {after.owner, //
               after.publicKeyX,
               after.publicKeyY,
               after.appKeyPublicKeyX,
               after.appKeyPublicKeyY,
               after.nonce,
               after.disableAppKeySpotTrade,
               after.disableAppKeyWithdraw,
               after.disableAppKeyTransferToOther,
               after.balancesRoot,
               after.storageRoot}),
            FMT(prefix, ".leafAfter")),
          
          // asset tree
          assetLeafBefore(
            pb,
            var_array(
              {before.owner,
               before.publicKeyX,
               before.publicKeyY,
               before.nonce,
               before.balancesRoot}),
            FMT(prefix, ".assetLeafBefore")),
          assetLeafAfter(
            pb,
            var_array(
              {after.owner,
               after.publicKeyX,
               after.publicKeyY,
               after.nonce,
               after.balancesRoot}),
            FMT(prefix, ".assetLeafAfter")),

          proof(make_var_array(pb, TREE_DEPTH_ACCOUNTS * 3, FMT(prefix, ".proof"))),
          proofVerifierBefore(
            pb,
            TREE_DEPTH_ACCOUNTS,
            address,
            leafBefore.result(),
            merkleRoot,
            proof,
            FMT(prefix, ".pathBefore")),
          rootCalculatorAfter( //
            pb,
            TREE_DEPTH_ACCOUNTS,
            address,
            leafAfter.result(),
            proof,
            FMT(prefix, ".pathAfter")),
          

          assetProof(make_var_array(pb, TREE_DEPTH_ACCOUNTS * 3, FMT(prefix, ".assetProof"))),
          assetProofVerifierBefore(
            pb,
            TREE_DEPTH_ACCOUNTS,
            address,
            assetLeafBefore.result(),
            merkleAssetRoot,
            assetProof,
            FMT(prefix, ".assetProofVerifierBefore")),
          assetRootCalculatorAfter( //
            pb,
            TREE_DEPTH_ACCOUNTS,
            address,
            assetLeafAfter.result(),
            assetProof,
            FMT(prefix, ".assetRootCalculatorAfter"))
    {
        std::cout << "in UpdateAccountGadget" << std::endl;
    }

    void generate_r1cs_witness(const AccountUpdate &update)
    {
        leafBefore.generate_r1cs_witness();
        leafAfter.generate_r1cs_witness();

        assetLeafBefore.generate_r1cs_witness();
        assetLeafAfter.generate_r1cs_witness();

        proof.fill_with_field_elements(pb, update.proof.data);
        proofVerifierBefore.generate_r1cs_witness();
        rootCalculatorAfter.generate_r1cs_witness();

        assetProof.fill_with_field_elements(pb, update.assetProof.data);
        assetProofVerifierBefore.generate_r1cs_witness();
        assetRootCalculatorAfter.generate_r1cs_witness();

        // LOG(LogDebug, "in AccountGadget before owner", update.before.owner);
        // LOG(LogDebug, "in AccountGadget before publicKeyX", update.before.publicKey.x);
        // LOG(LogDebug, "in AccountGadget before publicKeyY", update.before.publicKey.y);
        // LOG(LogDebug, "in AccountGadget before nonce", update.before.nonce);
        // LOG(LogDebug, "in AccountGadget before balancesRoot", update.before.balancesRoot);
        // LOG(LogDebug, "in AccountGadget before storageRoot", update.before.storageRoot);
        // LOG(LogDebug, "in AccountGadget before disableAppKeySpotTrade", update.before.disableAppKeySpotTrade);
        // LOG(LogDebug, "in AccountGadget before disableAppKeyWithdraw", update.before.disableAppKeyWithdraw);
        // LOG(LogDebug, "in AccountGadget before disableAppKeyTransferToOther", update.before.disableAppKeyTransferToOther);

        // LOG(LogDebug, "in AccountGadget after owner", update.after.owner);
        // LOG(LogDebug, "in AccountGadget after publicKeyX", update.after.publicKey.x);
        // LOG(LogDebug, "in AccountGadget after publicKeyY", update.after.publicKey.y);
        // LOG(LogDebug, "in AccountGadget after nonce", update.after.nonce);
        // LOG(LogDebug, "in AccountGadget after balancesRoot", update.after.balancesRoot);
        // LOG(LogDebug, "in AccountGadget after storageRoot", update.after.storageRoot);
        // LOG(LogDebug, "in AccountGadget after disableAppKeySpotTrade", update.after.disableAppKeySpotTrade);
        // LOG(LogDebug, "in AccountGadget after disableAppKeyWithdraw", update.after.disableAppKeyWithdraw);
        // LOG(LogDebug, "in AccountGadget after disableAppKeyTransferToOther", update.after.disableAppKeyTransferToOther);

        if (pb.val(rootCalculatorAfter.result()) != update.rootAfter)
        {
            printAccount(pb, valuesBefore);
            printAccount(pb, valuesAfter);
            ASSERT(pb.val(rootCalculatorAfter.result()) == update.rootAfter, annotation_prefix);
        }
        if (pb.val(assetRootCalculatorAfter.result()) != update.assetRootAfter)
        {
            printAccount(pb, valuesBefore);
            printAccount(pb, valuesAfter);
            ASSERT(pb.val(assetRootCalculatorAfter.result()) == update.assetRootAfter, annotation_prefix);
        }
    }

    void generate_r1cs_constraints()
    {
        leafBefore.generate_r1cs_constraints();
        leafAfter.generate_r1cs_constraints();

        assetLeafBefore.generate_r1cs_constraints();
        assetLeafAfter.generate_r1cs_constraints();

        proofVerifierBefore.generate_r1cs_constraints();
        rootCalculatorAfter.generate_r1cs_constraints();

        assetProofVerifierBefore.generate_r1cs_constraints();
        assetRootCalculatorAfter.generate_r1cs_constraints();
    }

    const VariableT &result() const
    {
        return rootCalculatorAfter.result();
    }

    const VariableT &assetResult() const
    {
        return assetRootCalculatorAfter.result();
    }
};

struct BalanceState
{
    VariableT balance;
};

static void printBalance(const ProtoboardT &pb, const BalanceState &state)
{
    std::cout << "- balance: " << pb.val(state.balance) << std::endl;
}

class BalanceGadget : public GadgetT
{
  public:
    VariableT balance;

    BalanceGadget( //
      ProtoboardT &pb,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          balance(make_variable(pb, FMT(prefix, ".balance")))
    {
        LOG(LogDebug, "in BalanceGadget", "");
    }

    void generate_r1cs_witness(const BalanceLeaf &balanceLeaf)
    {
        LOG(LogDebug, "in BalanceGadget", "generate_r1cs_witness");
        pb.val(balance) = balanceLeaf.balance;
    }
};

class UpdateBalanceGadget : public GadgetT
{
  public:
    HashBalanceLeaf leafBefore;
    HashBalanceLeaf leafAfter;

    BalanceState valuesBefore;
    BalanceState valuesAfter;

    const VariableArrayT proof;
    MerklePathCheckT proofVerifierBefore;
    MerklePathT rootCalculatorAfter;

    UpdateBalanceGadget(
      ProtoboardT &pb,
      const VariableT &merkleRoot,
      const VariableArrayT &tokenID,
      const BalanceState before,
      const BalanceState after,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          valuesBefore(before),
          valuesAfter(after),

          leafBefore( //
            pb,
            var_array({before.balance}),
            FMT(prefix, ".leafBefore")),
          leafAfter( //
            pb,
            var_array({after.balance}),
            FMT(prefix, ".leafAfter")),

          proof(make_var_array(pb, TREE_DEPTH_TOKENS * 3, FMT(prefix, ".proof"))),
          proofVerifierBefore(
            pb,
            TREE_DEPTH_TOKENS,
            tokenID,
            leafBefore.result(),
            merkleRoot,
            proof,
            FMT(prefix, ".pathBefore")),
          rootCalculatorAfter( //
            pb,
            TREE_DEPTH_TOKENS,
            tokenID,
            leafAfter.result(),
            proof,
            FMT(prefix, ".pathAfter"))
    {
        LOG(LogDebug, "in UpdateBalanceGadget", "");
    }

    void generate_r1cs_witness(const BalanceUpdate &update)
    {
        leafBefore.generate_r1cs_witness();
        leafAfter.generate_r1cs_witness();

        proof.fill_with_field_elements(pb, update.proof.data);
        proofVerifierBefore.generate_r1cs_witness();
        rootCalculatorAfter.generate_r1cs_witness();
 
        ASSERT(pb.val(proofVerifierBefore.m_expected_root) == update.rootBefore, annotation_prefix);
        if (pb.val(rootCalculatorAfter.result()) != update.rootAfter)
        {
            printBalance(pb, valuesBefore);
            printBalance(pb, valuesAfter);
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
// Calculcates the state of a user's open position
class DynamicBalanceGadget : public DynamicVariableGadget
{
  public:
    DynamicBalanceGadget( //
      ProtoboardT &pb,
      const VariableT &balance,
      const std::string &prefix)
        : DynamicVariableGadget(pb, balance, prefix)
    {
    }

    DynamicBalanceGadget( //
      ProtoboardT &pb,
      const BalanceGadget &balance,
      const std::string &prefix)
        : DynamicBalanceGadget(pb, balance.balance, prefix)
    {
    }

    void generate_r1cs_witness()
    {
    }

    void generate_r1cs_constraints()
    {
    }

    const VariableT &balance() const
    {
        return back();
    }
};

} // namespace Loopring

#endif
