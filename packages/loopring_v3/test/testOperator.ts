import BN = require("bn.js");
import { Constants, roundToFloatValue } from "loopringV3.js";
import { expectThrow } from "./expectThrow";
import { BalanceSnapshot, ExchangeTestUtil } from "./testExchangeUtil";
import { AuthMethod, Deposit, SpotTrade, OrderInfo, BatchSpotTrade, BatchSpotTradeUser } from "./types";

contract("Exchange", (accounts: string[]) => {
  let exchangeTestUtil: ExchangeTestUtil;
  let exchange: any;
  let depositContract: any;
  let loopring: any;
  let exchangeID = 0;

  const createExchange = async (setupTestState: boolean = true) => {
    exchangeID = await exchangeTestUtil.createExchange(
      exchangeTestUtil.testContext.stateOwners[1],
      { setupTestState }
    );
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
  });

  after(async () => {
    await exchangeTestUtil.stop();
  });

  const depositAll = async () => {
    const depositETHOwner1 = await exchangeTestUtil.deposit(
      exchangeTestUtil.testContext.orderOwners[1],
      exchangeTestUtil.testContext.orderOwners[1],
      "ETH",
      new BN(web3.utils.toWei("20", "ether"))
    );

    const depositWETHOwner1 = await exchangeTestUtil.deposit(
      exchangeTestUtil.testContext.orderOwners[1],
      exchangeTestUtil.testContext.orderOwners[1],
      "WETH",
      new BN(web3.utils.toWei("20", "ether"))
    );

    const depositGTOOwner1 = await exchangeTestUtil.deposit(
      exchangeTestUtil.testContext.orderOwners[1],
      exchangeTestUtil.testContext.orderOwners[1],
      "GTO",
      new BN(web3.utils.toWei("200", "ether"))
    );

    const depositETHOwner0 = await exchangeTestUtil.deposit(
      exchangeTestUtil.testContext.orderOwners[0],
      exchangeTestUtil.testContext.orderOwners[0],
      "ETH",
      new BN(web3.utils.toWei("20", "ether"))
    );

    const depositWETHOwner0 = await exchangeTestUtil.deposit(
      exchangeTestUtil.testContext.orderOwners[0],
      exchangeTestUtil.testContext.orderOwners[0],
      "WETH",
      new BN(web3.utils.toWei("20", "ether"))
    );

    const depositGTOOwner0 = await exchangeTestUtil.deposit(
      exchangeTestUtil.testContext.orderOwners[0],
      exchangeTestUtil.testContext.orderOwners[0],
      "GTO",
      new BN(web3.utils.toWei("200", "ether"))
    );
  };

  describe("DepositWithdraw", function() {
    this.timeout(0);

    it("BatchSpotTrade All", async () => {

      await createExchange(true);
      exchangeTestUtil.setupTestState(2)
      const ownerA = exchangeTestUtil.testContext.orderOwners[0];
      const ownerB = exchangeTestUtil.testContext.orderOwners[1];
      const balance = new BN(web3.utils.toWei("7", "ether"));
      const fee = new BN(web3.utils.toWei("0.1", "ether"));
      const token = exchangeTestUtil.getTokenAddress("LRC");
      const recipient = ownerB;

      await depositAll()

      {
        // spot trade
        const orderA: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: true,
          feeTokenID: 0,
          fee: new BN("0"),
          maxFee: new BN("12300000000000"),
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
        };
        const orderB: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 18,
          fillAmountBorS: false,
          feeTokenID: 2,
          fee: new BN("0"),
          maxFee: new BN("21000000000000"),
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
        };

        const orderC: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("2", "ether")),
          amountB: new BN(web3.utils.toWei("2", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: true,
          feeTokenID: 0,
          fee: new BN("0"),
          maxFee: new BN("22300000000000"),
          deltaFilledS: new BN("2000000000000000000"),
          deltaFilledB: new BN("2000000000000000000"),
        };
        const orderD: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("2", "ether")),
          amountB: new BN(web3.utils.toWei("2", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 3,
          fee: new BN("0"),
          maxFee: new BN("31000000000000"),
          deltaFilledS: new BN("2000000000000000000"),
          deltaFilledB: new BN("2000000000000000000"),
        };
        
        
        const orderE: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("2", "ether")),
          amountB: new BN(web3.utils.toWei("2", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: true,
          feeTokenID: 3,
          fee: new BN("0"),
          maxFee: new BN("42300000000000"),
          deltaFilledS: new BN("2000000000000000000"),
          deltaFilledB: new BN("2000000000000000000"),
        };
        const orderF: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("2", "ether")),
          amountB: new BN(web3.utils.toWei("2", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 0,
          feeBips: 18,
          fillAmountBorS: false,
          feeTokenID: 2,
          fee: new BN("0"),
          maxFee: new BN("51000000000000"),
          deltaFilledS: new BN("2000000000000000000"),
          deltaFilledB: new BN("2000000000000000000"),
        };
        const orderG: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: true,
          feeTokenID: 2,
          fee: new BN("0"),
          maxFee: new BN("62300000000000"),
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
        };
        const orderH: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 0,
          feeBips: 18,
          fillAmountBorS: false,
          feeTokenID: 3,
          fee: new BN("0"),
          maxFee: new BN("71000000000000"),
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
        };


        const batchSpotTradeUser1: BatchSpotTradeUser = {
          orders: [orderA, orderC],
        };
        const batchSpotTradeUser2: BatchSpotTradeUser = {
          orders: [orderB, orderD],
        };
        const batchSpotTradeUser3: BatchSpotTradeUser = {
          orders: [orderE],
        };
        const batchSpotTradeUser4: BatchSpotTradeUser = {
          orders: [orderF],
        };
        const batchSpotTradeUser5: BatchSpotTradeUser = {
          orders: [orderG],
        };
        const batchSpotTradeUser6: BatchSpotTradeUser = {
          orders: [orderH],
        };


        const spread = new BN(web3.utils.toWei("1", "ether"));
        const batchSpotTrade: BatchSpotTrade = {
          users: [batchSpotTradeUser1, batchSpotTradeUser2, batchSpotTradeUser3, batchSpotTradeUser4, batchSpotTradeUser5, batchSpotTradeUser6],
          tokens: [2, 3, 0],
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          },
          bindTokenID: 0
        };


        await exchangeTestUtil.setupBatchSpotTrade(batchSpotTrade);
        await exchangeTestUtil.sendBatchSpotTrade(batchSpotTrade);
      }

      exchangeTestUtil.setActiveOperator(2);
      await exchangeTestUtil.submitTransactions(20, true);

      // // Submit the block
      // await exchangeTestUtil.submitPendingBlocks();
    });
    
  });
});
