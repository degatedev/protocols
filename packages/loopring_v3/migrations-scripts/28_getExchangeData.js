// Deploy the ExchangeV3 library which is very large in terms of
// gas usage. We need to deploy most libraries linked from it as stand-alone
// libraries, otherwise we'll run into the 'exceeded block gas limit' issue.

const fs = require("fs");
const ExchangeV3 = artifacts.require("./impl/ExchangeV3.sol");
const LoopringV3 = artifacts.require("LoopringV3");
const DefaultDepositContract = artifacts.require("DefaultDepositContract");
const LoopringIOExchangeOwner = artifacts.require("LoopringIOExchangeOwner");
const BlockVerifier = artifacts.require("BlockVerifier");

module.exports = function(deployer, network, accounts) {
  deployer.then(async () => {
    const loopringV3 = await LoopringV3.deployed();

    let lv1 = await loopringV3.forcedWithdrawalFee();
    console.log("loopringV3.address", loopringV3.address);
    console.log("loopringV3.forcedWithdrawalFee old", lv1.toString(10));

    const exchangeV3 = await ExchangeV3.deployed();
    // let ePFv = await exchangeV3.getProtocolFeeValues();
    console.log("exchangeV3.address", exchangeV3.address);
    // console.log("exchangeV3.getProtocolFeeValues", ePFv.protocolFeeBips.toString(10));
    // console.log("exchangeV3.getProtocolFeeValues", ePFv.previousProtocolFeeBips.toString(10));

    let owner = "0x6871979c76a1313949b2E928F75dC316E02bfe33";
    let token = "0x0000000000000000000000000000000000000000";
    let amount = await exchangeV3.getPendingDepositAmount(owner, token);

    console.log("amount", amount.toString(10));
  });
};
