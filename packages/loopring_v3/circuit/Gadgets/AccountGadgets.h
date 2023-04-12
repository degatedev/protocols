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
    // DEG-370 add AppKeyKey
    VariableT disableAppKeySpotTrade;
    VariableT disableAppKeyWithdraw;
    VariableT disableAppKeyTransferToOther;
    VariableT balancesRoot;
    // DEG-265 auto market
    // // DEG-170 auto market - config add
    // VariableT autoMarketRoot;
    // DEG-347 Storage location change
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
    // DEG-265 auto market
    // // DEG-170 auto market - config add
    // std::cout << "- autoMarketRoot: " << pb.val(state.autoMarketRoot) << std::endl;
    // DEG-347 Storage location change
    std::cout << "- storageRoot: " << pb.val(state.storageRoot) << std::endl;

}

class AccountGadget : public GadgetT
{
  public:
    VariableT owner;
    const jubjub::VariablePointT publicKey;
    const jubjub::VariablePointT appKeyPublicKey;
    VariableT nonce;
    // DEG-370 add AssetKey
    VariableT disableAppKeySpotTrade;
    VariableT disableAppKeyWithdraw;
    VariableT disableAppKeyTransferToOther;
    VariableT balancesRoot;
    // DEG-265 auto market
    // // DEG-170 auto market - config add
    // VariableT autoMarketRoot;
    // DEG-347 Storage location change
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
          // DEG-265 auto market
          // // DEG-170 auto market - config add
          // autoMarketRoot(make_variable(pb, FMT(prefix, ".autoMarketRoot"))),
          // DEG-347 Storage location change
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
        // DEG-265 auto market
        // // DEG-170 auto market - config add
        // pb.val(autoMarketRoot) = account.autoMarketRoot;
        // DEG-347 Storage location change
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
               // DEG-265 auto market
              //  // DEG-170 auto market - config add
              //  before.autoMarketRoot,
               // DEG-347 Storage location change
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
               // DEG-265 auto market
              //  // DEG-170 auto market - config add
              //  after.autoMarketRoot,
               // DEG-347 Storage location change
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


        LOG(LogDebug, "in AccountGadget before owner", update.before.owner);
        LOG(LogDebug, "in AccountGadget before publicKeyX", update.before.publicKey.x);
        LOG(LogDebug, "in AccountGadget before publicKeyY", update.before.publicKey.y);
        LOG(LogDebug, "in AccountGadget before nonce", update.before.nonce);
        LOG(LogDebug, "in AccountGadget before balancesRoot", update.before.balancesRoot);
        LOG(LogDebug, "in AccountGadget before storageRoot", update.before.storageRoot);
        LOG(LogDebug, "in AccountGadget before disableAppKeySpotTrade", update.before.disableAppKeySpotTrade);
        LOG(LogDebug, "in AccountGadget before disableAppKeyWithdraw", update.before.disableAppKeyWithdraw);
        LOG(LogDebug, "in AccountGadget before disableAppKeyTransferToOther", update.before.disableAppKeyTransferToOther);

        LOG(LogDebug, "in AccountGadget after owner", update.after.owner);
        LOG(LogDebug, "in AccountGadget after publicKeyX", update.after.publicKey.x);
        LOG(LogDebug, "in AccountGadget after publicKeyY", update.after.publicKey.y);
        LOG(LogDebug, "in AccountGadget after nonce", update.after.nonce);
        LOG(LogDebug, "in AccountGadget after balancesRoot", update.after.balancesRoot);
        LOG(LogDebug, "in AccountGadget after storageRoot", update.after.storageRoot);
        LOG(LogDebug, "in AccountGadget after disableAppKeySpotTrade", update.after.disableAppKeySpotTrade);
        LOG(LogDebug, "in AccountGadget after disableAppKeyWithdraw", update.after.disableAppKeyWithdraw);
        LOG(LogDebug, "in AccountGadget after disableAppKeyTransferToOther", update.after.disableAppKeyTransferToOther);

