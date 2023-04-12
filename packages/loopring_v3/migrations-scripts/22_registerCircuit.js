// Deploy the ExchangeV3 library which is very large in terms of
// gas usage. We need to deploy most libraries linked from it as stand-alone
// libraries, otherwise we'll run into the 'exceeded block gas limit' issue.

const fs = require("fs");
const BlockVerifier = artifacts.require("BlockVerifier");

module.exports = function(deployer, network, accounts) {
  function flattenList(l) {
    return [].concat.apply([], l);
  }

  function flattenVK(vk) {
    return [
      flattenList([vk.alpha[0], vk.alpha[1], flattenList(vk.beta), flattenList(vk.gamma), flattenList(vk.delta)]),
      flattenList(vk.gammaABC)
    ];
  }

  deployer.then(async () => {
    // set VK in blockVerifier
    const vk = JSON.parse(fs.readFileSync("test/all_2_vk_testb.json", "ascii"));
    const vkFlattened = flattenList(flattenVK(vk));
    const blockVerifier = await BlockVerifier.deployed();

    console.log(accounts[0]);
    console.log("blockVerifier", blockVerifier.address);
    console.log("vkFlattened", vkFlattened);

    ret = await blockVerifier.registerCircuit(0, 2, 0, vkFlattened);

    console.log(ret);
  });
};
