import BN = require("bn.js");
import childProcess = require("child_process");
import ethUtil = require("ethereumjs-util");
import fs = require("fs");
import path = require("path");
import http = require("http");
import { performance } from "perf_hooks";
import { SHA256 } from "sha2";
import { Artifacts } from "../util/Artifacts";
import { SignatureType, sign, verifySignature } from "../util/Signature";
import {
  Bitstream,
  BitstreamEx,
  BlockType,
  calculateCalldataCost,
  compressZeros,
  CompressionType,
  Constants,
  EdDSA,
  Explorer,
  roundToFloatValue,
  toFloat,
  fromFloat,
  TransactionType,
  Poseidon,
  WithdrawFromMerkleTreeData,
  KeyPair
} from "loopringV3.js";
import { Context } from "./context";
import { doDebugLogging, logDebug, logInfo } from "./logs";
import * as sigUtil from "eth-sig-util";
import { Simulator } from "./simulator";
import { ExchangeTestContext } from "./testExchangeContext";
import {
  Account,
  AuthMethod,
  Block,
  BlockCallback,
  Deposit,
  Transfer,
  Noop,
  AutoMarketStartOrderInfo,
  OrderInfo,
  TxBlock,
  AccountUpdate,
  SpotTrade,
  WithdrawalRequest,
  // SignatureVerification,
  OrderCancel,
  AppKeyUpdate,
  BatchSpotTrade
  // AutoMarketUpdate
} from "./types";

const LoopringIOExchangeOwner = artifacts.require("LoopringIOExchangeOwner");

type TxType =
  | Noop
  | SpotTrade
  | Transfer
  | WithdrawalRequest
  | Deposit
  | AccountUpdate
  | OrderCancel
  | AppKeyUpdate
  | BatchSpotTrade;
// | AutoMarketUpdate;

// JSON replacer function for BN values
function replacer(name: any, val: any) {
  if (
    name === "balance" ||
    name === "amountS" ||
    name === "amountB" ||
    name === "amount" ||
    name === "fee" ||
    name === "maxFee" ||
    name === "lowPrice" ||
    name === "highPrice" ||
    name === "gridOffset" ||
    name === "orderOffset" ||
    name === "originalMaxFee" ||
    name === "tokenWeight" ||
    name === "mintMinAmount" ||
    name === "burnAmount" ||
    name === "deltaFilledS" ||
    name === "deltaFilledB" ||
    name === "tradingFee"
  ) {
    return new BN(val, 16).toString(10);
  } else if (
    name === "owner" ||
    name === "newOwner" ||
    name === "from" ||
    name === "to" ||
    name === "payerTo" ||
    name === "to" ||
    name === "exchange" ||
    name === "taker" ||
    name === "onchainDataHash"
  ) {
    return new BN(val.slice(2), 16).toString(10);
  } else if (name === "joinAmounts" || name === "exitMinAmounts") {
    const array: string[] = [];
    for (const v of val) {
      array.push(new BN(v, 16).toString(10));
    }
    return array;
  } else {
    return val;
  }
}

export interface ExchangeOptions {
  setupTestState?: boolean;
  deterministic?: boolean;
  useOwnerContract?: boolean;
}

export interface DepositOptions {
  autoSetKeys?: boolean;
  accountContract?: any;
  amountDepositedCanDiffer?: boolean;
}

export interface TransferOptions {
  authMethod?: AuthMethod;
  useDualAuthoring?: boolean;
  secretKnown?: boolean;
  amountToDeposit?: BN;
  feeToDeposit?: BN;
  transferToNew?: boolean;
  signer?: string;
  validUntil?: number;
  storageID?: number;
  maxFee?: BN;
  putAddressesInDA?: boolean;
}

export interface WithdrawOptions {
  authMethod?: AuthMethod;
  to?: string;
  minGas?: number;
  gas?: number;
  // extraData?: string;
  signer?: string;
  validUntil?: number;
  storageID?: number;
  maxFee?: BN;
  storeRecipient?: boolean;
  skipForcedAuthentication?: boolean;
}

export interface AccountUpdateOptions {
  authMethod?: AuthMethod;
  validUntil?: number;
  maxFee?: BN;
}

export interface OnchainBlock {
  blockType: number;
  blockSize: number;
  blockVersion: number;
  data: any;
  proof: any;
  storeBlockInfoOnchain: boolean;
  auxiliaryData?: any;
  offchainData?: any;
}

export interface AuxiliaryData {
  txIndex: number;
  txAuxiliaryData?: any;
}

export namespace AccountUpdateUtils {
  export function toTypedData(update: AccountUpdate, verifyingContract: string) {
    const typedData = {
      types: {
        EIP712Domain: [
          { name: "name", type: "string" },
          { name: "version", type: "string" },
          { name: "chainId", type: "uint256" },
          { name: "verifyingContract", type: "address" }
        ],
        AccountUpdate: [
          { name: "owner", type: "address" },
          { name: "accountID", type: "uint32" },
          { name: "feeTokenID", type: "uint32" },
          { name: "maxFee", type: "uint96" },
          { name: "publicKey", type: "uint256" },
          { name: "validUntil", type: "uint32" },
          { name: "nonce", type: "uint32" }
          // //refe-refe
          // { name: "referID", type: "uint32" }
        ]
      },
      primaryType: "AccountUpdate",
      domain: {
        name: "DeGate Protocol",
        version: "0.1.0",
        chainId: new BN(/*await web3.eth.net.getId()*/ 1),
        verifyingContract
      },
      message: {
        owner: update.owner,
        accountID: update.accountID,
        feeTokenID: update.feeTokenID,
        maxFee: update.maxFee,
        publicKey: new BN(EdDSA.pack(update.publicKeyX, update.publicKeyY), 16),
        validUntil: update.validUntil,
        nonce: update.nonce
        // //refe-refe
        // referID: update.referID
      }
    };
    return typedData;
  }

  export function getHash(update: AccountUpdate, verifyingContract: string) {
    const typedData = this.toTypedData(update, verifyingContract);
    return sigUtil.TypedDataUtils.sign(typedData);
  }

  export function sign(keyPair: any, update: AccountUpdate) {
    // Calculate hash
    //refe-refe
    const hasher = Poseidon.createHash(9, 6, 53);
    // const hasher = Poseidon.createHash(10, 6, 53);
    var accountID = update.accountID;
    //accoun0
    if (update.nonce == 0) {
      accountID = 0;
    }
    const inputs = [
      update.exchange,
      // update.accountID,
      accountID,
      update.feeTokenID,
      update.maxFee,
      update.publicKeyX,
      update.publicKeyY,
      update.validUntil,
      update.nonce
      // //refe-refe
      // update.referID
    ];
    const hash = hasher(inputs).toString(10);

    // Create signature
    const signature = EdDSA.sign(keyPair.secretKey, hash);

    // Verify signature
    const success = EdDSA.verify(hash, signature, [keyPair.publicKeyX, keyPair.publicKeyY]);
    assert(success, "Failed to verify signature");

    return signature;
  }
}

export namespace WithdrawalUtils {
  export function toTypedData(withdrawal: WithdrawalRequest, verifyingContract: string) {
    const typedData = {
      types: {
        EIP712Domain: [
          { name: "name", type: "string" },
          { name: "version", type: "string" },
          { name: "chainId", type: "uint256" },
          { name: "verifyingContract", type: "address" }
        ],
        Withdrawal: [
          { name: "owner", type: "address" },
          { name: "accountID", type: "uint32" },
          { name: "tokenID", type: "uint32" },
          // { name: "amount", type: "uint96" },
          { name: "amount", type: "uint248" },
          { name: "feeTokenID", type: "uint32" },
          { name: "maxFee", type: "uint96" },
          { name: "to", type: "address" },
          // { name: "extraData", type: "bytes" },
          { name: "minGas", type: "uint248" },
          { name: "validUntil", type: "uint32" },
          { name: "storageID", type: "uint32" }
        ]
      },
      primaryType: "Withdrawal",
      domain: {
        name: "DeGate Protocol",
        version: "0.1.0",
        chainId: new BN(/*await web3.eth.net.getId()*/ 1),
        verifyingContract
      },
      message: {
        owner: withdrawal.owner,
        accountID: withdrawal.accountID,
        tokenID: withdrawal.tokenID,
        amount: withdrawal.amount,
        feeTokenID: withdrawal.feeTokenID,
        maxFee: withdrawal.maxFee,
        to: withdrawal.to,
        // extraData: withdrawal.extraData,
        minGas: withdrawal.minGas,
        validUntil: withdrawal.validUntil,
        storageID: withdrawal.storageID
      }
    };
    return typedData;
  }

  export function getHash(withdrawal: WithdrawalRequest, verifyingContract: string) {
    const typedData = this.toTypedData(withdrawal, verifyingContract);
    return sigUtil.TypedDataUtils.sign(typedData);
  }

  export function sign(keyPair: any, withdrawal: WithdrawalRequest) {
    // Calculate hash
    // const hasher = Poseidon.createHash(10, 6, 53);
    const hasher = Poseidon.createHash(11, 6, 53);
    const inputs = [
      withdrawal.exchange,
      withdrawal.accountID,
      withdrawal.tokenID,
      withdrawal.amount,
      withdrawal.feeTokenID,
      withdrawal.maxFee,
      withdrawal.onchainDataHash,
      withdrawal.validUntil,
      withdrawal.storageID,
      withdrawal.useAppKey
    ];
    const hash = hasher(inputs).toString(10);

    // Create signature
    withdrawal.signature = EdDSA.sign(keyPair.secretKey, hash);

    // Verify signature
    const success = EdDSA.verify(hash, withdrawal.signature, [keyPair.publicKeyX, keyPair.publicKeyY]);
    assert(success, "Failed to verify signature");
  }
}

export namespace TransferUtils {
  export function toTypedData(transfer: Transfer, verifyingContract: string) {
    const typedData = {
      types: {
        EIP712Domain: [
          { name: "name", type: "string" },
          { name: "version", type: "string" },
          { name: "chainId", type: "uint256" },
          { name: "verifyingContract", type: "address" }
        ],
        Transfer: [
          { name: "from", type: "address" },
          { name: "to", type: "address" },
          { name: "tokenID", type: "uint32" },
          { name: "amount", type: "uint96" },
          { name: "feeTokenID", type: "uint32" },
          { name: "maxFee", type: "uint96" },
          { name: "validUntil", type: "uint32" },
          { name: "storageID", type: "uint32" }
        ]
      },
      primaryType: "Transfer",
      domain: {
        name: "DeGate Protocol",
        version: "0.1.0",
        chainId: new BN(/*await web3.eth.net.getId()*/ 1),
        verifyingContract
      },
      message: {
        from: transfer.from,
        to: transfer.to,
        tokenID: transfer.tokenID,
        amount: transfer.amount,
        feeTokenID: transfer.feeTokenID,
        maxFee: transfer.maxFee,
        validUntil: transfer.validUntil,
        storageID: transfer.storageID
      }
    };
    return typedData;
  }

  export function getHash(transfer: Transfer, verifyingContract: string) {
    const typedData = this.toTypedData(transfer, verifyingContract);
    return sigUtil.TypedDataUtils.sign(typedData);
  }

  export function sign(keyPair: any, transfer: Transfer, payer: boolean) {
    // Calculate hash
    // const hasher = Poseidon.createHash(13, 6, 53);
    const hasher = Poseidon.createHash(14, 6, 53);
    const inputs = [
      transfer.exchange,
      transfer.fromAccountID,
      payer ? transfer.payerToAccountID : transfer.toAccountID,
      transfer.tokenID,
      transfer.amount,
      transfer.feeTokenID,
      transfer.maxFee,
      payer ? transfer.payerTo : transfer.to,
      transfer.dualAuthorX,
      transfer.dualAuthorY,
      transfer.validUntil,
      transfer.storageID,
      transfer.useAppKey
    ];
    console.log("useAppKeyuseAppKeyuseAppKey:" + transfer.useAppKey);
    const hash = hasher(inputs).toString(10);

    // Create signature
    const signature = EdDSA.sign(keyPair.secretKey, hash);

    // Verify signature
    const success = EdDSA.verify(hash, signature, [keyPair.publicKeyX, keyPair.publicKeyY]);
    assert(success, "Failed to verify signature");

    return signature;
  }
}

export class ExchangeTestUtil {
  public context: Context;
  public testContext: ExchangeTestContext;

  public explorer: Explorer;

  public blockSizes = [8];

  public loopringV3: any;
  public blockVerifier: any;

  public lrcAddress: string;
  public wethAddress: string;

  public exchange: any;
  public depositContract: any;
  public exchangeOwner: string;
  public exchangeOperator: string;

  public exchangeIdGenerator: number = 0;
  public exchangeId: number;

  public operator: any;
  public activeOperator: number;

  // public userStakingPool: any;
  public protocolFeeVault: any;
  public protocolFeeVaultContract: any;

  public blocks: Block[][] = [];
  public accounts: Account[][] = [];

  public operators: number[] = [];

  public GENESIS_MERKLE_ROOT: BN;
  public GENESIS_MERKLE_ASSET_ROOT: BN;
  public SNARK_SCALAR_FIELD: BN;
  public MAX_OPEN_FORCED_REQUESTS: number;
  public MAX_AGE_FORCED_REQUEST_UNTIL_WITHDRAW_MODE: number;
  public TIMESTAMP_HALF_WINDOW_SIZE_IN_SECONDS: number;
  public MAX_NUM_TOKENS: number;
  public MIN_AGE_PROTOCOL_FEES_UNTIL_UPDATED: number;
  public MIN_TIME_IN_SHUTDOWN: number;
  public TX_DATA_AVAILABILITY_SIZE: number;
  public MAX_AGE_DEPOSIT_UNTIL_WITHDRAWABLE_UPPERBOUND: number;
  public MAX_FORCED_WITHDRAWAL_FEE: BN;

  public tokenAddressToIDMap = new Map<string, number>();
  public tokenIDToAddressMap = new Map<number, string>();

  public contracts = new Artifacts(artifacts);

  public pendingBlocks: Block[][] = [];

  public compressionType = CompressionType.LZ;

  public autoCommit = true;

  public useProverServer: boolean = false;

  // Enabling this will remove randomness so gas measurements
  // can be compared between different runs.
  public deterministic: boolean = false;

  private pendingTransactions: TxType[][] = [];
  private pendingBlockCallbacks: BlockCallback[][] = [];

  private storageIDGenerator: number = 0;

  private MAX_NUM_EXCHANGES: number = 512;

  private proverPorts = new Map<number, number>();
  private portGenerator = 1234;

  private emptyMerkleRoot = "0x3e1788bf14436c39a3841ae888ffb3e6ec8405bc2773afa28b6d4dfc309cf19";

  private emptyMerkleAssetRoot = "0x71c8b14d71d432750479f5fe6e08abe1ec04712835a83cdf84d0483b9382ae8";

  public async initialize(accounts: string[]) {
    this.context = await this.createContractContext();
    this.testContext = await this.createExchangeTestContext(accounts);

    this.explorer = new Explorer();
    await this.explorer.initialize(web3, 0);

    // Initialize LoopringV3
    this.protocolFeeVault = this.testContext.orderOwners[this.testContext.orderOwners.length - 1];

    await this.loopringV3.updateSettings(
      this.protocolFeeVault,
      this.blockVerifier.address,
      new BN(web3.utils.toWei("0.02", "ether")),
      { from: this.testContext.deployer }
    );

    // Register LoopringV3 to UniversalRegistry
    // await this.universalRegistry.registerProtocol(
    //   this.loopringV3.address,
    //   this.exchange.address,
    //   { from: this.testContext.deployer }
    // );

    await this.loopringV3.updateProtocolFeeSettings(50, {
      from: this.testContext.deployer
    });

    for (let i = 0; i < this.MAX_NUM_EXCHANGES; i++) {
      this.pendingTransactions.push([]);
      this.pendingBlockCallbacks.push([]);
      this.pendingBlocks.push([]);
      this.blocks.push([]);

      const protocolFeeAccount: Account = {
        accountID: 0,
        owner: Constants.zeroAddress,
        publicKeyX: "0",
        publicKeyY: "0",
        appKeyPublicKeyX: "0",
        appKeyPublicKeyY: "0",
        secretKey: "0",
        appKeySecretKey: "0",
        nonce: 0
      };
      this.accounts.push([protocolFeeAccount]);
    }

    await this.createExchange(this.testContext.deployer);

    const constants = await this.exchange.getConstants();
    this.SNARK_SCALAR_FIELD = new BN(constants.SNARK_SCALAR_FIELD);
    this.MAX_OPEN_FORCED_REQUESTS = new BN(constants.MAX_OPEN_FORCED_REQUESTS).toNumber();
    this.MAX_AGE_FORCED_REQUEST_UNTIL_WITHDRAW_MODE = new BN(
      constants.MAX_AGE_FORCED_REQUEST_UNTIL_WITHDRAW_MODE
    ).toNumber();
    this.TIMESTAMP_HALF_WINDOW_SIZE_IN_SECONDS = new BN(constants.TIMESTAMP_HALF_WINDOW_SIZE_IN_SECONDS).toNumber();
    this.MAX_NUM_TOKENS = new BN(constants.MAX_NUM_TOKENS).toNumber();
    this.MIN_AGE_PROTOCOL_FEES_UNTIL_UPDATED = new BN(constants.MIN_AGE_PROTOCOL_FEES_UNTIL_UPDATED).toNumber();
    this.MIN_TIME_IN_SHUTDOWN = new BN(constants.MIN_TIME_IN_SHUTDOWN).toNumber();
    this.TX_DATA_AVAILABILITY_SIZE = new BN(constants.TX_DATA_AVAILABILITY_SIZE).toNumber();
    this.MAX_AGE_DEPOSIT_UNTIL_WITHDRAWABLE_UPPERBOUND = new BN(
      constants.MAX_AGE_DEPOSIT_UNTIL_WITHDRAWABLE_UPPERBOUND
    ).toNumber();
    this.MAX_FORCED_WITHDRAWAL_FEE = new BN(constants.MAX_FORCED_WITHDRAWAL_FEE);
  }

  public async setupTestState(exchangeID: number) {
    this.operators[exchangeID] = await this.createOperator(this.exchangeOperator);
  }

  public async createOperator(owner: string) {
    // Make an account for the operator
    const deposit = await this.deposit(owner, owner, Constants.zeroAddress, new BN(0));
    return deposit.accountID;
  }

  public async getEventsFromContract(contract: any, eventName: string, fromBlock: number) {
    return await contract
      .getPastEvents(eventName, {
        fromBlock,
        toBlock: "latest"
      })
      .then((events: any) => {
        return events;
      });
  }

  // This works differently from truffleAssert.eventEmitted in that it also is able to
  // get events emmitted in `deep contracts` (i.e. events not emmitted in the contract
  // the function got called in).
  public async assertEventsEmitted(contract: any, event: string, numExpected: number, filter?: any) {
    const eventArr: any = await this.getEventsFromContract(contract, event, web3.eth.blockNumber);
    const items = eventArr.map((eventObj: any) => {
      if (filter !== undefined) {
        assert(filter(eventObj.args), "Event values unexpected: " + eventObj);
      }
      return eventObj.args;
    });
    assert.equal(items.length, numExpected, "Unexpected number of '" + event + "' events", event);
    return items;
  }

  public async assertEventEmitted(contract: any, event: string, filter?: any) {
    return (await this.assertEventsEmitted(contract, event, 1, filter))[0];
  }

  public async assertNoEventEmitted(contract: any, event: string) {
    this.assertEventsEmitted(contract, event, 0, undefined);
  }