        // ASSERT(pb.val(proofVerifierBefore.m_expected_root) == update.rootBefore, annotation_prefix);
        // if (pb.val(proofVerifierBefore.m_expected_root) != update.rootBefore) 
        // {
        //   std::cout << "rootBefore Before:" << std::endl;
        //   printAccount(pb, valuesBefore);
        //   std::cout << "rootBefore After:" << std::endl;
        //   printAccount(pb, valuesAfter);
        //   ASSERT(pb.val(proofVerifierBefore.m_expected_root) == update.rootBefore, annotation_prefix);
        // }
        if (pb.val(rootCalculatorAfter.result()) != update.rootAfter)
        {
            LOG(LogDebug, "in AccountGadget Before", "");
            printAccount(pb, valuesBefore);
            LOG(LogDebug, "in AccountGadget After", "");
            printAccount(pb, valuesAfter);
            ASSERT(pb.val(rootCalculatorAfter.result()) == update.rootAfter, annotation_prefix);
        }
        // if (pb.val(assetProofVerifierBefore.m_expected_root) == update.assetRootBefore) 
        // {
        //   std::cout << "assetRootBefore Before:" << std::endl;
        //   printAccount(pb, valuesBefore);
        //   std::cout << "assetRootBefore After:" << std::endl;
        //   printAccount(pb, valuesAfter);
        //   ASSERT(pb.val(assetProofVerifierBefore.m_expected_root) == update.assetRootBefore, annotation_prefix);
        // }
        // ASSERT(pb.val(assetProofVerifierBefore.m_expected_root) == update.assetRootBefore, annotation_prefix);
        if (pb.val(assetRootCalculatorAfter.result()) != update.assetRootAfter)
        {
            LOG(LogDebug, "in AccountGadget Asset before", "");
            printAccount(pb, valuesBefore);
            LOG(LogDebug, "in AccountGadget Asset After", "");
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
    // DEG-347 Storage location change
    // VariableT storageRoot;
};

static void printBalance(const ProtoboardT &pb, const BalanceState &state)
{
    std::cout << "- balance: " << pb.val(state.balance) << std::endl;
    // DEG-347 Storage location change
    // std::cout << "- storageRoot: " << pb.val(state.storageRoot) << std::endl;
}

class BalanceGadget : public GadgetT
{
  public:
    VariableT balance;
    // DEG-347 Storage location change
    // VariableT storageRoot;

    BalanceGadget( //
      ProtoboardT &pb,
      const std::string &prefix)
        : GadgetT(pb, prefix),

          balance(make_variable(pb, FMT(prefix, ".balance")))
          // DEG-347 Storage location change
          // storageRoot(make_variable(pb, FMT(prefix, ".storageRoot")))
    {
        LOG(LogDebug, "in BalanceGadget", "");
    }

