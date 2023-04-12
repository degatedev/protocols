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

  describe("DepositWithdraw", function() {
    this.timeout(0);

    it("zero test", async () => {
      await createExchange();

      const ownerA = exchangeTestUtil.testContext.orderOwners[0];
      const ownerB = exchangeTestUtil.testContext.orderOwners[1];
      const balance = new BN(web3.utils.toWei("7", "ether"));
      const fee = new BN(web3.utils.toWei("0.1", "ether"));
      const token = exchangeTestUtil.getTokenAddress("LRC");
      const recipient = ownerB;

      const depositTo3 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[3],
        exchangeTestUtil.testContext.orderOwners[3],
        "GTO",
        new BN(web3.utils.toWei("200", "ether")),
        { autoSetKeys: false }
      );
      const depositTo3WETH = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[3],
        exchangeTestUtil.testContext.orderOwners[3],
        "WETH",
        new BN(web3.utils.toWei("200", "ether")),
        { autoSetKeys: false }
      );
      // account update
      let keyPair = exchangeTestUtil.getKeyPairEDDSA();
      await exchangeTestUtil.requestAccountUpdate(
        exchangeTestUtil.testContext.orderOwners[3],
        "WETH",
        new BN(web3.utils.toWei("0.03", "ether")),
        keyPair,
        {
          authMethod: AuthMethod.ECDSA
        }
      );

      const depositGTOOwner1 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[1],
        exchangeTestUtil.testContext.orderOwners[1],
        "GTO",
        new BN(web3.utils.toWei("200", "ether"))
      );

      const depositWETHOwner1 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[1],
        exchangeTestUtil.testContext.orderOwners[1],
        "WETH",
        new BN(web3.utils.toWei("100", "ether"))
      );

      const depositETHOwner1 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[1],
        exchangeTestUtil.testContext.orderOwners[1],
        "ETH",
        new BN(web3.utils.toWei("20", "ether"))
      );

      const depositGTOOwner0 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[1],
        exchangeTestUtil.testContext.orderOwners[1],
        "GTO",
        new BN(web3.utils.toWei("200", "ether"))
      );

      const depositWETHOwner0 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[0],
        exchangeTestUtil.testContext.orderOwners[0],
        "WETH",
        new BN(web3.utils.toWei("100", "ether"))
      );

      const depositETHOwner0 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[0],
        exchangeTestUtil.testContext.orderOwners[0],
        "ETH",
        new BN(web3.utils.toWei("20", "ether"))
      );

      // auto market order
      // Auto Market StorageID，256-512，After 512, it is reserved for normal orders
      // type = 6: Sell Order，Fixed amountS means that the number of bases is fixed. Adjust the number of quotes. The larger the level, the larger the number of quotes that can be sold by the base of the fixed unit
      // In order to ensure that the boundary value seen by the user has no accuracy problem, levelmax sets the price for the user.
      // At this level, base and quote are determined, visible and understandable by the user. Levelmax to l0, the number of bases is fixed, and the number of quotes decreases layer by layer
      exchangeTestUtil.reserveMultiStorageID(1024);
      // Price:
      //             WETH：1
      //             GTO：0.5
      // What the user sees is the top sale order, that is, the highest price sale order. The sale order of level0 is at the bottom and needs to be calculated
      // amountS, calculate amountB。amountB = 0.5 - gridOffset * 10 = 0.495
      const autoMarketOrderA: OrderInfo = {
        tokenS: "WETH",
        tokenB: "GTO",
        amountS: new BN(web3.utils.toWei("1", "ether")),
        amountB: new BN(web3.utils.toWei("0.495", "ether")),
        owner: exchangeTestUtil.testContext.orderOwners[0],
        // maxFeeBips: 50,
        feeBips: 18, // can not more the protocolFeeBips
        fillAmountBorS: false,
        feeTokenID: 0,
        fee: new BN("12300000000000"),
        maxFee: new BN("12300000000000"),
        uiReferID: 3,
        // autoMarketID: 0,
        type: 6,
        level: 0,
        isNextOrder: false,
        // maxLevel: 255,
        maxLevel: 10,
        gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
        orderOffset: new BN(web3.utils.toWei("0.0002", "ether"))
      };
      const autoMarketOrderB: OrderInfo = {
        tokenS: "GTO",
        tokenB: "WETH",
        amountS: new BN(web3.utils.toWei("4", "ether")),
        amountB: new BN(web3.utils.toWei("2", "ether")),
        owner: exchangeTestUtil.testContext.orderOwners[1],
        // maxFeeBips: 0,
        feeBips: 0, // can not more the protocolFeeBips
        fillAmountBorS: false,
        feeTokenID: 0,
        fee: new BN("21000000000000"),
        maxFee: new BN("21000000000000"),
        uiReferID: 1,
        type: 0
      };

      const spread = new BN(web3.utils.toWei("1", "ether"));
      const ringAutoMarket: SpotTrade = {
        orderA: autoMarketOrderA,
        orderB: autoMarketOrderB,
        expected: {
          orderA: { filledFraction: 1.0, spread },
          orderB: { filledFraction: 0.5 }
        }
      };
      await exchangeTestUtil.setupRing(
        ringAutoMarket,
        true,
        true,
        false,
        false,
        256,
        513
      );
      await exchangeTestUtil.sendRing(ringAutoMarket);

      // AutoMarketOrder Level = 0 Subsequent orders
      // Reverse order: the forward order is the sales order, the reverse order is the purchase order, the number of base currencies is fixed, and the number of quote currencies is adjusted
      // Positive single fillAmountBorS = 0, fixed amountA quantity
      // Reverse single fillAmountBorS = 1, fixed amountB quantity
      // Example: 1eth sold for 6000 USDC, and then paid 4000 USDC to buy 1eth. Then you need to subtract orderoffset from the number of quotes in the reverse order
      // amountS: new BN(web3.utils.toWei("1", "ether")),
      // amountB: new BN(web3.utils.toWei("0.495", "ether")),
      // gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
      // orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
      {
        const autoMarketOrderANext: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.4948", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: true,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          uiReferID: 3,
          // autoMarketID: 0,
          type: 6,
          level: 0,
          startOrder: autoMarketOrderA,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
          isNextOrder: true,
          appointedStorageID: autoMarketOrderA.storageID
        };
        const autoMarketOrderB: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.4948", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("21000000000000"),
          maxFee: new BN("21000000000000"),
          uiReferID: 1
        };

        const spread = new BN(web3.utils.toWei("1", "ether"));
        const ringAutoMarket: SpotTrade = {
          orderA: autoMarketOrderANext,
          orderB: autoMarketOrderB,
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };
        await exchangeTestUtil.setupRing(
          ringAutoMarket,
          true,
          true,
          false,
          false,
          256,
          514
        );
        await exchangeTestUtil.sendRing(ringAutoMarket);
      }
      // AutoMarket,level = 1
      // Base == WETH, Quote == GTO
      // Price:
      //             WETH：1
      //             GTO：0.5
      // What the user sees is the top sale order, that is, the highest price sale order. The sale order of level0 is at the bottom and needs to be calculated
      // level0:
      //     amountS, calculate amountB。amountB = 0.5 - gridOffset * 10 = 0.495
      // level1:
      //     amountS, calculate amountB。amountB = 0.5 - gridOffset * 9 = 0.4955
      {
        const autoMarketOrderALevelOne: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.4955", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          // uiReferID: 3,
          // autoMarketID: 0,
          type: 6,
          level: 1,
          startOrder: autoMarketOrderA,
          isNextOrder: false,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether"))
        };
        const autoMarketOrderB: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.4955", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("21000000000000"),
          maxFee: new BN("21000000000000"),
          uiReferID: 1
        };

        const spread = new BN(web3.utils.toWei("1", "ether"));
        const ringAutoMarket: SpotTrade = {
          orderA: autoMarketOrderALevelOne,
          orderB: autoMarketOrderB,
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };
        await exchangeTestUtil.setupRing(
          ringAutoMarket,
          true,
          true,
          false,
          false,
          257,
          515
        );
        await exchangeTestUtil.sendRing(ringAutoMarket);
      }
      // level = 1 Reverse order
      // Base == WETH, Quote == GTO
      // Price:
      //             WETH：1
      //             GTO：0.5
      // What the user sees is the top sale order, that is, the highest price sale order. The sale order of level0 is at the bottom and needs to be calculated
      // level0:
      //     amountS, calculate amountB。amountB = 0.5 - gridOffset * 10 = 0.495
      // level1:
      //     amountS, calculate amountB。amountB = 0.5 - gridOffset * 9 = 0.4955
      {
        const autoMarketOrderALevelOneNext: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.4953", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: true,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          // uiReferID: 3,
          // autoMarketID: 0,
          type: 6,
          level: 1,
          startOrder: autoMarketOrderA,
          isNextOrder: true,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether"))
        };
        const autoMarketOrderB: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.4953", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("21000000000000"),
          maxFee: new BN("21000000000000"),
          uiReferID: 1
        };

        const spread = new BN(web3.utils.toWei("1", "ether"));
        const ringAutoMarket: SpotTrade = {
          orderA: autoMarketOrderALevelOneNext,
          orderB: autoMarketOrderB,
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };
        await exchangeTestUtil.setupRing(
          ringAutoMarket,
          true,
          true,
          false,
          false,
          257,
          516
        );
        await exchangeTestUtil.sendRing(ringAutoMarket);
      }

      // AutoMarket Buy Order Test
      // What users see is the bill at the bottom, that is, the bill with the lowest price. The bill of level0 is at the top, which needs to be calculated
      // Base == WETH, Quote == GTO
      // Price:
      //             WETH：1
      //             GTO：0.3
      // What the user sees is the top sale order, that is, the highest price sale order. The sale order of level0 is at the bottom and needs to be calculated
      // level0:
      //     amountB, calculate amountS。amountS = 0.3 + gridOffset * 10 = 0.305
      const autoMarketOrderABuy: OrderInfo = {
        tokenS: "GTO",
        tokenB: "WETH",
        amountS: new BN(web3.utils.toWei("0.305", "ether")),
        amountB: new BN(web3.utils.toWei("1", "ether")),
        owner: exchangeTestUtil.testContext.orderOwners[0],
        // maxFeeBips: 50,
        feeBips: 18,
        fillAmountBorS: true,
        feeTokenID: 0,
        fee: new BN("12300000000000"),
        maxFee: new BN("12300000000000"),
        // uiReferID: 3,
        // autoMarketID: 0,
        type: 7,
        level: 0,
        isNextOrder: false,
        // maxLevel: 255,
        maxLevel: 10,
        gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
        orderOffset: new BN(web3.utils.toWei("0.0002", "ether"))
      };
      const autoMarketOrderBBuy: OrderInfo = {
        tokenS: "WETH",
        tokenB: "GTO",
        amountS: new BN(web3.utils.toWei("1", "ether")),
        amountB: new BN(web3.utils.toWei("0.305", "ether")),
        owner: exchangeTestUtil.testContext.orderOwners[1],
        // maxFeeBips: 0,
        feeBips: 0,
        fillAmountBorS: false,
        feeTokenID: 0,
        fee: new BN("21000000000000"),
        maxFee: new BN("21000000000000"),
        // uiReferID: 1,
        type: 0
      };

      const ringAutoMarketBuy: SpotTrade = {
        orderA: autoMarketOrderABuy,
        orderB: autoMarketOrderBBuy,
        expected: {
          orderA: { filledFraction: 1.0, spread },
          orderB: { filledFraction: 0.5 }
        }
      };
      await exchangeTestUtil.setupRing(
        ringAutoMarketBuy,
        true,
        true,
        false,
        false,
        258,
        517
      );
      await exchangeTestUtil.sendRing(ringAutoMarketBuy);

      // buy order， level = 0 Reverse order
      // Base == WETH, Quote == GTO
      // Price:
      //             WETH：1
      //             GTO：0.3
      // What the user sees is the top sale order, that is, the highest price sale order. The sale order of level0 is at the bottom and needs to be calculated
      // level0:
      //     amountB, calculate amountS。amountS = 0.3 + gridOffset * 10 = 0.305
      // level1:
      //     amountB, calculate amountS。amountS = 0.3 + gridOffset * 9 = 0.3045
      {
        const autoMarketOrderABuyReverse: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.3052", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          // uiReferID: 3,
          // autoMarketID: 0,
          type: 7,
          level: 0,
          isNextOrder: true,
          // maxLevel: 255,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
          startOrder: autoMarketOrderABuy,
          appointedStorageID: autoMarketOrderABuy.storageID
        };
        const autoMarketOrderBBuyReverse: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.3052", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("21000000000000"),
          maxFee: new BN("21000000000000"),
          // uiReferID: 1,
          type: 0
        };

        const ringAutoMarketBuy: SpotTrade = {
          orderA: autoMarketOrderABuyReverse,
          orderB: autoMarketOrderBBuyReverse,
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };
        await exchangeTestUtil.setupRing(
          ringAutoMarketBuy,
          true,
          true,
          false,
          false,
          258,
          518
        );
        await exchangeTestUtil.sendRing(ringAutoMarketBuy);
      }

      // Buy Order， level = 1
      // Base == WETH, Quote == GTO
      // price:
      //             WETH：1
      //             GTO：0.3
      // What the user sees is the top sale order, that is, the highest price sale order. The sale order of level0 is at the bottom and needs to be calculated
      // level0:
      //     amountB, calculate amountS。amountS = 0.3 + gridOffset * 10 = 0.305
      // level1:
      //     amountB, calculate amountS。amountS = 0.3 + gridOffset * 9 = 0.3045
      {
        const autoMarketOrderABuyLevel1: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.3045", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: true,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          // uiReferID: 3,
          // autoMarketID: 0,
          type: 7,
          level: 1,
          isNextOrder: false,
          // maxLevel: 255,
          maxLevel: 10,
          startOrder: autoMarketOrderABuy,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether"))
        };
        const autoMarketOrderBBuy: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.3045", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("21000000000000"),
          maxFee: new BN("21000000000000"),
          // uiReferID: 1,
          type: 0
        };

        const ringAutoMarketBuy: SpotTrade = {
          orderA: autoMarketOrderABuyLevel1,
          orderB: autoMarketOrderBBuy,
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };
        await exchangeTestUtil.setupRing(
          ringAutoMarketBuy,
          true,
          true,
          false,
          false,
          259,
          519
        );
        await exchangeTestUtil.sendRing(ringAutoMarketBuy);
      }

      // buy order, level = 1 Reverse order
      // Base == WETH, Quote == GTO
      // Price：
      //             WETH：1
      //             GTO：0.3
      // What the user sees is the top sale order, that is, the highest price sale order. The sale order of level0 is at the bottom and needs to be calculated
      // level0:
      //     amountB, calculate amountS。amountS = 0.3 + gridOffset * 10 = 0.305
      // level1:
      //     amountB, calculate amountS。amountS = 0.3 + gridOffset * 9 = 0.3045
      {
        const autoMarketOrderABuyReverse: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.3047", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          // uiReferID: 3,
          // autoMarketID: 0,
          type: 7,
          level: 1,
          isNextOrder: false,
          startOrder: autoMarketOrderABuy,
          // maxLevel: 255,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether"))
        };
        const autoMarketOrderBBuyReverse: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.3047", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("21000000000000"),
          maxFee: new BN("21000000000000"),
          // uiReferID: 1,
          type: 0
        };

        const ringAutoMarketBuy: SpotTrade = {
          orderA: autoMarketOrderABuyReverse,
          orderB: autoMarketOrderBBuyReverse,
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };
        await exchangeTestUtil.setupRing(
          ringAutoMarketBuy,
          true,
          true,
          false,
          false,
          259,
          520
        );
        await exchangeTestUtil.sendRing(ringAutoMarketBuy);
      }

      await exchangeTestUtil.submitTransactions(20, true);
      // // // Submit the block
      // await exchangeTestUtil.submitPendingBlocks();
    });

    it("auto market matching split two Spottrade", async () => {
      await createExchange();

      const ownerA = exchangeTestUtil.testContext.orderOwners[0];
      const ownerB = exchangeTestUtil.testContext.orderOwners[1];

      const depositGTOTo0 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[0],
        exchangeTestUtil.testContext.orderOwners[0],
        "GTO",
        new BN(web3.utils.toWei("200", "ether")),
        { autoSetKeys: true }
      );
      const depositWETHTo0 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[0],
        exchangeTestUtil.testContext.orderOwners[0],
        "WETH",
        new BN(web3.utils.toWei("200", "ether")),
        { autoSetKeys: false }
      );
      const depositETHTo0 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[0],
        exchangeTestUtil.testContext.orderOwners[0],
        "ETH",
        new BN(web3.utils.toWei("20", "ether"))
      );

      const depositGTOOwner1 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[1],
        exchangeTestUtil.testContext.orderOwners[1],
        "GTO",
        new BN(web3.utils.toWei("200", "ether"))
      );

      const depositWETHOwner1 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[1],
        exchangeTestUtil.testContext.orderOwners[1],
        "WETH",
        new BN(web3.utils.toWei("100", "ether"))
      );

      const depositETHOwner1 = await exchangeTestUtil.deposit(
        exchangeTestUtil.testContext.orderOwners[1],
        exchangeTestUtil.testContext.orderOwners[1],
        "ETH",
        new BN(web3.utils.toWei("20", "ether"))
      );

      exchangeTestUtil.reserveMultiStorageID(1024);
      const autoMarketOrderA: OrderInfo = {
        tokenS: "WETH",
        tokenB: "GTO",
        amountS: new BN(web3.utils.toWei("1", "ether")),
        amountB: new BN(web3.utils.toWei("0.495", "ether")),
        owner: exchangeTestUtil.testContext.orderOwners[0],
        // maxFeeBips: 50,
        feeBips: 18,
        fillAmountBorS: false,
        feeTokenID: 0,
        fee: new BN("12300000000000"),
        maxFee: new BN("12300000000000"),
        uiReferID: 3,
        // autoMarketID: 0,
        type: 6,
        level: 0,
        isNextOrder: false,
        // maxLevel: 255,
        maxLevel: 10,
        gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
        orderOffset: new BN(web3.utils.toWei("0.0002", "ether"))
      };
      const autoMarketOrderB: OrderInfo = {
        tokenS: "GTO",
        tokenB: "WETH",
        amountS: new BN(web3.utils.toWei("4", "ether")),
        amountB: new BN(web3.utils.toWei("2", "ether")),
        owner: exchangeTestUtil.testContext.orderOwners[1],
        // maxFeeBips: 0,
        feeBips: 0,
        fillAmountBorS: false,
        feeTokenID: 0,
        fee: new BN("21000000000000"),
        maxFee: new BN("21000000000000"),
        uiReferID: 1,
        type: 0
      };

      const spread = new BN(web3.utils.toWei("1", "ether"));
      const ringAutoMarket: SpotTrade = {
        orderA: autoMarketOrderA,
        orderB: autoMarketOrderB,
        expected: {
          orderA: { filledFraction: 1.0, spread },
          orderB: { filledFraction: 0.5 }
        }
      };
      await exchangeTestUtil.setupRing(
        ringAutoMarket,
        true,
        true,
        false,
        false,
        256,
        513
      );
      await exchangeTestUtil.sendRing(ringAutoMarket);

      {
        const autoMarketOrderANext: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.4948", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: true,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          uiReferID: 3,
          // autoMarketID: 0,
          type: 6,
          level: 0,
          startOrder: autoMarketOrderA,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
          isNextOrder: true,
          appointedStorageID: autoMarketOrderA.storageID
        };
        const autoMarketOrderB: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.4948", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("21000000000000"),
          maxFee: new BN("21000000000000"),
          uiReferID: 1
        };

        const spread = new BN(web3.utils.toWei("1", "ether"));
        const ringAutoMarket: SpotTrade = {
          orderA: autoMarketOrderANext,
          orderB: autoMarketOrderB,
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };
        await exchangeTestUtil.setupRing(
          ringAutoMarket,
          true,
          true,
          false,
          false,
          256,
          514
        );
        await exchangeTestUtil.sendRing(ringAutoMarket);
      }

      {
        const autoMarketOrderALevelOne: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.4955", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          // uiReferID: 3,
          // autoMarketID: 0,
          type: 6,
          level: 1,
          startOrder: autoMarketOrderA,
          isNextOrder: false,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether"))
        };
        const autoMarketOrderB: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.4955", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("21000000000000"),
          maxFee: new BN("21000000000000"),
          uiReferID: 1
        };

        const spread = new BN(web3.utils.toWei("1", "ether"));
        const ringAutoMarket: SpotTrade = {
          orderA: autoMarketOrderALevelOne,
          orderB: autoMarketOrderB,
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };
        await exchangeTestUtil.setupRing(
          ringAutoMarket,
          true,
          true,
          false,
          false,
          257,
          515
        );
        await exchangeTestUtil.sendRing(ringAutoMarket);
      }

      {
        const autoMarketOrderALevelOneNext: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.4953", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: true,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          // uiReferID: 3,
          // autoMarketID: 0,
          type: 6,
          level: 1,
          startOrder: autoMarketOrderA,
          isNextOrder: true,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether"))
        };
        const autoMarketOrderB: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.4953", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("21000000000000"),
          maxFee: new BN("21000000000000"),
          uiReferID: 1
        };

        const spread = new BN(web3.utils.toWei("1", "ether"));
        const ringAutoMarket: SpotTrade = {
          orderA: autoMarketOrderALevelOneNext,
          orderB: autoMarketOrderB,
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };
        await exchangeTestUtil.setupRing(
          ringAutoMarket,
          true,
          true,
          false,
          false,
          257,
          516
        );
        await exchangeTestUtil.sendRing(ringAutoMarket);
      }

      const autoMarketOrderABuy: OrderInfo = {
        tokenS: "GTO",
        tokenB: "WETH",
        amountS: new BN(web3.utils.toWei("0.305", "ether")),
        amountB: new BN(web3.utils.toWei("1", "ether")),
        owner: exchangeTestUtil.testContext.orderOwners[0],
        // maxFeeBips: 50,
        feeBips: 18,
        fillAmountBorS: true,
        feeTokenID: 0,
        fee: new BN("12300000000000"),
        maxFee: new BN("12300000000000"),
        // uiReferID: 3,
        // autoMarketID: 0,
        type: 7,
        level: 0,
        isNextOrder: false,
        // maxLevel: 255,
        maxLevel: 10,
        gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
        orderOffset: new BN(web3.utils.toWei("0.0002", "ether"))
      };
      const autoMarketOrderBBuy: OrderInfo = {
        tokenS: "WETH",
        tokenB: "GTO",
        amountS: new BN(web3.utils.toWei("1", "ether")),
        amountB: new BN(web3.utils.toWei("0.305", "ether")),
        owner: exchangeTestUtil.testContext.orderOwners[1],
        // maxFeeBips: 0,
        feeBips: 0,
        fillAmountBorS: false,
        feeTokenID: 0,
        fee: new BN("21000000000000"),
        maxFee: new BN("21000000000000"),
        // uiReferID: 1,
        type: 0
      };

      const ringAutoMarketBuy: SpotTrade = {
        orderA: autoMarketOrderABuy,
        orderB: autoMarketOrderBBuy,
        expected: {
          orderA: { filledFraction: 1.0, spread },
          orderB: { filledFraction: 0.5 }
        }
      };
      await exchangeTestUtil.setupRing(
        ringAutoMarketBuy,
        true,
        true,
        false,
        false,
        258,
        517
      );
      await exchangeTestUtil.sendRing(ringAutoMarketBuy);

      {
        const autoMarketOrderABuyReverse: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.3052", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          // uiReferID: 3,
          // autoMarketID: 0,
          type: 7,
          level: 0,
          isNextOrder: true,
          // maxLevel: 255,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether")),
          startOrder: autoMarketOrderABuy,
          appointedStorageID: autoMarketOrderABuy.storageID
        };
        const autoMarketOrderBBuyReverse: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.3052", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("21000000000000"),
          maxFee: new BN("21000000000000"),
          // uiReferID: 1,
          type: 0
        };

        const ringAutoMarketBuy: SpotTrade = {
          orderA: autoMarketOrderABuyReverse,
          orderB: autoMarketOrderBBuyReverse,
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };
        await exchangeTestUtil.setupRing(
          ringAutoMarketBuy,
          true,
          true,
          false,
          false,
          258,
          518
        );
        await exchangeTestUtil.sendRing(ringAutoMarketBuy);
      }

      {
        const autoMarketOrderABuyLevel1: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.3045", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: true,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          // uiReferID: 3,
          // autoMarketID: 0,
          type: 7,
          level: 1,
          isNextOrder: false,
          // maxLevel: 255,
          maxLevel: 10,
          startOrder: autoMarketOrderABuy,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether"))
        };
        const autoMarketOrderBBuy: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.3045", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("21000000000000"),
          maxFee: new BN("21000000000000"),
          // uiReferID: 1,
          type: 0
        };

        const ringAutoMarketBuy: SpotTrade = {
          orderA: autoMarketOrderABuyLevel1,
          orderB: autoMarketOrderBBuy,
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };
        await exchangeTestUtil.setupRing(
          ringAutoMarketBuy,
          true,
          true,
          false,
          false,
          259,
          519
        );
        await exchangeTestUtil.sendRing(ringAutoMarketBuy);
      }

      {
        const autoMarketOrderABuyReverse: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.3047", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          // maxFeeBips: 50,
          feeBips: 18,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          // uiReferID: 3,
          // autoMarketID: 0,
          type: 7,
          level: 1,
          isNextOrder: false,
          startOrder: autoMarketOrderABuy,
          // maxLevel: 255,
          maxLevel: 10,
          gridOffset: new BN(web3.utils.toWei("0.0005", "ether")),
          orderOffset: new BN(web3.utils.toWei("0.0002", "ether"))
        };
        const autoMarketOrderBBuyReverse: OrderInfo = {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("0.3047", "ether")),
          amountB: new BN(web3.utils.toWei("1", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1],
          // maxFeeBips: 0,
          feeBips: 0,
          fillAmountBorS: false,
          feeTokenID: 0,
          fee: new BN("21000000000000"),
          maxFee: new BN("21000000000000"),
          // uiReferID: 1,
          type: 0
        };

        const ringAutoMarketBuy: SpotTrade = {
          orderA: autoMarketOrderABuyReverse,
          orderB: autoMarketOrderBBuyReverse,
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };
        await exchangeTestUtil.setupRing(
          ringAutoMarketBuy,
          true,
          true,
          false,
          false,
          259,
          520
        );
        await exchangeTestUtil.sendRing(ringAutoMarketBuy);
      }

      await exchangeTestUtil.submitTransactions(16, true);
      // // // Submit the block
      // await exchangeTestUtil.submitPendingBlocks();
    });

    /*
    [AuthMethod.EDDSA, AuthMethod.ECDSA].forEach(function(authMethod) {
      it("Withdrawal (fee > maxFee) (" + authMethod + ")", async () => {
        await createExchange();

        const owner = exchangeTestUtil.testContext.orderOwners[0];
        const balance = new BN(web3.utils.toWei("4", "ether"));
        const toWithdraw = new BN(web3.utils.toWei("2", "ether"));
        const token = "ETH";
        const feeToken = "ETH";
        const fee = new BN(web3.utils.toWei("1.5", "ether"));

        await exchangeTestUtil.deposit(owner, owner, token, balance);
        await exchangeTestUtil.requestWithdrawal(
          owner,
          token,
          toWithdraw,
          feeToken,
          fee,
          { maxFee: fee.div(new BN(3)), authMethod }
        );

        // Commit the transfers
        if (authMethod === AuthMethod.EDDSA) {
          await expectThrow(
            exchangeTestUtil.submitTransactions(),
            "invalid block"
          );
        } else {
          await exchangeTestUtil.submitTransactions();
          await expectThrow(
            exchangeTestUtil.submitPendingBlocks(),
            "WITHDRAWAL_FEE_TOO_HIGH"
          );
        }
      });
    });*/
  });
});