  public async transfer(
    from: string,
    to: string,
    token: string,
    amount: BN,
    feeToken: string,
    fee: BN,
    options: TransferOptions = {},
    useAppKey?: number
  ) {
    // amount = roundToFloatValue(amount, Constants.Float24Encoding);
    amount = roundToFloatValue(amount, Constants.Float32Encoding);
    fee = roundToFloatValue(fee, Constants.Float16Encoding);

    // Fill in defaults
    const amountToDeposit = options.amountToDeposit ? options.amountToDeposit : amount;
    const feeToDeposit = options.feeToDeposit ? options.feeToDeposit : fee;
    const authMethod = options.authMethod !== undefined ? options.authMethod : AuthMethod.EDDSA;
    const useDualAuthoring = options.useDualAuthoring !== undefined ? options.useDualAuthoring : false;
    const secretKnown = options.secretKnown !== undefined ? options.secretKnown : true;
    const transferToNew = options.transferToNew !== undefined ? options.transferToNew : false;
    const signer = options.signer !== undefined ? options.signer : from;
    const validUntil = options.validUntil !== undefined ? options.validUntil : 0xffffffff;
    const storageID = options.storageID !== undefined ? options.storageID : this.storageIDGenerator++;
    const maxFee = options.maxFee !== undefined ? options.maxFee : fee;
    const putAddressesInDA = options.putAddressesInDA !== undefined ? options.putAddressesInDA : false;
    useAppKey = useAppKey !== undefined ? useAppKey : 0;
    // From
    if (amountToDeposit.gt(new BN(0))) {
      await this.deposit(from, from, token, amountToDeposit);
    }
    if (feeToDeposit.gt(new BN(0))) {
      await this.deposit(from, from, feeToken, feeToDeposit);
    }

    // To
    let toAccountID = this.getAccountID(to);
    if (!transferToNew) {
      if (toAccountID === undefined) {
        await this.deposit(to, to, token, new BN(0));
        toAccountID = this.findAccount(to).accountID;
      }
    } else {
      const account: Account = {
        accountID: this.accounts[this.exchangeId].length,
        owner: to,
        publicKeyX: "0",
        publicKeyY: "0",
        appKeyPublicKeyX: "0",
        appKeyPublicKeyY: "0",
        secretKey: "0",
        appKeySecretKey: "0",
        nonce: 0
      };
      this.accounts[this.exchangeId].push(account);
      toAccountID = account.accountID;
    }

    // Tokens
    if (!token.startsWith("0x")) {
      token = this.testContext.tokenSymbolAddrMap.get(token);
    }
    if (!feeToken.startsWith("0x")) {
      feeToken = this.testContext.tokenSymbolAddrMap.get(feeToken);
    }
    const tokenID = this.tokenAddressToIDMap.get(token);
    const feeTokenID = this.tokenAddressToIDMap.get(feeToken);

    // Dual author key
    const dualAuthorkeyPair = this.getKeyPairEDDSA();
    let dualAuthorX = "0";
    let dualAuthorY = "0";
    let dualSecretKey = "0";
    if (useDualAuthoring) {
      dualAuthorX = dualAuthorkeyPair.publicKeyX;
      dualAuthorY = dualAuthorkeyPair.publicKeyY;
      dualSecretKey = dualAuthorkeyPair.secretKey;
    }

    // Setup the transfer tx
    const accountFrom = this.findAccount(from);
    const fromAccountID = accountFrom.accountID;
    const transfer: Transfer = {
      txType: "Transfer",
      exchange: this.exchange.address,
      fromAccountID,
      toAccountID,
      tokenID,
      amount,
      feeTokenID,
      fee,
      maxFee,
      originalMaxFee: maxFee,
      from,
      to,
      type: authMethod === AuthMethod.EDDSA ? 0 : 1,
      validUntil,
      putAddressesInDA,
      dualAuthorX,
      dualAuthorY,
      payerToAccountID: useDualAuthoring ? 0 : toAccountID,
      payerTo: useDualAuthoring ? Constants.zeroAddress : to,
      payeeToAccountID: toAccountID,
      storageID,
      dualSecretKey,
      useAppKey
    };
    const appKeyPair: KeyPair = {
      publicKeyX: accountFrom.appKeyPublicKeyX,
      publicKeyY: accountFrom.appKeyPublicKeyY,
      secretKey: accountFrom.appKeySecretKey
    };
    // Authorize the tx
    if (authMethod === AuthMethod.EDDSA) {
      transfer.signature = TransferUtils.sign(useAppKey == 1 ? appKeyPair : accountFrom, transfer, true);
      // transfer.signature = TransferUtils.sign(accountFrom, transfer, true);
      if (useDualAuthoring) {
        const dualKeyPair = secretKnown ? dualAuthorkeyPair : this.getKeyPairEDDSA();
        transfer.dualSignature = TransferUtils.sign(dualKeyPair, transfer, false);
      }
    }

    if (authMethod !== AuthMethod.EDDSA) {
      // Set the max fee to the fee so that it can always pass through the circuit
      transfer.maxFee = transfer.fee;
    }

    this.pendingTransactions[this.exchangeId].push(transfer);

    return transfer;
  }

  public async setupRing(
    ring: SpotTrade,
    bSetupOrderA: boolean = true,
    bSetupOrderB: boolean = true,
    bDepositA: boolean = true,
    bDepositB: boolean = true,
    storageIDA?: number,
    storageIDB?: number
  ) {
    if (bSetupOrderA) {
      // await this.setupOrder(ring.orderA, this.storageIDGenerator++, bDepositA);
      storageIDA = storageIDA === undefined ? this.storageIDGenerator++ : storageIDA;
      await this.setupOrder(ring.orderA, storageIDA, bDepositA);
    }
    if (bSetupOrderB) {
      // await this.setupOrder(ring.orderB, this.storageIDGenerator++, bDepositB);
      storageIDB = storageIDB === undefined ? this.storageIDGenerator++ : storageIDB;
      await this.setupOrder(ring.orderB, storageIDB, bDepositB);
    }
    // TODO？
    ring.tokenID = ring.tokenID !== undefined ? ring.tokenID : await this.getTokenIdFromNameOrAddress("LRC");
    ring.fee = ring.fee ? ring.fee : new BN(web3.utils.toWei("1", "ether"));
  }

  public async setupBatchSpotTrade(batchSpotTrade: BatchSpotTrade) {
    for (var i = 0; i < batchSpotTrade.users.length; i++) {
      for (var j = 0; j < batchSpotTrade.users[i].orders.length; j++) {
        await this.setupOrder(
          batchSpotTrade.users[i].orders[j],
          batchSpotTrade.users[i].orders[j].storageID
            ? batchSpotTrade.users[i].orders[j].storageID
            : this.storageIDGenerator++,
          false
        );
      }
      batchSpotTrade.users[i].accountID = batchSpotTrade.users[i].orders[0].accountID;
    }
    // batchSpotTrade.tokens[0] = batchSpotTrade.users[0].orders[0].tokenIdS;
    // batchSpotTrade.tokens[1] = batchSpotTrade.users[0].orders[0].tokenIdB;
    // batchSpotTrade.tokens[2] = batchSpotTrade.users[0].orders[0].feeTokenID;

    // if (batchSpotTrade.bindTokenID == batchSpotTrade.tokens[0]) {
    //   batchSpotTrade.tokens[0] = batchSpotTrade.tokens[1];
    //   batchSpotTrade.tokens[1] = batchSpotTrade.tokens[2];
    //   batchSpotTrade.tokens[2] = batchSpotTrade.bindTokenID;
    // } else if (batchSpotTrade.bindTokenID == batchSpotTrade.tokens[1]) {
    //   batchSpotTrade.tokens[1] = batchSpotTrade.tokens[2];
    //   batchSpotTrade.tokens[2] = batchSpotTrade.bindTokenID;
    // }

    batchSpotTrade.tokenID =
      batchSpotTrade.tokenID !== undefined ? batchSpotTrade.tokenID : await this.getTokenIdFromNameOrAddress("LRC");
    batchSpotTrade.fee = batchSpotTrade.fee ? batchSpotTrade.fee : new BN(web3.utils.toWei("1", "ether"));

    batchSpotTrade.enableDataCheck = batchSpotTrade.enableDataCheck !== undefined ? batchSpotTrade.enableDataCheck : false;
    batchSpotTrade.userAFirstEstimateExchange = batchSpotTrade.userAFirstEstimateExchange !== undefined ? batchSpotTrade.userAFirstEstimateExchange : "0";
    batchSpotTrade.userASecondEstimateExchange = batchSpotTrade.userASecondEstimateExchange !== undefined ? batchSpotTrade.userASecondEstimateExchange : "0";
    batchSpotTrade.userAThirdEstimateExchange = batchSpotTrade.userAThirdEstimateExchange !== undefined ? batchSpotTrade.userAThirdEstimateExchange : "0";

    batchSpotTrade.userBFirstEstimateExchange = batchSpotTrade.userBFirstEstimateExchange !== undefined ? batchSpotTrade.userBFirstEstimateExchange : "0";
    batchSpotTrade.userBSecondEstimateExchange = batchSpotTrade.userBSecondEstimateExchange !== undefined ? batchSpotTrade.userBSecondEstimateExchange : "0";
    batchSpotTrade.userBThirdEstimateExchange = batchSpotTrade.userBThirdEstimateExchange !== undefined ? batchSpotTrade.userBThirdEstimateExchange : "0";

    batchSpotTrade.userCFirstEstimateExchange = batchSpotTrade.userCFirstEstimateExchange !== undefined ? batchSpotTrade.userCFirstEstimateExchange : "0";
    batchSpotTrade.userCSecondEstimateExchange = batchSpotTrade.userCSecondEstimateExchange !== undefined ? batchSpotTrade.userCSecondEstimateExchange : "0";
    batchSpotTrade.userCThirdEstimateExchange = batchSpotTrade.userCThirdEstimateExchange !== undefined ? batchSpotTrade.userCThirdEstimateExchange : "0";

    batchSpotTrade.userDFirstEstimateExchange = batchSpotTrade.userDFirstEstimateExchange !== undefined ? batchSpotTrade.userDFirstEstimateExchange : "0";
    batchSpotTrade.userDSecondEstimateExchange = batchSpotTrade.userDSecondEstimateExchange !== undefined ? batchSpotTrade.userDSecondEstimateExchange : "0";
    batchSpotTrade.userDThirdEstimateExchange = batchSpotTrade.userDThirdEstimateExchange !== undefined ? batchSpotTrade.userDThirdEstimateExchange : "0";

    batchSpotTrade.userEFirstEstimateExchange = batchSpotTrade.userEFirstEstimateExchange !== undefined ? batchSpotTrade.userEFirstEstimateExchange : "0";
    batchSpotTrade.userESecondEstimateExchange = batchSpotTrade.userESecondEstimateExchange !== undefined ? batchSpotTrade.userESecondEstimateExchange : "0";
    batchSpotTrade.userEThirdEstimateExchange = batchSpotTrade.userEThirdEstimateExchange !== undefined ? batchSpotTrade.userEThirdEstimateExchange : "0";

    batchSpotTrade.userFFirstEstimateExchange = batchSpotTrade.userFFirstEstimateExchange !== undefined ? batchSpotTrade.userFFirstEstimateExchange : "0";
    batchSpotTrade.userFSecondEstimateExchange = batchSpotTrade.userFSecondEstimateExchange !== undefined ? batchSpotTrade.userFSecondEstimateExchange : "0";
    batchSpotTrade.userFThirdEstimateExchange = batchSpotTrade.userFThirdEstimateExchange !== undefined ? batchSpotTrade.userFThirdEstimateExchange : "0";

    batchSpotTrade.operatorFirstEstimateExchange = batchSpotTrade.operatorFirstEstimateExchange !== undefined ? batchSpotTrade.operatorFirstEstimateExchange : "0";
    batchSpotTrade.operatorSecondEstimateExchange = batchSpotTrade.operatorSecondEstimateExchange !== undefined ? batchSpotTrade.operatorSecondEstimateExchange : "0";
    batchSpotTrade.operatorThirdEstimateExchange = batchSpotTrade.operatorThirdEstimateExchange !== undefined ? batchSpotTrade.operatorThirdEstimateExchange : "0";
    
  }

  //order
  public async setupOrder(order: OrderInfo, index: number, bDeposit: boolean = true) {
    if (
      (order.type == 6 || order.type == 7) &&
      order.startOrder !== undefined &&
      order.isNextOrder &&
      order.appointedStorageID !== undefined
    ) {
      index = order.appointedStorageID;
    }
    if (order.owner === undefined) {
      const accountIndex = index % this.testContext.orderOwners.length;
      order.owner = this.testContext.orderOwners[accountIndex];
    } else if (order.owner !== undefined && !order.owner.startsWith("0x")) {
      const accountIndex = parseInt(order.owner, 10);
      assert(accountIndex >= 0 && accountIndex < this.testContext.orderOwners.length, "Invalid owner index");
      order.owner = this.testContext.orderOwners[accountIndex];
    }
    if (!order.tokenS.startsWith("0x")) {
      order.tokenS = this.testContext.tokenSymbolAddrMap.get(order.tokenS);
    }
    if (!order.tokenB.startsWith("0x")) {
      order.tokenB = this.testContext.tokenSymbolAddrMap.get(order.tokenB);
    }
    if (!order.validUntil) {
      // Set the order validUntil time to a bit after the current timestamp;
      const blockNumber = await web3.eth.getBlockNumber();
      order.validUntil = (await web3.eth.getBlock(blockNumber)).timestamp + 3600;
    }

    order.exchange = order.exchange !== undefined ? order.exchange : this.exchange.address;

    order.fillAmountBorS = order.fillAmountBorS !== undefined ? order.fillAmountBorS : true;

    order.taker = order.taker !== undefined ? order.taker : Constants.zeroAddress;
    //TradingFeGasFee-TradingFeProtocolFeeBips
    // order.maxFeeBips = order.maxFeeBips !== undefined ? order.maxFeeBips : 30;

    // order.feeBips = order.feeBips !== undefined ? order.feeBips : order.maxFeeBips;
    //protocolTakerFeeBipprotocolMakerFeeBips
    order.feeBips = order.feeBips !== undefined ? order.feeBips : 0;
    order.tradingFee = order.tradingFee !== undefined ? order.tradingFee : new BN(0);

    order.storageID = order.storageID !== undefined ? order.storageID : index;

    order.tokenIdS = this.tokenAddressToIDMap.get(order.tokenS);
    order.tokenIdB = this.tokenAddressToIDMap.get(order.tokenB);
    console.log("===================order.tokenIdS:" + order.tokenIdS);
    console.log("===================order.tokenIdB:" + order.tokenIdB);
    //trading fegas fee
    //fee
    order.fee = order.fee !== undefined ? order.fee : new BN(0);
    //maxFee
    order.maxFee = order.maxFee !== undefined ? order.maxFee : new BN(0);
    //feeTokenId
    order.feeTokenID =
      order.feeTokenID !== undefined ? order.feeTokenID : this.tokenAddressToIDMap.get(Constants.zeroAddress);

    // //refeUrefe
    // //
    // order.uiReferID = order.uiReferID !== undefined ? order.uiReferID : 1;
    // order.accountReferID = order.accountReferID !== undefined ? order.accountReferID : 1;

    // order.autoMarketID = order.autoMarketID !== undefined ? order.autoMarketID : 0;
    order.type = order.type !== undefined ? order.type : 0;
    order.level = order.level !== undefined ? order.level : 0;
    order.gridOffset = order.gridOffset !== undefined ? order.gridOffset : new BN(0);
    order.orderOffset = order.orderOffset !== undefined ? order.orderOffset : new BN(0);
    order.maxLevel = order.maxLevel !== undefined ? order.maxLevel : 0;

    order.deltaFilledB = order.deltaFilledB !== undefined ? order.deltaFilledB : new BN(0);
    order.deltaFilledS = order.deltaFilledS !== undefined ? order.deltaFilledS : new BN(0);
    order.useAppKey = order.useAppKey !== undefined ? order.useAppKey : 0;

    // if (order.stableToken !== undefined && !order.stableToken.startsWith("0x")) {
    //   //ETHToke
    //   order.stableToken = this.testContext.tokenSymbolAddrMap.get("ETH");
    // }
    if (bDeposit) {
      // setup initial balances:
      await this.setOrderBalances(order);
    } else {
      order.accountID = this.findAccount(order.owner).accountID;
    }

    if (order.startOrder === undefined && (order.type == 6 || order.type == 7) && order.level == 0) {
      //orde,autoMarketStartOrder
      const startOrder: AutoMarketStartOrderInfo = {
        exchange: order.exchange,
        storageID: order.storageID,
        accountID: order.accountID,
        tokenIdS: order.tokenIdS,
        tokenIdB: order.tokenIdB,
        amountS: order.amountS,
        amountB: order.amountB,
        validUntil: order.validUntil,
        fillAmountBorS: order.fillAmountBorS,
        taker: order.taker,
        feeTokenID: order.feeTokenID,
        feeBips: order.feeBips,
        tradingFee: order.tradingFee,
        maxFee: order.maxFee,
        // uiReferID: order.uiReferID,
        type: order.type,
        gridOffset: order.gridOffset,
        orderOffset: order.orderOffset,
        maxLevel: order.maxLevel,
        useAppKey: order.useAppKey
        // autoMarketID: order.autoMarketID
      };
      console.log("AutoMarket-====++++++, type:" + startOrder.type);
      order.startOrder = startOrder;
    }
    // AutoMarke
    if ((order.type == 6 || order.type == 7) && order.startOrder.signature !== undefined) {
      order.signature = order.startOrder.signature;
    } else {
      // Sign the order
      this.signOrder(order);
    }
  }

  public signOrder(order: OrderInfo) {
    if (order.signature !== undefined) {
      return;
    }
    const account = this.accounts[this.exchangeId][order.accountID];

    //trading fegas fee
    // Calculate hash
    // const hasher = Poseidon.createHash(14, 6, 53);
    //refeUrefe
    //TradingFeGasFee-TradingFeProtocolFeeBips
    // const hasher = Poseidon.createHash(15, 6, 53);
    // const hasher = Poseidon.createHash(14, 6, 53);
    // const hasher = Poseidon.createHash(15, 6, 53);
    // const hasher = Poseidon.createHash(17, 6, 53);
    const hasher = Poseidon.createHash(18, 6, 53);
    const inputs = [
      order.exchange,
      order.storageID,
      order.accountID,
      order.tokenIdS,
      order.tokenIdB,
      order.amountS,
      order.amountB,
      order.validUntil,
      //TradingFeGasFee-TradingFeProtocolFeeBips
      // order.maxFeeBips,
      order.fillAmountBorS ? 1 : 0,
      order.taker,
      //trading fegas fee
      order.feeTokenID,
      order.maxFee, //maxFee
      //refeUrefe
      // order.uiReferID,
      order.type,
      order.gridOffset,
      order.orderOffset,
      order.maxLevel,
      order.useAppKey
      // order.autoMarketID
    ];
    order.hash = hasher(inputs).toString(10);

    const appKeyPair: KeyPair = {
      publicKeyX: account.appKeyPublicKeyX,
      publicKeyY: account.appKeyPublicKeyY,
      secretKey: account.appKeySecretKey
    };

    const pair = order.useAppKey == 1 ? appKeyPair : account;

    // Create signature
    order.signature = EdDSA.sign(pair.secretKey, order.hash);

    // Verify signature
    const success = EdDSA.verify(order.hash, order.signature, [pair.publicKeyX, pair.publicKeyY]);
    if ((order.type == 6 || order.type == 7) && order.level == 0) {
      order.startOrder.signature = order.signature;
    }
    assert(success, "Failed to verify signature");
  }

  public signRingBlock(block: any, publicDataInput: any) {
    if (block.signature !== undefined) {
      return;
    }

    const hasher = Poseidon.createHash(3, 6, 51);
    const account = this.accounts[this.exchangeId][block.operatorAccountID];

    // Calculate hash
    const inputs = [new BN(publicDataInput, 10), account.nonce++];
    const hash = hasher(inputs).toString(10);

    // Create signature
    block.signature = EdDSA.sign(account.secretKey, hash);

    // Verify signature
    const success = EdDSA.verify(hash, block.signature, [account.publicKeyX, account.publicKeyY]);
    assert(success, "Failed to verify signature");
  }

