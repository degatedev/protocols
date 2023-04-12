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

    it("StorageID Gas Add Up", async () => {
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
          fillAmountBorS: true,
          feeTokenID: 0, //
          fee: new BN("12300000000000"), //
          maxFee: new BN("24600000000000"), //
          deltaFilledS: new BN("500000000000000000"),
          deltaFilledB: new BN("500000000000000000"),
          storageID: 0
        };
        const orderB: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.5", "ether")),
          amountB: new BN(web3.utils.toWei("0.5", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 2, //
          fee: new BN("21000000000000"), //
          maxFee: new BN("21000000000000"), //
          deltaFilledS: new BN("500000000000000000"),
          deltaFilledB: new BN("500000000000000000"),
          storageID: 0
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
          feeTokenID: 3, //
          fee: new BN("22300000000000"), //
          maxFee: new BN("22300000000000"), //
          deltaFilledS: new BN("2000000000000000000"),
          deltaFilledB: new BN("2000000000000000000"),
          storageID: 1
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
          feeTokenID: 3, //
          fee: new BN("31000000000000"), //
          maxFee: new BN("31000000000000"), //
          deltaFilledS: new BN("2000000000000000000"),
          deltaFilledB: new BN("2000000000000000000"),
          storageID: 1
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
          fee: new BN("42300000000000"), //
          maxFee: new BN("42300000000000"), //
          deltaFilledS: new BN("2000000000000000000"),
          deltaFilledB: new BN("2000000000000000000"),
          storageID: 2
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
          storageID: 2
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
          feeTokenID: 2, //
          fee: new BN("62300000000000"), //
          maxFee: new BN("62300000000000"), //
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
          storageID: 3
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
          feeTokenID: 3, //
          fee: new BN("71000000000000"), //
          maxFee: new BN("71000000000000"), //
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
          storageID: 3
        };


        const orderJ: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 50, //protocolTakerFeeBips
          fillAmountBorS: true,
          feeTokenID: 3, //
          fee: new BN("12300000000000"), //
          maxFee: new BN("24600000000000"), //
          deltaFilledS: new BN("500000000000000000"),
          deltaFilledB: new BN("500000000000000000"),
          storageID: 0
        };
        const orderK: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.5", "ether")),
          amountB: new BN(web3.utils.toWei("0.5", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 40, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 3, //
          fee: new BN("21000000000000"), //
          maxFee: new BN("21000000000000"), //
          deltaFilledS: new BN("500000000000000000"),
          deltaFilledB: new BN("500000000000000000"),
          storageID: 3
        };


        const batchSpotTradeUser1: BatchSpotTradeUser = {
          orders: [orderA, orderC, orderJ],
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
          orders: [orderK],
        };
        


        const spread = new BN(web3.utils.toWei("1", "ether"));
        const batchSpotTrade: BatchSpotTrade = {
          users: [batchSpotTradeUser1, batchSpotTradeUser2, batchSpotTradeUser3, batchSpotTradeUser4, batchSpotTradeUser5],
          tokens: [3,2,0],
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          },
          bindTokenID: 0
        };


        await exchangeTestUtil.setupBatchSpotTrade(batchSpotTrade);
        await exchangeTestUtil.sendBatchSpotTrade(batchSpotTrade);
      }

      await exchangeTestUtil.submitTransactions(20, true);

      // // Submit the block
      // await exchangeTestUtil.submitPendingBlocks();
    });

    // it("StorageID Gas Add Up Error", async () => {
    //   await createExchange();

    //   const ownerA = exchangeTestUtil.testContext.orderOwners[0];
    //   const ownerB = exchangeTestUtil.testContext.orderOwners[1];
    //   const balance = new BN(web3.utils.toWei("7", "ether"));
    //   const fee = new BN(web3.utils.toWei("0.1", "ether"));
    //   const token = exchangeTestUtil.getTokenAddress("LRC");
    //   const recipient = ownerB;

    //   await depositAll()

    //   {
    //     // spot trade
    //     const orderA: OrderInfo = {
    //       tokenS: "WETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("1", "ether")),
    //       amountB: new BN(web3.utils.toWei("1", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("12300000000000"), //
    //       maxFee: new BN("24600000000000"), //
    //       deltaFilledS: new BN("500000000000000000"),
    //       deltaFilledB: new BN("500000000000000000"),
    //       storageID: 0
    //     };
    //     const orderB: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "WETH",
    //       amountS: new BN(web3.utils.toWei("0.5", "ether")),
    //       amountB: new BN(web3.utils.toWei("0.5", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("21000000000000"), //
    //       maxFee: new BN("21000000000000"), //
    //       deltaFilledS: new BN("500000000000000000"),
    //       deltaFilledB: new BN("500000000000000000"),
    //       storageID: 0
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
    //       storageID: 1
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
    //       storageID: 1
    //     };
        
        
    //     const orderE: OrderInfo = {
    //       tokenS: "WETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("2", "ether")),
    //       amountB: new BN(web3.utils.toWei("2", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("42300000000000"), //
    //       maxFee: new BN("42300000000000"), //
    //       deltaFilledS: new BN("2000000000000000000"),
    //       deltaFilledB: new BN("2000000000000000000"),
    //       storageID: 2
    //     };
    //     const orderF: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "WETH",
    //       amountS: new BN(web3.utils.toWei("2", "ether")),
    //       amountB: new BN(web3.utils.toWei("2", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("51000000000000"), //
    //       maxFee: new BN("51000000000000"), //
    //       deltaFilledS: new BN("2000000000000000000"),
    //       deltaFilledB: new BN("2000000000000000000"),
    //       storageID: 2
    //     };
    //     const orderG: OrderInfo = {
    //       tokenS: "WETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("1", "ether")),
    //       amountB: new BN(web3.utils.toWei("1", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("62300000000000"), //
    //       maxFee: new BN("62300000000000"), //
    //       deltaFilledS: new BN("1000000000000000000"),
    //       deltaFilledB: new BN("1000000000000000000"),
    //       storageID: 3
    //     };
    //     const orderH: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "WETH",
    //       amountS: new BN(web3.utils.toWei("1", "ether")),
    //       amountB: new BN(web3.utils.toWei("1", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("71000000000000"), //
    //       maxFee: new BN("71000000000000"), //
    //       deltaFilledS: new BN("1000000000000000000"),
    //       deltaFilledB: new BN("1000000000000000000"),
    //       storageID: 3
    //     };


    //     const orderJ: OrderInfo = {
    //       tokenS: "WETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("1", "ether")),
    //       amountB: new BN(web3.utils.toWei("1", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("22300000000000"), //
    //       maxFee: new BN("24600000000000"), //
    //       deltaFilledS: new BN("500000000000000000"),
    //       deltaFilledB: new BN("500000000000000000"),
    //       storageID: 0
    //     };
    //     const orderK: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "WETH",
    //       amountS: new BN(web3.utils.toWei("0.5", "ether")),
    //       amountB: new BN(web3.utils.toWei("0.5", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("21000000000000"), //
    //       maxFee: new BN("21000000000000"), //
    //       deltaFilledS: new BN("500000000000000000"),
    //       deltaFilledB: new BN("500000000000000000"),
    //       storageID: 3
    //     };


    //     const batchSpotTradeUser1: BatchSpotTradeUser = {
    //       orders: [orderA, orderC, orderJ],
    //     };
    //     const batchSpotTradeUser2: BatchSpotTradeUser = {
    //       orders: [orderB, orderD],
    //     };
    //     const batchSpotTradeUser3: BatchSpotTradeUser = {
    //       orders: [orderE],
    //     };
    //     const batchSpotTradeUser4: BatchSpotTradeUser = {
    //       orders: [orderF],
    //     };
    //     const batchSpotTradeUser5: BatchSpotTradeUser = {
    //       orders: [orderK],
    //     };


    //     const spread = new BN(web3.utils.toWei("1", "ether"));
    //     const batchSpotTrade: BatchSpotTrade = {
    //       users: [batchSpotTradeUser1, batchSpotTradeUser2, batchSpotTradeUser3, batchSpotTradeUser4, batchSpotTradeUser5],
    //       tokens: [2,3,0],
    //       expected: {
    //         orderA: { filledFraction: 1.0, spread },
    //         orderB: { filledFraction: 0.5 }
    //       },
    //       bindTokenID: 0
    //     };


    //     await exchangeTestUtil.setupBatchSpotTrade(batchSpotTrade);
    //     await exchangeTestUtil.sendBatchSpotTrade(batchSpotTrade);
    //   }

    //   await expectThrow(exchangeTestUtil.submitTransactions(20, true), "StorageID Gas Add Up Error");

    //   // // Submit the block
    //   // await exchangeTestUtil.submitPendingBlocks();
    // });
    

    it("StorageID BatchSpotTrade Turn Over", async () => {
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
          fillAmountBorS: true,
          feeTokenID: 0, //
          fee: new BN("12300000000000"), //
          maxFee: new BN("24600000000000"), //
          deltaFilledS: new BN("500000000000000000"),
          deltaFilledB: new BN("500000000000000000"),
          storageID: 0
        };
        const orderB: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.5", "ether")),
          amountB: new BN(web3.utils.toWei("0.5", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0, //protocolMakerFeeBips
          fillAmountBorS: false,
          feeTokenID: 0, //
          fee: new BN("21000000000000"), //
          maxFee: new BN("21000000000000"), //
          deltaFilledS: new BN("500000000000000000"),
          deltaFilledB: new BN("500000000000000000"),
          storageID: 0
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
          storageID: 1
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
          feeTokenID: 0, //
          fee: new BN("31000000000000"), //
          maxFee: new BN("31000000000000"), //
          deltaFilledS: new BN("2000000000000000000"),
          deltaFilledB: new BN("2000000000000000000"),
          storageID: 1
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
          feeTokenID: 0, //
          fee: new BN("42300000000000"), //
          maxFee: new BN("42300000000000"), //
          deltaFilledS: new BN("2000000000000000000"),
          deltaFilledB: new BN("2000000000000000000"),
          storageID: 2
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
          feeTokenID: 0, //
          fee: new BN("51000000000000"), //
          maxFee: new BN("51000000000000"), //
          deltaFilledS: new BN("2000000000000000000"),
          deltaFilledB: new BN("2000000000000000000"),
          storageID: 2
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
          feeTokenID: 0, //
          fee: new BN("62300000000000"), //
          maxFee: new BN("62300000000000"), //
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
          storageID: 3
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
          feeTokenID: 0, //
          fee: new BN("71000000000000"), //
          maxFee: new BN("71000000000000"), //
          deltaFilledS: new BN("1000000000000000000"),
          deltaFilledB: new BN("1000000000000000000"),
          storageID: 16384
        };


        const batchSpotTradeUser1: BatchSpotTradeUser = {
          orders: [orderA, orderC, orderH],
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


        const spread = new BN(web3.utils.toWei("1", "ether"));
        const batchSpotTrade: BatchSpotTrade = {
          users: [batchSpotTradeUser1, batchSpotTradeUser2, batchSpotTradeUser3, batchSpotTradeUser4, batchSpotTradeUser5],
          tokens: [2,3,0],
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          },
          bindTokenID: 0
        };


        await exchangeTestUtil.setupBatchSpotTrade(batchSpotTrade);
        await exchangeTestUtil.sendBatchSpotTrade(batchSpotTrade);
      }

      await exchangeTestUtil.submitTransactions(20, true);

      // // Submit the block
      // await exchangeTestUtil.submitPendingBlocks();
    });
    //BatchSpotTrade
    it("StorageID SpotTrade Turn Over - Order Cancelled", async () => {
      await createExchange();

      const ownerA = exchangeTestUtil.testContext.orderOwners[0];
      const ownerB = exchangeTestUtil.testContext.orderOwners[1];
      const balance = new BN(web3.utils.toWei("7", "ether"));
      const fee = new BN(web3.utils.toWei("0.1", "ether"));
      const token = exchangeTestUtil.getTokenAddress("LRC");
      const recipient = ownerB;

      await depositAll()

      {
        const order: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.5", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 50, //protocolTakerFeeBips
          fillAmountBorS: true,
          feeTokenID: 0, //
          fee: new BN("12300000000000"), //
          maxFee: new BN("12300000000000"),
          storageID: 0
        };

        const spread = new BN(web3.utils.toWei("1", "ether"));
        const ringA: SpotTrade = {
          orderA: order,
          orderB: {
            tokenS: "GTO",
            tokenB: "WETH",
            amountS: new BN(web3.utils.toWei("4", "ether")),
            amountB: new BN(web3.utils.toWei("2", "ether")),
            owner: exchangeTestUtil.testContext.orderOwners[1],
            // maxFeeBips: 0,
            feeBips: 0, //protocolMakerFeeBips
            fillAmountBorS: false,
            feeTokenID: 0, //
            fee: new BN("21000000000000"), //
            maxFee: new BN("21000000000000"),
            storageID: 0
          },
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };

        await exchangeTestUtil.setupRing(ringA, true, true, false, false);
        await exchangeTestUtil.sendRing(ringA);
      }

      // cancel storageID 0
      await exchangeTestUtil.requestOrderCancel(
        exchangeTestUtil.testContext.orderOwners[0],
        0,
        new BN("21000000000000"), //
        new BN("21000000000000"), //
        "GTO"
      );

      //16384 storageID
      {
        const order: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.5", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 50, //protocolTakerFeeBips
          fillAmountBorS: true,
          feeTokenID: 0, //
          fee: new BN("12300000000000"), //
          maxFee: new BN("12300000000000"),
          storageID: 16384
        };

        const spread = new BN(web3.utils.toWei("1", "ether"));
        const ringA: SpotTrade = {
          orderA: order,
          orderB: {
            tokenS: "GTO",
            tokenB: "WETH",
            amountS: new BN(web3.utils.toWei("4", "ether")),
            amountB: new BN(web3.utils.toWei("2", "ether")),
            owner: exchangeTestUtil.testContext.orderOwners[1],
            // maxFeeBips: 0,
            feeBips: 0, //protocolMakerFeeBips
            fillAmountBorS: false,
            feeTokenID: 0, //
            fee: new BN("21000000000000"), //
            maxFee: new BN("21000000000000"),
            storageID: 1
          },
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };

        await exchangeTestUtil.setupRing(ringA, true, true, false, false);
        await exchangeTestUtil.sendRing(ringA);
      }

      await exchangeTestUtil.submitTransactions(20, true);

      // // Submit the block
      // await exchangeTestUtil.submitPendingBlocks();
    });


    // it("StorageID SpotTrade Turn Over - Order Cancelled - BatchSpotTrade", async () => {
    //   await createExchange();

    //   const ownerA = exchangeTestUtil.testContext.orderOwners[0];
    //   const ownerB = exchangeTestUtil.testContext.orderOwners[1];
    //   const balance = new BN(web3.utils.toWei("7", "ether"));
    //   const fee = new BN(web3.utils.toWei("0.1", "ether"));
    //   const token = exchangeTestUtil.getTokenAddress("LRC");
    //   const recipient = ownerB;

    //   await depositAll()

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
    //       storageID: 0
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
    //       storageID: 0
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
    //       storageID: 1
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
    //       storageID: 1
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

    //   // cancel storageID 0
    //   await exchangeTestUtil.requestOrderCancel(
    //     exchangeTestUtil.testContext.orderOwners[0],
    //     0,
    //     new BN("21000000000000"), //
    //     new BN("21000000000000"), //
    //     "GTO"
    //   );

    //   //16384 storageID
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
    //       storageID: 16384
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
    //       storageID: 2
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
    //       storageID: 2
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
    //       storageID: 3
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

    //   await exchangeTestUtil.submitTransactions(20, true);

    //   // // Submit the block
    //   // await exchangeTestUtil.submitPendingBlocks();
    // });

    // //StorageID 0forwar0SpotTrad16384 Iforwar1
    // it("StorageID SpotTrade Turn Over - Forward 0", async () => {
    //   await createExchange();

    //   const ownerA = exchangeTestUtil.testContext.orderOwners[0];
    //   const ownerB = exchangeTestUtil.testContext.orderOwners[1];
    //   const balance = new BN(web3.utils.toWei("7", "ether"));
    //   const fee = new BN(web3.utils.toWei("0.1", "ether"));
    //   const token = exchangeTestUtil.getTokenAddress("LRC");
    //   const recipient = ownerB;

    //   await depositAll()

      
    //   //：
    //   //             WETH：1
    //   //             GTO：0.5
    //   //，Level
    //   //amountSamountB。amountB = 0.5 - gridOffset * 10 = 0.495
    //   const autoMarketOrderA: OrderInfo = {
    //     tokenS: "WETH",
    //     tokenB: "GTO",
    //     amountS: new BN(web3.utils.toWei("1", "ether")),
    //     amountB: new BN(web3.utils.toWei("0.495", "ether")),
    //     owner: exchangeTestUtil.testContext.orderOwners[0],
    //     // maxFeeBips: 50,
    //     feeBips: 50, //protocolTakerFeeBips
    //     fillAmountBorS: false,
    //     feeTokenID: 0, //
    //     fee: new BN("12300000000000"), //
    //     maxFee: new BN("12300000000000"), //
    //     type: 6,
    //     level: 0,
    //     isNextOrder: false,
    //     maxLevel: 10,
    //     gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
    //     orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
    //     storageID: 0
    //   };
    //   const autoMarketOrderB: OrderInfo = {
    //     tokenS: "GTO",
    //     tokenB: "WETH",
    //     amountS: new BN(web3.utils.toWei("4", "ether")),
    //     amountB: new BN(web3.utils.toWei("2", "ether")),
    //     owner: exchangeTestUtil.testContext.orderOwners[1],
    //     feeBips: 0, //protocolMakerFeeBips
    //     fillAmountBorS: false,
    //     feeTokenID: 0, //
    //     fee: new BN("21000000000000"), //
    //     maxFee: new BN("21000000000000"), //
    //     type: 0,
    //     storageID: 0
    //   };

    //   const spread = new BN(web3.utils.toWei("1", "ether"));
    //   const ringAutoMarket: SpotTrade = {
    //     orderA: autoMarketOrderA,
    //     orderB: autoMarketOrderB,
    //     expected: {
    //       orderA: { filledFraction: 1.0, spread },
    //       orderB: { filledFraction: 0.5 }
    //     }
    //   };
    //   await exchangeTestUtil.setupRing(
    //     ringAutoMarket,
    //     true,
    //     true,
    //     false,
    //     false,
    //     0,
    //     0
    //   );
    //   await exchangeTestUtil.sendRing(ringAutoMarket);
      
    //   // AutoMarketOrder Level = 0
    //   //，BASquot
    //   //fillAmountBorS = 0amount
    //   //fillAmountBorS = 1amount
    //   //：1ET6004001ETHQuotorderOffset
    //   // amountS: new BN(web3.utils.toWei("1", "ether")),
    //   // amountB: new BN(web3.utils.toWei("0.495", "ether")),
    //   // gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
    //   // orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
    //   {
    //     const autoMarketOrderANext: OrderInfo = {
    //       tokenS: "GTO",
    //       tokenB: "WETH",
    //       amountS: new BN(web3.utils.toWei("0.4948", "ether")),
    //       amountB: new BN(web3.utils.toWei("1", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("12300000000000"), //
    //       maxFee: new BN("12300000000000"), //
    //       type: 6,
    //       level: 0,
    //       startOrder: autoMarketOrderA,
    //       maxLevel: 10,
    //       gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
    //       orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
    //       //，isNextOrdefalseStorageIOrdeStorageIStorageI
    //       //ordeStorageI0AutoMarketStorage beforTokenStorageID 
    //       isNextOrder: true,
    //       appointedStorageID: autoMarketOrderA.storageID
    //     };
    //     const autoMarketOrderB: OrderInfo = {
    //       tokenS: "WETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("0.5", "ether")),
    //       amountB: new BN(web3.utils.toWei("0.2474", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[1],
    //       // maxFeeBips: 0,
    //       feeBips: 0, //protocolMakerFeeBips
    //       fillAmountBorS: false,
    //       feeTokenID: 0, //
    //       fee: new BN("21000000000000"), //
    //       maxFee: new BN("21000000000000")
    //     };

    //     const spread = new BN(web3.utils.toWei("1", "ether"));
    //     const ringAutoMarket: SpotTrade = {
    //       orderA: autoMarketOrderANext,
    //       orderB: autoMarketOrderB,
    //       expected: {
    //         orderA: { filledFraction: 1.0, spread },
    //         orderB: { filledFraction: 0.5 }
    //       }
    //     };
    //     await exchangeTestUtil.setupRing(
    //       ringAutoMarket,
    //       true,
    //       true,
    //       false,
    //       false,
    //       0,
    //       1
    //     );
    //     await exchangeTestUtil.sendRing(ringAutoMarket);
    //   }
    //   // cancel storageID 0
    //   await exchangeTestUtil.requestOrderCancel(
    //     exchangeTestUtil.testContext.orderOwners[0],
    //     0,
    //     new BN("21000000000000"), //
    //     new BN("21000000000000"), //
    //     "GTO"
    //   );

    //   //16384 storageID
    //   {
    //     const order: OrderInfo = {
    //       tokenS: "WETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("1", "ether")),
    //       amountB: new BN(web3.utils.toWei("0.5", "ether")),
    //       owner: exchangeTestUtil.testContext.orderOwners[0],
    //       // maxFeeBips: 50,
    //       feeBips: 50, //protocolTakerFeeBips
    //       fillAmountBorS: true,
    //       feeTokenID: 0, //
    //       fee: new BN("12300000000000"), //
    //       maxFee: new BN("12300000000000"),
    //       storageID: 16384
    //     };

    //     const spread = new BN(web3.utils.toWei("1", "ether"));
    //     const ringA: SpotTrade = {
    //       orderA: order,
    //       orderB: {
    //         tokenS: "GTO",
    //         tokenB: "WETH",
    //         amountS: new BN(web3.utils.toWei("1", "ether")),
    //         amountB: new BN(web3.utils.toWei("2", "ether")),
    //         owner: exchangeTestUtil.testContext.orderOwners[1],
    //         // maxFeeBips: 0,
    //         feeBips: 0, //protocolMakerFeeBips
    //         fillAmountBorS: false,
    //         feeTokenID: 0, //
    //         fee: new BN("21000000000000"), //
    //         maxFee: new BN("21000000000000"),
    //         storageID: 2
    //       },
    //       expected: {
    //         orderA: { filledFraction: 1.0, spread },
    //         orderB: { filledFraction: 0.5 }
    //       }
    //     };

    //     await exchangeTestUtil.setupRing(ringA, true, true, false, false);
    //     await exchangeTestUtil.sendRing(ringA);
    //   }

    //   await exchangeTestUtil.submitTransactions(20, true);

    //   // // Submit the block
    //   // await exchangeTestUtil.submitPendingBlocks();
    // });

    // it("StorageID SpotTrade Turn Over - Transfer - Withdraw - AccountUpdate - AppKeyUpdate", async () => {
    //   await createExchange();

    //   const ownerA = exchangeTestUtil.testContext.orderOwners[0];
    //   const ownerB = exchangeTestUtil.testContext.orderOwners[1];
    //   const balance = new BN(web3.utils.toWei("7", "ether"));
    //   const fee = new BN(web3.utils.toWei("0.1", "ether"));
    //   const token = exchangeTestUtil.getTokenAddress("LRC");
    //   const recipient = ownerB;

    //   await depositAll()

    //   await exchangeTestUtil.transfer(
    //     exchangeTestUtil.testContext.orderOwners[0],
    //     exchangeTestUtil.testContext.orderOwners[1],
    //     "GTO",
    //     new BN(web3.utils.toWei("1", "ether")),
    //     "WETH",
    //     new BN(web3.utils.toWei("0.01", "ether")),
    //     {
    //       authMethod: AuthMethod.EDDSA,
    //       storageID: 0
    //     }
    //   );

    //   await exchangeTestUtil.requestOrderCancel(
    //     exchangeTestUtil.testContext.orderOwners[0],
    //     0,
    //     new BN("21000000000000"), //
    //     new BN("21000000000000"), //
    //     "GTO"
    //   );

    //   await exchangeTestUtil.transfer(
    //     exchangeTestUtil.testContext.orderOwners[0],
    //     exchangeTestUtil.testContext.orderOwners[1],
    //     "GTO",
    //     new BN(web3.utils.toWei("3", "ether")),
    //     "WETH",
    //     new BN(web3.utils.toWei("0.02", "ether")),
    //     {
    //       authMethod: AuthMethod.EDDSA,
    //       storageID: 16384
    //     }
    //   );
      
    //   // await exchangeTestUtil.requestWithdrawal(
    //   //   exchangeTestUtil.testContext.orderOwners[0],
    //   //   "GTO",
    //   //   new BN(web3.utils.toWei("1", "ether")),
    //   //   "WETH",
    //   //   new BN(web3.utils.toWei("0.01", "ether")),
    //   //   { authMethod: AuthMethod.EDDSA, storeRecipient: true, storageID: 1 }
    //   // );
      
    //   await exchangeTestUtil.requestOrderCancel(
    //     exchangeTestUtil.testContext.orderOwners[0],
    //     1,
    //     new BN("21000000000000"), //
    //     new BN("21000000000000"), //
    //     "GTO"
    //   );

    //   await exchangeTestUtil.requestWithdrawal(
    //     exchangeTestUtil.testContext.orderOwners[0],
    //     "GTO",
    //     new BN(web3.utils.toWei("1", "ether")),
    //     "WETH",
    //     new BN(web3.utils.toWei("0.01", "ether")),
    //     { authMethod: AuthMethod.EDDSA, storeRecipient: true, storageID: 16385 }
    //   );

    //   let keyPair = exchangeTestUtil.getKeyPairEDDSA();
    //   await exchangeTestUtil.requestAccountUpdate(
    //     exchangeTestUtil.testContext.orderOwners[1],
    //     "WETH",
    //     new BN(web3.utils.toWei("0.03", "ether")),
    //     keyPair,
    //     {
    //       authMethod: AuthMethod.EDDSA,
    //     }
    //   );
    //   await exchangeTestUtil.requestAppKeyUpdate(
    //     exchangeTestUtil.testContext.orderOwners[1],
    //     "WETH",
    //     new BN(web3.utils.toWei("0.05", "ether")),
    //     keyPair,
    //     0,
    //     0,
    //     0
    //   );

    //   await exchangeTestUtil.submitTransactions(25, true);

    //   // // Submit the block
    //   // await exchangeTestUtil.submitPendingBlocks();
    // });
    // //，StorageID
    // it("StorageID SpotTrade Turn Over - Transfer - Withdraw - AccountUpdate - AppKeyUpdate", async () => {
    //   await createExchange();

    //   const ownerA = exchangeTestUtil.testContext.orderOwners[0];
    //   const ownerB = exchangeTestUtil.testContext.orderOwners[1];
    //   const balance = new BN(web3.utils.toWei("7", "ether"));
    //   const fee = new BN(web3.utils.toWei("0.1", "ether"));
    //   const token = exchangeTestUtil.getTokenAddress("LRC");
    //   const recipient = ownerB;

    //   await depositAll()


    //   await exchangeTestUtil.requestOrderCancel(
    //     exchangeTestUtil.testContext.orderOwners[0],
    //     0,
    //     new BN("21000000000000"), //
    //     new BN("21000000000000"), //
    //     "GTO"
    //   );

    //   await exchangeTestUtil.requestWithdrawal(
    //     exchangeTestUtil.testContext.orderOwners[0],
    //     "GTO",
    //     new BN(web3.utils.toWei("1", "ether")),
    //     "WETH",
    //     new BN(web3.utils.toWei("0.01", "ether")),
    //     { authMethod: AuthMethod.FORCE, storeRecipient: true, storageID: 16384 }
    //   );


    //   await exchangeTestUtil.submitTransactions(8, true);

    //   // // Submit the block
    //   // await exchangeTestUtil.submitPendingBlocks();
    // });
  });
});
