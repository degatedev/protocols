import BN = require("bn.js");
import { Constants, roundToFloatValue } from "loopringV3.js";
import { expectThrow } from "./expectThrow";
import { BalanceSnapshot, ExchangeTestUtil } from "./testExchangeUtil";
import { AuthMethod, Deposit, SpotTrade, OrderInfo } from "./types";

contract("Exchange", (accounts: string[]) => {
  let exchangeTestUtil: ExchangeTestUtil;
  let exchange: any;
  let depositContract: any;
  let loopring: any;
  let exchangeID = 0;

  let ownerA: string;

  const createExchange = async (setupTestState: boolean = true) => {
    exchangeID = await exchangeTestUtil.createExchange(exchangeTestUtil.testContext.stateOwners[0], { setupTestState });
    exchange = exchangeTestUtil.exchange;
    depositContract = exchangeTestUtil.depositContract;
  };

  before(async () => {
    exchangeTestUtil = new ExchangeTestUtil();
    await exchangeTestUtil.initialize(accounts);
    exchange = exchangeTestUtil.exchange;
    loopring = exchangeTestUtil.loopringV3;
    depositContract = exchangeTestUtil.depositContract;
    exchangeID = 1;

    ownerA = exchangeTestUtil.testContext.orderOwners[0];
  });

  after(async () => {
    await exchangeTestUtil.stop();
  });

  describe("ProtocolFeeBips", function() {
    this.timeout(0);

    it("update ProtocolFeeBips with delay", async () => {
      await createExchange();

      // 1. old ProtocolFeeBips
      {
        const protocolFees = await exchange.getProtocolFeeValues();
        console.log("syncedAt", protocolFees.syncedAt.toNumber());
        console.log("protocolFeeBips", protocolFees.protocolFeeBips.toNumber());
        console.log("previousProtocolFeeBips", protocolFees.previousProtocolFeeBips.toNumber());
        console.log("executeTimeOfNextProtocolFeeBips", protocolFees.executeTimeOfNextProtocolFeeBips.toNumber());
        console.log("nextProtocolFeeBips", protocolFees.nextProtocolFeeBips.toNumber());
      }

      // 2. set newProtocolfeeBips to loopring
      const newProtocolfeeBips = 80;
      await loopring.updateProtocolFeeSettings(newProtocolfeeBips, {
        from: exchangeTestUtil.testContext.deployer
      });

      const protocolFeeValuesInLoopringV3 = await loopring.getProtocolFeeValues();
      console.log("protocolFeeValuesInLoopringV3", protocolFeeValuesInLoopringV3.toNumber());

      await exchangeTestUtil.submitTransactions(2);
      await exchangeTestUtil.submitPendingBlocks();

      // 3. check exchange protocolFees is still old
      {
        const protocolFees = await exchange.getProtocolFeeValues();

        console.log("syncedAt", protocolFees.syncedAt.toNumber());
        console.log("protocolFeeBips", protocolFees.protocolFeeBips.toNumber());
        console.log("previousProtocolFeeBips", protocolFees.previousProtocolFeeBips.toNumber());
        console.log("executeTimeOfNextProtocolFeeBips", protocolFees.executeTimeOfNextProtocolFeeBips.toNumber());
        console.log("nextProtocolFeeBips", protocolFees.nextProtocolFeeBips.toNumber());

        assert.equal(protocolFees.syncedAt.toNumber(), 0, "syncedAt unexpected");
        assert.equal(
          protocolFees.protocolFeeBips.toNumber(),
          exchangeTestUtil.DEFAULT_PROTOCOL_FEE_BIPS.toNumber(),
          "protocolFeeBips unexpected"
        );
        assert.equal(
          protocolFees.previousProtocolFeeBips.toNumber(),
          exchangeTestUtil.DEFAULT_PROTOCOL_FEE_BIPS.toNumber(),
          "previousProtocolFeeBips unexpected"
        );
        assert.equal(protocolFees.nextProtocolFeeBips.toNumber(), newProtocolfeeBips, "nextProtocolFeeBips unexpected");
      }

      // 4. Wait
      await exchangeTestUtil.advanceBlockTimestamp(exchangeTestUtil.MIN_AGE_PROTOCOL_FEES_UNTIL_UPDATED);

      // 5. submit some tx
      {
        const token = exchangeTestUtil.getTokenAddress("LRC");
        const fee = new BN(0);

        let newKeyPair = exchangeTestUtil.getKeyPairEDDSA();
        await exchangeTestUtil.requestAccountUpdate(ownerA, token, fee, newKeyPair, {
          authMethod: AuthMethod.ECDSA
        });

        await exchangeTestUtil.submitTransactions(2);
        await exchangeTestUtil.submitPendingBlocks();
      }

      // 6. check exchange protocolFees is new
      {
        const protocolFees = await exchange.getProtocolFeeValues();

        console.log("syncedAt", protocolFees.syncedAt.toNumber());
        console.log("protocolFeeBips", protocolFees.protocolFeeBips.toNumber());
        console.log("previousProtocolFeeBips", protocolFees.previousProtocolFeeBips.toNumber());
        console.log("executeTimeOfNextProtocolFeeBips", protocolFees.executeTimeOfNextProtocolFeeBips.toNumber());
        console.log("nextProtocolFeeBips", protocolFees.nextProtocolFeeBips.toNumber());

        assert.equal(protocolFees.protocolFeeBips.toNumber(), newProtocolfeeBips, "protocolFeeBips unexpected");
        assert.equal(
          protocolFees.previousProtocolFeeBips.toNumber(),
          exchangeTestUtil.DEFAULT_PROTOCOL_FEE_BIPS.toNumber(),
          "6previousProtocolFeeBips unexpected"
        );
        assert.equal(protocolFees.nextProtocolFeeBips.toNumber(), newProtocolfeeBips, "nextProtocolFeeBips unexpected");
        assert.equal(
          protocolFees.executeTimeOfNextProtocolFeeBips.toNumber(),
          0,
          "executeTimeOfNextProtocolFeeBips unexpected"
        );
      }
    });
  });
});