  public async setOrderBalances(order: OrderInfo) {
    const balanceS = order.balanceS !== undefined ? order.balanceS : order.amountS;
    const deposit = await this.deposit(order.owner, order.owner, order.tokenS, balanceS);
    order.accountID = deposit.accountID;

    const balanceB = order.balanceB !== undefined ? order.balanceB : new BN(0);
    if (balanceB.gt(new BN(0)) || order.accountID === undefined) {
      const deposit = await this.deposit(order.owner, order.owner, order.tokenB, balanceB);
      order.accountID = deposit.accountID;
    }
  }

  public reserveStorageID() {
    return this.storageIDGenerator++;
  }

  public reserveMultiStorageID(reserve: number) {
    this.storageIDGenerator = this.storageIDGenerator + reserve;
  }

  public getAddressBook(ring: SpotTrade, index?: number, addressBook: { [id: number]: string } = {}) {
    const addAccount = (addrBook: { [id: string]: any }, accountID: number, name: string) => {
      addrBook[accountID] = (addrBook[accountID] ? addrBook[accountID] + "=" : "") + name;
    };
    const bIndex = index !== undefined;
    addAccount(addressBook, 0, "ProtocolFeePool");
    addAccount(addressBook, ring.orderA.accountID, "OwnerA" + (bIndex ? "[" + index + "]" : ""));
    addAccount(addressBook, ring.orderB.accountID, "OwnerB" + (bIndex ? "[" + index + "]" : ""));
    return addressBook;
  }

  public getAddressBookBlock(block: TxBlock) {
    const addAccount = (addrBook: { [id: string]: any }, accountID: number, name: string) => {
      addrBook[accountID] = (addrBook[accountID] ? addrBook[accountID] + "=" : "") + name;
    };

    let addressBook: { [id: number]: string } = {};
    let index = 0;
    for (const tx of block.transactions) {
      if (tx.txType === "SpotTrade") {
        addressBook = this.getAddressBook(tx, index++, addressBook);
      }
    }
    addAccount(addressBook, block.operatorAccountID, "Operator");
    return addressBook;
  }

  public getKeyPairEDDSA() {
    return EdDSA.getKeyPair();
  }

  public getZeroKeyPairEDDSA() {
    return {
      publicKeyX: "0",
      publicKeyY: "0",
      secretKey: "0"
    };
  }

  public flattenList = (l: any[]) => {
    return [].concat.apply([], l);
  };

  public flattenVK = (vk: any) => {
    return [
      this.flattenList([
        vk.alpha[0],
        vk.alpha[1],
        this.flattenList(vk.beta),
        this.flattenList(vk.gamma),
        this.flattenList(vk.delta)
      ]),
      this.flattenList(vk.gammaABC)
    ];
  };

  public flattenProof = (proof: any) => {
    return this.flattenList([proof.A, this.flattenList(proof.B), proof.C]);
  };

  public async deposit(
    from: string,
    to: string,
    token: string,
    amount: BN,
    options: DepositOptions = {},
    type?: number
  ) {
    // Fill in defaults
    const autoSetKeys = options.autoSetKeys !== undefined ? options.autoSetKeys : true;
    const contract = options.accountContract !== undefined ? options.accountContract : this.exchange;
    const amountDepositedCanDiffer =
      options.amountDepositedCanDiffer !== undefined ? options.amountDepositedCanDiffer : this.exchange;

    type = type !== undefined ? type : 0;

    //console.log("token:" + token);
    //console.log("amount:" + amount.toString(10));

    if (!token.startsWith("0x")) {
      token = this.testContext.tokenSymbolAddrMap.get(token);
    }
    const tokenID = await this.getTokenID(token);

    const caller = options.accountContract ? this.testContext.orderOwners[0] : from;

    let accountID = await this.getAccountID(to);
    let accountNewCreated = false;
    if (accountID === undefined) {
      const account: Account = {
        accountID: this.accounts[this.exchangeId].length,
        owner: to,
        publicKeyX: "0",
        publicKeyY: "0",
        appKeyPublicKeyX: "0",
        appKeyPublicKeyY: "0",
        secretKey: "0",
        appKeySecretKey: "0",
        nonce: 0
      };
      this.accounts[this.exchangeId].push(account);
      accountID = account.accountID;

      accountNewCreated = true;
    }

    let ethToSend = new BN(0);
    if (amount.gt(0)) {
      if (token !== Constants.zeroAddress) {
        const Token = this.testContext.tokenAddrInstanceMap.get(token);
        await Token.setBalance(from, amount);
        await Token.approve(this.depositContract.address, amount, { from });
      } else {
        ethToSend = ethToSend.add(web3.utils.toBN(amount));
      }
    }

    const callerEthBalanceBefore = await this.getOnchainBalance(from, Constants.zeroAddress);

    const tx = await contract.deposit(from, to, token, web3.utils.toBN(amount), web3.utils.hexToBytes("0x"), {
      from: caller,
      value: ethToSend,
      gasPrice: 0
    });
    const ethBlock = await web3.eth.getBlock(tx.receipt.blockNumber);
    logInfo("\x1b[46m%s\x1b[0m", "[Deposit] Gas used: " + tx.receipt.gasUsed);

    // Check if the correct fee amount was paid
    const callerEthBalanceAfter = await this.getOnchainBalance(from, Constants.zeroAddress);
    assert(
      callerEthBalanceAfter.eq(callerEthBalanceBefore.sub(ethToSend)),
      "fee paid by the depositer needs to match exactly with the fee needed"
    );

    const event = await this.assertEventEmitted(this.exchange, "DepositRequested");
    if (amountDepositedCanDiffer) {
      amount = event.amount;
    }

    const deposit = await this.requestDeposit(to, token, amount, ethBlock.timestamp, tx.receipt.transactionHash, type);

    if (accountNewCreated && autoSetKeys) {
      let keyPair = this.getKeyPairEDDSA();
      await this.requestAccountUpdate(to, token, new BN(0), keyPair, {
        authMethod: AuthMethod.ECDSA
      });
    }

    return deposit;
  }

  public async depositByTransfer(
    from: string,
    to: string,
    token: string,
    amount: BN,
    options: DepositOptions = {},
    type?: number
  ) {
    // Fill in defaults
    const autoSetKeys = options.autoSetKeys !== undefined ? options.autoSetKeys : true;
    const contract = options.accountContract !== undefined ? options.accountContract : this.exchange;
    const amountDepositedCanDiffer =
      options.amountDepositedCanDiffer !== undefined ? options.amountDepositedCanDiffer : this.exchange;

    type = type !== undefined ? type : 1;

    //console.log("token:" + token);
    //console.log("amount:" + amount.toString(10));

    if (!token.startsWith("0x")) {
      token = this.testContext.tokenSymbolAddrMap.get(token);
    }
    const tokenID = await this.getTokenID(token);
    console.log("depositByTransfer, tokenID", tokenID);

    const caller = options.accountContract ? this.testContext.orderOwners[0] : from;

    let accountID = await this.getAccountID(to);
    let accountNewCreated = false;
    if (accountID === undefined) {
      const account: Account = {
        accountID: this.accounts[this.exchangeId].length,
        owner: to,
        publicKeyX: "0",
        publicKeyY: "0",
        appKeyPublicKeyX: "0",
        appKeyPublicKeyY: "0",
        secretKey: "0",
        appKeySecretKey: "0",
        nonce: 0
      };
      this.accounts[this.exchangeId].push(account);
      accountID = account.accountID;

      accountNewCreated = true;
    }

    let ethToSend = new BN(0);
    if (amount.gt(0)) {
      if (token !== Constants.zeroAddress) {
        const Token = this.testContext.tokenAddrInstanceMap.get(token);
        await Token.setBalance(from, amount);
        // await Token.approve(this.depositContract.address, amount, { from });
      } else {
        ethToSend = ethToSend.add(web3.utils.toBN(amount));
      }
    }

    let txReceipt;

    if (token !== Constants.zeroAddress) {
      const Token = this.testContext.tokenAddrInstanceMap.get(token);
      let tx = await Token.transfer(this.depositContract.address, amount, { from });
      txReceipt = tx.receipt;
    } else {
      txReceipt = await web3.eth.sendTransaction({
        from: from,
        to: this.depositContract.address,
        value: ethToSend
      });
    }
    console.log("txReceipt", txReceipt);

    const ethBlock = await web3.eth.getBlock(txReceipt.blockNumber);
    logInfo("\x1b[46m%s\x1b[0m", "[Deposit by transfer] Gas used: " + txReceipt.gasUsed);

    const deposit = await this.requestDeposit(to, token, amount, ethBlock.timestamp, txReceipt.transactionHash, type);

    if (accountNewCreated && autoSetKeys) {
      let keyPair = this.getKeyPairEDDSA();
      await this.requestAccountUpdate(to, token, new BN(0), keyPair, {
        authMethod: AuthMethod.ECDSA
      });
    }

    return deposit;
  }

  public async requestDeposit(
    owner: string,
    token: string,
    amount: BN,
    timestamp?: number,
    transactionHash?: string,
    type?: number
  ) {
    const accountID = await this.getAccountID(owner);
    const deposit: Deposit = {
      txType: "Deposit",
      owner,
      accountID,
      tokenID: this.getTokenIdFromNameOrAddress(token),
      amount,
      token,
      timestamp,
      transactionHash,
      type
    };
    this.pendingTransactions[this.exchangeId].push(deposit);
    return deposit;
  }

  public hexToDecString(hex: string) {
    return new BN(hex.slice(2), 16).toString(10);
  }

  public async requestWithdrawal(
    owner: string,
    token: string,
    amount: BN,
    feeToken: string,
    fee: BN,
    options: WithdrawOptions = {},
    useAppKey?: number
  ) {
    // Fill in defaults
    const authMethod = options.authMethod !== undefined ? options.authMethod : AuthMethod.EDDSA;
    const to = options.to !== undefined ? options.to : owner;
    const minGas = options.minGas !== undefined ? options.minGas : 0;
    const gas = options.gas !== undefined ? options.gas : minGas > 0 ? minGas : 100000;
    const signer = options.signer !== undefined ? options.signer : owner;
    // const extraData = options.extraData !== undefined ? options.extraData : "0x";
    const validUntil = options.validUntil !== undefined ? options.validUntil : 0xffffffff;
    const maxFee = options.maxFee !== undefined ? options.maxFee : fee;
    let storageID = options.storageID !== undefined ? options.storageID : this.storageIDGenerator++;
    let storeRecipient = options.storeRecipient !== undefined ? options.storeRecipient : false;
    let skipForcedAuthentication =
      options.skipForcedAuthentication !== undefined ? options.skipForcedAuthentication : false;
    useAppKey = useAppKey !== undefined ? useAppKey : 0;

    let type = 1;
    if (authMethod === AuthMethod.EDDSA) {
      type = 0;
    }
    if (authMethod === AuthMethod.FORCE) {
      if (signer === owner) {
        type = 2;
      } else {
        type = 3;
        amount = new BN(0);
      }
    }

    if (!token.startsWith("0x")) {
      token = this.testContext.tokenSymbolAddrMap.get(token);
    }
    const tokenID = this.tokenAddressToIDMap.get(token);
    if (!feeToken.startsWith("0x")) {
      feeToken = this.testContext.tokenSymbolAddrMap.get(feeToken);
    }

    let accountID = this.getAccountID(owner);
    if (authMethod === AuthMethod.FORCE && !skipForcedAuthentication) {
      const withdrawalFee = await this.loopringV3.forcedWithdrawalFee();
      if (owner != Constants.zeroAddress) {
        const numAvailableSlotsBefore = (await this.exchange.getNumAvailableForcedSlots()).toNumber();
        await this.exchange.forceWithdraw(signer, token, accountID, {
          from: signer,
          value: withdrawalFee
        });
        const numAvailableSlotsAfter = (await this.exchange.getNumAvailableForcedSlots()).toNumber();
        assert.equal(numAvailableSlotsAfter, numAvailableSlotsBefore - 1, "available slots should have decreased by 1");
      } else {
        accountID = 0;
        await this.exchange.withdrawProtocolFees(token, {
          value: withdrawalFee
        });
      }
      //withdrawalRequest.timestamp = ethBlock.timestamp;
      //withdrawalRequest.transactionHash = tx.receipt.transactionHash;
    }

    // Calculate the data hash
    const onchainData = new Bitstream();
    onchainData.addNumber(minGas, 31);
    onchainData.addAddress(to);
    onchainData.addBN(amount, 31);
    // onchainData.addHex(extraData);
    // const onchainDataHash =
    //   "0x" +
    //   ethUtil
    //     .keccak(Buffer.from(onchainData.getData().slice(2), "hex"))
    //     .toString("hex")
    //     .slice(0, 40);
    console.log("minGas:" + minGas);
    console.log("to:" + to);
    console.log("amount:" + amount);
    // console.log("extraData:" + extraData);
    const onchainDataHash =
      "0x" +
      SHA256(Buffer.from(onchainData.getData().slice(2), "hex"))
        .toString("hex")
        .slice(0, 40);

    console.log("onchainDataHash:" + onchainDataHash);

    const account = this.accounts[this.exchangeId][accountID];
    const feeTokenID = this.tokenAddressToIDMap.get(feeToken);
    const withdrawalRequest: WithdrawalRequest = {
      txType: "Withdraw",
      exchange: this.exchange.address,
      type,
      owner,
      accountID,
      storageID,
      validUntil,
      tokenID,
      amount,
      feeTokenID,
      fee,
      maxFee,
      originalMaxFee: maxFee,
      to,
      storeRecipient,
      // extraData,
      withdrawalFee: await this.loopringV3.forcedWithdrawalFee(),
      minGas,
      gas,
      onchainDataHash,
      useAppKey
    };
    const appKeyPair: KeyPair = {
      publicKeyX: account.appKeyPublicKeyX,
      publicKeyY: account.appKeyPublicKeyY,
      secretKey: account.appKeySecretKey
    };

    if (authMethod === AuthMethod.EDDSA) {
      WithdrawalUtils.sign(useAppKey == 1 ? appKeyPair : account, withdrawalRequest);
    } else if (authMethod === AuthMethod.ECDSA) {
      const hash = WithdrawalUtils.getHash(withdrawalRequest, this.exchange.address);
      withdrawalRequest.onchainSignature = await sign(owner, hash, SignatureType.EIP_712);
      await verifySignature(owner, hash, withdrawalRequest.onchainSignature);
    } else if (authMethod === AuthMethod.APPROVE) {
      const hash = WithdrawalUtils.getHash(withdrawalRequest, this.exchange.address);
      await this.exchange.approveTransaction(owner, hash, { from: owner });
    }

    if (authMethod !== AuthMethod.EDDSA) {
      // Set the max fee to the fee so that it can always pass through the circuit
      withdrawalRequest.maxFee = withdrawalRequest.fee;
    }

    this.pendingTransactions[this.exchangeId].push(withdrawalRequest);
    return withdrawalRequest;
  }

  public async requestAccountUpdate(
    owner: string,
    feeToken: string,
    fee: BN,
    keyPair: any,
    options: AccountUpdateOptions = {}
    // //refe-refe
    // referID?: number,
  ) {
    // referID = referID == undefined ? 1 : referID;
    fee = roundToFloatValue(fee, Constants.Float16Encoding);

    // Fill in defaults
    const authMethod = options.authMethod !== undefined ? options.authMethod : AuthMethod.EDDSA;
    const validUntil = options.validUntil !== undefined ? options.validUntil : 0xffffffff;
    const maxFee = options.maxFee !== undefined ? options.maxFee : fee;

    // Type
    let type = 0;
    if (authMethod !== AuthMethod.EDDSA) {
      type = 1;
    }

    if (!feeToken.startsWith("0x")) {
      feeToken = this.testContext.tokenSymbolAddrMap.get(feeToken);
    }
    const feeTokenID = this.tokenAddressToIDMap.get(feeToken);

    let isNewAccount = false;
    let account = this.findAccount(owner);
    if (account === undefined) {
      account = {
        accountID: this.accounts[this.exchangeId].length,
        owner: owner,
        publicKeyX: "0",
        publicKeyY: "0",
        appKeyPublicKeyX: "0",
        appKeyPublicKeyY: "0",
        secretKey: "0",
        appKeySecretKey: "0",
        nonce: 0
        // //refe-refe
        // referID: 1
      };
      this.accounts[this.exchangeId].push(account);
      isNewAccount = true;
    }

    const accountUpdate: AccountUpdate = {
      txType: "AccountUpdate",
      exchange: this.exchange.address,
      type,
      owner,
      accountID: account.accountID,
      nonce: account.nonce++,
      validUntil,
      publicKeyX: keyPair.publicKeyX,
      publicKeyY: keyPair.publicKeyY,
      feeTokenID,
      fee,
      maxFee,
      originalMaxFee: maxFee
      // //refe-refe
      // referID: referID
    };

    // Sign the public key update
    if (authMethod === AuthMethod.EDDSA) {
      // New accounts should not be able to set keys with EDDSA.
      // Try to sign with the keys we're setting (which shouldn't work).
      accountUpdate.signature = AccountUpdateUtils.sign(isNewAccount ? keyPair : account, accountUpdate);
    } else if (authMethod === AuthMethod.ECDSA) {
      const tempAccountID = accountUpdate.accountID;
      if (accountUpdate.nonce == 0) {
        accountUpdate.accountID = 0;
      }
      const hash = AccountUpdateUtils.getHash(accountUpdate, this.exchange.address);
      accountUpdate.accountID = tempAccountID;
      accountUpdate.onchainSignature = await sign(owner, hash, SignatureType.EIP_712);
      console.log("accountUpdate: ", accountUpdate);
      let acPublicKey = new BN(EdDSA.pack(accountUpdate.publicKeyX, accountUpdate.publicKeyY), 16);
      console.log("accountUpdate.publicKey: ", acPublicKey.toString(10));

      console.log(
        "onchainSignature sign: " + owner + "," + hash.toString("hex") + "," + accountUpdate.onchainSignature
      );
      await verifySignature(owner, hash, accountUpdate.onchainSignature);
    } else if (authMethod === AuthMethod.APPROVE) {
      const hash = AccountUpdateUtils.getHash(accountUpdate, this.exchange.address);
      await this.exchange.approveTransaction(owner, hash, { from: owner });
    }

    if (authMethod !== AuthMethod.EDDSA) {
      // Set the max fee to the fee so that it can always pass through the circuit
      accountUpdate.maxFee = accountUpdate.fee;
    }

    this.pendingTransactions[this.exchangeId].push(accountUpdate);

    // Update local account state
    account.publicKeyX = keyPair.publicKeyX;
    account.publicKeyY = keyPair.publicKeyY;
    account.secretKey = keyPair.secretKey;

    return accountUpdate;
  }
  public async requestAppKeyUpdate(
    owner: string,
    feeToken: string,
    fee: BN,
    keyPair: any,
    disableAppKeySpotTrade: number,
    disableAppKeyWithdraw: number,
    disableAppKeyTransferToOther: number,
    options: AccountUpdateOptions = {}
    // //refe-refe
    // referID?: number,
  ) {
    // referID = referID == undefined ? 1 : referID;
    fee = roundToFloatValue(fee, Constants.Float16Encoding);

    // Fill in defaults
    const validUntil = options.validUntil !== undefined ? options.validUntil : 0xffffffff;
    const maxFee = options.maxFee !== undefined ? options.maxFee : fee;

    if (!feeToken.startsWith("0x")) {
      feeToken = this.testContext.tokenSymbolAddrMap.get(feeToken);
    }
    const feeTokenID = this.tokenAddressToIDMap.get(feeToken);

    let account = this.findAccount(owner);
    if (account === undefined) {
      console.log("account not exist");
      return;
    }

    const appKeyUpdate: AppKeyUpdate = {
      txType: "AppKeyUpdate",
      exchange: this.exchange.address,
      // owner,
      accountID: account.accountID,
      nonce: account.nonce++,
      validUntil,
      appKeyPublicKeyX: keyPair.publicKeyX,
      appKeyPublicKeyY: keyPair.publicKeyY,
      feeTokenID,
      fee,
      maxFee,
      originalMaxFee: maxFee,
      disableAppKeySpotTrade: disableAppKeySpotTrade,
      disableAppKeyWithdraw: disableAppKeyWithdraw,
      disableAppKeyTransferToOther: disableAppKeyTransferToOther
    };

    // Sign the public key update
    //accounKeEDDS
    console.log("==========account public KeyX:" + account.publicKeyX);
    console.log("==========account public KeyY:" + account.publicKeyY);
    appKeyUpdate.signature = this.appKeyUpdateSign(account, appKeyUpdate);

    // Set the max fee to the fee so that it can always pass through the circuit
    appKeyUpdate.maxFee = appKeyUpdate.fee;
    this.pendingTransactions[this.exchangeId].push(appKeyUpdate);

    // Update local account state
    account.appKeyPublicKeyX = keyPair.publicKeyX;
    account.appKeyPublicKeyY = keyPair.publicKeyY;
    account.appKeySecretKey = keyPair.secretKey;

    return appKeyUpdate;
  }

