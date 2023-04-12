// Deploy the ExchangeV3 library which is very large in terms of
// gas usage. We need to deploy most libraries linked from it as stand-alone
// libraries, otherwise we'll run into the 'exceeded block gas limit' issue.

const fs = require("fs");
const ExchangeAdmins = artifacts.require("ExchangeAdmins");
const ExchangeBalances = artifacts.require("ExchangeBalances");
const ExchangeBlocks = artifacts.require("ExchangeBlocks");
const ExchangeDeposits = artifacts.require("ExchangeDeposits");
const ExchangeGenesis = artifacts.require("ExchangeGenesis");
const ExchangeTokens = artifacts.require("ExchangeTokens");
const ExchangeWithdrawals = artifacts.require("ExchangeWithdrawals");
const Cloneable = artifacts.require("./thirdparty/Cloneable.sol");
const ExchangeV3 = artifacts.require("./impl/ExchangeV3.sol");
const LoopringV3 = artifacts.require("LoopringV3");
const DefaultDepositContract = artifacts.require("DefaultDepositContract");
const LoopringIOExchangeOwner = artifacts.require("LoopringIOExchangeOwner");

const LRCToken = artifacts.require("./test/tokens/LRC.sol");
const GTOToken = artifacts.require("./test/tokens/GTO.sol");

const BlockVerifier = artifacts.require("BlockVerifier");

module.exports = function(deployer, network, accounts) {
  deployer.then(async () => {
    const loopringV3 = await LoopringV3.deployed();
    const exchangeV3 = await ExchangeV3.deployed();

    let pfv = await loopringV3.getProtocolFeeValues();
    console.log("loopringV3.getProtocolFeeValues old", pfv.toString(10));

    let ePFv = await exchangeV3.getProtocolFeeValues();
    console.log("exchangeV3.getProtocolFeeValues", ePFv.syncedAt.toString(10));
    console.log("exchangeV3.getProtocolFeeValues", ePFv.protocolFeeBips.toString(10));
    console.log("exchangeV3.getProtocolFeeValues", ePFv.previousProtocolFeeBips.toString(10));

    pfv = 30;
    await loopringV3.updateProtocolFeeSettings(pfv);
  });
};
