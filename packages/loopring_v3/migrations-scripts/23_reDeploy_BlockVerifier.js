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
    console.log("BlockVerifier.address old", BlockVerifier.address);

    // 1. deploy new BlockVerifier
    await deployer.link(BatchVerifier, BlockVerifier);
    await deployer.deploy(BlockVerifier);

    console.log("BlockVerifier.address new", BlockVerifier.address);

    // 2. update setting
    const loopringV3 = await LoopringV3.deployed();
    await loopringV3.updateSettings(ProtocolFeeVault.address, BlockVerifier.address, 0);

    // 3. refreshBlockVerifier
    const ownerContract = await LoopringIOExchangeOwner.deployed();
    const exchangeV3 = await ExchangeV3.deployed();
    const refreshData = exchangeV3.contract.methods.refreshBlockVerifier().encodeABI();
    console.log("refreshData:", refreshData);
    await ownerContract.transact(refreshData);
  });
};