  public appKeyUpdateSign(keyPair: any, update: AppKeyUpdate) {
    // Calculate hash
    const hasher = Poseidon.createHash(12, 6, 53);
    const inputs = [
      update.exchange,
      update.accountID,
      update.feeTokenID,
      update.maxFee,
      update.appKeyPublicKeyX,
      update.appKeyPublicKeyY,
      update.validUntil,
      update.nonce,
      update.disableAppKeySpotTrade,
      update.disableAppKeyWithdraw,
      update.disableAppKeyTransferToOther
    ];
    const hash = hasher(inputs).toString(10);

    // Create signature
    const signature = EdDSA.sign(keyPair.secretKey, hash);

    // Verify signature
    const success = EdDSA.verify(hash, signature, [keyPair.publicKeyX, keyPair.publicKeyY]);
    assert(success, "Failed to verify signature");

    return signature;
  }

  public async requestOrderCancel(
    owner: string,
    storageID: number,
    fee: BN,
    maxFee: BN,
    feeToken: string,
    useAppKey?: number
  ) {
    // if (!token.startsWith("0x")) {
    //   token = this.testContext.tokenSymbolAddrMap.get(token);
    // }
    if (!feeToken.startsWith("0x")) {
      feeToken = this.testContext.tokenSymbolAddrMap.get(feeToken);
    }
    // const tokenID = this.tokenAddressToIDMap.get(token);
    const feeTokenID = this.tokenAddressToIDMap.get(feeToken);
    const account = this.findAccount(owner);
    useAppKey = useAppKey !== undefined ? useAppKey : 0;
    const orderCancel: OrderCancel = {
      txType: "OrderCancel",
      exchange: this.exchange.address,
      // owner: owner,
      accountID: account.accountID,
      // tokenID: tokenID,
      storageID: storageID,
      fee: fee,
      maxFee: maxFee,
      feeTokenID: feeTokenID,
      useAppKey: useAppKey
    };
    console.log(
      "feeToken:" +
        feeToken +
        ";" +
        feeTokenID +
        ";exchange:" +
        orderCancel.exchange +
        ";" +
        orderCancel.accountID +
        ";" +
        orderCancel.storageID +
        ";" +
        orderCancel.maxFee +
        ";" +
        orderCancel.feeTokenID
    );
    const hasher = Poseidon.createHash(7, 6, 52);
    // const hasher = Poseidon.createHash(8, 6, 53);
    const inputs = [
      orderCancel.exchange,
      // orderCancel.owner,
      orderCancel.accountID,
      // orderCancel.tokenID,
      orderCancel.storageID,
      orderCancel.maxFee,
      orderCancel.feeTokenID,
      orderCancel.useAppKey
    ];
    orderCancel.hash = hasher(inputs).toString(10);

    // Create signature
    orderCancel.signature = EdDSA.sign(useAppKey == 1 ? account.appKeySecretKey : account.secretKey, orderCancel.hash);

    // Verify signature
    const publicKeyX = useAppKey == 1 ? account.appKeyPublicKeyX : account.publicKeyX;
    const publicKeyY = useAppKey == 1 ? account.appKeyPublicKeyY : account.publicKeyY;
    const success = EdDSA.verify(orderCancel.hash, orderCancel.signature, [publicKeyX, publicKeyY]);

    console.log("hash===========++++++++++++:" + orderCancel.hash);
    console.log("storageID===========++++++++++++:" + storageID);
    console.log("hash===========++++++++++++ account.publicKeyX" + account.publicKeyX);
    console.log("hash===========++++++++++++ account.publicKeyY" + account.publicKeyY);
    console.log("hash===========++++++++++++ orderCancel.signature" + orderCancel.signature);
    assert(success, "in requestOrderCancel Failed to verify signature");
    this.pendingTransactions[this.exchangeId].push(orderCancel);
    return orderCancel;
  }

  // public async requestAutoMarketUpdate(
  //   owner: string,
  //   autoMarketID: string,
  //   tokenS: string,
  //   tokenB: string,
  //   maxLevel: number,
  //   gridOffset: BN,
  //   orderOffset: BN,
  //   cancelled: number,
  //   validUntil: number,
  //   fee: BN,
  //   maxFee: BN,
  //   feeToken: string
  //   ) {
  //   if (!tokenS.startsWith("0x")) {
  //     tokenS = this.testContext.tokenSymbolAddrMap.get(tokenS);
  //   }
  //   if (!tokenB.startsWith("0x")) {
  //     tokenB = this.testContext.tokenSymbolAddrMap.get(tokenB);
  //   }
  //   if (!feeToken.startsWith("0x")) {
  //     feeToken = this.testContext.tokenSymbolAddrMap.get(feeToken);
  //   }
  //   const account = this.findAccount(owner);
  //   validUntil = validUntil !== undefined ? validUntil : 0xffffffff;
  //   const autoMarketUpdate: AutoMarketUpdate= {
  //     txType: "AutoMarketUpdate",
  //     exchange: this.exchange.address,
  //     accountID: account.accountID,
  //     autoMarketID: autoMarketID,
  //     tokenSID: this.tokenAddressToIDMap.get(tokenS),
  //     tokenBID: this.tokenAddressToIDMap.get(tokenB),
  //     maxLevel: maxLevel,
  //     gridOffset: gridOffset,
  //     orderOffset: orderOffset,
  //     cancelled: cancelled,
  //     validUntil: validUntil,
  //     fee: fee,
  //     maxFee: maxFee,
  //     feeTokenID: this.tokenAddressToIDMap.get(feeToken)
  //   }

  //   const hasher = Poseidon.createHash(13, 6, 53);
  //   // const hasher = Poseidon.createHash(5, 6, 52);
  //   const inputs = [
  //     autoMarketUpdate.exchange,
  //     autoMarketUpdate.accountID,
  //     autoMarketUpdate.autoMarketID,
  //     autoMarketUpdate.tokenSID,
  //     autoMarketUpdate.tokenBID,
  //     autoMarketUpdate.maxLevel,
  //     autoMarketUpdate.gridOffset,
  //     autoMarketUpdate.orderOffset,
  //     autoMarketUpdate.cancelled,
  //     autoMarketUpdate.validUntil,
  //     autoMarketUpdate.maxFee,
  //     autoMarketUpdate.feeTokenID
  //   ];
  //   console.log("AutoMarketUpdate-====, exchange:" + autoMarketUpdate.exchange);
  //   console.log("AutoMarketUpdate-====, accountID:" + autoMarketUpdate.accountID);
  //   console.log("AutoMarketUpdate-====, autoMarketID:" + autoMarketUpdate.autoMarketID);
  //   console.log("AutoMarketUpdate-====, tokenSID:" + autoMarketUpdate.tokenSID);
  //   console.log("AutoMarketUpdate-====, tokenBID:" + autoMarketUpdate.tokenBID);
  //   console.log("AutoMarketUpdate-====, maxLevel:" + autoMarketUpdate.maxLevel);
  //   console.log("AutoMarketUpdate-====, gridOffset:" + autoMarketUpdate.gridOffset);
  //   console.log("AutoMarketUpdate-====, orderOffset:" + autoMarketUpdate.orderOffset);
  //   console.log("AutoMarketUpdate-====, cancelled:" + autoMarketUpdate.cancelled);
  //   console.log("AutoMarketUpdate-====, validUntil:" + autoMarketUpdate.validUntil);
  //   console.log("AutoMarketUpdate-====, maxFee:" + autoMarketUpdate.maxFee);
  //   console.log("AutoMarketUpdate-====, feeTokenID:" + autoMarketUpdate.feeTokenID);
  //   autoMarketUpdate.hash = hasher(inputs).toString(10);
  //   // Create signature
  //   autoMarketUpdate.signature = EdDSA.sign(account.secretKey, autoMarketUpdate.hash);

  //   console.log("AutoMarketUpdate-====, signature:" + autoMarketUpdate.signature);
  //   // Verify signature
  //   const success = EdDSA.verify(autoMarketUpdate.hash, autoMarketUpdate.signature, [
  //     account.publicKeyX,
  //     account.publicKeyY
  //   ]);
  //   assert(success, "in requestAutoMarketUpdate Failed to verify signature");
  //   this.pendingTransactions[this.exchangeId].push(autoMarketUpdate);
  //   return autoMarketUpdate;
  // }

  public sendRing(ring: SpotTrade) {
    ring.txType = "SpotTrade";
    this.pendingTransactions[this.exchangeId].push(ring);
  }

  public sendBatchSpotTrade(batchSpotTrade: BatchSpotTrade) {
    batchSpotTrade.txType = "BatchSpotTrade";

    this.pendingTransactions[this.exchangeId].push(batchSpotTrade);
  }

  public ensureDirectoryExists(filePath: string) {
    const dirname = path.dirname(filePath);
    if (fs.existsSync(dirname)) {
      return true;
    }
    this.ensureDirectoryExists(dirname);
    fs.mkdirSync(dirname);
  }

  public async createBlock(exchangeID: number, blockType: BlockType, data: string, validate: boolean = true) {
    const nextBlockIdx = this.blocks[exchangeID].length;
    const inputFilename = "./blocks/block_" + exchangeID + "_" + nextBlockIdx + "_info.json";
    const outputFilename = "./blocks/block_" + exchangeID + "_" + nextBlockIdx + ".json";

    this.ensureDirectoryExists(inputFilename);
    fs.writeFileSync(inputFilename, data, "utf8");

    // Create the block
    const result = childProcess.spawnSync(
      "python3",
      ["operator/create_block.py", "" + exchangeID, "" + nextBlockIdx, "" + blockType, inputFilename, outputFilename],
      // { stdio: doDebugLogging() ? "inherit" : "ignore" }
      { stdio: "inherit" }
    );
    assert(result.status === 0, "create_block failed: " + blockType);

    if (validate) {
      await this.validateBlock(outputFilename);
    }

    return {
      blockIdx: nextBlockIdx,
      infoFilename: inputFilename,
      blockFilename: outputFilename
    };
  }

  public hashToFieldElement(hash: string) {
    const fieldHash = new BN(hash.slice(2), 16).shrn(3).toString(10);
    return fieldHash;
  }

  public getPublicDataHashAndInput(data: string) {
    const publicDataHash = "0x" + SHA256(Buffer.from(data.slice(2), "hex")).toString("hex");
    return {
      publicDataHash,
      publicInput: this.hashToFieldElement(publicDataHash)
    };
  }

  public async validateBlock(filename: string) {
    // Validate the block
    console.log("before childProcess.spawnSync filename:" + filename);
    const result = childProcess.spawnSync(
      "build/circuit/dex_circuit",
      ["-validate", filename],
      // { stdio: doDebugLogging() ? "inherit" : "ignore" }
      { stdio: "inherit" }
    );
    assert(result.status === 0, "invalid block: " + filename);
  }

  public async commitBlock(
    operatorId: number,
    blockType: BlockType,
    blockSize: number,
    data: string,
    filename: string,
    txBlock: TxBlock,
    auxiliaryData: any[]
  ) {
    const publicDataHashAndInput = this.getPublicDataHashAndInput(data);
    const publicDataHash = publicDataHashAndInput.publicDataHash;
    const publicInput = publicDataHashAndInput.publicInput;
    console.log("- " + filename);
    console.log("[EVM]PublicData: " + data);
    console.log("[EVM]PublicDataHash: " + publicDataHash);
    console.log("[EVM]PublicInput: " + publicInput);
    console.log("[EVM]AuxiliaryData: " + auxiliaryData);

    // Make sure the keys are generated
    console.log("in commitBlock before registerCircuit");
    await this.registerCircuit(blockType, blockSize, 0);

    const blockVersion = 0;
    let offchainData = this.getRandomInt(2) === 0 ? "0x0ff" + this.blocks[this.exchangeId].length : "0x";
    if (offchainData.length % 2 == 1) {
      offchainData += "0";
    }

    const blockFile = JSON.parse(fs.readFileSync(filename, "ascii"));

    logDebug("[EVM]Block merkleRoot: " + "0x" + new BN(blockFile.merkleRootAfter, 10).toString(16, 64));

    logDebug("[EVM]Block merkleAssetRoot: " + "0x" + new BN(blockFile.merkleAssetRootAfter, 10).toString(16, 64));

    const block: Block = {
      blockIdx: this.blocks[this.exchangeId].length,
      filename,
      blockType,
      blockSize,
      blockVersion,
      operator: this.operator ? this.operator.address : this.exchangeOperator,
      origin: this.exchangeOperator,
      operatorId,
      merkleRoot: "0x" + new BN(blockFile.merkleRootAfter, 10).toString(16, 64),
      merkleAssetRoot: "0x" + new BN(blockFile.merkleAssetRootAfter, 10).toString(16, 64),
      data,
      auxiliaryData,
      offchainData,
      publicDataHash,
      publicInput,
      blockFee: new BN(0),
      timestamp: 0,
      transactionHash: "0",
      internalBlock: txBlock,
      callbacks: this.pendingBlockCallbacks[this.exchangeId]
    };
    this.pendingBlocks[this.exchangeId].push(block);
    this.blocks[this.exchangeId].push(block);

    console.log("in commitBlock after return block");
    return block;
  }

  public async registerCircuit(blockType: BlockType, blockSize: number, blockVersion: number) {
    const blockFilename = "./blocks/protoblock_" + blockType + "_blockSize_" + blockSize + ".json";

    const block: any = {};
    block.blockType = blockType;
    block.blockSize = blockSize;
    fs.writeFileSync(blockFilename, JSON.stringify(block, undefined, 4), "ascii");

    console.log("before check isCircuitRegistered");
    const isCircuitRegistered = await this.blockVerifier.isCircuitRegistered(
      block.blockType,
      block.blockSize,
      blockVersion
    );
    if (!isCircuitRegistered) {
      console.log("before create keys");
      const result = childProcess.spawnSync("build/circuit/dex_circuit", ["-createkeys", blockFilename], {
        stdio: doDebugLogging() ? "inherit" : "ignore"
      });
      assert(result.status === 0, "generateKeys failed: " + blockFilename);

      let verificationKeyFilename = "keys/";
      verificationKeyFilename += "all_";
      verificationKeyFilename += block.blockSize + "_vk.json";

      // Read the verification key and set it in the smart contract
      const vk = JSON.parse(fs.readFileSync(verificationKeyFilename, "ascii"));
      const vkFlattened = this.flattenList(this.flattenVK(vk));
      // console.log(vkFlattened);

      console.log("before registerCircuit");
      await this.blockVerifier.registerCircuit(block.blockType, block.blockSize, blockVersion, vkFlattened);
    }
  }

  public getKey(block: Block) {
    let key = 0;
    key |= block.blockType;
    key <<= 16;
    key |= block.blockSize;
    key <<= 8;
    key |= block.blockVersion;
    return key;
  }

  public sleep(millis: number) {
    return new Promise(resolve => setTimeout(resolve, millis));
  }

  // function returns a Promise
  public async httpGetSync(url: string, port: number) {
    return new Promise((resolve, reject) => {
      http.get(url, { port, timeout: 600 }, response => {
        let chunks_of_data: Buffer[] = [];

        response.on("data", fragments => {
          chunks_of_data.push(fragments);
        });

        response.on("end", () => {
          let response_body = Buffer.concat(chunks_of_data);
          resolve(response_body.toString());
        });

        response.on("error", error => {
          reject(error);
        });
      });
    });
  }

  public async stop() {
    // Stop all prover servers
    for (const port of this.proverPorts.values()) {
      await this.httpGetSync("http://localhost/stop", port);
    }
  }

  public getCallbackConfig(blockCallbacks: BlockCallback[][]) {
    interface TxCallback {
      txIdx: number;
      numTxs: number;
      receiverIdx: number;
      data: string;
    }

    interface OnchainBlockCallback {
      blockIdx: number;
      txCallbacks: TxCallback[];
    }

    interface CallbackConfig {
      blockCallbacks: OnchainBlockCallback[];
      receivers: string[];
    }

    const callbackConfig: CallbackConfig = {
      blockCallbacks: [],
      receivers: []
    };

    //console.log("Block callbacks: ");
    for (const [blockIdx, callbacks] of blockCallbacks.entries()) {
      //console.log(blockIdx);
      //console.log(block.callbacks);
      if (callbacks.length > 0) {
        const onchainBlockCallback: OnchainBlockCallback = {
          blockIdx,
          txCallbacks: []
        };
        callbackConfig.blockCallbacks.push(onchainBlockCallback);

        for (const blockCallback of callbacks) {
          // Find receiver index
          let receiverIdx = callbackConfig.receivers.findIndex(target => target === blockCallback.target);
          if (receiverIdx === -1) {
            receiverIdx = callbackConfig.receivers.length;
            callbackConfig.receivers.push(blockCallback.target);
          }
          // Add the block callback to the list
          onchainBlockCallback.txCallbacks.push({
            txIdx: blockCallback.txIdx,
            numTxs: blockCallback.numTxs,
            receiverIdx,
            data: blockCallback.auxiliaryData
          });
        }
        //console.log(onchainBlockCallback);
      }
    }
    //console.log(callbackConfig);
    //for (const bc of callbackConfig.blockCallbacks) {
    //  console.log(bc);
    //}
    return callbackConfig;
  }

  public setPreApprovedTransactions(blocks: Block[]) {
    for (const block of blocks) {
      for (const blockCallback of block.callbacks) {
        for (let i = 0; i < blockCallback.numTxs; i++) {
          for (const auxiliaryData of block.auxiliaryData) {
            if (auxiliaryData[0] === Number(blockCallback.txIdx) + i) {
              auxiliaryData[1] = true;
              // No auxiliary data needed for the tx
              auxiliaryData[2] = "0x";
            }
          }
        }
      }
    }
  }

  public encodeAuxiliaryData(auxiliaryData: any[]) {
    const encodedAuxiliaryData = web3.eth.abi.encodeParameter(
      {
        "struct AuxiliaryData[]": {
          data: "bytes"
        }
      },
      auxiliaryData
    );

    return encodedAuxiliaryData;
  }

  public decodeAuxiliaryData(encodedAuxiliaryData: string) {
    const auxiliaryData = web3.eth.abi.decodeParameter(
      {
        "struct AuxiliaryData[]": {
          txIndex: "uint",
          approved: "bool",
          data: "bytes"
        }
      },
      encodedAuxiliaryData
    );
    return auxiliaryData;
  }

  public getOnchainBlock(
    blockType: number,
    blockSize: number,
    data: string,
    auxiliaryData: any[],
    proof: any,
    offchainData: string = "0x",
    storeBlockInfoOnchain: boolean = false,
    blockVersion: number = 0
  ) {
    const onchainBlock: OnchainBlock = {
      blockType,
      blockSize,
      blockVersion,
      data,
      proof,
      storeBlockInfoOnchain,
      offchainData: offchainData,
      auxiliaryData: this.encodeAuxiliaryData(auxiliaryData)
    };
    return onchainBlock;
  }

  public getSubmitCallbackData(blocks: OnchainBlock[]) {
    return this.exchange.contract.methods.submitBlocks(blocks).encodeABI();
  }

  public getSubmitBlocksWithCallbacks(parameters: any) {
    const operatorContract = this.operator ? this.operator : this.exchange;
    return operatorContract.contract.methods
      .submitBlocksWithCallbacks(parameters.isDataCompressed, parameters.data, parameters.callbackConfig)
      .encodeABI();
  }

