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
const OwnedUpgradabilityProxy = artifacts.require("./thirdparty/proxies/OwnedUpgradabilityProxy");

const LRCToken = artifacts.require("./test/tokens/LRC.sol");
const GTOToken = artifacts.require("./test/tokens/GTO.sol");

const BatchVerifier = artifacts.require("BatchVerifier");
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
    await deployer.link(ExchangeBalances, ExchangeV3);
    await deployer.link(ExchangeAdmins, ExchangeV3);
    await deployer.link(ExchangeBlocks, ExchangeV3);
    await deployer.link(ExchangeTokens, ExchangeV3);
    await deployer.link(ExchangeGenesis, ExchangeV3);
    await deployer.link(ExchangeDeposits, ExchangeV3);
    await deployer.link(ExchangeWithdrawals, ExchangeV3);

    await deployer.deploy(ExchangeV3, { gas: 6700000 });
    await deployer.deploy(DefaultDepositContract, { gas: 6700000 });

    if (network == "development") {
      await deployer.deploy(LoopringIOExchangeOwner, ExchangeV3.address, {
        gas: 6700000
      });
    }

    if (process.env.TEST_ENV == "docker") {
      console.log("setup exchange:");
      const emptyMerkleRoot = "0x3e1788bf14436c39a3841ae888ffb3e6ec8405bc2773afa28b6d4dfc309cf19";
      const emptyMerkleAssetRoot = "0x71c8b14d71d432750479f5fe6e08abe1ec04712835a83cdf84d0483b9382ae8";

      const exchangeV3 = await ExchangeV3.deployed();
      await exchangeV3.initialize(LoopringV3.address, process.env.FROM, emptyMerkleRoot, emptyMerkleAssetRoot);

      // await exchangeV3.setAgentRegistry(AgentRegistry.address);

      const depositContract = await DefaultDepositContract.deployed();
      await depositContract.initialize(ExchangeV3.address);
      await exchangeV3.setDepositContract(depositContract.address);

      console.log("setup amms:");
      // setup amm:
      // register tokens:
      await exchangeV3.registerToken(GTOToken.address);

      // set ownerContract:
      const ownerContract = await LoopringIOExchangeOwner.deployed();
      await exchangeV3.transferOwnership(ownerContract.address);
      const claimData = exchangeV3.contract.methods.claimOwnership().encodeABI();
      // console.log("claimData:", claimData);
      await ownerContract.transact(claimData);
      await ownerContract.openAccessToSubmitBlocks(true);

      // do setup:

      const lrcToken = await LRCToken.deployed();
      const gtoToken = await GTOToken.deployed();
      // do the deposit for all accounts:
      for (const account of accounts) {
        console.log("do deposit for:", account);
        await lrcToken.approve(depositContract.address, "1" + "0".repeat(28), {
          from: account
        });
        await gtoToken.approve(depositContract.address, "1" + "0".repeat(28), {
          from: account
        });

        await exchangeV3.deposit(account, account, LRCToken.address, "1" + "0".repeat(26), [], {
          from: account,
          gas: 200000
        });

        await exchangeV3.deposit(account, account, GTOToken.address, "1" + "0".repeat(26), [], {
          from: account,
          gas: 200000
        });

        await exchangeV3.deposit(account, account, "0x" + "00".repeat(20), "1" + "0".repeat(20), [], {
          from: account,
          gas: 200000,
          value: "1" + "0".repeat(20)
        });
      }

      // set VK in blockVerifier:
      const vk = JSON.parse(fs.readFileSync("test/all_16_vk.json", "ascii"));
      const vkFlattened = flattenList(flattenVK(vk));
      const blockVerifier = await BlockVerifier.deployed();
      await blockVerifier.registerCircuit(0, 16, 0, vkFlattened);
    }

    if (network == "live" || network == "live-fork" || network == "goerli") {
      const depositContractProxy = await deployer.deploy(OwnedUpgradabilityProxy);
      const exchangeV3Proxy = await deployer.deploy(OwnedUpgradabilityProxy);

      await deployer.deploy(LoopringIOExchangeOwner, exchangeV3Proxy.address, {
        gas: 6700000
      });

      console.log("exchange init:");
      const emptyMerkleRoot = "0x3e1788bf14436c39a3841ae888ffb3e6ec8405bc2773afa28b6d4dfc309cf19";
      const emptyMerkleAssetRoot = "0x71c8b14d71d432750479f5fe6e08abe1ec04712835a83cdf84d0483b9382ae8";

      const exchangeV3Imp = await ExchangeV3.deployed();
      await exchangeV3Proxy.upgradeTo(exchangeV3Imp.address);

      const exchangeV3 = await ExchangeV3.at(exchangeV3Proxy.address);

      await exchangeV3.initialize(LoopringV3.address, accounts[0], emptyMerkleRoot, emptyMerkleAssetRoot);

      console.log("exchange setDepositContract:");

      const depositContractImp = await DefaultDepositContract.deployed();
      await depositContractProxy.upgradeTo(depositContractImp.address);
      const depositContract = await DefaultDepositContract.at(depositContractProxy.address);

      await depositContract.initialize(exchangeV3.address);
      await exchangeV3.setDepositContract(depositContract.address);

      console.log("exchange transferOwnership:");

      const ownerContract = await LoopringIOExchangeOwner.deployed();
      await exchangeV3.transferOwnership(ownerContract.address);
      const claimData = exchangeV3.contract.methods.claimOwnership().encodeABI();
      await ownerContract.transact(claimData);

      console.log(">>> deployed to network: " + network);

      console.log("- BatchVerifier:", BatchVerifier.address);
      console.log("- BlockVerifier:", BlockVerifier.address);

      console.log("- LoopringV3:", LoopringV3.address);
      console.log("- ExchangeAdmins:", ExchangeAdmins.address);
      console.log("- ExchangeBalances:", ExchangeBalances.address);
      console.log("- ExchangeBlocks:", ExchangeBlocks.address);
      console.log("- ExchangeDeposits:", ExchangeDeposits.address);
      console.log("- ExchangeGenesis:", ExchangeGenesis.address);
      console.log("- ExchangeTokens:", ExchangeTokens.address);
      console.log("- ExchangeWithdrawals:", ExchangeWithdrawals.address);
      console.log("- ExchangeV3Imp:", ExchangeV3.address);
      console.log("- DefaultDepositContractImp:", DefaultDepositContract.address);

      if (network != "development") {
        console.log("- ExchangeV3Proxy:", exchangeV3.address);
        console.log("- DepositContractProxy:", depositContract.address);
      }

      console.log("- LoopringIOExchangeOwner:", LoopringIOExchangeOwner.address);
    }
  });
};
