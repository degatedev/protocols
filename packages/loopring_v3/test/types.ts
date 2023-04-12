import BN = require("bn.js");
import { Signature } from "loopringV3.js";

export enum AuthMethod {
  NONE,
  EDDSA,
  ECDSA,
  APPROVE,
  FORCE
}

export interface AutoMarketStartOrderInfo {
  exchange?: string;
  storageID?: number;
  accountID?: number;
  tokenIdS?: number;
  tokenIdB?: number;
  amountS: BN;
  amountB: BN;
  validUntil?: number;
  fillAmountBorS?: boolean;
  taker?: string;
  feeTokenID?: number;
  feeBips?: number;
  tradingFee?: BN;
  maxFee?: BN;
  // uiReferID?: number;
  type?: number;
  // autoMarketID?: number;
  // (fillAmountBorS = 1 ? sell token: buy token
  gridOffset?: BN;
  // (fillAmountBorS = 1 ? sell token: buy token
  orderOffset?: BN;
  //(<256)
  maxLevel?: number;
  useAppKey?: number;

  signature?: Signature;
}
export interface OrderInfo {
  owner?: string;
  tokenS?: string;
  tokenB?: string;
  amountS: BN;
  amountB: BN;

  exchange?: string;
  accountID?: number;
  storageID?: number;

  tokenIdS?: number;
  tokenIdB?: number;

  validUntil?: number;
  //TradingFeGasFee-TradingFeProtocolFeeBips
  // maxFeeBips?: number;
  fillAmountBorS?: boolean;
  taker?: string;

  feeBips?: number;
  tradingFee?: BN;

  //trading fegas fee
  feeTokenID?: number; //
  fee?: BN; //
  maxFee?: BN; //

  // //refeUrefe
  // uiReferID?: number, //
  // accountReferID?: number, //

  //(<6),(6),(7)
  type?: number;
  // autoMarketID?: number;
  level?: number;
  startOrder?: AutoMarketStartOrderInfo;

  // (fillAmountBorS = 1 ? sell token: buy token
  gridOffset?: BN;
  // (fillAmountBorS = 1 ? sell token: buy token
  orderOffset?: BN;
  //(<256)
  maxLevel?: number;

  //BatchSpotTrade
  deltaFilledS?: BN;
  deltaFilledB?: BN;

  useAppKey?: number;

  isNextOrder?: boolean;
  appointedStorageID?: number;

  balanceS?: BN;
  balanceB?: BN;

  hash?: string;
  signature?: Signature;

  [key: string]: any;
}

export interface OrderExpectation {
  filledFraction: number;
  spread?: BN;
}

export interface RingExpectation {
  orderA?: OrderExpectation;
  orderB?: OrderExpectation;
}

// Transactions

export interface Noop {
  txType?: "Noop";
}

export interface SpotTrade {
  txType?: "SpotTrade";

  orderA: OrderInfo;
  orderB: OrderInfo;

  tokenID?: number;
  fee?: BN;

  expected?: RingExpectation;
}

export interface BatchSpotTradeUser {
  accountID?: number;
  [key: string]: any;
  orders: Array<OrderInfo>;
}

export interface BatchSpotTrade {
  txType?: string;
  users: Array<BatchSpotTradeUser>;
  tokens: Array<number>;
  // stableToken: number;

  tokenID?: number;
  fee?: BN;

  expected?: RingExpectation;

  userAFirstTokenExchange?: number;
  userASecondTokenExchange?: number;
  userAThirdTokenExchange?: number;

  userBFirstTokenExchange?: number;
  userBSecondTokenExchange?: number;
  userBThirdTokenExchange?: number;

  userCFirstTokenExchange?: number;
  userCSecondTokenExchange?: number;
  userCThirdTokenExchange?: number;

  userDFirstTokenExchange?: number;
  userDSecondTokenExchange?: number;
  userDThirdTokenExchange?: number;

  userEFirstTokenExchange?: number;
  userESecondTokenExchange?: number;
  userEThirdTokenExchange?: number;

  userFFirstTokenExchange?: number;
  userFSecondTokenExchange?: number;
  userFThirdTokenExchange?: number;

  userAAccountID?: number;
  userBAccountID?: number;
  userCAccountID?: number;
  userDAccountID?: number;
  userEAccountID?: number;
  userFAccountID?: number;

  bindTokenID: number;

  //
  enableDataCheck?: boolean;
  userAFirstEstimateExchange?: string;
  userASecondEstimateExchange?: string;
  userAThirdEstimateExchange?: string;

  userBFirstEstimateExchange?: string;
  userBSecondEstimateExchange?: string;
  userBThirdEstimateExchange?: string;

  userCFirstEstimateExchange?: string;
  userCSecondEstimateExchange?: string;
  userCThirdEstimateExchange?: string;

  userDFirstEstimateExchange?: string;
  userDSecondEstimateExchange?: string;
  userDThirdEstimateExchange?: string;

  userEFirstEstimateExchange?: string;
  userESecondEstimateExchange?: string;
  userEThirdEstimateExchange?: string;

  userFFirstEstimateExchange?: string;
  userFSecondEstimateExchange?: string;
  userFThirdEstimateExchange?: string;