  public getSubmitBlocksWithCallbacksData(
    isDataCompressed: boolean,
    txData: string,
    blockCallbacks: BlockCallback[][]
  ) {
    const data = isDataCompressed ? compressZeros(txData) : txData;
    //console.log(data);

    // Block callbacks
    const callbackConfig = this.getCallbackConfig(blockCallbacks);

    return {
      isDataCompressed,
      data,
      callbackConfig
    };
  }

  public readProof(filename: string) {
    return this.flattenProof(JSON.parse(fs.readFileSync(filename, "ascii")));
  }

  public async submitBlocks(blocks: Block[], testCallback?: any) {
    if (blocks.length === 0) {
      return;
    }

    // Generate proofs
    for (const [i, block] of blocks.entries()) {
      console.log("submitBlocks: " + block.filename);

      const blockData = JSON.parse(fs.readFileSync(block.filename, "ascii"));

      const proofFilename = "./blocks/block_" + this.exchangeId + "_" + block.blockIdx + "_proof.json";

      console.log("Generating proof: " + proofFilename);

      if (this.useProverServer) {
        const key = this.getKey(block);
        if (!this.proverPorts.has(key)) {
          const port = this.portGenerator++;
          const process = childProcess.spawn("build/circuit/dex_circuit", ["-server", block.filename, "" + port], {
            detached: false,
            stdio: doDebugLogging() ? "inherit" : "ignore"
          });
          let connected = false;
          let numTries = 0;
          while (!connected) {
            // Wait for the prover server to start up
            http
              .get("http://localhost/status", { port }, res => {
                connected = true;
                this.proverPorts.set(key, port);
              })
              .on("error", e => {
                numTries++;
                if (numTries > 240) {
                  assert(false, "prover server failed to start: " + e);
                }
              });
            await this.sleep(1000);
          }
        }
        const port = this.proverPorts.get(key);
        // Generate the proof
        let proveQuery = "http://localhost/prove?block_filename=" + block.filename;
        proveQuery += "&proof_filename=" + proofFilename;
        proveQuery += "&validate=true";
        await this.httpGetSync(proveQuery, port);
      } else {
        // Generate the proof by starting a dedicated circuit binary app instance
        const result = childProcess.spawnSync("build/circuit/dex_circuit", ["-prove", block.filename, proofFilename], {
          stdio: doDebugLogging() ? "inherit" : "ignore"
        });
        assert(result.status === 0, "Block proof generation failed: " + block.filename);
      }

      // Read the proof
      block.proof = this.readProof(proofFilename);
      // console.log(proof);
    }

    // Set pool transactions as approved
    this.setPreApprovedTransactions(blocks);

    // Prepare block data
    const onchainBlocks: OnchainBlock[] = [];
    const blockCallbacks: BlockCallback[][] = [];
    for (const block of blocks) {
      //console.log(block.blockIdx);
      const onchainBlock = this.getOnchainBlock(
        block.blockType,
        block.blockSize,
        block.data,
        block.auxiliaryData,
        block.proof,
        block.offchainData,
        this.getRandomBool(),
        block.blockVersion
      );
      onchainBlocks.push(onchainBlock);
      blockCallbacks.push(block.callbacks);

      console.log("onchainBlock.auxiliaryData: ", onchainBlock.auxiliaryData);
    }

    // Callback that allows modifying the blocks
    if (testCallback !== undefined) {
      testCallback(onchainBlocks, blocks);
    }

    const numBlocksSubmittedBefore = (await this.exchange.getBlockHeight()).toNumber();

    // Forced requests
    const numAvailableSlotsBefore = (await this.exchange.getNumAvailableForcedSlots()).toNumber();

    // SubmitBlocks raw tx data
    const txData = this.getSubmitCallbackData(onchainBlocks);
    //console.log(txData);

    // const gasTokenConfig: GasTokenConfig = {
    //   gasTokenVault: Constants.zeroAddress,
    //   maxToBurn: 0,
    //   expectedGasRefund: 0,
    //   calldataCost: 0
    // };

    const parameters = this.getSubmitBlocksWithCallbacksData(true, txData, blockCallbacks);

    // Submit the blocks onchain
    const operatorContract = this.operator ? this.operator : this.exchange;

    let bestGasTokensToBurn = 0;
    /*let bestGasUsed = 20000000;
    for (let i = 0; i < 15; i++) {
      gasTokenConfig.maxToBurn = i;
      const gasUsed = await operatorContract.submitBlocksWithCallbacks.estimateGas(
        parameters.isDataCompressed,
        parameters.data,
        parameters.callbackConfig,
        gasTokenConfig,
        { from: this.exchangeOperator, gasPrice: 0 }
      );
      if (gasUsed < bestGasUsed) {
        bestGasUsed = gasUsed;
        bestGasTokensToBurn = i;
      }
      console.log("" + i + ": " + gasUsed);
    }
    console.log("Best gas used: " + bestGasUsed);
    console.log("Num gas tokens burned: " + bestGasTokensToBurn);*/
    // gasTokenConfig.maxToBurn = bestGasTokensToBurn;

    let numDeposits = 0;
    for (const block of blocks) {
      for (const tx of block.internalBlock.transactions) {
        if (tx.txType === "Deposit") {
          numDeposits++;
        }
      }
    }
    //console.log("num deposits: " + numDeposits);

    const msg_data = this.getSubmitBlocksWithCallbacks(parameters);
    // console.log("submitBlocksWithCallbacks msg_data:", msg_data);

    let tx: any = undefined;
    tx = await operatorContract.submitBlocksWithCallbacks(
      parameters.isDataCompressed,
      parameters.data,
      parameters.callbackConfig,
      //txData,
      { from: this.exchangeOperator, gasPrice: 0 }
    );
    /*tx = await operatorContract.submitBlocks(
      onchainBlocks,
      { from: this.exchangeOperator, gasPrice: 0 }
    );*/
    /*tx = await operatorContract.transact(
      txData,
      { from: this.exchangeOperator, gasPrice: 0 }
    );*/
    /*tx = await operatorContract.submitBlocks(onchainBlocks, {
      from: this.exchangeOwner,
      gasPrice: 0
    });*/
    /*const wrapper = await this.contracts.ExchangeV3.at(operatorContract.address);
    tx = await wrapper.submitBlocks(
      onchainBlocks,
      { from: this.exchangeOwner, gasPrice: 0 }
    );*/
    /*tx = await operatorContract.submitBlocksWithCallbacks(
      onchainBlocks,
      blockCallbacks,
      { from: this.exchangeOperator, gasPrice: 0 }
    );*/
    logInfo("\x1b[46m%s\x1b[0m", "[submitBlocks] Gas used: " + tx.receipt.gasUsed);
    const ethBlock = await web3.eth.getBlock(tx.receipt.blockNumber);

    // Check number of blocks submitted
    console.log("testExchangeUtils: Check number of blocks submitted");
    const numBlocksSubmittedAfter = (await this.exchange.getBlockHeight()).toNumber();
    assert.equal(numBlocksSubmittedAfter, numBlocksSubmittedBefore + blocks.length, "unexpected block height");

    // Check the BlockSubmitted event(s)
    console.log("testExchangeUtils: Check the BlockSubmitted event");
    {
      const events = await this.assertEventsEmitted(this.exchange, "BlockSubmitted", blocks.length);
      for (const [i, event] of events.entries()) {
        const blockIdx = event.blockIdx.toNumber();
        assert.equal(blockIdx, blocks[i].blockIdx, "unexpected block idx");
        assert.equal(event.merkleRoot, blocks[i].merkleRoot, "unexpected Merkle root");
        assert.equal(event.publicDataHash, blocks[i].publicDataHash, "unexpected public data hash");
        const block = this.blocks[this.exchangeId][event.blockIdx.toNumber()];
        block.transactionHash = tx.receipt.transactionHash;
        block.timestamp = ethBlock.timestamp;
        block.blockFee = new BN(event.blockFee);
      }
    }

    // Check the new Merkle root
    console.log("testExchangeUtils: Check the new Merkle root");
    const merkleRoot = await this.exchange.getMerkleRoot();
    assert.equal(merkleRoot, blocks[blocks.length - 1].merkleRoot, "unexpected Merkle root");

    // Check the Block info stored onchain
    console.log("testExchangeUtils: Check the Block info stored onchain");
    for (const [i, block] of blocks.entries()) {
      const blockInfo = await this.exchange.getBlockInfo(block.blockIdx);
      const expectedHash = onchainBlocks[i].storeBlockInfoOnchain
        ? block.publicDataHash.slice(0, 2 + 28 * 2)
        : "0x" + "00".repeat(28);
      assert.equal(blockInfo.blockDataHash, expectedHash, "unexpected blockInfo public data hash");
      const expectedTimestamp = onchainBlocks[i].storeBlockInfoOnchain ? Number(ethBlock.timestamp) : 0;
      assert.equal(blockInfo.timestamp, expectedTimestamp, "unexpected blockInfo timestamp");
    }

    // Forced requests
    console.log("testExchangeUtils: Forced requests");
    const numAvailableSlotsAfter = (await this.exchange.getNumAvailableForcedSlots()).toNumber();
    let numForcedRequestsProcessed = 0;
    for (const block of blocks) {
      for (const tx of block.internalBlock.transactions) {
        if (tx.txType === "Withdraw" && tx.type > 1) {
          numForcedRequestsProcessed++;
        }
      }
    }
    assert.equal(
      numAvailableSlotsAfter - numForcedRequestsProcessed,
      numAvailableSlotsBefore,
      "unexpected num available slots"
    );

    // TODO: restore explorer state check
    // // Check the current state against the explorer state
    // console.log(
    //   "testExchangeUtils: Check the current state against the explorer state"
    // );
    // await this.checkExplorerState();

    console.log("done submitBlocks");
  }

  public addBlockCallback(target: string) {
    const blockCallback: BlockCallback = {
      target,
      auxiliaryData: Constants.emptyBytes,
      txIdx: this.pendingTransactions[this.exchangeId].length,
      numTxs: 0
    };
    this.pendingBlockCallbacks[this.exchangeId].push(blockCallback);
    return blockCallback;
  }

  public async submitPendingBlocks(testCallback?: any) {
    await this.submitBlocks(this.pendingBlocks[this.exchangeId], testCallback);
    this.pendingBlocks[this.exchangeId] = [];
  }

  public async getActiveOperator(exchangeID: number) {
    return this.activeOperator ? this.activeOperator : this.operators[exchangeID];
  }

  public async setOperatorContract(operator: any) {
    this.operator = operator;
  }

  public async setActiveOperator(operator: number) {
    this.activeOperator = operator;
  }

  public getTransferAuxData(transfer: Transfer) {
    return web3.eth.abi.encodeParameter("tuple(bytes,uint96,uint32)", [
      transfer.onchainSignature ? transfer.onchainSignature : "0x",
      transfer.originalMaxFee ? transfer.originalMaxFee : transfer.maxFee,
      transfer.validUntil
    ]);
  }

  public getAccountUpdateAuxData(accountUpdate: AccountUpdate) {
    return web3.eth.abi.encodeParameter("tuple(bytes,uint96,uint32)", [
      accountUpdate.onchainSignature ? accountUpdate.onchainSignature : "0x",
      accountUpdate.originalMaxFee ? accountUpdate.originalMaxFee : accountUpdate.maxFee,
      accountUpdate.validUntil
    ]);
  }

  public getWithdrawalAuxData(withdrawal: WithdrawalRequest) {
    // Hack: fix json deserializing when the to address is serialized as a decimal string
    if (!withdrawal.to.startsWith("0x")) {
      withdrawal.to = "0x" + new BN(withdrawal.to).toString(16, 40);
    }
    return web3.eth.abi.encodeParameter("tuple(bool,uint256,bytes,uint248,address,uint96,uint32,uint248)", [
      withdrawal.storeRecipient,
      withdrawal.gas,
      withdrawal.onchainSignature ? withdrawal.onchainSignature : "0x",
      withdrawal.minGas,
      withdrawal.to,
      // withdrawal.extraData ? withdrawal.extraData : "0x",
      withdrawal.originalMaxFee ? withdrawal.originalMaxFee : withdrawal.maxFee,
      withdrawal.validUntil,
      withdrawal.amount
    ]);
  }

  public async submitTransactions(forcedBlockSize?: number, justValidBlock?: boolean) {
    const exchangeID = this.exchangeId;
    const pendingTransactions = this.pendingTransactions[exchangeID];
    if (pendingTransactions.length === 0) {
      return [];
    }

    console.log("submitTransactions, pendingTransactions.length", pendingTransactions.length);

    // Generate the token transfers for the ring
    const blockNumber = await web3.eth.getBlockNumber();
    const timestamp = (await web3.eth.getBlock(blockNumber)).timestamp + 30;

    let numTransactionsDone = 0;
    const blocks: Block[] = [];
    while (numTransactionsDone < pendingTransactions.length) {
      // Get all rings for the block
      const blockSize = forcedBlockSize
        ? forcedBlockSize
        : this.getBestBlockSize(pendingTransactions.length - numTransactionsDone, this.blockSizes);
      const transactionsTemp: TxType[] = [];
      for (let b = numTransactionsDone; b < numTransactionsDone + blockSize; b++) {
        if (b < pendingTransactions.length) {
          transactionsTemp.push(pendingTransactions[b]);
        } else {
          const noop: Noop = {
            txType: "Noop"
          };
          transactionsTemp.push(noop);
        }
      }
      assert(transactionsTemp.length === blockSize);
      numTransactionsDone += blockSize;

      const currentBlockIdx = this.blocks[exchangeID].length - 1;

      const protocolFees = await this.exchange.getProtocolFeeValues();
      const protocolFeeBips = protocolFees.protocolFeeBips.toNumber();
      // const protocolMakerFeeBips = protocolFees.makerFeeBips.toNumber();

      //refeUrefe
      // const referFees = await this.exchange.getReferFeeValues();
      // const referFeeBips = referFees.referFeeBips.toNumber();
      // const uiFeeBips = referFees.uiFeeBips.toNumber();
      // const referFeeBips = 40;
      // const uiFeeBips = 50;

      const depositTransactions: TxType[] = [];
      const accountTransactions: TxType[] = [];
      const othersTransactions: TxType[] = [];
      const withdrawTransactions: TxType[] = [];

      for (const tx of transactionsTemp) {
        if (tx.txType === "Deposit") {
          depositTransactions.push(tx);
        } else if (tx.txType === "AccountUpdate") {
          accountTransactions.push(tx);
        } else if (tx.txType === "Withdraw") {
          withdrawTransactions.push(tx);
        } else {
          othersTransactions.push(tx);
        }
      }

      const transactions = depositTransactions.concat(accountTransactions, othersTransactions, withdrawTransactions);

      for (const tx of transactions) {
        logDebug(tx.txType);
      }

      const operator = await this.getActiveOperator(exchangeID);
      const txBlock: TxBlock = {
        transactions,
        timestamp,
        protocolFeeBips,
        // protocolMakerFeeBips,
        // //refeUrefe
        // referFeeBips,
        // uiFeeBips,
        exchange: this.exchange.address,
        operatorAccountID: operator
      };

      // Store state before
      // const stateBefore = await Simulator.loadExchangeState(
      //   exchangeID,
      //   currentBlockIdx
      // );

      console.log("this.createBlock...");
      // Create the block
      const { blockIdx, infoFilename, blockFilename } = await this.createBlock(
        exchangeID,
        0,
        JSON.stringify(txBlock, replacer, 4),
        false
      );

      const blockInfoData = JSON.parse(fs.readFileSync(infoFilename, "ascii"));
      const block = JSON.parse(fs.readFileSync(blockFilename, "ascii"));

      // Create the auxiliary data
      const auxiliaryData = this.getBlockAuxiliaryData(blockInfoData);
      const blockData = this.getBlockData(block, auxiliaryData.length);
      console.log("auxiliaryData.length: ", auxiliaryData.length);
      console.log("auxiliaryData: ", auxiliaryData);
      console.log("blockData: ", blockData);

      // Write the block signature
      const publicDataHashAndInput = this.getPublicDataHashAndInput(blockData);

      console.log("[EVM]PublicData: " + blockData);
      console.log("[EVM]PublicDataHash: " + publicDataHashAndInput.publicDataHash);
      console.log("[EVM]PublicInput: " + publicDataHashAndInput.publicInput);

      console.log("before signRingBlock");
      this.signRingBlock(block, publicDataHashAndInput.publicInput);
      console.log("before writeFileSync");
      fs.writeFileSync(blockFilename, JSON.stringify(block, undefined, 4), "utf8");
      // Validate the block after generating the signature
      console.log("before validateBlock");
      await this.validateBlock(blockFilename);

      if (justValidBlock) {
        return;
      }

      // Load state after
      // const stateAfter = await Simulator.loadExchangeState(
      //   exchangeID,
      //   currentBlockIdx + 1
      // );

      // Validate state change
      // Simulator.executeBlock(txBlock, stateBefore, stateAfter);

      // Commit the block
      console.log("before commitBlock");
      const blockInfo = await this.commitBlock(
        operator,
        0,
        blockSize,
        blockData,
        blockFilename,
        txBlock,
        auxiliaryData
      );
      blockInfo.blockInfoData = blockInfoData;
      blocks.push(blockInfo);

      console.log("in submitTransactions before writeFileSync _auxiliaryData");
      // Write auxiliary data
      fs.writeFileSync(
        blockFilename.slice(0, -5) + "_auxiliaryData.json",
        JSON.stringify(blockInfo.auxiliaryData, undefined, 4),
        "utf8"
      );

      console.log("in submitTransactions before writeFileSync _callbacks");
      // Write callbacks
      fs.writeFileSync(
        blockFilename.slice(0, -5) + "_callbacks.json",
        JSON.stringify(blockInfo.callbacks, undefined, 4),
        "utf8"
      );
    }

    this.pendingTransactions[exchangeID] = [];
    this.pendingBlockCallbacks[exchangeID] = [];
    return blocks;
  }

  public getBlockAuxiliaryData(block: any) {
    const auxiliaryData: any[] = [];
    for (const [i, transaction] of block.transactions.entries()) {
      // if (transaction.txType === "Transfer") {
      //   if (transaction.type > 0) {
      //     const encodedTransferData = this.getTransferAuxData(transaction);
      //     auxiliaryData.push([i, false, encodedTransferData]);
      //   }
      // } else
      if (transaction.txType === "Withdraw") {
        const encodedWithdrawalData = this.getWithdrawalAuxData(transaction);
        auxiliaryData.push([encodedWithdrawalData]);
      } else if (transaction.txType === "Deposit") {
        auxiliaryData.push(["0x"]);
      } else if (transaction.txType === "AccountUpdate") {
        if (transaction.type > 0) {
          const encodedAccountUpdateData = this.getAccountUpdateAuxData(transaction);
          auxiliaryData.push([encodedAccountUpdateData]);
        }
      }
    }
    logDebug("numConditionalTransactions: " + auxiliaryData.length);
    return auxiliaryData;
  }

  public addFloatDataForBatchSpotTrade(da: BitstreamEx, exchange: Number) {
    // const MAX = 4294967296;
    // const MAX = 16777216;
    const MAX = 1073741824;
    console.log("============addFloatDataForBatchSpotTrade exchange:" + exchange);
    if (exchange === undefined || exchange == 0) {
      exchange = 0;
    }
    if (exchange < 0) {
      const newExchange = -exchange;
      const firstExchange = toFloat(new BN(newExchange.toString()), Constants.Float29Encoding);
      // da.addNumber(MAX - firstExchange, 3);
      da.addNumberForBits(MAX - firstExchange, 30);
      console.log("============addFloatDataForBatchSpotTrade float reduce:" + (MAX - firstExchange));
    } else {
      const firstExchange = toFloat(new BN(exchange.toString()), Constants.Float29Encoding);
      console.log("============addFloatDataForBatchSpotTrade float increase:" + firstExchange);
      const recovery = fromFloat(firstExchange, Constants.Float29Encoding);
      console.log("============addFloatDataForBatchSpotTrade float increase from float:" + recovery);

      // da.addNumber(firstExchange, 3);
      da.addNumberForBits(firstExchange, 30);
    }
  }

