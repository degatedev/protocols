#include "../ThirdParty/catch.hpp"
#include "TestUtils.h"

#include "../Gadgets/AccountGadgets.h"

AccountState createAccountState2(ProtoboardT &pb, const AccountLeaf &state)
{
    AccountState accountState;
    accountState.owner = make_variable(pb, state.owner, ".owner");
    accountState.publicKeyX = make_variable(pb, state.publicKey.x, ".publicKeyX");
    accountState.publicKeyY = make_variable(pb, state.publicKey.y, ".publicKeyY");
    accountState.nonce = make_variable(pb, state.nonce, ".nonce");
    accountState.balancesRoot = make_variable(pb, state.balancesRoot, ".balancesRoot");
    return accountState;
}

TEST_CASE("Account", "[AccountGadget]")
{
    Block block = getBlock();
    const UniversalTransaction &tx = getAccountUpdate(block);
    std::cout << "in Account Update test: tx:" << tx.witness.balanceUpdateFee_A.after.balance << std::endl;
    
    const FieldT &_exchange = block.exchange;
    // const Order &order = tx.spotTrade.orderA;
    const AccountUpdateTx &accountUpdate = tx.accountUpdate;
    const AccountUpdate &accountUpdate_A = tx.witness.accountUpdate_A;

    //const AccountLeaf &account2 = tx.witness.accountUpdate_B.before;

    SECTION("Valid account update")
    {
        std::cout << "in account update test: start valid account update" << std::endl;
        // accountUpdateChecked(_exchange, accountUpdate, accountUpdate_A, true);

        bool expectedSatisfied = true;
        protoboard<FieldT> pb;

        VariableT exchange = make_variable(pb, _exchange, "exchange");

        Constants constants(pb, "constants");
        jubjub::Params params;
        VariableT timestamp = make_variable(pb, 0, "timestamp");;
        
        // DualVariableGadget protocolTakerFeeBips(pb, NUM_BITS_PROTOCOL_FEE_BIPS, ".protocolTakerFeeBips");
        // DualVariableGadget protocolMakerFeeBips(pb, NUM_BITS_PROTOCOL_FEE_BIPS, ".protocolMakerFeeBips");
        // VariableT numConditionalTransactionsBefore = make_variable(pb, _timestamp, "timestamp");
        // DualVariableGadget type(pb, NUM_BITS_TX_TYPE, ".type");
        DualVariableGadget accountIDAddress(pb, NUM_BITS_ACCOUNT, ".accountID");
        AccountState accountBefore = createAccountState2(pb, accountUpdate_A.before);
        AccountState accountAfter = createAccountState2(pb, accountUpdate_A.after);
        DualVariableGadget merkleRootBefore(pb, 256, ".merkleRootBefore");
        DualVariableGadget merkleAssetRootBefore(pb, 256, ".merkleAssetRootBefore");
        UpdateAccountGadget updateAccountGadget(pb, merkleRootBefore.packed, merkleAssetRootBefore.packed, accountIDAddress.bits, accountBefore, accountAfter, ".updateAccountGadget");

        merkleRootBefore.generate_r1cs_witness(pb, accountUpdate_A.rootBefore);
        merkleAssetRootBefore.generate_r1cs_witness(pb, accountUpdate_A.assetRootBefore);
        accountIDAddress.generate_r1cs_witness(pb, accountUpdate_A.accountID);
        updateAccountGadget.generate_r1cs_witness(accountUpdate_A);

        merkleRootBefore.generate_r1cs_constraints(true);
        merkleAssetRootBefore.generate_r1cs_constraints(true);
        accountIDAddress.generate_r1cs_constraints();
        updateAccountGadget.generate_r1cs_constraints();
        REQUIRE(pb.is_satisfied() == expectedSatisfied);
    }
    // SECTION("Valid account update circuit") 
    // {
    //     std::cout << "in account update test: start valid account update circuit" << std::endl;
    //     // accountUpdateChecked(_exchange, accountUpdate, accountUpdate_A, true);

    //     bool expectedSatisfied = true;
    //     protoboard<FieldT> pb;

    //     VariableT exchange = make_variable(pb, _exchange, "exchange");

    //     Constants constants(pb, "constants");
    //     jubjub::Params params;
    //     VariableT timestamp = make_variable(pb, 0, "timestamp");;

    //     DualVariableGadget protocolTakerFeeBips(pb, NUM_BITS_PROTOCOL_FEE_BIPS, ".protocolTakerFeeBips");
    //     DualVariableGadget protocolMakerFeeBips(pb, NUM_BITS_PROTOCOL_FEE_BIPS, ".protocolMakerFeeBips");
    //     VariableT numConditionalTransactionsBefore = make_variable(pb, 0, "timestamp");
    //     DualVariableGadget type(pb, NUM_BITS_TX_TYPE, ".type");

    //     Loopring::TransactionState state(pb,
    //         params,
    //         constants,
    //         exchange,
    //         timestamp,
    //         protocolTakerFeeBips,
    //         protocolMakerFeeBips,
    //         numConditionalTransactionsBefore,
    //         type.packed,
    //         ".transactionState");
    //     AccountUpdateCircuit accountUpdateCircuit(pb, state, ".accountUpdateCircuit");

    //     accountUpdateCircuit.generate_r1cs_witness(accountUpdate);
    //     accountUpdateCircuit.generate_r1cs_constraints();
    // }
}