  operatorFirstEstimateExchange?: string;
  operatorSecondEstimateExchange?: string;
  operatorThirdEstimateExchange?: string;
}

export interface Deposit {
  txType?: "Deposit";
  owner: string;
  accountID: number;
  tokenID: number;
  amount: BN;

  token: string;
  timestamp?: number;
  transactionHash?: string;
  type?: number;
}

export interface AccountUpdate {
  txType?: "AccountUpdate";
  exchange: string;

  type: number;

  owner: string;
  accountID: number;
  nonce: number;
  validUntil: number;

  publicKeyX: string;
  publicKeyY: string;
  feeTokenID: number;
  fee: BN;
  maxFee: BN;
  originalMaxFee?: BN;

  signature?: Signature;
  onchainSignature?: any;

  // //refe-refe
  // referID?: number;
}

export interface AppKeyUpdate {
  txType?: "AppKeyUpdate";
  exchange: string;

  // owner: string;
  accountID: number;
  nonce: number;
  validUntil: number;

  appKeyPublicKeyX: string;
  appKeyPublicKeyY: string;
  feeTokenID: number;
  fee: BN;
  maxFee: BN;
  originalMaxFee?: BN;

  disableAppKeySpotTrade: number;
  disableAppKeyWithdraw: number;
  disableAppKeyTransferToOther: number;

  signature?: Signature;
  onchainSignature?: any;
}

export class Transfer {
  txType?: "Transfer";
  exchange: string;

  type: number;

  fromAccountID: number;
  toAccountID: number;

  tokenID: number;
  amount: BN;

  feeTokenID: number;
  fee: BN;
  maxFee: BN;
  originalMaxFee?: BN;

  from: string;
  to: string;

  dualAuthorX: string;
  dualAuthorY: string;
  payerToAccountID: number;
  payerTo: string;
  payeeToAccountID: number;

  storageID: number;
  validUntil: number;

  putAddressesInDA: boolean;

  dualSecretKey?: string;

  useAppKey?: number;

  signature?: Signature;
  dualSignature?: Signature;

  onchainSignature?: any;
}

export interface WithdrawalRequest {
  txType?: "Withdraw";
  exchange: string;

  type: number;

  owner: string;
  accountID: number;
  storageID: number;
  validUntil: number;
  tokenID: number;
  amount: BN;

  feeTokenID?: number;
  fee?: BN;
  maxFee: BN;
  originalMaxFee?: BN;

  storeRecipient: boolean;

  minGas: number;
  gas?: number;

  to?: string;
  // extraData?: string;

  onchainDataHash?: string;

  withdrawalFee?: BN;

  useAppKey?: number;

  signature?: Signature;
  onchainSignature?: any;

  timestamp?: number;
  transactionHash?: string;
}

export interface OrderCancel {
  txType?: "OrderCancel";
  exchange: string;

  // owner: string;
  accountID: number;
  // tokenID: number;
  storageID: number;
  fee: BN;
  maxFee: BN;
  feeTokenID: number;
  useAppKey: number;

  hash?: string;
  signature?: Signature;
}

export interface AutoMarketUpdate {
  txType?: "AutoMarketUpdate";
  exchange: string;

  accountID: number;
  autoMarketID: string;
  tokenSID: number;
  tokenBID: number;
  maxLevel: number;
  gridOffset: BN;
  orderOffset: BN;
  cancelled: number;
  validUntil: number;

  fee: BN;
  maxFee: BN;
  feeTokenID: number;

  hash?: string;
  signature?: Signature;
}

// Blocks
export interface TxBlock {
  transactions: any[];
  protocolFeeBips?: number;
  // protocolTakerFeeBips?: number;
  // protocolMakerFeeBips?: number;

  //refeUrefe
  referFeeBips?: number;
  uiFeeBips?: number;

  timestamp?: number;
  exchange?: string;
  operatorAccountID?: number;

  signature?: Signature;
}

export interface BlockCallback {
  target: string;
  txIdx: number;
  numTxs: number;
  auxiliaryData: any;
  tx?: any;
}

export interface GasTokenConfig {
  gasTokenVault: string;
  maxToBurn: number;
  expectedGasRefund: number;
  calldataCost: number;
}

export interface Block {
  blockIdx: number;
  filename: string;
  blockType: number;
  blockSize: number;
  blockVersion: number;
  operator: string;
  origin: string;
  operatorId: number;
  merkleRoot: string;
  merkleAssetRoot: string;
  data: string;
  auxiliaryData: any[];
  offchainData: string;
  publicDataHash: string;
  publicInput: string;
  proof?: string[];
  blockFee?: BN;
  timestamp: number;
  transactionHash: string;
  internalBlock: TxBlock;
  blockInfoData?: any;
  shutdown?: boolean;
  callbacks?: BlockCallback[];
}

export interface Account {
  accountID: number;
  owner: string;
  publicKeyX: string;
  publicKeyY: string;
  appKeyPublicKeyX: string;
  appKeyPublicKeyY: string;
  secretKey: string;
  appKeySecretKey: string;
  nonce: number;
  //refe-refe
  referID?: number;
}