  public getSingleBatchSpotTradeTokenType(
    order: OrderInfo,
    firstToken: Number,
    secondToken: Number,
    thirdToken: Number
  ) {
    if (order.isNoop == 1) {
      return 0;
    }
    const orderTokenS = order.tokenIdS ? order.tokenIdS : Number(order.tokenS);
    const orderTokenB = order.tokenIdB ? order.tokenIdB : Number(order.tokenB);
    console.log("in getSingleBatchSpotTradeTokenType orderTokenS:" + orderTokenS);
    console.log("in getSingleBatchSpotTradeTokenType orderTokenB:" + orderTokenB);
    if (
      (orderTokenS == firstToken && orderTokenB == secondToken) ||
      (orderTokenS == secondToken && orderTokenB == firstToken)
    ) {
      // 00
      console.log("in getSingleBatchSpotTradeTokenType return 0");
      return 0;
    } else if (
      (orderTokenS == firstToken && orderTokenB == thirdToken) ||
      (orderTokenS == thirdToken && orderTokenB == firstToken)
    ) {
      // 01
      console.log("in getSingleBatchSpotTradeTokenType return 1");
      return 1;
    } else if (
      (orderTokenS == secondToken && orderTokenB == thirdToken) ||
      (orderTokenS == thirdToken && orderTokenB == secondToken)
    ) {
      // 10
      console.log("in getSingleBatchSpotTradeTokenType return 2");
      return 2;
    }
  }

  public feedBatchSpotTradeTokenType(da: BitstreamEx, batchSpotTrade: BatchSpotTrade) {
    var secondUserTokenType = 0;
    var thirdUserTokenType = 0;
    var fourthUserTokenType = 0;
    var fifthUserTokenType = 0;
    var sixthUserTokenType = 0;
    const firstToken = batchSpotTrade.tokens[0];
    const secondToken = batchSpotTrade.tokens[1];
    const thirdToken = batchSpotTrade.tokens[2];
    if (batchSpotTrade.users.length >= 2) {
      secondUserTokenType = this.getSingleBatchSpotTradeTokenType(
        batchSpotTrade.users[1].orders[0],
        firstToken,
        secondToken,
        thirdToken
      );
      console.log("firstToken:" + firstToken + ";secondToken:" + secondToken + ";thirdToken:" + thirdToken);
    }
    if (batchSpotTrade.users.length >= 3) {
      thirdUserTokenType = this.getSingleBatchSpotTradeTokenType(
        batchSpotTrade.users[2].orders[0],
        firstToken,
        secondToken,
        thirdToken
      );
    }
    if (batchSpotTrade.users.length >= 4) {
      fourthUserTokenType = this.getSingleBatchSpotTradeTokenType(
        batchSpotTrade.users[3].orders[0],
        firstToken,
        secondToken,
        thirdToken
      );
    }
    if (batchSpotTrade.users.length >= 5) {
      fifthUserTokenType = this.getSingleBatchSpotTradeTokenType(
        batchSpotTrade.users[4].orders[0],
        firstToken,
        secondToken,
        thirdToken
      );
    }
    if (batchSpotTrade.users.length >= 6) {
      sixthUserTokenType = this.getSingleBatchSpotTradeTokenType(
        batchSpotTrade.users[5].orders[0],
        firstToken,
        secondToken,
        thirdToken
      );
    }
    // 10 -> 10000000
    // 2 -> 128 = 2 * 
    var tokenType = secondUserTokenType * 64;
    // 10 -> 100000
    // 2 -> 32 = 2 * 
    tokenType = tokenType + thirdUserTokenType * 16;
    // 10 -> 1000
    // 2 -> 8 = 2 * 
    tokenType = tokenType + fourthUserTokenType * 4;
    // 10 -> 10
    // 2 -> 2
    tokenType = tokenType + fifthUserTokenType;
    // console.log("secondUserTokenType:" + secondUserTokenType + ";thirdUserTokenType:" + thirdUserTokenType + ";fourthUserTokenType:" + fourthUserTokenType + ";tokenType:" + tokenType);
    // da.addNumber(tokenType, 1);
    da.addNumberForBits(tokenType, 8);
    //byte
    // da.addNumber(sixthUserTokenType, 1);
    da.addNumberForBits(sixthUserTokenType, 2);
    return [secondUserTokenType, thirdUserTokenType, fourthUserTokenType, fifthUserTokenType, sixthUserTokenType];
  }

  public feedBatchSpotTradeOtherUserAmountExchangePublicData(
    da: BitstreamEx,
    typeType: Number,
    firstTokenExchange: Number,
    secondTokenExchange: Number,
    thirdTokenExchange: Number
  ) {
    if (typeType == 0) {
      this.addFloatDataForBatchSpotTrade(da, firstTokenExchange);
      this.addFloatDataForBatchSpotTrade(da, secondTokenExchange);
    } else if (typeType == 1) {
      this.addFloatDataForBatchSpotTrade(da, firstTokenExchange);
      this.addFloatDataForBatchSpotTrade(da, thirdTokenExchange);
    } else if (typeType == 2) {
      this.addFloatDataForBatchSpotTrade(da, secondTokenExchange);
      this.addFloatDataForBatchSpotTrade(da, thirdTokenExchange);
    }
  }

  public feedBatchSpotTradePublicData(da: BitstreamEx, batchSpotTrade: BatchSpotTrade, bindTokenID: number) {
    const firstOrder = batchSpotTrade.users[0].orders[0];
    // tokeorde
    // var firstTokenID = firstOrder.tokenIdS ? firstOrder.tokenIdS : Number(firstOrder.tokenS);
    // var secondTokenID = firstOrder.tokenIdB ? firstOrder.tokenIdB : Number(firstOrder.tokenB);
    // var thirdTokenID = firstOrder.feeTokenID;
    // const ethTokenID = Number(this.testContext.tokenSymbolAddrMap.get("ETH"));
    console.log("bindTokenID Token:" + bindTokenID);

    // if (bindTokenID == firstTokenID) {
    //   da.addNumber(secondTokenID, 4);
    //   da.addNumber(thirdTokenID, 4);
    // } else if (bindTokenID == secondTokenID) {
    //   da.addNumber(firstTokenID, 4);
    //   da.addNumber(thirdTokenID, 4);
    // } else {
    //   da.addNumber(firstTokenID, 4);
    //   da.addNumber(secondTokenID, 4);
    // }
    // da.addNumber(batchSpotTrade.tokens[0], 4);
    // da.addNumber(batchSpotTrade.tokens[1], 4);
    da.addNumberForBits(batchSpotTrade.tokens[0], 32);
    da.addNumberForBits(batchSpotTrade.tokens[1], 32);
    console.log("ID 0:" + batchSpotTrade.tokens[0]);
    console.log("ID 1:" + batchSpotTrade.tokens[1]);
    da.print();
    // console.log("A firstTokenExchange:" + batchSpotTrade.userAFirstTokenExchange);
    // console.log("A secondTokenExchange:" + batchSpotTrade.userASecondTokenExchange);
    // console.log("A thirdTokenExchange:" + batchSpotTrade.userAThirdTokenExchange);

    // console.log("B firstTokenExchange:" + batchSpotTrade.userBFirstTokenExchange);
    // console.log("B secondTokenExchange:" + batchSpotTrade.userBSecondTokenExchange);
    // console.log("B thirdTokenExchange:" + batchSpotTrade.userBThirdTokenExchange);

    // console.log("C firstTokenExchange:" + batchSpotTrade.userCFirstTokenExchange);
    // console.log("C secondTokenExchange:" + batchSpotTrade.userCSecondTokenExchange);
    // console.log("C thirdTokenExchange:" + batchSpotTrade.userCThirdTokenExchange);

    // console.log("D firstTokenExchange:" + batchSpotTrade.userDFirstTokenExchange);
    // console.log("D secondTokenExchange:" + batchSpotTrade.userDSecondTokenExchange);
    // console.log("D thirdTokenExchange:" + batchSpotTrade.userDThirdTokenExchange);

    //tokenType
    const tokenTypes = this.feedBatchSpotTradeTokenType(da, batchSpotTrade);

    da.print();
    // da.addNumber(batchSpotTrade.userBAccountID ? batchSpotTrade.userBAccountID : 0, 4);
    da.addNumberForBits(batchSpotTrade.userBAccountID ? batchSpotTrade.userBAccountID : 0, 32);
    this.feedBatchSpotTradeOtherUserAmountExchangePublicData(
      da,
      tokenTypes[0],
      batchSpotTrade.userBFirstTokenExchange,
      batchSpotTrade.userBSecondTokenExchange,
      batchSpotTrade.userBThirdTokenExchange
    );
    da.print();

    // da.addNumber(batchSpotTrade.userCAccountID ? batchSpotTrade.userCAccountID : 0, 4);
    da.addNumberForBits(batchSpotTrade.userCAccountID ? batchSpotTrade.userCAccountID : 0, 32);
    this.feedBatchSpotTradeOtherUserAmountExchangePublicData(
      da,
      tokenTypes[1],
      batchSpotTrade.userCFirstTokenExchange,
      batchSpotTrade.userCSecondTokenExchange,
      batchSpotTrade.userCThirdTokenExchange
    );
    da.print();

    // da.addNumber(batchSpotTrade.userDAccountID ? batchSpotTrade.userDAccountID : 0, 4);
    da.addNumberForBits(batchSpotTrade.userDAccountID ? batchSpotTrade.userDAccountID : 0, 32);
    this.feedBatchSpotTradeOtherUserAmountExchangePublicData(
      da,
      tokenTypes[2],
      batchSpotTrade.userDFirstTokenExchange,
      batchSpotTrade.userDSecondTokenExchange,
      batchSpotTrade.userDThirdTokenExchange
    );
    da.print();

    // da.addNumber(batchSpotTrade.userEAccountID ? batchSpotTrade.userEAccountID : 0, 4);
    da.addNumberForBits(batchSpotTrade.userEAccountID ? batchSpotTrade.userEAccountID : 0, 32);
    this.feedBatchSpotTradeOtherUserAmountExchangePublicData(
      da,
      tokenTypes[3],
      batchSpotTrade.userEFirstTokenExchange,
      batchSpotTrade.userESecondTokenExchange,
      batchSpotTrade.userEThirdTokenExchange
    );
    da.print();

    // da.addNumber(batchSpotTrade.userFAccountID ? batchSpotTrade.userFAccountID : 0, 4);
    da.addNumberForBits(batchSpotTrade.userFAccountID ? batchSpotTrade.userFAccountID : 0, 32);
    this.feedBatchSpotTradeOtherUserAmountExchangePublicData(
      da,
      tokenTypes[4],
      batchSpotTrade.userFFirstTokenExchange,
      batchSpotTrade.userFSecondTokenExchange,
      batchSpotTrade.userFThirdTokenExchange
    );
    da.print();

    // da.addNumber(batchSpotTrade.userAAccountID ? batchSpotTrade.userAAccountID : 0, 4);
    da.addNumberForBits(batchSpotTrade.userAAccountID ? batchSpotTrade.userAAccountID : 0, 32);
    this.addFloatDataForBatchSpotTrade(da, batchSpotTrade.userAFirstTokenExchange);
    this.addFloatDataForBatchSpotTrade(da, batchSpotTrade.userASecondTokenExchange);
    this.addFloatDataForBatchSpotTrade(da, batchSpotTrade.userAThirdTokenExchange);
    da.print();
  }

  public getBlockData(block: any, numConditionalTransactions: number) {
    // Pack the data that needs to be committed onchain
    const bs = new Bitstream();
    bs.addBN(new BN(block.exchange), 20);
    bs.addBN(new BN(block.merkleRootBefore, 10), 32);
    bs.addBN(new BN(block.merkleRootAfter, 10), 32);
    bs.addBN(new BN(block.merkleAssetRootBefore, 10), 32);
    bs.addBN(new BN(block.merkleAssetRootAfter, 10), 32);
    bs.addNumber(block.timestamp, 4);
    // bs.addNumber(block.protocolTakerFeeBips, 1);
    // bs.addNumber(block.protocolMakerFeeBips, 1);
    bs.addNumber(block.protocolFeeBips, 1);
    // //refeUrefe
    // bs.addNumber(block.referFeeBips, 1);
    // bs.addNumber(block.uiFeeBips, 1);
    bs.addNumber(numConditionalTransactions, 4);
    bs.addNumber(block.operatorAccountID, 4);
    //Deposit、AccountUpdate、Withdra
    var depositSize = 0;
    var accountUpdateSize = 0;
    var withdrawSize = 0;
    for (const tx of block.transactions) {
      if (tx.deposit || tx.txType === "Deposit") {
        depositSize++;
      } else if (tx.withdraw || tx.txType === "Withdraw") {
        withdrawSize++;
      } else if (tx.accountUpdate || tx.txType === "AccountUpdate") {
        accountUpdateSize++;
      }
    }
    bs.addNumber(depositSize, 2);
    bs.addNumber(accountUpdateSize, 2);
    bs.addNumber(withdrawSize, 2);
    const allDa = new Bitstream();
    for (const tx of block.transactions) {
      //console.log(tx);
      const da = new Bitstream();
      if (tx.noop || tx.txType === "Noop") {
        // da.addNumber(TransactionType.NOOP, 1);
        // typ3bit1bit
        // 000 0
        // 001 0 = 1 * 2
        // 010 0 = 2 * 2
        const typeNum = TransactionType.NOOP * 2;
        da.addNumberWithChar(typeNum, 1);
      } else if (tx.spotTrade || tx.txType === "SpotTrade") {
        const spotTrade = tx.spotTrade ? tx.spotTrade : tx;
        const orderA = spotTrade.orderA;
        const orderB = spotTrade.orderB;

        // da.addNumber(TransactionType.SPOT_TRADE, 1);
        // typ3bit1bit
        // 000 0
        // 001 0 = 1 * 2
        // 010 0 = 2 * 2
        const typeNum = TransactionType.SPOT_TRADE * 2;
        da.addNumberWithChar(typeNum, 1);
        // da.addNumber(orderA.storageID, 4);
        // da.addNumber(orderB.storageID, 4);
        da.addNumber(orderA.accountID, 4);
        da.addNumber(orderB.accountID, 4);
        // da.addNumber(orderA.tokenIdS ? orderA.tokenIdS : orderA.tokenS, 2);
        // da.addNumber(orderB.tokenIdS ? orderB.tokenIdS : orderB.tokenS, 2);
        da.addNumber(orderA.tokenIdS ? orderA.tokenIdS : orderA.tokenS, 4);
        da.addNumber(orderB.tokenIdS ? orderB.tokenIdS : orderB.tokenS, 4);
        // da.addNumber(spotTrade.fFillS_A ? spotTrade.fFillS_A : 0, 3);
        // da.addNumber(spotTrade.fFillS_B ? spotTrade.fFillS_B : 0, 3);
        da.addNumber(spotTrade.fFillS_A ? spotTrade.fFillS_A : 0, 4);
        da.addNumber(spotTrade.fFillS_B ? spotTrade.fFillS_B : 0, 4);

        //trading fegas fee
        da.addNumber(orderA.feeTokenID, 4);
        da.addNumber(toFloat(new BN(orderA.fee), Constants.Float16Encoding), 2);
        // //refeUrefe
        // // UI refer Account ID
        // da.addNumber(orderA.uiReferID);

        da.addNumber(orderB.feeTokenID, 4);
        da.addNumber(toFloat(new BN(orderB.fee), Constants.Float16Encoding), 2);
        // //refeUrefe
        // // UI refer Account ID
        // da.addNumber(orderB.uiReferID);

        //：10000000 = 128
        let limitMask = orderA.fillAmountBorS ? 0b10000000 : 0;
        let feeData = orderA.feeBips >= 64 ? 64 + orderA.feeBips / Constants.FEE_MULTIPLIER : orderA.feeBips;
        da.addNumber(limitMask + feeData, 1);

        limitMask = orderB.fillAmountBorS ? 0b10000000 : 0;
        //feeBip641feeBips / 50
        //orderB.feeBips / 50 * 50 == feeBipsfeeBip5
        feeData = orderB.feeBips >= 64 ? 64 + orderB.feeBips / Constants.FEE_MULTIPLIER : orderB.feeBips;
        da.addNumber(limitMask + feeData, 1);

        // // autoMarket
        // da.addNumber(orderA.autoMarketOrder, 1);
        // da.addNumber(orderB.autoMarketOrder, 1);
        // // autoMarketID max = 3byte
        // da.addNumber(orderA.autoMarketID, 3);
        // da.addNumber(orderB.autoMarketID, 3);
        // da.addNumber(orderA.autoMarketLevel, 1);
        // da.addNumber(orderB.autoMarketLevel, 1);
      } else if (tx.batchSpotTrade || tx.txType === "BatchSpotTrade") {
        console.log("in getBlockData BatchSpotTrade");
        var batchSpotTrade = tx.batchSpotTrade ? tx.batchSpotTrade : tx;
        // bind tokenIDtyp1byt，typ3bit，bindTokenIbits
        // 000 00000
        // 001 00000 = 1 * 32
        // 010 00000 = 2 * 32
        // 011 00000 = 3 * 32
        const typeNum = TransactionType.BATCH_SPOT_TRADE * 32;
        console.log("typeNum:" + typeNum + ";batchSpotTrade.bindTokenID:" + batchSpotTrade.bindTokenID);
        var dataBitEx = new BitstreamEx();
        // da.addNumber(typeNum + batchSpotTrade.bindTokenID, 1);
        dataBitEx.addNumberForBits(typeNum + batchSpotTrade.bindTokenID, 8);

        this.feedBatchSpotTradePublicData(dataBitEx, batchSpotTrade, batchSpotTrade.bindTokenID);
        dataBitEx.print();
        console.log("BatchSpotTrade Public Data:" + dataBitEx.toHex(Constants.TX_DATA_AVAILABILITY_SIZE));

        da.addHex(dataBitEx.toHex(Constants.TX_DATA_AVAILABILITY_SIZE));
        console.log("BatchSpotTrade Public Data:" + da.getData());
      } else if (tx.transfer || tx.txType === "Transfer") {
        const transfer = tx.transfer ? tx.transfer : tx;
        // da.addNumber(TransactionType.TRANSFER, 1);
        // typ3bit1bit
        // 000 0
        // 001 0 = 1 * 2
        // 010 0 = 2 * 2
        const typeNum = TransactionType.TRANSFER * 2;
        da.addNumberWithChar(typeNum, 1);
        da.addNumber(transfer.type, 1);
        da.addNumber(transfer.fromAccountID, 4);
        da.addNumber(transfer.toAccountID, 4);
        // da.addNumber(transfer.tokenID, 2);
        da.addNumber(transfer.tokenID, 4);
        // da.addNumber(
        //   toFloat(new BN(transfer.amount), Constants.Float24Encoding),
        //   3
        // );
        da.addNumber(toFloat(new BN(transfer.amount), Constants.Float32Encoding), 4);
        // da.addNumber(transfer.feeTokenID, 2);
        da.addNumber(transfer.feeTokenID, 4);
        da.addNumber(toFloat(new BN(transfer.fee), Constants.Float16Encoding), 2);
        da.addNumber(transfer.storageID, 4);
        const needsToAddress = transfer.type > 0 || transfer.toNewAccount || transfer.putAddressesInDA;
        da.addBN(new BN(needsToAddress ? transfer.to : "0"), 20);
        const needsFromAddress = transfer.type > 0 || transfer.putAddressesInDA;
        da.addBN(new BN(needsFromAddress ? transfer.from : "0"), 20);

        da.addNumber(transfer.useAppKey, 1);
      } else if (tx.withdraw || tx.txType === "Withdraw") {
        const withdraw = tx.withdraw ? tx.withdraw : tx;
        // da.addNumber(TransactionType.WITHDRAWAL, 1);
        da.addNumber(withdraw.type, 1);
        da.addBN(new BN(withdraw.owner), 20);
        da.addNumber(withdraw.accountID, 4);
        // da.addNumber(withdraw.tokenID, 2);
        da.addNumber(withdraw.tokenID, 4);
        // // da.addBN(new BN(withdraw.amount), 12);
        // // da.addNumber(withdraw.feeTokenID, 2);
        da.addNumber(withdraw.feeTokenID, 4);
        da.addNumber(toFloat(new BN(withdraw.fee), Constants.Float16Encoding), 2);
        da.addNumber(withdraw.storageID, 4);
        da.addBN(new BN(withdraw.onchainDataHash), 20);
        da.addNumber(withdraw.useAppKey, 1);
      } else if (tx.deposit || tx.txType === "Deposit") {
        const deposit = tx.deposit ? tx.deposit : tx;
        // da.addNumber(TransactionType.DEPOSIT, 1);
        da.addNumber(deposit.type, 1);
        da.addBN(new BN(deposit.owner), 20);
        da.addNumber(deposit.accountID, 4);
        // da.addNumber(deposit.tokenID, 2);
        da.addNumber(deposit.tokenID, 4);
        // da.addBN(new BN(deposit.amount), 12);
        da.addBN(new BN(deposit.amount), 31);
      } else if (tx.accountUpdate || tx.txType === "AccountUpdate") {
        const update = tx.accountUpdate ? tx.accountUpdate : tx;

        console.log("getBlockData::accountUpdate:", update);
        let pub = new BN(EdDSA.pack(update.publicKeyX, update.publicKeyY), 16);
        console.log("getBlockData::accountUpdate pub:", pub.toString(10));

        // da.addNumber(TransactionType.ACCOUNT_UPDATE, 1);
        da.addNumber(update.type, 1);
        da.addBN(new BN(update.owner), 20);
        // da.addNumber(update.accountID, 4);
        //accoun0
        if (update.nonce == 0) {
          da.addNumber(0, 4);
        } else {
          da.addNumber(update.accountID, 4);
        }
        // da.addNumber(update.feeTokenID, 2);
        da.addNumber(update.feeTokenID, 4);
        da.addNumber(toFloat(new BN(update.fee), Constants.Float16Encoding), 2);
        da.addBN(new BN(EdDSA.pack(update.publicKeyX, update.publicKeyY), 16), 32);
        da.addNumber(update.nonce, 4);
        // //refe-refe；referIpublic data
        // da.addNumber(update.referID, 4);
      } else if (tx.orderCancel || tx.txType === "OrderCancel") {
        const update = tx.orderCancel ? tx.orderCancel : tx;
        // da.addNumber(TransactionType.ORDER_CANCEL, 1);
        // typ3bit1bit
        // 000 0
        // 001 0 = 1 * 2
        // 010 0 = 2 * 2
        const typeNum = TransactionType.ORDER_CANCEL * 2;
        da.addNumberWithChar(typeNum, 1);
        // da.addBN(new BN(update.owner), 20);
        da.addNumber(update.accountID, 4);
        // da.addNumber(update.tokenID, 4);
        da.addNumber(update.storageID, 4);
        da.addNumber(update.feeTokenID, 4);
        da.addNumber(toFloat(new BN(update.fee), Constants.Float16Encoding), 2);
        da.addNumber(update.useAppKey, 1);
        console.log("getBlockData::orderCancel", update);
      } else if (tx.appKeyUpdate || tx.txType === "AppKeyUpdate") {
        const update = tx.appKeyUpdate ? tx.appKeyUpdate : tx;
        // typ3bit1bit
        // 000 0
        // 001 0 = 1 * 2
        // 010 0 = 2 * 2
        const typeNum = TransactionType.APPKEY_UPDATE * 2;
        da.addNumberWithChar(typeNum, 1);

        console.log("getBlockData::appKeyUpdate:", update);
        // let pub = new BN(EdDSA.pack(update.appKeyPublicKeyX, update.appKeyPublicKeyY), 16);
        // console.log("getBlockData::appKeyUpdate pub:", pub.toString(10));

        // da.addBN(new BN(update.owner), 20);
        da.addNumber(update.accountID, 4);
        da.addNumber(update.feeTokenID, 4);
        da.addNumber(toFloat(new BN(update.fee), Constants.Float16Encoding), 2);
        // da.addBN(
        //   new BN(EdDSA.pack(update.appKeyPublicKeyX, update.appKeyPublicKeyY), 16),
        //   32
        // );
        da.addNumber(update.nonce, 4);
      }
      // else if (tx.autoMarketUpdate || tx.txType === "AutoMarketUpdate") {
      //   const update = tx.autoMarketUpdate ? tx.autoMarketUpdate : tx;

      //   // console.log("AutoMarketUpdate-====++++++, accountID:" + update.accountID);
      //   // console.log("AutoMarketUpdate-====++++++, tokenSIDTest:" + update.tokenSIDTest);
      //   // console.log("AutoMarketUpdate-====++++++, tokenBID:" + update.tokenBID);
      //   console.log("AutoMarketUpdate-====++++++, lowPrice:" + update.lowPrice);
      //   console.log("AutoMarketUpdate-====++++++, highPrice:" + update.highPrice);
      //   da.addNumber(TransactionType.AUTO_MARKET_UPDATE, 1);
      //   da.addNumber(update.accountID, 4);
      //   da.addNumber(update.autoMarketID, 3);
      //   da.addNumber(update.tokenSID, 4);
      //   da.addNumber(update.tokenBID, 4);
      //   da.addNumber(update.maxLevel, 1);
      //   // da.addNumber(
      //   //   toFloat(new BN(update.lowPrice), Constants.Float24Encoding),
      //   //   3
      //   // );
      //   // da.addNumber(
      //   //   toFloat(new BN(update.highPrice), Constants.Float24Encoding),
      //   //   3
      //   // );
      //   da.addNumber(
      //     toFloat(new BN(update.gridOffset), Constants.Float24Encoding),
      //     3
      //   );
      //   da.addNumber(
      //     toFloat(new BN(update.orderOffset), Constants.Float24Encoding),
      //     3
      //   );
      //   da.addNumber(update.cancelled, 1);
      //   da.addNumber(update.validUntil, 4);
      //   da.addNumber(update.feeTokenID, 4);
      //   da.addNumber(toFloat(new BN(update.fee), Constants.Float16Encoding), 2);
      // }
      // console.log("type: " + da.extractUint8(0));
      console.log("da.length(): " + da.length());
      assert(da.bitLength() <= Constants.TX_DATA_AVAILABILITY_SIZE * 2, "tx uses too much da");
      while (da.bitLength() < Constants.TX_DATA_AVAILABILITY_SIZE * 2) {
        // da.addNumber(0, 1);
        da.addNumberWithChar(0, 1);
      }
      allDa.addHex(da.getData());

      console.log("da length", da.length());
      console.log("da data", da.getData());

      // console.log('allDa222 length', allDa.length())
      // console.log('allDa222', allDa.getData())
    }

    // Transform DA
    const transformedDa = new Bitstream();
    const size = Constants.TX_DATA_AVAILABILITY_SIZE;
    // TODO 2，SpotTrade
    //，operatodeposi。
    //，toke71bytes
    const size1 = Constants.TX_DATA_AVAILABILITY_SIZE_PART_1;

    // const size2 = 39;
    // const size2 = 44;
    const size2 = Constants.TX_DATA_AVAILABILITY_SIZE_PART_2;
    assert.equal(size1 + size2, size, "invalid transform sizes");
    for (let i = 0; i < block.transactions.length; i++) {
      transformedDa.addHex(allDa.extractData(i * size, size1));
    }
    for (let i = 0; i < block.transactions.length; i++) {
      transformedDa.addHex(allDa.extractData(i * size + size1, size2));
    }
    bs.addHex(transformedDa.getData());

    // assert.equal(1, 2, "stop");
    return bs.getData();
  }

