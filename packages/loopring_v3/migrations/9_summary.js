const BatchVerifier = artifacts.require("BatchVerifier");
const BlockVerifier = artifacts.require("BlockVerifier");

const ExchangeAdmins = artifacts.require("ExchangeAdmins");
const ExchangeBalances = artifacts.require("ExchangeBalances");
const ExchangeBlocks = artifacts.require("ExchangeBlocks");
const ExchangeDeposits = artifacts.require("ExchangeDeposits");
const ExchangeGenesis = artifacts.require("ExchangeGenesis");
const ExchangeTokens = artifacts.require("ExchangeTokens");
const ExchangeWithdrawals = artifacts.require("ExchangeWithdrawals");
const ExchangeV3 = artifacts.require("./impl/ExchangeV3.sol");
const DefaultDepositContract = artifacts.require("DefaultDepositContract");
const LoopringIOExchangeOwner = artifacts.require("LoopringIOExchangeOwner");
const OwnedUpgradabilityProxy = artifacts.require("./thirdparty/proxies/OwnedUpgradabilityProxy");

const LoopringV3 = artifacts.require("LoopringV3");

module.exports = function(deployer, network, accounts) {
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
  console.log("- LoopringIOExchangeOwner:", LoopringIOExchangeOwner.address);

  if (network != "development") {
    console.log("- ExchangeV3Proxy:", OwnedUpgradabilityProxy.address);
  }
};
