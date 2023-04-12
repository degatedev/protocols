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

  const depositChecked = async (
    from: string,
    to: string,
    token: string,
    amount: BN
  ) => {
    const snapshot = new BalanceSnapshot(exchangeTestUtil);
    await snapshot.watchBalance(to, token, "recipient");
    await snapshot.transfer(
      from,
      depositContract.address,
      token,
      amount,
      "from",
      "depositContract"
    );

    const ethAddress = exchangeTestUtil.getTokenAddress("ETH");
    const ethValue = token === ethAddress ? amount : 0;
    // Deposit
    await exchange.deposit(from, to, token, amount, "0x", {
      from: from,
      value: ethValue,
      gasPrice: 0
    });

    // Verify balances
    await snapshot.verifyBalances();

    // Get the Deposit event
    const event = await exchangeTestUtil.assertEventEmitted(
      exchange,
      "DepositRequested"
    );
    assert.equal(event.to, to, "owner unexpected");
    assert.equal(
      event.token,
      exchangeTestUtil.getTokenAddress(token),
      "token unexpected"
    );
    assert(event.amount.eq(amount), "amount unexpected");
  };

  const submitWithdrawalBlockChecked = async (
    deposits: Deposit[],
    expectedSuccess?: boolean[],
    expectedTo?: string[],
    blockFee?: BN
  ) => {
    assert.equal(
      exchangeTestUtil.pendingBlocks[exchangeID].length,
      1,
      "unexpected number of pending blocks"
    );
    const block = exchangeTestUtil.pendingBlocks[exchangeID][0];

    // Block fee
    const feeRecipient = exchangeTestUtil.exchangeOperator;
    blockFee = new BN(0);
    let numWithdrawals = 0;
    for (const tx of block.internalBlock.transactions) {
      if (tx.txType === "Withdraw") {
        numWithdrawals++;
        if (tx.type >= 2) {
          blockFee.iadd(tx.withdrawalFee);
        }
      } else if (tx.txType === "Deposit") {
      }
    }

    if (expectedSuccess === undefined) {
      expectedSuccess = new Array(numWithdrawals).fill(true);
    }

    if (expectedTo === undefined) {
      expectedTo = new Array(deposits.length).fill(Constants.zeroAddress);
      for (const [i, deposit] of deposits.entries()) {
        expectedTo[i] =
          deposit.owner === Constants.zeroAddress
            ? await loopring.protocolFeeVault()
            : deposit.owner;
      }
    }

    // Simulate all transfers
    const snapshot = new BalanceSnapshot(exchangeTestUtil);
    // Simulate withdrawals
    for (const [i, deposit] of deposits.entries()) {
      await snapshot.transfer(
        depositContract.address,
        expectedTo[i],
        deposit.token,
        expectedSuccess[i] ? deposit.amount : new BN(0),
        "depositContract",
        "to"
      );
    }
    // Simulate block fee payment
    // await snapshot.transfer(
    //   exchange.address,
    //   feeRecipient,
    //   "ETH",
    //   blockFee,
    //   "exchange",
    //   "feeRecipient"
    // );

    // Submit the block
    await exchangeTestUtil.submitPendingBlocks();

    // Verify balances
    await snapshot.verifyBalances();

    // Check events
    // WithdrawalCompleted events
    {
      const numEventsExpected = expectedSuccess.filter(x => x === true).length;
      const events = await exchangeTestUtil.assertEventsEmitted(
        exchange,
        "WithdrawalCompleted",
        numEventsExpected
      );
      let c = 0;
      for (const [i, deposit] of deposits.entries()) {
        if (expectedSuccess[i]) {
          assert.equal(events[c].from, deposit.owner, "from should match");
          assert.equal(events[c].to, expectedTo[i], "to should match");
          assert.equal(events[c].token, deposit.token, "token should match");
          assert(events[c].amount.eq(deposit.amount), "amount should match");
          c++;
        }
      }
      assert.equal(
        events.length,
        c,
        "Unexpected num WithdrawalCompleted events"
      );
    }
    // WithdrawalFailed events
    {
      const numEventsExpected = expectedSuccess.filter(x => x === false).length;
      const events = await exchangeTestUtil.assertEventsEmitted(
        exchange,
        "WithdrawalFailed",
        numEventsExpected
      );
      let c = 0;
      for (const [i, deposit] of deposits.entries()) {
        if (!expectedSuccess[i]) {
          assert.equal(events[c].from, deposit.owner, "from should match");
          assert.equal(events[c].to, expectedTo[i], "to should match");
          assert.equal(events[c].token, deposit.token, "token should match");
          assert(events[c].amount.eq(deposit.amount), "amount should match");
          c++;
        }
      }
      assert.equal(events.length, c, "Unexpected num WithdrawalFailed events");
    }

    // Check the BlockSubmitted event
    const event = await exchangeTestUtil.assertEventEmitted(
      exchange,
      "BlockSubmitted"
    );
    assert.equal(
      event.blockIdx.toNumber(),
      block.blockIdx,
      "Unexpected block idx"
    );
  };

  const withdrawOnceChecked = async (
    owner: string,
    token: string,
    expectedAmount: BN
  ) => {
    const snapshot = new BalanceSnapshot(exchangeTestUtil);
    await snapshot.transfer(
      depositContract.address,
      owner,
      token,
      expectedAmount,
      "depositContract",
      "owner"
    );

    // Check how much will be withdrawn
    const onchainAmountWithdrawableBefore = await exchange.getAmountWithdrawable(
      owner,
      token
    );
    assert(
      onchainAmountWithdrawableBefore.eq(expectedAmount),
      "unexpected withdrawable amount"
    );

    await exchange.withdrawFromApprovedWithdrawals([owner], [token], {
      from: exchangeTestUtil.testContext.orderOwners[10]
    });

    // Complete amount needs to be withdrawn
    const onchainAmountWithdrawableAfter = await exchange.getAmountWithdrawable(
      owner,
      token
    );
    assert(
      onchainAmountWithdrawableAfter.eq(new BN(0)),
      "unexpected withdrawable amount"
    );

    // Verify balances
    await snapshot.verifyBalances();

    // Get the WithdrawalCompleted event
    const event = await exchangeTestUtil.assertEventEmitted(
      exchange,
      "WithdrawalCompleted"
    );
    assert.equal(event.from, owner, "from unexpected");
    assert.equal(event.to, owner, "to unexpected");
    assert.equal(event.token, token, "token unexpected");
    assert(event.amount.eq(expectedAmount), "amount unexpected");
  };

  const withdrawChecked = async (
    owner: string,
    token: string,
    expectedAmount: BN
  ) => {
    // Withdraw
    await withdrawOnceChecked(owner, token, expectedAmount);
    // Withdraw again, no tokens should be transferred
    await withdrawOnceChecked(owner, token, new BN(0));
  };

  const createExchange = async (setupTestState: boolean = true) => {
    exchangeID = await exchangeTestUtil.createExchange(
      exchangeTestUtil.testContext.stateOwners[0],
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
      await createExchange();

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
          feeBips: 50, //protocolTakerFeeBips
          tradingFee: new BN("21000000000000"),
          fillAmountBorS: true,
          feeTokenID: 0, //
          fee: new BN("12300000000000"), //
          maxFee: new BN("12300000000000"), //
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
          feeBips: 0, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 2, //
          fee: new BN("21000000000000"), //
          maxFee: new BN("21000000000000"), //
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
          feeBips: 50, //protocolTakerFeeBips
          fillAmountBorS: true,
          feeTokenID: 0, //
          fee: new BN("22300000000000"), //
          maxFee: new BN("22300000000000"), //
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
          feeBips: 0, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 2, //
          fee: new BN("31000000000000"), //
          maxFee: new BN("31000000000000"), //
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
          feeBips: 50, //protocolTakerFeeBips
          fillAmountBorS: true,
          feeTokenID: 3, //
          fee: new BN("42000000000000"), //
          maxFee: new BN("42000000000000"), //
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
          feeBips: 0, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 2, //
          fee: new BN("51000000000000"), //
          maxFee: new BN("51000000000000"), //
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
          feeBips: 50, //protocolTakerFeeBips
          fillAmountBorS: true,
          feeTokenID: 3, //
          fee: new BN("62300000000000"), //
          maxFee: new BN("62300000000000"), //
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
          feeBips: 0, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 2, //
          fee: new BN("71000000000000"), //
          maxFee: new BN("71000000000000"), //
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
          bindTokenID: 0,
          enableDataCheck: true,
          // WETH: -1000000000000000000 -2000000000000000000 = -3000000000000000000
          // GTO: 1000000000000000000 -21000000000000 2000000000000000000 = 2999979000000000000
          // ETH: -12300000000000 -22300000000000 = 
          userAFirstEstimateExchange: "-3000000000000000000",
          userASecondEstimateExchange: "2999979000000000000",
          userAThirdEstimateExchange: "-34600000000000",
          
          // WETH: 1000000000000000000 2000000000000000000 -21000000000000  -31000000000000
          // GTO: -1000000000000000000 -2000000000000000000
          // ETH: 0
          userBFirstEstimateExchange: "2999948000000000000",
          userBSecondEstimateExchange: "-3000000000000000000",
          userBThirdEstimateExchange: "0",
          
          // WETH: -2000000000000000000
          // GTO: 2000000000000000000-42000000000000
          // ETH: 
          userCFirstEstimateExchange: "-2000000000000000000",
          userCSecondEstimateExchange: "1999958000000000000",
          userCThirdEstimateExchange: "0",

          // WETH: 2000000000000000000-51000000000000
          // GTO: -2000000000000000000
          // ETH: 
          userDFirstEstimateExchange: "1999949000000000000",
          userDSecondEstimateExchange: "-2000000000000000000",
          userDThirdEstimateExchange: "0",
          
          // WETH: -1000000000000000000
          // GTO: 1000000000000000000-62300000000000
          // ETH: 
          userEFirstEstimateExchange: "-1000000000000000000",
          userESecondEstimateExchange: "999937700000000000",
          userEThirdEstimateExchange: "0",

          // WETH: 1000000000000000000 -71000000000000
          // GTO: -1000000000000000000
          // ETH: 0
          userFFirstEstimateExchange: "999929000000000000",
          userFSecondEstimateExchange: "-1000000000000000000",
          userFThirdEstimateExchange: "0",

          // WETH: 71000000000000+51000000000000+21000000000000+31000000000000=174000000000000
          // GTO: 21000000000000+62300000000000+42000000000000=125300000000000
          // ETH: 12300000000000+22300000000000 = 34600000000000
          operatorFirstEstimateExchange: "174000000000000",
          operatorSecondEstimateExchange: "125300000000000",
          operatorThirdEstimateExchange: "34600000000000"

        };


        await exchangeTestUtil.setupBatchSpotTrade(batchSpotTrade);
        await exchangeTestUtil.sendBatchSpotTrade(batchSpotTrade);
      }


      //
      {
        // spot trade
        const orderA: OrderInfo = {
          tokenS: "ETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 50, //protocolTakerFeeBips
          fillAmountBorS: true,
          feeTokenID: 0, //
          fee: new BN("12000000000000"), //
          maxFee: new BN("12300000000000"), //
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
        };
        const orderB: OrderInfo = {
          tokenS: "GTO",
          tokenB: "ETH",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 0, //
          fee: new BN("21000000000000"), //
          maxFee: new BN("21000000000000"), //
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
        };

        const orderC: OrderInfo = {
          tokenS: "ETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("2", "ether")),
          amountB: new BN(web3.utils.toWei("2", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 50, //protocolTakerFeeBips
          fillAmountBorS: true,
          feeTokenID: 0, //
          fee: new BN("22000000000000"), //
          maxFee: new BN("22300000000000"), //
          deltaFilledS: new BN("2000000000000000000"),
          deltaFilledB: new BN("2000000000000000000"),
        };
        const orderD: OrderInfo = {
          tokenS: "GTO",
          tokenB: "ETH",
          amountS: new BN(web3.utils.toWei("2", "ether")),
          amountB: new BN(web3.utils.toWei("2", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 0, //
          fee: new BN("31000000000000"), //
          maxFee: new BN("31000000000000"), //
          deltaFilledS: new BN("2000000000000000000"),
          deltaFilledB: new BN("2000000000000000000"),
        };
        
        const batchSpotTradeUser1: BatchSpotTradeUser = {
          orders: [orderA, orderC],
        };
        const batchSpotTradeUser2: BatchSpotTradeUser = {
          orders: [orderB, orderD],
        };


        const spread = new BN(web3.utils.toWei("1", "ether"));
        const batchSpotTrade: BatchSpotTrade = {
          users: [batchSpotTradeUser1, batchSpotTradeUser2],
          tokens: [1,3,0],
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          },
          bindTokenID: 0,
          enableDataCheck: true,
          // Nothing:
          // GTO: 1000000000000000000  2000000000000000000
          // ETH: -1000000000000000000 -2000000000000000000 -12000000000000 -22000000000000 = 3000034000000000000
          userAFirstEstimateExchange: "0",
          userASecondEstimateExchange: "3000000000000000000",
          userAThirdEstimateExchange: "-3000034000000000000",
          
          // Nothing:
          // GTO: -1000000000000000000 -2000000000000000000
          // ETH: 1000000000000000000 2000000000000000000 -21000000000000 -31000000000000
          userBFirstEstimateExchange: "0",
          userBSecondEstimateExchange: "-3000000000000000000",
          userBThirdEstimateExchange: "2999948000000000000",
          
          userCFirstEstimateExchange: "0",
          userCSecondEstimateExchange: "0",
          userCThirdEstimateExchange: "0",

          userDFirstEstimateExchange: "0",
          userDSecondEstimateExchange: "0",
          userDThirdEstimateExchange: "0",
          
          userEFirstEstimateExchange: "0",
          userESecondEstimateExchange: "0",
          userEThirdEstimateExchange: "0",

          userFFirstEstimateExchange: "0",
          userFSecondEstimateExchange: "0",
          userFThirdEstimateExchange: "0",
          
          // 12000000000000 + 22000000000000 + 21000000000000 + 31000000000000 = 86000000000000
          operatorFirstEstimateExchange: "0",
          operatorSecondEstimateExchange: "0",
          operatorThirdEstimateExchange: "86000000000000"
        };

        await exchangeTestUtil.setupBatchSpotTrade(batchSpotTrade);
        await exchangeTestUtil.sendBatchSpotTrade(batchSpotTrade);
      }

      //level
      //，level0
      //           level1
      {
        // auto market order
        //StorageID，256-51，51
        // type = 6，amountbasquot，levebasquot
        //，LevelMaleve，basquot。LevelMaL0，bas，quot
        exchangeTestUtil.reserveMultiStorageID(1024);
        // =============================AutoMarket=========================================================
        //：
        //             WETH：1
        //             GTO：0.5
        //，Level
        //amountSamountB。amountB = 0.5 - gridOffset * 10 = 0.495
        const autoMarketOrderA: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.495", "ether")),
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("495000000000000000"),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 50, //protocolTakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 0, //
          fee: new BN("12300000000000"), //
          maxFee: new BN("12300000000000"), //
          // autoMarketID: 0,
          type: 6,
          level: 0,
          isNextOrder: false,
          // maxLevel: 255,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
          storageID: 256
        };
        const autoMarketOrderB: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.495", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          deltaFilledS: new BN("495000000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 2, //
          fee: new BN("21000000000000"), //
          maxFee: new BN("21000000000000"), //
          uiReferID: 1, //
          type: 0,
          storageID: 513
        };

        // AutoMarketOrder Level = 0
        //，BASquot
        //fillAmountBorS = 0amount
        //fillAmountBorS = 1amount
        //：1ET6004001ETHQuotorderOffset
        // amountS: new BN(web3.utils.toWei("1", "ether")),
        // amountB: new BN(web3.utils.toWei("0.495", "ether")),
        // gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
        // orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
        const autoMarketOrderANext: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.4948", "ether")),
          //1
          amountB: new BN(web3.utils.toWei("1", "ether")),
          deltaFilledS: new BN("494810000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 50, //protocolTakerFeeBips
          fillAmountBorS: true,
          feeTokenID: 0, //
          fee: new BN("12300000000000"), //
          maxFee: new BN("12300000000000"), //
          // autoMarketID: 0,
          type: 6,
          level: 0,
          startOrder: autoMarketOrderA,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
          //，isNextOrdefalseStorageIOrdeStorageIStorageI
          //ordeStorageI0AutoMarketStorage beforTokenStorageID 
          isNextOrder: true,
          appointedStorageID: autoMarketOrderA.storageID
        };
        const autoMarketOrderBNext: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.49481", "ether")),
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("494810000000000000"),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 2, //
          fee: new BN("21000000000000"), //
          maxFee: new BN("21000000000000"), //
          uiReferID: 1, //
          storageID: 514
        };


        // AutoMarketOrder Level = 0
        //，BASquot
        //fillAmountBorS = 0amount
        //fillAmountBorS = 1amount
        //：1ET6004001ETHQuotorderOffset
        // amountS: new BN(web3.utils.toWei("1", "ether")),
        // amountB: new BN(web3.utils.toWei("0.495", "ether")),
        // gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
        // orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
        const autoMarketOrderANextNext: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.495", "ether")),
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("495000000000000000"),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 50, //protocolTakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 2, //
          fee: new BN("12300000000000"), //
          maxFee: new BN("12300000000000"), //
          // autoMarketID: 0,
          type: 6,
          level: 0,
          isNextOrder: true,
          // maxLevel: 255,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
          storageID: 256
        };
        const autoMarketOrderBNextNext: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.495", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          deltaFilledS: new BN("495000000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 2, //
          fee: new BN("21000000000000"), //
          maxFee: new BN("21000000000000"), //
          uiReferID: 1, //
          type: 0,
          storageID: 515
        };
        // =============================AutoMarket=========================================================

        // =============================AutoMarket=========================================================
        //，Level
        // Base == WETH, Quote == GTO
        //：
        //             WETH：1
        //             GTO：0.3
        //，Level
        // level0:
        //    amountBamountS。amountS = 0.3 + gridOffset * 10 = 0.305
        const autoMarketOrderABuy: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.305", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          deltaFilledS: new BN("305000000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 50, //protocolTakerFeeBips
          fillAmountBorS: true,
          feeTokenID: 2, //
          fee: new BN("12300000000000"), //
          maxFee: new BN("12300000000000"), //
          type: 7,
          level: 0,
          isNextOrder: false,
          // maxLevel: 255,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
          storageID: 265
        };
        const autoMarketOrderBBuy1: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.305", "ether")),
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("305000000000000000"),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 2, //
          fee: new BN("21000000000000"), //
          maxFee: new BN("21000000000000"), //
          type: 0,
          storageID: 522
        };

        //， level = 1
        // Base == WETH, Quote == GTO
        //：
        //             WETH：1
        //             GTO：0.3
        //，Level
        // level0:
        //    amountBamountS。amountS = 0.3 + gridOffset * 10 = 0.305
        // level1:
        //    amountBamountS。amountS = 0.3 + gridOffset * 9 = 0.3045
    
        const autoMarketOrderABuyLevel1: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.3045", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          deltaFilledS: new BN("304500000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 50, //protocolTakerFeeBips
          fillAmountBorS: true,
          feeTokenID: 2, //
          fee: new BN("12300000000000"), //
          maxFee: new BN("12300000000000"), //
          // uiReferID: 3, //
          // autoMarketID: 0,
          type: 7,
          level: 1,
          isNextOrder: false,
          // maxLevel: 255,
          maxLevel: 10,
          startOrder: autoMarketOrderABuy,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
          storageID: 266
        };
        const autoMarketOrderBBuyLevel1: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.3045", "ether")),
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("304500000000000000"),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 2, //
          fee: new BN("21000000000000"), //
          maxFee: new BN("21000000000000"), //
          // uiReferID: 1, //
          type: 0,
          storageID: 523
        };
        // =============================AutoMarket=========================================================

        const batchSpotTradeUser1: BatchSpotTradeUser = {
          orders: [autoMarketOrderA, autoMarketOrderANext],
        };
        const batchSpotTradeUser2: BatchSpotTradeUser = {
          orders: [autoMarketOrderB, autoMarketOrderBNext],
        };
        const batchSpotTradeUser3: BatchSpotTradeUser = {
          orders: [autoMarketOrderABuy],
        };
        const batchSpotTradeUser4: BatchSpotTradeUser = {
          orders: [autoMarketOrderBBuy1],
        };
        const batchSpotTradeUser5: BatchSpotTradeUser = {
          orders: [autoMarketOrderANextNext],
        };
        const batchSpotTradeUser6: BatchSpotTradeUser = {
          orders: [autoMarketOrderBNextNext],
        };

        const spread = new BN(web3.utils.toWei("1", "ether"));
        const batchSpotTrade: BatchSpotTrade = {
          users: [batchSpotTradeUser1, batchSpotTradeUser2, batchSpotTradeUser3, batchSpotTradeUser4, batchSpotTradeUser5, batchSpotTradeUser6],
          tokens: [2,3,0],
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          },
          bindTokenID: 0,
          enableDataCheck: false,

          // WETH: -1000000000000000000 1000000000000000000
          // GTO: 495000000000000000 -494800000000000000
          // ETH: -12300000000000 -12300000000000
          userAFirstEstimateExchange: "0",
          userASecondEstimateExchange: "200000000000000",
          userAThirdEstimateExchange: "-24600000000000",
          
          // WETH: 1000000000000000000 -1000000000000000000 -21000000000000 -21000000000000
          // GTO: -495000000000000000 +494800000000000000 
          // ETH:
          userBFirstEstimateExchange: "-42000000000000",
          userBSecondEstimateExchange: "-200000000000000",
          userBThirdEstimateExchange: "0",
          
          // WETH: 1000000000000000000 -12300000000000
          // GTO: -305000000000000000
          // ETH:
          userCFirstEstimateExchange: "999987700000000000",
          userCSecondEstimateExchange: "-305000000000000000",
          userCThirdEstimateExchange: "0",

          // WETH: -1000000000000000000 - 21000000000000
          // GTO: 305000000000000000
          // ETH: 
          userDFirstEstimateExchange: "-1000021000000000000",
          userDSecondEstimateExchange: "305000000000000000",
          userDThirdEstimateExchange: "0",
          
          // WETH: -1000000000000000000 -12300000000000
          // GTO: 495000000000000000
          // ETH:
          userEFirstEstimateExchange: "-1000012300000000000",
          userESecondEstimateExchange: "495000000000000000",
          userEThirdEstimateExchange: "0",

          // WETH: 1000000000000000000 -21000000000000
          // GTO: -495000000000000000
          // ETH:
          userFFirstEstimateExchange: "999979000000000000",
          userFSecondEstimateExchange: "-495000000000000000",
          userFThirdEstimateExchange: "0",

          // WETH: 21000000000000 + 21000000000000 + 12300000000000 + 21000000000000 + 12300000000000 + 21000000000000
          // GTO:
          // ETH: 12300000000000 + 12300000000000
          operatorFirstEstimateExchange: "108600000000000",
          operatorSecondEstimateExchange: "0",
          operatorThirdEstimateExchange: "24600000000000"
        };

        await exchangeTestUtil.setupBatchSpotTrade(batchSpotTrade);
        await exchangeTestUtil.sendBatchSpotTrade(batchSpotTrade);
      }

      const depositGTOOwner0 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[0],
        exchangeTestUtil.testContext.orderOwners[0],
        "GTO",
        new BN(web3.utils.toWei("200", "ether"))
      );

      await exchangeTestUtil.submitTransactions(16, true);

      // // Submit the block
      // await exchangeTestUtil.submitPendingBlocks();
    });

    // it("BatchSpotTrade All Simple", async () => {
    //   await createExchange();

    //   const ownerA = exchangeTestUtil.testContext.orderOwners[0];
    //   const ownerB = exchangeTestUtil.testContext.orderOwners[1];
    //   const balance = new BN(web3.utils.toWei("7", "ether"));
    //   const fee = new BN(web3.utils.toWei("0.1", "ether"));
    //   const token = exchangeTestUtil.getTokenAddress("LRC");
    //   const recipient = ownerB;

    //   await depositAll()

    //   {
        
    //     const depositETHOwner1 = await exchangeTestUtil.deposit(
    //       exchangeTestUtil.testContext.orderOwners[0],
    //       exchangeTestUtil.testContext.orderOwners[0],
    //       "WETH",
    //       new BN(web3.utils.toWei("0.650958231", "ether"))
    //     );
    //     const depositETHOwner0 = await exchangeTestUtil.deposit(
    //       exchangeTestUtil.testContext.orderOwners[0],
    //       exchangeTestUtil.testContext.orderOwners[0],
    //       "GTO",
    //       new BN(web3.utils.toWei("0.810343245", "ether"))
    //     );
    //     // spot trade
    //     const orderA: OrderInfo = {
    //       tokenS: "WETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("0.5652151234", "ether")),
    //       amountB: new BN(web3.utils.toWei("0.1765854321", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("12345678000001"), //
    //       maxFee: new BN("12345678000001"), //
    //       deltaFilledS: new BN("565215123400000000"),
    //       deltaFilledB: new BN("176585432100000000"),
    //     };
    //     const orderB: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "WETH",
    //       amountS: new BN(web3.utils.toWei("0.1765854321", "ether")),
    //       amountB: new BN(web3.utils.toWei("0.5652151234", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("21111111200000"), //
    //       maxFee: new BN("21111111200000"), //
    //       deltaFilledS: new BN("176585432100000000"),
    //       deltaFilledB: new BN("565215123400000000"),
    //     };

    //     const orderC: OrderInfo = {
    //       tokenS: "WETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("2", "ether")),
    //       amountB: new BN(web3.utils.toWei("2", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("22300000000000"), //
    //       maxFee: new BN("22300000000000"), //
    //       deltaFilledS: new BN("2000000000000000000"),
    //       deltaFilledB: new BN("2000000000000000000"),
    //     };
    //     const orderD: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "WETH",
    //       amountS: new BN(web3.utils.toWei("2", "ether")),
    //       amountB: new BN(web3.utils.toWei("2", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("31000000000000"), //
    //       maxFee: new BN("31000000000000"), //
    //       deltaFilledS: new BN("2000000000000000000"),
    //       deltaFilledB: new BN("2000000000000000000"),
    //     };

    //     const batchSpotTradeUser1: BatchSpotTradeUser = {
    //       orders: [orderA],
    //     };
    //     const batchSpotTradeUser2: BatchSpotTradeUser = {
    //       orders: [orderB],
    //     };
    //     // const batchSpotTradeUser3: BatchSpotTradeUser = {
    //     //   orders: [orderC],
    //     // };


    //     const spread = new BN(web3.utils.toWei("1", "ether"));
    //     const batchSpotTrade: BatchSpotTrade = {
    //       users: [batchSpotTradeUser1, batchSpotTradeUser2],
    //       tokens: [2, 3, 0],
    //       expected: {
    //         orderA: { filledFraction: 1.0, spread },
    //         orderB: { filledFraction: 0.5 }
    //       },
    //       bindTokenID: 0
    //     };


    //     await exchangeTestUtil.setupBatchSpotTrade(batchSpotTrade);
    //     await exchangeTestUtil.sendBatchSpotTrade(batchSpotTrade);
    //   }

    //   const depositGTOOwner0 = await exchangeTestUtil.deposit(
    //     exchangeTestUtil.testContext.orderOwners[0],
    //     exchangeTestUtil.testContext.orderOwners[0],
    //     "GTO",
    //     new BN(web3.utils.toWei("200", "ether"))
    //   );

    //   await exchangeTestUtil.submitTransactions(13, true);

    //   // // Submit the block
    //   // await exchangeTestUtil.submitPendingBlocks();
    // });
    // it("BatchSpotTrade All Simple", async () => {
    //   await createExchange();

    //   const ownerA = exchangeTestUtil.testContext.orderOwners[0];
    //   const ownerB = exchangeTestUtil.testContext.orderOwners[1];
    //   const balance = new BN(web3.utils.toWei("7", "ether"));
    //   const fee = new BN(web3.utils.toWei("0.1", "ether"));
    //   const token = exchangeTestUtil.getTokenAddress("LRC");
    //   const recipient = ownerB;

    //   await depositAll()

    //   {
    //     // 
    //     // "amountS": "62522510000",
    //     // "amountB": "303066000000000000000000",
    //     // "deltaFilledS": "491238400",
    //     // "deltaFilledB": "2381300000000000000000",
    //     // "tokenS": 3,
    //     // "tokenB": 2,
    //     // "validUntil": 1653099401,
    //     // "fillAmountBorS": true,
    //     // "taker": "0",
    //     // "feeBips": 0,
    //     // "feeTokenID": 3,
    //     // "fee": "516000",
    //     // "maxFee": "1057700",
    //     // spot trade
    //     const orderA: OrderInfo = {
    //       tokenS: "WETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("0.5652151234", "ether")),
    //       amountB: new BN(web3.utils.toWei("0.1765854321", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("12345678000001"), //
    //       maxFee: new BN("12345678000001"), //
    //       deltaFilledS: new BN("565215123400000000"),
    //       deltaFilledB: new BN("176585432100000000"),
    //     };
    //     const orderB: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "WETH",
    //       amountS: new BN(web3.utils.toWei("0.1765854321", "ether")),
    //       amountB: new BN(web3.utils.toWei("0.5652151234", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("21111111200000"), //
    //       maxFee: new BN("21111111200000"), //
    //       deltaFilledS: new BN("176585432100000000"),
    //       deltaFilledB: new BN("565215123400000000"),
    //     };

    //     const orderC: OrderInfo = {
    //       tokenS: "WETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("2", "ether")),
    //       amountB: new BN(web3.utils.toWei("2", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("22300000000000"), //
    //       maxFee: new BN("22300000000000"), //
    //       deltaFilledS: new BN("2000000000000000000"),
    //       deltaFilledB: new BN("2000000000000000000"),
    //     };
    //     const orderD: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "WETH",
    //       amountS: new BN(web3.utils.toWei("2", "ether")),
    //       amountB: new BN(web3.utils.toWei("2", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("31000000000000"), //
    //       maxFee: new BN("31000000000000"), //
    //       deltaFilledS: new BN("2000000000000000000"),
    //       deltaFilledB: new BN("2000000000000000000"),
    //     };

    //     const batchSpotTradeUser1: BatchSpotTradeUser = {
    //       orders: [orderA],
    //     };
    //     const batchSpotTradeUser2: BatchSpotTradeUser = {
    //       orders: [orderB],
    //     };
    //     // const batchSpotTradeUser3: BatchSpotTradeUser = {
    //     //   orders: [orderC],
    //     // };


    //     const spread = new BN(web3.utils.toWei("1", "ether"));
    //     const batchSpotTrade: BatchSpotTrade = {
    //       users: [batchSpotTradeUser1, batchSpotTradeUser2],
    //       tokens: [2, 3, 0],
    //       expected: {
    //         orderA: { filledFraction: 1.0, spread },
    //         orderB: { filledFraction: 0.5 }
    //       },
    //       bindTokenID: 0
    //     };


    //     await exchangeTestUtil.setupBatchSpotTrade(batchSpotTrade);
    //     await exchangeTestUtil.sendBatchSpotTrade(batchSpotTrade);
    //   }

    //   const depositGTOOwner0 = await exchangeTestUtil.deposit(
    //     exchangeTestUtil.testContext.orderOwners[0],
    //     exchangeTestUtil.testContext.orderOwners[0],
    //     "GTO",
    //     new BN(web3.utils.toWei("200", "ether"))
    //   );

    //   await exchangeTestUtil.submitTransactions(13, true);

    //   // // Submit the block
    //   // await exchangeTestUtil.submitPendingBlocks();
    // });
    // it("BatchSpotTrade Error Amount Match", async () => {
    //   await createExchange();

    //   const ownerA = exchangeTestUtil.testContext.orderOwners[0];
    //   const ownerB = exchangeTestUtil.testContext.orderOwners[1];

    //   await depositAll();

    //   {
    //     // spot trade
    //     const orderA: OrderInfo = {
    //       tokenS: "ETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("1", "ether")),
    //       amountB: new BN(web3.utils.toWei("1", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("12300000000000"), //
    //       maxFee: new BN("12300000000000"), //
    //       deltaFilledS: new BN("1000000000000000000"),
    //       deltaFilledB: new BN("1000000000000000000"),
    //     };
    //     const orderB: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "ETH",
    //       amountS: new BN(web3.utils.toWei("1", "ether")),
    //       amountB: new BN(web3.utils.toWei("1", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("21000000000000"), //
    //       maxFee: new BN("21000000000000"), //
    //       deltaFilledS: new BN("1000000000000000000"),
    //       deltaFilledB: new BN("1000000000000000000"),
    //     };

    //     const orderC: OrderInfo = {
    //       tokenS: "ETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("2", "ether")),
    //       amountB: new BN(web3.utils.toWei("2", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("22300000000000"), //
    //       maxFee: new BN("22300000000000"), //
    //       deltaFilledS: new BN("2000000000000000000"),
    //       deltaFilledB: new BN("2000000000000000000"),
    //     };
    //     // const orderD: OrderInfo = {
    //     //   tokenS: "GTO",
    //     //   tokenB: "ETH",
    //     //   amountS: new BN(web3.utils.toWei("2", "ether")),
    //     //   amountB: new BN(web3.utils.toWei("2", "ether")),
    //     //   owner: exchangeTestUtil.testContext.orderOwners[1],
    //     //   // maxFeeBips: 0,
    //     //   feeBips: 0, //protocolMakerFeeBips
    //     //   fillAmountBorS: false,
    //     //   feeTokenID: 0, //
    //     //   fee: new BN("31000000000000"), //
    //     //   maxFee: new BN("31000000000000"), //
    //     //   deltaFilledS: new BN("2000000000000000000"),
    //     //   deltaFilledB: new BN("2000000000000000000"),
    //     // };
        
    //     const batchSpotTradeUser1: BatchSpotTradeUser = {
    //       orders: [orderA, orderC],
    //     };
    //     const batchSpotTradeUser2: BatchSpotTradeUser = {
    //       // orders: [orderB, orderD],
    //       orders: [orderB],
    //     };

    //     const spread = new BN(web3.utils.toWei("1", "ether"));
    //     const batchSpotTrade: BatchSpotTrade = {
    //       users: [batchSpotTradeUser1, batchSpotTradeUser2],
    //       tokens: [1,3,0],
    //       expected: {
    //         orderA: { filledFraction: 1.0, spread },
    //         orderB: { filledFraction: 0.5 }
    //       },
    //       bindTokenID: 0
    //     };

    //     await exchangeTestUtil.setupBatchSpotTrade(batchSpotTrade);
    //     await exchangeTestUtil.sendBatchSpotTrade(batchSpotTrade);
    //   }

    //   await expectThrow(exchangeTestUtil.submitTransactions(12, true), "invalid block not matched amount");
    // });
    
    // //
    // it("BatchSpotTrade Error Order Cancelled", async () => {
    //   await createExchange();

    //   const ownerA = exchangeTestUtil.testContext.orderOwners[0];
    //   const ownerB = exchangeTestUtil.testContext.orderOwners[1];

    //   await depositAll();

    //   {
    //     // spot trade
    //     const orderA: OrderInfo = {
    //       tokenS: "ETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("1", "ether")),
    //       amountB: new BN(web3.utils.toWei("1", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("12300000000000"), //
    //       maxFee: new BN("12300000000000"), //
    //       deltaFilledS: new BN("1000000000000000000"),
    //       deltaFilledB: new BN("1000000000000000000"),
    //     };
    //     const orderB: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "ETH",
    //       amountS: new BN(web3.utils.toWei("1", "ether")),
    //       amountB: new BN(web3.utils.toWei("1", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("21000000000000"), //
    //       maxFee: new BN("21000000000000"), //
    //       deltaFilledS: new BN("1000000000000000000"),
    //       deltaFilledB: new BN("1000000000000000000"),
    //     };

    //     const orderC: OrderInfo = {
    //       tokenS: "ETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("2", "ether")),
    //       amountB: new BN(web3.utils.toWei("2", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("22300000000000"), //
    //       maxFee: new BN("22300000000000"), //
    //       deltaFilledS: new BN("2000000000000000000"),
    //       deltaFilledB: new BN("2000000000000000000"),
    //     };
    //     const orderD: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "ETH",
    //       amountS: new BN(web3.utils.toWei("2", "ether")),
    //       amountB: new BN(web3.utils.toWei("2", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("31000000000000"), //
    //       maxFee: new BN("31000000000000"), //
    //       deltaFilledS: new BN("2000000000000000000"),
    //       deltaFilledB: new BN("2000000000000000000"),
    //     };
        
    //     const batchSpotTradeUser1: BatchSpotTradeUser = {
    //       orders: [orderA, orderC],
    //     };
    //     const batchSpotTradeUser2: BatchSpotTradeUser = {
    //       orders: [orderB, orderD],
    //     };

    //     const spread = new BN(web3.utils.toWei("1", "ether"));
    //     const batchSpotTrade: BatchSpotTrade = {
    //       users: [batchSpotTradeUser1, batchSpotTradeUser2],
    //       tokens: [1,3,0],
    //       expected: {
    //         orderA: { filledFraction: 1.0, spread },
    //         orderB: { filledFraction: 0.5 }
    //       },
    //       bindTokenID: 0
    //     };

    //     await exchangeTestUtil.setupBatchSpotTrade(batchSpotTrade);
    //     // cancel order
    //     await exchangeTestUtil.requestOrderCancel(
    //       exchangeTestUtil.testContext.orderOwners[0],
    //       orderA.storageID,
    //       new BN("21000000000000"), //
    //       new BN("21000000000000"), //
    //       "ETH");

    //     await exchangeTestUtil.sendBatchSpotTrade(batchSpotTrade);
    //   }

    //   // await exchangeTestUtil.submitTransactions(12, true)
    //   await expectThrow(exchangeTestUtil.submitTransactions(12, true), "invalid block order cancelled");
    // });

    // //，OrdeTokenID
    // it("BatchSpotTrade Error Order Cancelled", async () => {
    //   await createExchange();

    //   const ownerA = exchangeTestUtil.testContext.orderOwners[0];
    //   const ownerB = exchangeTestUtil.testContext.orderOwners[1];

    //   await depositAll();

    //   {
    //     // spot trade
    //     const orderA: OrderInfo = {
    //       tokenS: "ETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("1", "ether")),
    //       amountB: new BN(web3.utils.toWei("1", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("12300000000000"), //
    //       maxFee: new BN("12300000000000"), //
    //       deltaFilledS: new BN("1000000000000000000"),
    //       deltaFilledB: new BN("1000000000000000000"),
    //     };
    //     const orderB: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "ETH",
    //       amountS: new BN(web3.utils.toWei("1", "ether")),
    //       amountB: new BN(web3.utils.toWei("1", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("21000000000000"), //
    //       maxFee: new BN("21000000000000"), //
    //       deltaFilledS: new BN("1000000000000000000"),
    //       deltaFilledB: new BN("1000000000000000000"),
    //     };

    //     const orderC: OrderInfo = {
    //       tokenS: "ETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("2", "ether")),
    //       amountB: new BN(web3.utils.toWei("2", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("22300000000000"), //
    //       maxFee: new BN("22300000000000"), //
    //       deltaFilledS: new BN("2000000000000000000"),
    //       deltaFilledB: new BN("2000000000000000000"),
    //     };
    //     const orderD: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "ETH",
    //       amountS: new BN(web3.utils.toWei("2", "ether")),
    //       amountB: new BN(web3.utils.toWei("2", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("31000000000000"), //
    //       maxFee: new BN("31000000000000"), //
    //       deltaFilledS: new BN("2000000000000000000"),
    //       deltaFilledB: new BN("2000000000000000000"),
    //     };
        
    //     const batchSpotTradeUser1: BatchSpotTradeUser = {
    //       orders: [orderA, orderC],
    //     };
    //     const batchSpotTradeUser2: BatchSpotTradeUser = {
    //       orders: [orderB, orderD],
    //     };

    //     const spread = new BN(web3.utils.toWei("1", "ether"));
    //     const batchSpotTrade: BatchSpotTrade = {
    //       users: [batchSpotTradeUser1, batchSpotTradeUser2],
    //       tokens: [1,3,0],
    //       expected: {
    //         orderA: { filledFraction: 1.0, spread },
    //         orderB: { filledFraction: 0.5 }
    //       },
    //       bindTokenID: 0
    //     };

    //     await exchangeTestUtil.setupBatchSpotTrade(batchSpotTrade);

    //     await exchangeTestUtil.sendBatchSpotTrade(batchSpotTrade);
    //   }

    //   // await exchangeTestUtil.submitTransactions(12, true)
    //   await expectThrow(exchangeTestUtil.submitTransactions(12, true), "invalid block order cancelled");
    // });

      //tokentokenordetoken

  });
});