  public async registerToken(tokenAddress: string) {
    const tx = await this.exchange.registerToken(tokenAddress, {
      from: this.exchangeOwner
    });
    // logInfo("\x1b[46m%s\x1b[0m", "[TokenRegistration] Gas used: " + tx.receipt.gasUsed);
  }

  public async registerTokens() {
    for (const token of this.testContext.allTokens) {
      const tokenAddress = token === null ? Constants.zeroAddress : token.address;
      const symbol = this.testContext.tokenAddrSymbolMap.get(tokenAddress);
      // console.log(symbol + ": " + tokenAddress);

      if (symbol !== "ETH" && symbol !== "LRC") {
        await this.registerToken(tokenAddress);
      }
      await this.addTokenToMaps(tokenAddress);
    }
    // console.log(this.tokenIDMap);
  }

  public async addTokenToMaps(tokenAddress: string) {
    const tokenID = await this.getTokenID(tokenAddress);
    this.tokenAddressToIDMap.set(tokenAddress, tokenID);
    this.tokenIDToAddressMap.set(tokenID, tokenAddress);
  }

  public async getTokenID(token: string) {
    if (!token.startsWith("0x")) {
      token = this.testContext.tokenSymbolAddrMap.get(token);
    }
    const tokenID = await this.exchange.getTokenID(token);
    return tokenID.toNumber();
  }

  public getTokenAddressFromID(tokenID: number) {
    return this.tokenIDToAddressMap.get(tokenID);
  }

  public getAccountID(owner: string) {
    for (const account of this.accounts[this.exchangeId]) {
      if (account.owner === owner) {
        return account.accountID;
      }
    }
    return undefined;
  }

  public getAccount(accountId: number) {
    return this.accounts[this.exchangeId][accountId];
  }

  public findAccount(owner: string) {
    for (let i = 0; i < this.accounts[this.exchangeId].length; i++) {
      if (this.accounts[this.exchangeId][i].owner === owner) {
        return this.accounts[this.exchangeId][i];
      }
    }
    return undefined;
  }

  public async createExchange(owner: string, options: ExchangeOptions = {}) {
    const setupTestState = options.setupTestState !== undefined ? options.setupTestState : true;
    const deterministic = options.deterministic !== undefined ? options.deterministic : false;
    const useOwnerContract = options.useOwnerContract !== undefined ? options.useOwnerContract : true;

    this.deterministic = deterministic;

    const newExchange = await this.contracts.ExchangeV3.new();
    await newExchange.initialize(this.loopringV3.address, owner, this.emptyMerkleRoot, this.emptyMerkleAssetRoot);

    // const tx = await exchangePrototype.cloneExchange(
    //   owner,
    //   this.emptyMerkleRoot
    // );
    // logInfo(
    //   "\x1b[46m%s\x1b[0m",
    //   "[CreateExchange] Gas used: " + tx.receipt.gasUsed
    // );
    // const event = await this.assertEventEmitted(
    //   exchangePrototype,
    //   "ExchangeCloned"
    // );
    const exchangeAddress = newExchange.address;

    console.log("exchangeAddress: ", exchangeAddress);

    const stateMerkleRoot = await newExchange.getMerkleRoot();

    console.log("exchangeAddress stateMerkleRoot: ", stateMerkleRoot);
    this.exchange = newExchange;
    const exchangeId = this.exchangeIdGenerator++;

    await this.explorer.addExchange(this.exchange.address, owner);

    // Create a deposit contract impl
    const depositContractImpl = await this.contracts.DefaultDepositContract.new();
    // Create the proxy contract for the exchange using the implementation
    const depositContractProxy = await this.contracts.OwnedUpgradabilityProxy.new({ from: owner });
    await depositContractProxy.upgradeTo(depositContractImpl.address, {
      from: owner
    });
    // Wrap the proxy contract
    this.depositContract = await this.contracts.DefaultDepositContract.at(depositContractProxy.address);
    // Initialize the deposit contract
    await this.depositContract.initialize(this.exchange.address);

    // Set the deposit contract on the exchange
    await this.exchange.setDepositContract(this.depositContract.address, {
      from: owner
    });
    // Check the deposit contract
    const onchainDepositContract = await this.exchange.getDepositContract();
    assert.equal(onchainDepositContract, this.depositContract.address, "unexpected deposit contract");

    this.exchangeOwner = owner;
    this.exchangeOperator = owner;
    this.exchangeId = exchangeId;
    this.activeOperator = undefined;

    const exchangeCreationTimestamp = (await this.exchange.getBlockInfo(0)).timestamp;
    this.GENESIS_MERKLE_ROOT = new BN((await this.exchange.getMerkleRoot()).slice(2), 16);
    this.GENESIS_MERKLE_ASSET_ROOT = new BN((await this.exchange.getMerkleAssetRoot()).slice(2), 16);

    const genesisBlock: Block = {
      blockIdx: 0,
      filename: null,
      blockType: BlockType.UNIVERSAL,
      blockSize: 0,
      blockVersion: 0,
      operator: Constants.zeroAddress,
      origin: Constants.zeroAddress,
      operatorId: 0,
      merkleRoot: "0x" + this.GENESIS_MERKLE_ROOT.toString(16, 64),
      merkleAssetRoot: "0x" + this.GENESIS_MERKLE_ASSET_ROOT.toString(16, 64),
      data: "0x",
      auxiliaryData: [],
      offchainData: "0x",
      publicDataHash: "0",
      publicInput: "0",
      blockFee: new BN(0),
      timestamp: exchangeCreationTimestamp,
      internalBlock: undefined,
      transactionHash: Constants.zeroAddress
    };
    this.blocks[exchangeId] = [genesisBlock];

    if (setupTestState) {
      await this.registerTokens();
      await this.setupTestState(exchangeId);
    }

    // Deposit some LRC to stake for the exchange
    const depositer = this.testContext.operators[2];
    const stakeAmount = new BN(web3.utils.toWei("" + (1 + this.getRandomInt(1000))));
    await this.setBalanceAndApprove(depositer, "LRC", stakeAmount, this.loopringV3.address);

    // Stake it
    await this.loopringV3.depositExchangeStake(this.exchange.address, stakeAmount, {
      from: depositer
    });

    // Set the owner
    if (useOwnerContract) {
      const ownerContract = await LoopringIOExchangeOwner.new(this.exchange.address, { from: this.exchangeOwner });
      await this.setOperatorContract(ownerContract);

      await this.exchange.transferOwnership(ownerContract.address, {
        from: this.exchangeOwner
      });
      const txData = this.exchange.contract.methods.claimOwnership().encodeABI();
      await ownerContract.transact(txData, { from: this.exchangeOwner });
    }

    return exchangeId;
  }

  public async syncExplorer() {
    await this.explorer.sync(await web3.eth.getBlockNumber());
  }

  public getTokenAddress(token: string) {
    if (!token.startsWith("0x")) {
      token = this.testContext.tokenSymbolAddrMap.get(token);
    }
    return token;
  }

  public getTokenIdFromNameOrAddress(token: string) {
    if (!token.startsWith("0x")) {
      token = this.testContext.tokenSymbolAddrMap.get(token);
    }
    const tokenID = this.tokenAddressToIDMap.get(token);
    return tokenID;
  }

  public async createMerkleTreeInclusionProof(accountID: number, token: string) {
    const tokenID = this.getTokenIdFromNameOrAddress(token);

    await this.syncExplorer();
    const explorerExchange = this.explorer.getExchangeByAddress(this.exchange.address);
    explorerExchange.buildMerkleTreeForWithdrawalMode();
    return explorerExchange.getWithdrawFromMerkleTreeData(accountID, tokenID);
  }

  public async withdrawFromMerkleTreeWithProof(data: WithdrawFromMerkleTreeData) {
    const assetRoot = await this.exchange.getMerkleAssetRoot();
    console.log("withdrawFromMerkleTreeWithProof assetRoot", assetRoot);

    const tx = await this.exchange.withdrawFromMerkleTree(data);
    logInfo("\x1b[46m%s\x1b[0m", "[WithdrawFromMerkleTree] Gas used: " + tx.receipt.gasUsed);
  }

  public async withdrawFromMerkleTree(accountID: number, token: string) {
    const proof = await this.createMerkleTreeInclusionProof(accountID, token);
    await this.withdrawFromMerkleTreeWithProof(proof);
  }

  public async setBalanceAndApprove(owner: string, token: string, amount: BN, contractAddress?: string) {
    if (contractAddress === undefined) {
      contractAddress = this.depositContract.address;
    }
    const Token = await this.getTokenContract(token);
    if (owner !== this.testContext.deployer) {
      // Burn complete existing balance
      const existingBalance = await this.getOnchainBalance(owner, token);
      await Token.transfer(Constants.zeroAddress, existingBalance, {
        from: owner
      });
    }
    await Token.transfer(owner, amount, { from: this.testContext.deployer });
    await Token.approve(contractAddress, amount, { from: owner });
  }

  public async transferBalance(to: string, token: string, amount: BN) {
    const Token = await this.getTokenContract(token);
    await Token.transfer(to, amount, { from: this.testContext.deployer });
  }

  public evmIncreaseTime(seconds: number) {
    return new Promise((resolve, reject) => {
      web3.currentProvider.send(
        {
          jsonrpc: "2.0",
          method: "evm_increaseTime",
          params: [seconds]
        },
        (err: any, res: any) => {
          return err ? reject(err) : resolve(res);
        }
      );
    });
  }

  public async getMerkleRootOnchain() {
    return await this.exchange.getMerkleRoot();
  }

  public async getNumBlocksOnchain() {
    return (await this.exchange.getBlockHeight()).toNumber();
  }

  public evmMine() {
    return new Promise((resolve, reject) => {
      web3.currentProvider.send(
        {
          jsonrpc: "2.0",
          method: "evm_mine",
          id: Date.now()
        },
        (err: any, res: any) => {
          return err ? reject(err) : resolve(res);
        }
      );
    });
  }

  public async advanceBlockTimestamp(seconds: number) {
    const previousTimestamp = (await web3.eth.getBlock(await web3.eth.getBlockNumber())).timestamp;
    await this.evmIncreaseTime(seconds);
    await this.evmMine();
    const currentTimestamp = (await web3.eth.getBlock(await web3.eth.getBlockNumber())).timestamp;
    assert(
      Math.abs(currentTimestamp - (previousTimestamp + seconds)) < 60,
      "Timestamp should have been increased by roughly the expected value"
    );
  }

  public async getOffchainBalance(owner: string, token: string) {
    const accountID = this.getAccountID(owner);
    const tokenID = this.getTokenIdFromNameOrAddress(token);
    const latestBlockIdx = this.blocks[this.exchangeId].length - 1;
    const state = await Simulator.loadExchangeState(this.exchangeId, latestBlockIdx);
    try {
      return state.accounts[accountID].balances[tokenID].balance;
    } catch {
      return new BN(0);
    }
  }

  public async getTokenContract(token: string) {
    if (!token.startsWith("0x")) {
      token = this.testContext.tokenSymbolAddrMap.get(token);
    }
    return await this.contracts.DummyToken.at(token);
  }

  public async getOnchainBalance(owner: string, token: string) {
    if (!token.startsWith("0x")) {
      token = this.testContext.tokenSymbolAddrMap.get(token);
    }
    if (token === Constants.zeroAddress) {
      return new BN(await web3.eth.getBalance(owner));
    } else {
      const Token = await this.contracts.DummyToken.at(token);
      return await Token.balanceOf(owner);
    }
  }

  public async checkOffchainBalance(
    account: number | string,
    token: number | string,
    expectedBalance: BN,
    desc: string
  ) {
    let accountID: number;
    if (typeof account === "number") {
      accountID = account;
    } else {
      accountID = this.findAccount(account).accountID;
    }

    let tokenID: number;
    if (typeof token === "number") {
      tokenID = token;
    } else {
      tokenID = await this.getTokenID(token);
    }

    const balance = await this.getOffchainBalance(
      this.getAccount(accountID).owner,
      this.getTokenAddressFromID(tokenID)
    );
    assert(
      balance.eq(expectedBalance),
      desc + ". " + balance.toString(10) + " but expected " + expectedBalance.toString(10)
    );
  }

  public async randomizeWithdrawalFee() {
    await this.loopringV3.updateSettings(
      await this.loopringV3.protocolFeeVault(),
      await this.loopringV3.blockVerifierAddress(),
      this.getRandomFee(),
      { from: this.testContext.deployer }
    );
  }