    void generate_r1cs_witness(const BalanceLeaf &balanceLeaf)
    {
        LOG(LogDebug, "in BalanceGadget", "generate_r1cs_witness");
        pb.val(balance) = balanceLeaf.balance;
        // DEG-347 Storage location change
        // pb.val(storageRoot) = balanceLeaf.storageRoot;
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
            // DEG-347 Storage location change
            // var_array({before.balance, before.storageRoot}),
            var_array({before.balance}),
            FMT(prefix, ".leafBefore")),
          leafAfter( //
            pb,
            // DEG-347 Storage location change
            // var_array({after.balance, after.storageRoot}),
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
        LOG(LogDebug, "in UpdateBalanceGadget before balance", update.before.balance);
        leafBefore.generate_r1cs_witness();
        LOG(LogDebug, "in UpdateBalanceGadget after balance", update.after.balance);
        leafAfter.generate_r1cs_witness();

        proof.fill_with_field_elements(pb, update.proof.data);
        proofVerifierBefore.generate_r1cs_witness();
        rootCalculatorAfter.generate_r1cs_witness();
 
        ASSERT(pb.val(proofVerifierBefore.m_expected_root) == update.rootBefore, annotation_prefix);
        if (pb.val(rootCalculatorAfter.result()) != update.rootAfter)
        {
            LOG(LogDebug, "in UpdateBalanceGadget Before", "");
            printBalance(pb, valuesBefore);
            LOG(LogDebug, "in UpdateBalanceGadget After", "");
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
// DEG-265 auto market
// // DEG-170 auto market - config add
// // DEG-170 AutoMarket-AutoMarket config edit
// struct AutoMarketState
// {
//     VariableT autoMarketID;
//     VariableT tokenSID;
//     VariableT tokenBID;
//     VariableT maxLevel;
//     VariableT gridOffset;
//     VariableT orderOffset;
//     VariableT cancelled;
//     VariableT validUntil;
//     // DEG-170 auto market - storage add
//     VariableT autoMarketStorageRoot;
// };
// static void printBalance(const ProtoboardT &pb, const AutoMarketState &state)
// {
//     std::cout << "- autoMarketID: " << pb.val(state.autoMarketID) << std::endl;
//     std::cout << "- tokenSID: " << pb.val(state.tokenSID) << std::endl;
//     std::cout << "- tokenBID: " << pb.val(state.tokenBID) << std::endl;
//     std::cout << "- maxLevel: " << pb.val(state.maxLevel) << std::endl;
//     std::cout << "- gridOffset: " << pb.val(state.gridOffset) << std::endl;
//     std::cout << "- orderOffset: " << pb.val(state.orderOffset) << std::endl;
//     std::cout << "- cancelled: " << pb.val(state.cancelled) << std::endl;
//     std::cout << "- validUntil: " << pb.val(state.validUntil) << std::endl;
//     std::cout << "- autoMarketStorageRoot: " << pb.val(state.autoMarketStorageRoot) << std::endl;
// }
// // DEG-170 auto market - config add
// class AutoMarketGadget : public GadgetT {
//     public:
//       VariableT autoMarketID;
//       VariableT tokenSID;
//       VariableT tokenBID;
//       VariableT maxLevel;
//       VariableT gridOffset;
//       VariableT orderOffset;
//       VariableT cancelled;
//       VariableT validUntil;
//       // DEG-170 auto market - storage add
//       VariableT autoMarketStorageRoot;
//     AutoMarketGadget(
//       ProtoboardT &pb,
//       const std::string &prefix
//     ) : GadgetT(pb, prefix),
//       autoMarketID(make_variable(pb, FMT(prefix, ".autoMarketID"))),
//       tokenSID(make_variable(pb, FMT(prefix, ".tokenSID"))),
//       tokenBID(make_variable(pb, FMT(prefix, ".tokenBID"))),
//       maxLevel(make_variable(pb, FMT(prefix, ".maxLevel"))),
//       gridOffset(make_variable(pb, FMT(prefix, ".gridOffset"))),
//       orderOffset(make_variable(pb, FMT(prefix, ".orderOffset"))),
//       cancelled(make_variable(pb, FMT(prefix, ".cancelled"))),
//       validUntil(make_variable(pb, FMT(prefix, ".validUntil"))),
//       // DEG-170 auto market - storage add
//       autoMarketStorageRoot(make_variable(pb, FMT(prefix, ".autoMarketStorageRoot")))
//     {

//     }
//     void generate_r1cs_witness(const AutoMarketLeaf &autoMarketLeaf)
//     {
//         pb.val(autoMarketID) = autoMarketLeaf.autoMarketID;
//         pb.val(tokenSID) = autoMarketLeaf.tokenSID;
//         pb.val(tokenBID) = autoMarketLeaf.tokenBID;
//         pb.val(maxLevel) = autoMarketLeaf.maxLevel;
//         pb.val(gridOffset) = autoMarketLeaf.gridOffset;
//         pb.val(orderOffset) = autoMarketLeaf.orderOffset;
//         pb.val(cancelled) = autoMarketLeaf.cancelled;
//         pb.val(validUntil) = autoMarketLeaf.validUntil;
//         // DEG-170 auto market - storage add
//         pb.val(autoMarketStorageRoot) = autoMarketLeaf.autoMarketStorageRoot;
//     }
// };
// // DEG-170 auto market - config add
// // DEG-170 AutoMarket-AutoMarket config edit
// class UpdateAutoMarketGadget : public GadgetT
// {
//   public:
//     HashAutoMarketLeaf leafBefore;
//     HashAutoMarketLeaf leafAfter;

//     AutoMarketState valuesBefore;
//     AutoMarketState valuesAfter;

//     const VariableArrayT proof;
//     MerklePathCheckT proofVerifierBefore;
//     MerklePathT rootCalculatorAfter;

//     UpdateAutoMarketGadget(
//       ProtoboardT &pb,
//       const VariableT &merkleRoot,
//       const VariableArrayT &autoMarketID,
//       const AutoMarketState before,
//       const AutoMarketState after,
//       const std::string &prefix)
//         : GadgetT(pb, prefix),
//           valuesBefore(before),
//           valuesAfter(after),

//           leafBefore( //
//             pb,
//             // DEG-170 auto market - storage add
//             var_array({before.autoMarketID, before.tokenSID, before.tokenBID, before.maxLevel, before.gridOffset, before.orderOffset, before.cancelled, before.validUntil, before.autoMarketStorageRoot}),
//             FMT(prefix, ".leafBefore")),
//           leafAfter( //
//             pb,
//             // DEG-170 auto market - storage add
//             var_array({after.autoMarketID, after.tokenSID, after.tokenBID, after.maxLevel, after.gridOffset, after.orderOffset, after.cancelled, after.validUntil, after.autoMarketStorageRoot}),
//             FMT(prefix, ".leafAfter")),

//           proof(make_var_array(pb, TREE_DEPTH_AUTOMARKET * 3, FMT(prefix, ".proof"))),
//           proofVerifierBefore(
//             pb,
//             TREE_DEPTH_AUTOMARKET,
//             autoMarketID,
//             leafBefore.result(),
//             merkleRoot,
//             proof,
//             FMT(prefix, ".pathBefore")),
//           rootCalculatorAfter( //
//             pb,
//             TREE_DEPTH_AUTOMARKET,
//             autoMarketID,
//             leafAfter.result(),
//             proof,
//             FMT(prefix, ".pathAfter"))
//     {
//     }
//     void generate_r1cs_witness(const AutoMarketUpdate &update)
//     {    
//         std::cout << "in AutoMarketGadget Before leafBefore" << std::endl;
//         std::cout << "in AutoMarketGadget Before leafBefore autoMarketID:" << update.before.autoMarketID << std::endl;
//         std::cout << "in AutoMarketGadget Before leafBefore tokenSID:" << update.before.tokenSID << std::endl;
//         std::cout << "in AutoMarketGadget Before leafBefore tokenBID:" << update.before.tokenBID << std::endl;
//         std::cout << "in AutoMarketGadget Before leafBefore maxLevel:" << update.before.maxLevel << std::endl;
//         std::cout << "in AutoMarketGadget Before leafBefore gridOffset:" << update.before.gridOffset << std::endl;
//         std::cout << "in AutoMarketGadget Before leafBefore orderOffset:" << update.before.orderOffset << std::endl;
//         std::cout << "in AutoMarketGadget Before leafBefore cancelled:" << update.before.cancelled << std::endl;
//         std::cout << "in AutoMarketGadget Before leafBefore validUntil:" << update.before.validUntil << std::endl;
//         std::cout << "in AutoMarketGadget Before leafBefore autoMarketStorageRoot:" << update.before.autoMarketStorageRoot << std::endl;
//         leafBefore.generate_r1cs_witness();
//         std::cout << "in AutoMarketGadget Before leafAfter" << std::endl;
//         std::cout << "in AutoMarketGadget Before leafAfter autoMarketID:" << update.after.autoMarketID << std::endl;
//         std::cout << "in AutoMarketGadget Before leafAfter tokenSID:" << update.after.tokenSID << std::endl;
//         std::cout << "in AutoMarketGadget Before leafAfter tokenBID:" << update.after.tokenBID << std::endl;
//         std::cout << "in AutoMarketGadget Before leafAfter maxLevel:" << update.after.maxLevel << std::endl;
//         std::cout << "in AutoMarketGadget Before leafAfter gridOffset:" << update.after.gridOffset << std::endl;
//         std::cout << "in AutoMarketGadget Before leafAfter orderOffset:" << update.after.orderOffset << std::endl;
//         std::cout << "in AutoMarketGadget Before leafAfter cancelled:" << update.after.cancelled << std::endl;
//         std::cout << "in AutoMarketGadget Before leafAfter validUntil:" << update.after.validUntil << std::endl;
//         std::cout << "in AutoMarketGadget Before leafAfter autoMarketStorageRoot:" << update.after.autoMarketStorageRoot << std::endl;
//         leafAfter.generate_r1cs_witness();
//         std::cout << "in AutoMarketGadget Before proof" << std::endl;
//         proof.fill_with_field_elements(pb, update.proof.data);
//         std::cout << "in AutoMarketGadget Before proofVerifierBefore" << std::endl;
//         proofVerifierBefore.generate_r1cs_witness();
//         std::cout << "in AutoMarketGadget Before rootCalculatorAfter" << std::endl;
//         rootCalculatorAfter.generate_r1cs_witness();

//         // ASSERT(pb.val(proofVerifierBefore.m_expected_root) == update.rootBefore,
//         // annotation_prefix);
//         if (pb.val(rootCalculatorAfter.result()) != update.rootAfter)
//         {
//             std::cout << "Before:" << std::endl;
//             printBalance(pb, valuesBefore);
//             std::cout << "After:" << std::endl;
//             printBalance(pb, valuesAfter);
//             ASSERT(pb.val(rootCalculatorAfter.result()) == update.rootAfter, annotation_prefix);
//         }
//     }

//     void generate_r1cs_constraints()
//     {
//         leafBefore.generate_r1cs_constraints();
//         leafAfter.generate_r1cs_constraints();

//         proofVerifierBefore.generate_r1cs_constraints();
//         rootCalculatorAfter.generate_r1cs_constraints();
//     }

//     const VariableT &result() const
//     {
//         return rootCalculatorAfter.result();
//     }
// };
// // DEG-170 auto market - storage add
// // AutoMarketStorage
// struct AutoMarketStorageState
// {
//     VariableT autoMarketID;
//     VariableT tokenStorageID;
//     VariableT forward;
//     VariableT data;
// };
// static void printBalance(const ProtoboardT &pb, const AutoMarketStorageState &state)
// {
//     std::cout << "- autoMarketID: " << pb.val(state.autoMarketID) << std::endl;
//     std::cout << "- tokenStorageID: " << pb.val(state.tokenStorageID) << std::endl;
//     std::cout << "- forward: " << pb.val(state.forward) << std::endl;
//     std::cout << "- data: " << pb.val(state.data) << std::endl;
// }
// // DEG-170 auto market - storage add
// class AutoMarketStorageGadget : public GadgetT {
//     public:
//       VariableT autoMarketID;
//       VariableT tokenStorageID;
//       VariableT forward;
//       VariableT data;
//     AutoMarketStorageGadget(
//       ProtoboardT &pb,
//       const std::string &prefix
//     ) : GadgetT(pb, prefix),
//       autoMarketID(make_variable(pb, FMT(prefix, ".autoMarketID"))),
//       tokenStorageID(make_variable(pb, FMT(prefix, ".tokenStorageID"))),
//       forward(make_variable(pb, FMT(prefix, ".forward"))),
//       data(make_variable(pb, FMT(prefix, ".data")))
//     {

//     }
//     void generate_r1cs_witness(const AutoMarketStorageLeaf &autoMarketStorageLeaf)
//     {
//         pb.val(autoMarketID) = autoMarketStorageLeaf.autoMarketID;
//         pb.val(tokenStorageID) = autoMarketStorageLeaf.tokenStorageID;
//         std::cout << "in AutoMarketStorageGadget tokenStorageID: " << autoMarketStorageLeaf.tokenStorageID << std::endl;
//         pb.val(forward) = autoMarketStorageLeaf.forward;
//         pb.val(data) = autoMarketStorageLeaf.data;
//     }
// };
// // DEG-170 auto market - storage add
// class UpdateAutoMarketStorageGadget : public GadgetT
// {
//   public:
//     const VariableT m_expected_root;
//     HashAutoMarketStorageLeaf leafBefore;
//     HashAutoMarketStorageLeaf leafAfter;

//     AutoMarketStorageState valuesBefore;
//     AutoMarketStorageState valuesAfter;

//     const VariableArrayT proof;
//     MerklePathCheckT proofVerifierBefore;
//     MerklePathT rootCalculatorAfter;

//     UpdateAutoMarketStorageGadget(
//       ProtoboardT &pb,
//       const VariableT &merkleRoot,
//       const VariableArrayT &autoMarketStorageID,
//       const AutoMarketStorageState before,
//       const AutoMarketStorageState after,
//       const std::string &prefix)
//         : GadgetT(pb, prefix),
//           m_expected_root(merkleRoot),
//           valuesBefore(before),
//           valuesAfter(after),

//           leafBefore( //
//             pb,
//             var_array({before.autoMarketID, before.tokenStorageID, before.forward, before.data}),
//             FMT(prefix, ".leafBefore")),
//           leafAfter( //
//             pb,
//             var_array({after.autoMarketID, after.tokenStorageID, after.forward, after.data}),
//             FMT(prefix, ".leafAfter")),

//           proof(make_var_array(pb, TREE_DEPTH_AUTOMARKET_STORAGE * 3, FMT(prefix, ".proof"))),
//           proofVerifierBefore(
//             pb,
//             TREE_DEPTH_AUTOMARKET_STORAGE,
//             autoMarketStorageID,
//             leafBefore.result(),
//             merkleRoot,
//             proof,
//             FMT(prefix, ".pathBefore")),
//           rootCalculatorAfter( //
//             pb,
//             TREE_DEPTH_AUTOMARKET_STORAGE,
//             autoMarketStorageID,
//             leafAfter.result(),
//             proof,
//             FMT(prefix, ".pathAfter"))
//     {
//     }
//     void generate_r1cs_witness(const AutoMarketStorageUpdate &update)
//     {    
//         std::cout << "in UpdateAutoMarketStorageGadget Before leafBefore" << std::endl;
//         std::cout << "in UpdateAutoMarketStorageGadget Before leafBefore autoMarketID:" << update.before.autoMarketID << std::endl;
//         std::cout << "in UpdateAutoMarketStorageGadget Before leafBefore tokenStorageID:" << update.before.tokenStorageID << std::endl;
//         std::cout << "in UpdateAutoMarketStorageGadget Before leafBefore forward:" << update.before.forward << std::endl;
//         std::cout << "in UpdateAutoMarketStorageGadget Before leafBefore data:" << update.before.data << std::endl;
//         leafBefore.generate_r1cs_witness();
//         std::cout << "in UpdateAutoMarketStorageGadget Before leafAfter" << std::endl;
//         std::cout << "in UpdateAutoMarketStorageGadget Before leafAfter autoMarketID:" << update.after.autoMarketID << std::endl;
//         std::cout << "in UpdateAutoMarketStorageGadget Before leafAfter tokenStorageID:" << update.after.tokenStorageID << std::endl;
//         std::cout << "in UpdateAutoMarketStorageGadget Before leafAfter forward:" << update.after.forward << std::endl;
//         std::cout << "in UpdateAutoMarketStorageGadget Before leafAfter data:" << update.after.data << std::endl;
//         leafAfter.generate_r1cs_witness();
//         std::cout << "in UpdateAutoMarketStorageGadget Before proof" << std::endl;
//         proof.fill_with_field_elements(pb, update.proof.data);
//         std::cout << "in UpdateAutoMarketStorageGadget Before proofVerifierBefore" << std::endl;
//         proofVerifierBefore.generate_r1cs_witness();
//         std::cout << "in UpdateAutoMarketStorageGadget Before rootCalculatorAfter" << std::endl;
//         std::cout << "in UpdateAutoMarketStorageGadget Before pb.val(proofVerifierBefore.m_expected_root)：" << pb.val(proofVerifierBefore.m_expected_root) << std::endl;
//         std::cout << "in UpdateAutoMarketStorageGadget Before update.rootBefore：" << update.rootBefore << std::endl;
//         rootCalculatorAfter.generate_r1cs_witness();
//         std::cout << "in UpdateAutoMarketStorageGadget Before pb.val(proofVerifierBefore.m_expected_root)：" << pb.val(proofVerifierBefore.m_expected_root) << std::endl;
//         std::cout << "in UpdateAutoMarketStorageGadget Before pb.val(m_expected_root)：" << pb.val(m_expected_root) << std::endl;
//         if (pb.val(proofVerifierBefore.m_expected_root) != update.rootBefore) {
//             std::cout << "Before:" << std::endl;
//             printBalance(pb, valuesBefore);
//             std::cout << "After:" << std::endl;
//             printBalance(pb, valuesAfter);
//             ASSERT(pb.val(proofVerifierBefore.m_expected_root) == update.rootBefore, annotation_prefix);
//         }
//         std::cout << "in UpdateAutoMarketStorageGadget Before pb.val(rootCalculatorAfter.result())：" << pb.val(rootCalculatorAfter.result()) << std::endl;
//         std::cout << "in UpdateAutoMarketStorageGadget Before pb.val(update.rootAfter)：" << update.rootAfter << std::endl;
//         if (pb.val(rootCalculatorAfter.result()) != update.rootAfter)
//         {
//             std::cout << "Before:" << std::endl;
//             printBalance(pb, valuesBefore);
//             std::cout << "After:" << std::endl;
//             printBalance(pb, valuesAfter);
//             ASSERT(pb.val(rootCalculatorAfter.result()) == update.rootAfter, annotation_prefix);
//         }
//     }

//     void generate_r1cs_constraints()
//     {
//         leafBefore.generate_r1cs_constraints();
//         leafAfter.generate_r1cs_constraints();

//         proofVerifierBefore.generate_r1cs_constraints();
//         rootCalculatorAfter.generate_r1cs_constraints();
//     }

//     const VariableT &result() const
//     {
//         return rootCalculatorAfter.result();
//     }
// };

// class AutoMarketReaderGadget : public GadgetT
// {
//     NotEqualGadget verify;
//     LeqGadget autoMarketID_leq_leafAutoMarketID;
//     IfThenRequireGadget requireValidAutoMarketID;

//     TernaryGadget autoMarketID;
//     TernaryGadget tokenSID;
//     TernaryGadget tokenBID;
//     TernaryGadget maxLevel;
//     TernaryGadget gridOffset;
//     TernaryGadget orderOffset;
//     TernaryGadget cancelled;
//     TernaryGadget validUntil;

//   public:
//     AutoMarketReaderGadget(
//       ProtoboardT &pb,
//       const Constants &constants,
//       const AutoMarketGadget &storage,
//       const DualVariableGadget &autoMarketID,
//       const VariableT &autoMarketOrder,
//       const std::string &prefix)
//         : GadgetT(pb, prefix),
//           verify(pb, autoMarketOrder, constants._0, FMT(prefix, ".requireValidAutoMarketID")),
//           autoMarketID_leq_leafAutoMarketID(pb, autoMarketID.packed, storage.autoMarketID, NUM_BITS_AUTOMARKETID, FMT(prefix, ".autoMarketID_leq_leafAutoMarketID")),
//           requireValidAutoMarketID(pb, verify.result(), autoMarketID_leq_leafAutoMarketID.gte(), FMT(prefix, ".requireValidAutoMarketID")),

//           autoMarketID(pb, autoMarketID_leq_leafAutoMarketID.eq(), storage.autoMarketID, constants._0, FMT(prefix, ".autoMarketID")),
//           tokenSID(pb, autoMarketID_leq_leafAutoMarketID.eq(), storage.tokenSID, constants._0, FMT(prefix, ".tokenSID")),
//           tokenBID(pb, autoMarketID_leq_leafAutoMarketID.eq(), storage.tokenBID, constants._0, FMT(prefix, ".tokenBID")),
//           maxLevel(pb, autoMarketID_leq_leafAutoMarketID.eq(), storage.maxLevel, constants._0, FMT(prefix, ".maxLevel")),
//           gridOffset(pb, autoMarketID_leq_leafAutoMarketID.eq(), storage.gridOffset, constants._0, FMT(prefix, ".gridOffset")),
//           orderOffset(pb, autoMarketID_leq_leafAutoMarketID.eq(), storage.orderOffset, constants._0, FMT(prefix, ".orderOffset")),
//           cancelled(pb, autoMarketID_leq_leafAutoMarketID.eq(), storage.cancelled, constants._0, FMT(prefix, ".cancelled")),
//           validUntil(pb, autoMarketID_leq_leafAutoMarketID.eq(), storage.validUntil, constants._0, FMT(prefix, ".validUntil"))
//     {
//     }

//     void generate_r1cs_witness()
//     {
//         verify.generate_r1cs_witness();
//         autoMarketID_leq_leafAutoMarketID.generate_r1cs_witness();
//         requireValidAutoMarketID.generate_r1cs_witness();

//         autoMarketID.generate_r1cs_witness();
//         tokenSID.generate_r1cs_witness();
//         tokenBID.generate_r1cs_witness();
//         maxLevel.generate_r1cs_witness();
//         gridOffset.generate_r1cs_witness();
//         orderOffset.generate_r1cs_witness();
//         cancelled.generate_r1cs_witness();
//         validUntil.generate_r1cs_witness();
//     }

//     void generate_r1cs_constraints()
//     {
//         verify.generate_r1cs_constraints();
//         autoMarketID_leq_leafAutoMarketID.generate_r1cs_constraints();
//         requireValidAutoMarketID.generate_r1cs_constraints();

//         autoMarketID.generate_r1cs_constraints();
//         tokenSID.generate_r1cs_constraints();
//         tokenBID.generate_r1cs_constraints();
//         maxLevel.generate_r1cs_constraints();
//         gridOffset.generate_r1cs_constraints();
//         orderOffset.generate_r1cs_constraints();
//         cancelled.generate_r1cs_constraints();
//         validUntil.generate_r1cs_constraints();
//     }

//     const VariableT &getAutoMarketID() const
//     {
//         return autoMarketID.result();
//     }
//     const VariableT &getTokenSID() const
//     {
//         return tokenSID.result();
//     }
//     const VariableT &getTokenBID() const
//     {
//         return tokenBID.result();
//     }
//     const VariableT &getMaxLevel() const
//     {
//         return maxLevel.result();
//     }
//     const VariableT &getGridOffset() const
//     {
//         return gridOffset.result();
//     }
//     const VariableT &getOrderOffset() const
//     {
//         return orderOffset.result();
//     }
//     const VariableT &getCancelled() const
//     {
//         return cancelled.result();
//     }
//     const VariableT &getValidUntil() const
//     {
//         return validUntil.result();
//     }
// };

// class AutoMarketStorageReaderGadget : public GadgetT
// {
//     EqualGadget autoMarketID_eq_storaged;

//     // TernaryGadget autoMarketID;
//     TernaryGadget tokenStorageID;
//     TernaryGadget forward;
//     TernaryGadget data;

//   public:
//     AutoMarketStorageReaderGadget(
//       ProtoboardT &pb,
//       const Constants &constants,
//       const AutoMarketStorageGadget &storage,
//       const DualVariableGadget &autoMarketID,
//       // const VariableT &verify,
//       // const VariableT &isNewOrder,
//       const std::string &prefix)
//         : GadgetT(pb, prefix),
//           autoMarketID_eq_storaged(pb, autoMarketID.packed, storage.autoMarketID, FMT(prefix, ".autoMarketID_eq_storaged")),
//           // TODO need check logic, default is 0
//           // autoMarketID(pb, autoMarketID_eq_storaged.result(), storage.autoMarketID, constants._0, FMT(prefix, ".autoMarketID")),
//           tokenStorageID(pb, autoMarketID_eq_storaged.result(), storage.tokenStorageID, constants._0, FMT(prefix, ".tokenStorageID")),
//           forward(pb, autoMarketID_eq_storaged.result(), storage.forward, constants._1, FMT(prefix, ".forward")),
//           data(pb, autoMarketID_eq_storaged.result(), storage.data, constants._0, FMT(prefix, ".data"))
//           // data(pb, isNewOrder, storage.data, constants._0, FMT(prefix, ".data"))
//     {
//     }

//     void generate_r1cs_witness()
//     {
//         autoMarketID_eq_storaged.generate_r1cs_witness();

//         // autoMarketID.generate_r1cs_witness();
//         tokenStorageID.generate_r1cs_witness();
//         forward.generate_r1cs_witness();
//         data.generate_r1cs_witness();
//         // std::cout << "in AutoMarketStorageReaderGadget autoMarketID_eq_storaged："<< pb.val(autoMarketID_eq_storaged.result()) << std::endl;
//         // std::cout << "in AutoMarketStorageReaderGadget tokenStorageID"<< pb.val(tokenStorageID.result()) << std::endl;
//     }

//     void generate_r1cs_constraints()
//     {
//         autoMarketID_eq_storaged.generate_r1cs_constraints();

//         // autoMarketID.generate_r1cs_constraints();
//         tokenStorageID.generate_r1cs_constraints();
//         forward.generate_r1cs_constraints();
//         data.generate_r1cs_constraints();
//     }

//     // const VariableT &getAutoMarketID() const
//     // {
//     //     return autoMarketID.result();
//     // }
//     const VariableT &getTokenStorageID() const
//     {
//         return tokenStorageID.result();
//     }
//     const VariableT &getForward() const
//     {
//         return forward.result();
//     }
//     const VariableT &getData() const
//     {
//         return data.result();
//     }
// };
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
