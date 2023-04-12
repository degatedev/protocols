// Deploy all auxiliary contracts used by either Exchange, LoopringV3,
// or UniversalRegistry.

const ProtocolFeeVault = artifacts.require("ProtocolFeeVault");
const BatchVerifier = artifacts.require("BatchVerifier");
const BlockVerifier = artifacts.require("BlockVerifier");
const ExchangeV3 = artifacts.require("./impl/ExchangeV3.sol");
const LoopringV3 = artifacts.require("LoopringV3");
const LoopringIOExchangeOwner = artifacts.require("LoopringIOExchangeOwner");

module.exports = function(deployer, network, accounts) {
  deployer.then(async () => {
    const ownerContract = await LoopringIOExchangeOwner.deployed();
    const exchangeV3 = await ExchangeV3.deployed();

    console.log("ownerContract", ownerContract.address);
    console.log("exchangeV3", exchangeV3.address);

    const freeDepositMax = 500;
    const freeDepositRemained = 3;
    const freeSlotPerBlock = 0;
    const depositFee = "10000000000000000";

    const transactData = exchangeV3.contract.methods
      .setDepositParams(freeDepositMax, freeDepositRemained, freeSlotPerBlock, depositFee)
      .encodeABI();
    console.log("transactData:", transactData);
    await ownerContract.transact(transactData);

    const freeDepositSlot = await exchangeV3.getFreeDepositRemained();
    console.log("freeDepositSlot", freeDepositSlot.toString(10));
  });
};
