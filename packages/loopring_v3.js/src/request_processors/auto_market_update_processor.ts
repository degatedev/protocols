import BN from "bn.js";
import { Bitstream } from "../bitstream";
import { Constants } from "../constants";
import { EdDSA } from "../eddsa";
import { fromFloat } from "../float";
import { BlockContext, ExchangeState } from "../types";
interface AutoMarketUpdate {
  autoMarketID?: number;
  accountID?: number;
  tokenSID?: number;
  tokenBID?: number;
  lowPrice?: BN;
  highPrice?: BN;
  gridOffset?: BN;
  orderOffset?: BN;
  cancelled?: number;
  validUtil?: number;
}

/**
 * Processes account update requests.
 */
export class AutoMarketUpdateProcessor {
  public static process(
    state: ExchangeState,
    block: BlockContext,
    txData: Bitstream
  ) {
    console.log(
      "++++++++++++++++++++++++++++++in degate js AutoMarketUpdate processer"
    );
    const update = AutoMarketUpdateProcessor.extractData(txData);

    //   const account = state.getAccount(update.accountID);
    // //   account.owner = update.owner;
    // //   account.publicKeyX = update.publicKeyX;
    // //   account.publicKeyY = update.publicKeyY;
    // //   account.nonce++;

    //   const balance = account.getBalance(update.feeTokenID);
    //   balance.balance.isub(update.fee);

    //   const balanceOrder = account.getBalance(update.tokenID);

    //   const storage = balanceOrder.getStorage(update.storageID);
    // //   storage.

    //   const operator = state.getAccount(block.operatorAccountID);
    //   const balanceO = operator.getBalance(update.feeTokenID);
    //   balanceO.balance.iadd(update.fee);

    return update;
  }

  public static extractData(dataIn: Bitstream) {
    const autoMarketUpdate: AutoMarketUpdate = {};

    // '0x' + txType(0.5 byte)
    let dataInString = dataIn.getData();
    let data = new Bitstream(dataInString.slice(3));

    let offset = 0;
    autoMarketUpdate.autoMarketID = data.extractUint16(offset);
    offset += 2;
    autoMarketUpdate.tokenSID = data.extractUint32(offset);
    offset += 4;
    autoMarketUpdate.tokenBID = data.extractUint32(offset);
    offset += 4;
    autoMarketUpdate.lowPrice = fromFloat(
      data.extractUint24(offset),
      Constants.Float24Encoding
    );
    offset += 3;
    autoMarketUpdate.highPrice = fromFloat(
      data.extractUint24(offset),
      Constants.Float24Encoding
    );
    offset += 3;
    autoMarketUpdate.gridOffset = fromFloat(
      data.extractUint24(offset),
      Constants.Float24Encoding
    );
    offset += 3;
    autoMarketUpdate.orderOffset = fromFloat(
      data.extractUint24(offset),
      Constants.Float24Encoding
    );
    offset += 3;
    autoMarketUpdate.cancelled = data.extractUint8(offset);
    offset += 1;
    return autoMarketUpdate;
  }
}
