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
    const loopringV3 = await LoopringV3.deployed();

    const protocolFeeVault = await loopringV3.protocolFeeVault();
    const blockVerifierAddress = await loopringV3.blockVerifierAddress();

    console.log("loopringV3 ", loopringV3.address);
    console.log("protocolFeeVault ", protocolFeeVault);
    console.log("BlockVerifier.address ", blockVerifierAddress);

    const forceWithdrawFee = "10000000000000000"; // 0.01 eth
    await loopringV3.updateSettings(protocolFeeVault, blockVerifierAddress, forceWithdrawFee);
  });
};
