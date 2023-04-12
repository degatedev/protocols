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

    const tokenAddress = "0x351be5fde823e1d3ecc8b13645e649663bc07b13"; // test token

    const transactData = exchangeV3.contract.methods.registerToken(tokenAddress).encodeABI();
    console.log("transactData:", transactData);
    await ownerContract.transact(transactData);

    const tokenId = await exchangeV3.getTokenID(tokenAddress);
    console.log("tokenId", tokenId.toString(10));
  });
};