  public async doRandomDeposit(ownerIndex?: number) {
    const orderOwners = this.testContext.orderOwners;
    ownerIndex = ownerIndex !== undefined ? ownerIndex : this.getRandomInt(orderOwners.length);
    const owner = orderOwners[Number(ownerIndex)];
    const amount = this.getRandomAmount();
    const token = this.getTokenAddress("LRC");
    return await this.deposit(owner, owner, token, amount);
  }

  public async doRandomOnchainWithdrawal(deposit: Deposit) {
    return await this.requestWithdrawal(deposit.owner, deposit.token, this.getRandomAmount(), "ETH", new BN(0), {
      authMethod: AuthMethod.FORCE
    });
  }

  public async doRandomOffchainWithdrawal(deposit: Deposit) {
    assert(false);
    this.requestWithdrawal(deposit.owner, deposit.token, this.getRandomAmount(), "LRC", new BN(0));
  }

  public shuffle(a: any[]) {
    for (let i = a.length - 1; i > 0; i--) {
      const j = Math.floor(Math.random() * (i + 1));
      [a[i], a[j]] = [a[j], a[i]];
    }
    return a;
  }

  public async checkExplorerState() {
    // Get the current state
    const numBlocksOnchain = this.blocks[this.exchangeId].length;
    const state = await Simulator.loadExchangeState(this.exchangeId, numBlocksOnchain - 1);

    console.log("this.syncExplorer", this.exchange.address);
    await this.syncExplorer();
    const exchange = this.explorer.getExchangeByAddress(this.exchange.address);

    // Compare accounts
    console.log("Compare accounts");
    assert.equal(exchange.getNumAccounts(), state.accounts.length, "number of accounts does not match");
    for (let accountID = 0; accountID < state.accounts.length; accountID++) {
      const accountA = state.accounts[accountID];
      const accountB = exchange.getAccount(accountID);
      Simulator.compareAccounts(accountA, accountB);
    }

    // Compare blocks
    console.log("Compare blocks");
    assert.equal(exchange.getNumBlocks(), this.blocks[this.exchangeId].length, "number of blocks does not match");
    for (let blockIdx = 0; blockIdx < exchange.getNumBlocks(); blockIdx++) {
      console.log("Testing blockIdx: " + blockIdx);
      const explorerBlock = exchange.getBlock(blockIdx);
      const testBlock = this.blocks[this.exchangeId][blockIdx];
      assert.equal(explorerBlock.exchange, this.exchange.address, "unexpected exchangeId");
      assert.equal(explorerBlock.blockIdx, testBlock.blockIdx, "unexpected blockIdx");
      assert.equal(explorerBlock.blockType, testBlock.blockType, "unexpected blockType");
      assert.equal(explorerBlock.blockVersion, testBlock.blockVersion, "unexpected blockVersion");
      assert.equal(explorerBlock.data, testBlock.data, "unexpected data");
      assert.equal(explorerBlock.offchainData, testBlock.offchainData, "unexpected offchainData");
      /*assert.equal(
        explorerBlock.operator,
        testBlock.operator,
        "unexpected operator"
      );
      assert.equal(explorerBlock.origin, testBlock.origin, "unexpected origin");
      assert(
        explorerBlock.blockFee.eq(testBlock.blockFee),
        "unexpected blockFee"
      );*/
      assert.equal(explorerBlock.timestamp, testBlock.timestamp, "unexpected timestamp");
      assert.equal(explorerBlock.transactionHash, testBlock.transactionHash, "unexpected transactionHash");
    }

    // Compare deposits
    /*assert.equal(
      exchange.getNumDeposits(),
      this.deposits[this.exchangeId].length,
      "number of deposits does not match"
    );
    for (
      let depositIdx = 0;
      depositIdx < exchange.getNumDeposits();
      depositIdx++
    ) {
      const explorerDeposit = exchange.getDeposit(depositIdx);
      const testDeposit = this.deposits[this.exchangeId][depositIdx];
      assert.equal(
        explorerDeposit.exchangeId,
        testDeposit.exchangeId,
        "unexpected exchangeId"
      );
      assert.equal(
        explorerDeposit.depositIdx,
        testDeposit.depositIdx,
        "unexpected depositIdx"
      );
      assert.equal(
        explorerDeposit.timestamp,
        testDeposit.timestamp,
        "unexpected timestamp"
      );
      assert.equal(
        explorerDeposit.accountID,
        testDeposit.accountID,
        "unexpected accountID"
      );
      assert.equal(
        explorerDeposit.tokenID,
        testDeposit.tokenID,
        "unexpected tokenID"
      );
      assert(
        explorerDeposit.amount.eq(testDeposit.amount),
        "unexpected amount"
      );
      assert.equal(
        explorerDeposit.publicKeyX,
        testDeposit.publicKeyX,
        "unexpected publicKeyX"
      );
      assert.equal(
        explorerDeposit.publicKeyY,
        testDeposit.publicKeyY,
        "unexpected publicKeyY"
      );
    }

    // Compare on-chain withdrawal requests
    assert.equal(
      exchange.getNumOnchainWithdrawalRequests(),
      this.onchainWithdrawals[this.exchangeId].length,
      "number of on-chain withdrawals does not match"
    );
    for (
      let withdrawalIdx = 0;
      withdrawalIdx < exchange.getNumOnchainWithdrawalRequests();
      withdrawalIdx++
    ) {
      const explorerWithdrawal = exchange.getOnchainWithdrawalRequest(
        withdrawalIdx
      );
      const testWithdrawal = this.onchainWithdrawals[this.exchangeId][
        withdrawalIdx
      ];
      assert.equal(
        explorerWithdrawal.exchangeId,
        this.exchangeId,
        "unexpected exchangeId"
      );
      assert.equal(
        explorerWithdrawal.withdrawalIdx,
        testWithdrawal.withdrawalIdx,
        "unexpected withdrawalIdx"
      );
      assert.equal(
        explorerWithdrawal.timestamp,
        testWithdrawal.timestamp,
        "unexpected timestamp"
      );
      assert.equal(
        explorerWithdrawal.accountID,
        testWithdrawal.accountID,
        "unexpected accountID"
      );
      assert.equal(
        explorerWithdrawal.tokenID,
        testWithdrawal.tokenID,
        "unexpected tokenID"
      );
      assert(
        explorerWithdrawal.amountRequested.eq(testWithdrawal.amount),
        "unexpected amountRequested"
      );
    }*/
  }

  public getRandomInt(max: number) {
    return Math.floor(Math.random() * max);
  }

  public getRandomBool() {
    return this.deterministic ? false : this.getRandomInt(1000) >= 500;
  }

  public getRandomAmount() {
    return new BN(web3.utils.toWei("" + this.getRandomInt(100000000) / 1000));
  }

  public getRandomSmallAmount() {
    return new BN(web3.utils.toWei("" + this.getRandomInt(1000) / 1000));
  }

  public getRandomFee() {
    return this.deterministic ? new BN(0) : new BN(web3.utils.toWei("" + this.getRandomInt(10000) / 1000000));
  }

  public async depositExchangeStakeChecked(amount: BN, owner: string) {
    const snapshot = new BalanceSnapshot(this);
    await snapshot.transfer(owner, this.loopringV3.address, "LRC", amount, "owner", "loopringV3");

    const stakeBefore = await this.exchange.getExchangeStake();
    const totalStakeBefore = await this.loopringV3.totalStake();

    await this.loopringV3.depositExchangeStake(this.exchange.address, amount, {
      from: owner
    });

    await snapshot.verifyBalances();

    const stakeAfter = await this.exchange.getExchangeStake();
    const totalStakeAfter = await this.loopringV3.totalStake();

    assert(stakeAfter.eq(stakeBefore.add(amount)), "Stake should be increased by amount");
    assert(totalStakeAfter.eq(totalStakeBefore.add(amount)), "Total stake should be increased by amount");

    // Get the ExchangeStakeDeposited event
    const event = await this.assertEventEmitted(this.loopringV3, "ExchangeStakeDeposited");
    assert.equal(event.exchangeAddr, this.exchange.address, "exchange should match");
    assert(event.amount.eq(amount), "amount should match");
  }

  public async withdrawExchangeStakeChecked(recipient: string, amount: BN) {
    const snapshot = new BalanceSnapshot(this);
    await snapshot.transfer(this.loopringV3.address, recipient, "LRC", amount, "loopringV3", "recipient");

    const stakeBefore = await this.exchange.getExchangeStake();
    const totalStakeBefore = await this.loopringV3.totalStake();

    await this.exchange.withdrawExchangeStake(recipient, {
      from: this.exchangeOwner
    });

    await snapshot.verifyBalances();

    const stakeAfter = await this.exchange.getExchangeStake();
    const totalStakeAfter = await this.loopringV3.totalStake();

    assert(stakeBefore.eq(stakeAfter.add(amount)), "Stake should be decreased by amount");
    assert(totalStakeAfter.eq(totalStakeBefore.sub(amount)), "Total stake should be decreased by amount");

    // Get the ExchangeStakeWithdrawn event
    const event = await this.assertEventEmitted(this.loopringV3, "ExchangeStakeWithdrawn");
    assert.equal(event.exchangeAddr, this.exchange.address, "exchange should match");
    assert(event.amount.eq(amount), "amount should match");
  }

  public async depositProtocolFeeStakeChecked(amount: BN, owner: string) {
    const snapshot = new BalanceSnapshot(this);
    await snapshot.transfer(owner, this.loopringV3.address, "LRC", amount, "owner", "loopringV3");

    const stakeBefore = await this.loopringV3.getProtocolFeeStake(this.exchangeId);
    const totalStakeBefore = await this.loopringV3.totalStake();

    await this.loopringV3.depositProtocolFeeStake(this.exchangeId, amount, {
      from: owner
    });

    await snapshot.verifyBalances();

    const stakeAfter = await this.loopringV3.getProtocolFeeStake(this.exchangeId);
    const totalStakeAfter = await this.loopringV3.totalStake();

    assert(stakeAfter.eq(stakeBefore.add(amount)), "Stake should be increased by amount");
    assert(totalStakeAfter.eq(totalStakeBefore.add(amount)), "Total stake should be increased by amount");

    // Get the ProtocolFeeStakeDeposited event
    const event = await this.assertEventEmitted(this.loopringV3, "ProtocolFeeStakeDeposited");
    assert.equal(event.exchangeId.toNumber(), this.exchangeId, "exchangeId should match");
    assert(event.amount.eq(amount), "amount should match");
  }

  public async withdrawProtocolFeeStakeChecked(recipient: string, amount: BN) {
    const snapshot = new BalanceSnapshot(this);
    await snapshot.transfer(this.loopringV3.address, recipient, "LRC", amount, "loopringV3", "recipient");

    const stakeBefore = await this.loopringV3.getProtocolFeeStake(this.exchangeId);
    const totalStakeBefore = await this.loopringV3.totalStake();

    await this.exchange.withdrawProtocolFeeStake(recipient, amount, {
      from: this.exchangeOwner
    });

    await snapshot.verifyBalances();

    const stakeAfter = await this.loopringV3.getProtocolFeeStake(this.exchangeId);
    const totalStakeAfter = await this.loopringV3.totalStake();

    assert(stakeBefore.eq(stakeAfter.add(amount)), "Stake should be decreased by amount");
    assert(totalStakeAfter.eq(totalStakeBefore.sub(amount)), "Total stake should be decreased by amount");

    // Get the ProtocolFeeStakeWithdrawn event
    const event = await this.assertEventEmitted(this.loopringV3, "ProtocolFeeStakeWithdrawn");
    assert.equal(event.exchangeId.toNumber(), this.exchangeId, "exchangeId should match");
    assert(event.amount.eq(amount), "amount should match");
  }

  // private functions:
  private async createContractContext() {
    const [loopringV3, exchange, blockVerifier, lrcToken, wethToken] = await Promise.all([
      this.contracts.LoopringV3.deployed(),
      this.contracts.ExchangeV3.deployed(),
      this.contracts.BlockVerifier.deployed(),
      this.contracts.LRCToken.deployed(),
      this.contracts.WETHToken.deployed()
    ]);

    const [protocolFeeVaultContract] = await Promise.all([
      // this.contracts.UserStakingPool.deployed(),
      this.contracts.ProtocolFeeVault.deployed()
    ]);

    // this.userStakingPool = userStakingPool;
    this.protocolFeeVaultContract = protocolFeeVaultContract;

    this.loopringV3 = loopringV3;
    this.exchange = exchange;
    this.blockVerifier = blockVerifier;

    this.lrcAddress = lrcToken.address;
    this.wethAddress = wethToken.address;

    const currBlockNumber = await web3.eth.getBlockNumber();
    const currBlockTimestamp = (await web3.eth.getBlock(currBlockNumber)).timestamp;
    return new Context(currBlockNumber, currBlockTimestamp, lrcToken.address);
  }

  private async createExchangeTestContext(accounts: string[]) {
    const tokenSymbolAddrMap = new Map<string, string>();
    const tokenAddrSymbolMap = new Map<string, string>();
    const tokenAddrDecimalsMap = new Map<string, number>();
    const tokenAddrInstanceMap = new Map<string, any>();

    const [eth, weth, lrc, gto, rdn, rep, inda, indb, test] = await Promise.all([
      null,
      this.contracts.WETHToken.deployed(),
      this.contracts.LRCToken.deployed(),
      this.contracts.GTOToken.deployed(),
      this.contracts.RDNToken.deployed(),
      this.contracts.REPToken.deployed(),
      this.contracts.INDAToken.deployed(),
      this.contracts.INDBToken.deployed(),
      this.contracts.TESTToken.deployed()
    ]);

    const allTokens = [eth, weth, lrc, gto, rdn, rep, inda, indb, test];

    tokenSymbolAddrMap.set("ETH", Constants.zeroAddress);
    tokenSymbolAddrMap.set("WETH", this.contracts.WETHToken.address);
    tokenSymbolAddrMap.set("LRC", this.contracts.LRCToken.address);
    tokenSymbolAddrMap.set("GTO", this.contracts.GTOToken.address);
    tokenSymbolAddrMap.set("RDN", this.contracts.RDNToken.address);
    tokenSymbolAddrMap.set("REP", this.contracts.REPToken.address);
    tokenSymbolAddrMap.set("INDA", this.contracts.INDAToken.address);
    tokenSymbolAddrMap.set("INDB", this.contracts.INDBToken.address);
    tokenSymbolAddrMap.set("TEST", this.contracts.TESTToken.address);

    for (const token of allTokens) {
      if (token === null) {
        tokenAddrDecimalsMap.set(Constants.zeroAddress, 18);
      } else {
        tokenAddrDecimalsMap.set(token.address, await token.decimals());
      }
    }

    tokenAddrSymbolMap.set(Constants.zeroAddress, "ETH");
    tokenAddrSymbolMap.set(this.contracts.WETHToken.address, "WETH");
    tokenAddrSymbolMap.set(this.contracts.LRCToken.address, "LRC");
    tokenAddrSymbolMap.set(this.contracts.GTOToken.address, "GTO");
    tokenAddrSymbolMap.set(this.contracts.RDNToken.address, "RDN");
    tokenAddrSymbolMap.set(this.contracts.REPToken.address, "REP");
    tokenAddrSymbolMap.set(this.contracts.INDAToken.address, "INDA");
    tokenAddrSymbolMap.set(this.contracts.INDBToken.address, "INDB");
    tokenAddrSymbolMap.set(this.contracts.TESTToken.address, "TEST");

    tokenAddrInstanceMap.set(Constants.zeroAddress, null);
    tokenAddrInstanceMap.set(this.contracts.WETHToken.address, weth);
    tokenAddrInstanceMap.set(this.contracts.LRCToken.address, lrc);
    tokenAddrInstanceMap.set(this.contracts.GTOToken.address, gto);
    tokenAddrInstanceMap.set(this.contracts.RDNToken.address, rdn);
    tokenAddrInstanceMap.set(this.contracts.REPToken.address, rep);
    tokenAddrInstanceMap.set(this.contracts.INDAToken.address, inda);
    tokenAddrInstanceMap.set(this.contracts.INDBToken.address, indb);
    tokenAddrInstanceMap.set(this.contracts.TESTToken.address, test);

    const deployer = accounts[0];
    const stateOwners = accounts.slice(1, 5);
    const operators = accounts.slice(5, 10);
    const orderOwners = accounts.slice(10, 40);
    const wallets = accounts.slice(40, 50);

    return new ExchangeTestContext(
      deployer,
      stateOwners,
      operators,
      orderOwners,
      wallets,
      tokenSymbolAddrMap,
      tokenAddrSymbolMap,
      tokenAddrDecimalsMap,
      tokenAddrInstanceMap,
      allTokens
    );
  }

  private getBestBlockSize(count: number, blockSizes: number[]) {
    let blockSize = blockSizes[0];
    let i = 1;
    while (count > blockSize && i < blockSizes.length) {
      blockSize = blockSizes[i];
      i++;
    }
    return blockSize;
  }
}

export class BalanceSnapshot {
  private exchangeTestUtil: ExchangeTestUtil;
  private balances: Map<string, BN>[] = [];
  private addressBook: Map<string, string>;

  constructor(util: ExchangeTestUtil) {
    // this.exchangeTestUtil = util;
    // for (let i = 0; i < this.exchangeTestUtil.MAX_NUM_TOKENS; i++) {
    //   this.balances[i] = new Map<string, BN>();
    // }
    // this.addressBook = new Map<string, string>();
  }

  public async watchBalance(owner: string, token: string, name?: string) {
    // const tokenID = await this.exchangeTestUtil.getTokenID(token);
    // const balance = await this.exchangeTestUtil.getOnchainBalance(owner, token);
    // if (!this.balances[tokenID].has(owner)) {
    //   this.balances[tokenID].set(owner, balance);
    // }
    // if (name !== undefined) {
    //   this.addressBook.set(owner, name);
    // }
  }

  public async transfer(from: string, to: string, token: string, amount: BN, fromName?: string, toName?: string) {
    // const tokenID = await this.exchangeTestUtil.getTokenID(token);
    // if (!this.balances[tokenID].has(from)) {
    //   await this.watchBalance(from, token, fromName);
    // }
    // if (!this.balances[tokenID].has(to)) {
    //   await this.watchBalance(to, token, toName);
    // }
    // //const symbol = this.exchangeTestUtil.testContext.tokenAddrSymbolMap.get(this.exchangeTestUtil.getTokenAddress(token));
    // const balanceFrom = this.balances[tokenID].get(from);
    // const balanceTo = this.balances[tokenID].get(to);
    // //console.log(
    // //  amount.toString(10) + symbol + " from " +
    // //  this.addressBook.get(from) + " (" + balanceFrom.toString(10) + ") to " +
    // //  this.addressBook.get(to) + " (" + balanceTo.toString(10) + ").");
    // this.balances[tokenID].set(from, balanceFrom.sub(amount));
    // this.balances[tokenID].set(to, balanceTo.add(amount));
  }

  public async verifyBalances(allowedDelta: BN = new BN(0)) {
    // DEG token deep to 16
    // for (let i = 0; i < this.exchangeTestUtil.MAX_NUM_TOKENS; i++) {
    //   for (const [owner, balance] of this.balances[i].entries()) {
    //     const token = this.exchangeTestUtil.getTokenAddressFromID(i);
    //     const symbol = this.exchangeTestUtil.testContext.tokenAddrSymbolMap.get(
    //       this.exchangeTestUtil.getTokenAddress(token)
    //     );
    //     const currentBalance = await this.exchangeTestUtil.getOnchainBalance(
    //       owner,
    //       token
    //     );
    //     const ownerName = this.addressBook.get(owner);
    //     let descr = symbol + " balance of " + ownerName + " does not match: ";
    //     descr += currentBalance.toString(10) + " != " + balance.toString(10);
    //     assert(
    //       balance
    //         .sub(currentBalance)
    //         .abs()
    //         .lte(allowedDelta),
    //       descr
    //     );
    //   }
    // }
  }
}
