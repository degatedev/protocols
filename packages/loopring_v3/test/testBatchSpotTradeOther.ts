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
    
    // order cancelled but to batch test
    it("BatchSpotTradeThree Test", async () => {

      await createExchange();

      const ownerA = exchangeTestUtil.testContext.orderOwners[0];
      const ownerB = exchangeTestUtil.testContext.orderOwners[1];

      await depositAll();

      {
        // spot trade
        const orderA: OrderInfo = {
          tokenS: "ETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 0,
          fillAmountBorS: true,
          feeTokenID: 2,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
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
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("21000000000000"),
          maxFee: new BN("21000000000000"),
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
          feeBips: 0,
          fillAmountBorS: true,
          feeTokenID: 3,
          fee: new BN("20000000000000"),
          maxFee: new BN("22300000000000"),
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
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("30000000000000"),
          maxFee: new BN("31000000000000"),
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
          tokens: [3, 2, 0],
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          },
          bindTokenID: 0,
        };

        await exchangeTestUtil.setupBatchSpotTrade(batchSpotTrade);

        await exchangeTestUtil.sendBatchSpotTrade(batchSpotTrade);
      }

      // await exchangeTestUtil.submitTransactions(12, true)
      await exchangeTestUtil.submitTransactions(12, true);
      // // Submit the block
      // await exchangeTestUtil.submitPendingBlocks();
    });

  });
});
