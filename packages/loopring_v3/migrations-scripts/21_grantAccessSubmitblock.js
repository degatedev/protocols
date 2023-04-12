// Deploy the ExchangeV3 library which is very large in terms of
// gas usage. We need to deploy most libraries linked from it as stand-alone
// libraries, otherwise we'll run into the 'exceeded block gas limit' issue.

const fs = require("fs");
const LoopringIOExchangeOwner = artifacts.require("LoopringIOExchangeOwner");

module.exports = function(deployer, network, accounts) {
  deployer.then(async () => {
    console.log("LoopringIOExchangeOwner grantAccess:");

    const ownerContract = await LoopringIOExchangeOwner.deployed();
    const selector = "0x53228430"; // submitBlocks

    const postmanAddress = "0xD587a78e828Fd312282E076b602141951F92829c";

    let ret = await ownerContract.grantAccess(postmanAddress, selector, true);
    console.log(ret);
  });
};
