import BN from "bn.js";
const fs = require("fs");
import Web3 from "web3";
import { Bitstream } from "./bitstream";
import { Constants } from "./constants";
import { decompressLZ, decompressZeros } from "./compression";
const poseidon = require("./poseidon");
import { ProtocolV3 } from "./protocol_v3";
import { SparseMerkleTree } from "./sparse_merkle_tree";
import {
  BlockContext,
  TransactionType,
  ForgeMode,
  Block,
  Deposit,
  OnchainWithdrawal,
  Storage,
  Token,
  Balance,
  Account,
  OnchainAccountLeaf,
  OnchainBalanceLeaf,
  WithdrawFromMerkleTreeData,
  ExchangeState,
  ProtocolFees
} from "./types";
import { DepositProcessor } from "./request_processors/deposit_processor";
import { AccountUpdateProcessor } from "./request_processors/account_update_processor";
import { SpotTradeProcessor } from "./request_processors/spot_trade_processor";
import { BatchSpotTradeProcessor } from "./request_processors/batch_spot_trade_processor";
import { TransferProcessor } from "./request_processors/transfer_processor";
import { WithdrawalProcessor } from "./request_processors/withdrawal_processor";
import { OrderCancelProcessor } from "./request_processors/order_cancel_processor";
import { AppKeyUpdateProcessor } from "./request_processors/appkey_update_processor";
import * as log from "./logs";

/**
 * Processes all data of an Exchange v3 exchange contract.
 */
export class ExchangeV3 {
  private web3: Web3;

  private exchangeV3Abi: string;
  private exchangeAddress: string;
  private exchange: any;

  private syncedToEthereumBlockIdx: number;

  private owner: string;
  private operator: string;

  private shutdown: boolean;
  private shutdownStartTime: number;
  private inWithdrawalMode: boolean;
  private withdrawalModeStartTime: number;

  private tokens: Token[] = [];

  private state: ExchangeState;

  private blocks: Block[] = [];

  private merkleTree: SparseMerkleTree;
  private merkleAssetTree: SparseMerkleTree;

  private genesisMerkleRoot: string;

  private protocolFees: ProtocolFees;

  /**
   * Initializes an Exchange
   * @param   web3                      The web3 instance that will be used to get the necessary data from Ethereum
   * @param   exchangeAddress           The address of the exchange
   * @param   owner                     The owner of the exchange
   */
  public async initialize(web3: Web3, exchangeAddress: string, owner: string) {
    this.web3 = web3;
    this.exchangeAddress = exchangeAddress;
    this.owner = owner;
    this.operator = owner;

    this.syncedToEthereumBlockIdx = 0;

    const ABIPath = "ABI/version36/";
    this.exchangeV3Abi = fs.readFileSync(ABIPath + "IExchangeV3.abi", "ascii");

    this.exchange = new web3.eth.Contract(JSON.parse(this.exchangeV3Abi));
    this.exchange.options.address = this.exchangeAddress;

    const exchangeCreationTimestamp = (await this.exchange.methods
      .getBlockInfo(0)
      .call()).timestamp;
    const genesisMerkleRoot = new BN(
      (await this.exchange.methods.getMerkleRoot().call()).slice(2),
      16
    ).toString(10);

    const genesisMerkleAssetRoot = new BN(
      (await this.exchange.methods.getMerkleAssetRoot().call()).slice(2),
      16
    ).toString(10);

    this.shutdown = false;
    this.shutdownStartTime = 0;
    this.inWithdrawalMode = false;
    this.withdrawalModeStartTime = 0;

    // Reset state
    this.state = new ExchangeState(exchangeAddress, []);

    // Create the genesis block
    const genesisBlock: Block = {
      exchange: exchangeAddress,
      blockIdx: 0,

      blockType: 0,
      blockSize: 0,
      blockVersion: 0,
      data: "0x",
      offchainData: "0x",

      operator: Constants.zeroAddress,
      origin: Constants.zeroAddress,

      blockFee: new BN(0),

      merkleRoot: genesisMerkleRoot,
      merkleAssetRoot: genesisMerkleAssetRoot,
      timestamp: exchangeCreationTimestamp,

      numRequestsProcessed: 0,
      totalNumRequestsProcessed: 0,

      transactionHash: Constants.zeroAddress
    };
    this.blocks.push(genesisBlock);

    // Get the protocol fees from the contract
    const protocolFeeValues = await this.exchange.methods
      .getProtocolFeeValues()
      .call();
    this.protocolFees = {
      exchange: exchangeAddress,
      protocolFeeBips: parseInt(protocolFeeValues.protocolFeeBips),
      previousProtocolFeeBips: parseInt(
        protocolFeeValues.previousProtocolFeeBips
      )
    };
  }

  public async syncWithStep(ethereumBlockTo: number, step: number) {
    const fromBlock = this.syncedToEthereumBlockIdx + 1;
    for (let i = fromBlock; i < ethereumBlockTo; i += step) {
      if (i > ethereumBlockTo) {
        await this.sync(ethereumBlockTo);
      } else {
        await this.sync(i);
      }
    }
  }

  /**
   * Syncs the protocol up to (and including) the given Ethereum block index.
   * @param   ethereumBlockTo   The Ethereum block index to sync to
   */
  public async sync(ethereumBlockTo: number) {
    log.DEBUG(
      "exchange",
      this.exchangeAddress,
      " sync, fromBlock:",
      this.syncedToEthereumBlockIdx + 1,
      ", toBlock:",
      ethereumBlockTo
    );

    if (ethereumBlockTo <= this.syncedToEthereumBlockIdx) {
      return;
    }

    // Process the events
    const events = await this.exchange.getPastEvents("allEvents", {
      fromBlock: this.syncedToEthereumBlockIdx + 1,
      toBlock: ethereumBlockTo
    });
    for (const event of events) {
      if (event.event === "BlockSubmitted") {
        await this.processBlockSubmitted(event);
      } else if (event.event === "DepositRequested") {
        await this.processDepositRequested(event);
      } else if (event.event === "WithdrawalRequested") {
        await this.processWithdrawalRequested(event);
      } else if (event.event === "TokenRegistered") {
        await this.processTokenRegistered(event);
      } else if (event.event === "Shutdown") {
        await this.processShutdown(event);
      } else if (event.event === "WithdrawalModeActivated") {
        await this.processWithdrawalModeActivated(event);
      } else if (event.event === "OperatorChanged") {
        await this.processOperatorChanged(event);
      } else if (event.event === "ProtocolFeesUpdated") {
        await this.processProtocolFeesUpdated(event);
      } else if (event.event === "OwnershipTransferred") {
        await this.processOwnershipTransferred(event);
      }
    }
    this.syncedToEthereumBlockIdx = ethereumBlockTo;
  }

  /**
   * Builds the Merkle tree on the current state
   */
  public buildMerkleTree() {
    const balanceHasher = poseidon.createHash(5, 6, 52);
    const accountHasher = poseidon.createHash(12, 6, 53);
    const accountAssetHasher = poseidon.createHash(6, 6, 52);
    const storageHasher = poseidon.createHash(8, 6, 53);

    // Make empty trees so we have all necessary default values
    const storageMerkleTree = new SparseMerkleTree(
      Constants.BINARY_TREE_DEPTH_STORAGE / 2
    );
    // # DEG-146:order cancel
    // storageMerkleTree.newTree(hasher([0, 0]).toString(10));
    storageMerkleTree.newTree(
      storageHasher([0, 0, 0, 0, 0, 0, 1]).toString(10)
    );
    console.log(
      "Empty storage tree in buildMerkleTree:",
      storageMerkleTree.getRoot()
    );

    const balancesMerkleTree = new SparseMerkleTree(
      Constants.BINARY_TREE_DEPTH_TOKENS / 2
    );
    balancesMerkleTree.newTree(
      // hasher([0, 0, storageMerkleTree.getRoot()]).toString(10)
      balanceHasher([0]).toString(10)
    );

    console.log(
      "Empty balances tree in buildMerkleTree:",
      balancesMerkleTree.getRoot()
    );

    this.merkleTree = new SparseMerkleTree(
      Constants.BINARY_TREE_DEPTH_ACCOUNTS / 2
    );

    this.merkleTree.newTree(
      // accountHasher([0, 0, 0, 0, 0, balancesMerkleTree.getRoot()]).toString(10)
      accountHasher([
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        balancesMerkleTree.getRoot(),
        storageMerkleTree.getRoot()
      ]).toString(10)
    );

    console.log(
      "empty merkle tree in buildMerkleTree: " + this.merkleTree.getRoot()
    );

    this.merkleAssetTree = new SparseMerkleTree(
      Constants.BINARY_TREE_DEPTH_ACCOUNTS / 2
    );

    this.merkleAssetTree.newTree(
      accountAssetHasher([0, 0, 0, 0, balancesMerkleTree.getRoot()]).toString(
        10
      )
    );

    console.log(
      "empty asset merkle tree in buildMerkleTree: " +
        this.merkleAssetTree.getRoot()
    );

    // Run over all account data and build the Merkle tree
    for (const account of this.state.accounts) {
      console.log("this.state.accounts account.owner: ", account.owner);

      account.balancesMerkleTree = new SparseMerkleTree(
        Constants.BINARY_TREE_DEPTH_TOKENS / 2
      );
      account.balancesMerkleTree.newTree(
        // hasher([0, 0, storageMerkleTree.getRoot()]).toString(10)
        balanceHasher([0]).toString(10)
      );

      account.storageTree = new SparseMerkleTree(
        Constants.BINARY_TREE_DEPTH_STORAGE / 2
      );
      account.storageTree.newTree(
        storageHasher([0, 0, 0, 0, 0, 0, 1]).toString(10)
      );

      for (const orderID of Object.keys(account.storage)) {
        const storageValue = account.storage[Number(orderID)];
        account.storageTree.update(
          Number(orderID),
          storageHasher([
            storageValue.tokenSID,
            storageValue.tokenBID,
            storageValue.data,
            storageValue.storageID,
            storageValue.gasFee,
            storageValue.cancelled,
            storageValue.forward
          ]).toString(10)
        );

        console.log("orderID: ", orderID);

        let storageLeafhasher = storageHasher([
          storageValue.tokenSID,
          storageValue.tokenBID,
          storageValue.data,
          storageValue.storageID,
          storageValue.gasFee,
          storageValue.cancelled,
          storageValue.forward
        ]).toString(10);
        console.log("storageLeafhasher: ", storageLeafhasher);
      }

      for (const tokenID of Object.keys(account.balances)) {
        const balanceValue = account.balances[Number(tokenID)];

        account.balancesMerkleTree.update(
          Number(tokenID),
          balanceHasher([
            balanceValue.balance
            // balanceValue.storageTree.getRoot()
          ]).toString(10)
        );

        console.log(
          "balanceValue: ",
          tokenID,
          balanceValue.balance.toString(10)
        );

        let tokenIDBalanceLeafhasher = balanceHasher([
          balanceValue.balance
        ]).toString(10);
        console.log("tokenIDBalanceLeafhasher: ", tokenIDBalanceLeafhasher);
      }

      this.merkleTree.update(
        account.accountId,
        accountHasher([
          account.owner,
          account.publicKeyX,
          account.publicKeyY,
          account.appKeyPublicKeyX,
          account.appKeyPublicKeyY,
          account.nonce,
          account.disableAppKeySpotTrade,
          account.disableAppKeyWithdraw,
          account.disableAppKeyTransferToOther,
          account.balancesMerkleTree.getRoot(),
          account.storageTree.getRoot()
        ]).toString(10)
      );

      this.merkleAssetTree.update(
        account.accountId,
        accountAssetHasher([
          account.owner,
          account.publicKeyX,
          account.publicKeyY,
          account.nonce,
          account.balancesMerkleTree.getRoot()
        ]).toString(10)
      );

      console.log("account: ", account);
      console.log(
        "account.balancesMerkleTree: ",
        account.balancesMerkleTree.getRoot()
      );
      console.log("account.storageTree: ", account.storageTree.getRoot());

      let accountIdLeafHasher = accountHasher([
        account.owner,
        account.publicKeyX,
        account.publicKeyY,
        account.appKeyPublicKeyX,
        account.appKeyPublicKeyY,
        account.nonce,
        account.disableAppKeySpotTrade,
        account.disableAppKeyWithdraw,
        account.disableAppKeyTransferToOther,
        account.balancesMerkleTree.getRoot(),
        account.storageTree.getRoot()
      ]).toString(10);

      console.log("accountIdLeafHasher: ", accountIdLeafHasher);
      console.log("this.merkleTree.getRoot: " + this.merkleTree.getRoot());
      console.log(
        "this.merkleAssetTree.getRoot: " + this.merkleAssetTree.getRoot()
      );

      console.log("==================  next account ================== ");
    }

    // console.log("this.merkleTree", this.merkleTree);
    // console.log("this.state", this.state);

    console.log(
      "buildMerkleTree merkleTree.getRoot: " + this.merkleTree.getRoot()
    );
    console.log(
      "buildMerkleTree merkleAssetTree.getRoot: " +
        this.merkleAssetTree.getRoot()
    );

    assert.equal(
      this.merkleTree.getRoot(),
      this.blocks[this.blocks.length - 1].merkleRoot,
      "Merkle tree root inconsistent"
    );

    assert.equal(
      this.merkleAssetTree.getRoot(),
      this.blocks[this.blocks.length - 1].merkleAssetRoot,
      "Merkle asset tree root inconsistent"
    );
  }

  /**
   * Builds the Merkle tree on the state necessary for withdrawal mode
   * (on the state of the last finalized block).
   */
  public buildMerkleTreeForWithdrawalMode() {
    this.buildMerkleTree();
  }

  /**
   * Returns the data necessary to withdraw directly from the Merkle tree on-chain
   * (only avaible in withdrawal mode).
   * @param   accountID   The account ID of the balance to withdraw
   * @param   tokenID     The token ID of the balance to withdraw
   * @return  The necessary data for withdrawFromMerkleTree(for)
   */
  public getWithdrawFromMerkleTreeData(accountID: number, tokenID: number) {
    assert(accountID < this.state.accounts.length, "invalid account ID");
    assert(tokenID < this.tokens.length, "invalid token ID");

    const account = this.state.accounts[accountID];
    const accountMerkleProof = this.merkleAssetTree.createProof(accountID);
    const balanceMerkleProof = account.balancesMerkleTree.createProof(tokenID);

    // const hasher = poseidon.createHash(5, 6, 52);
    const storageHasher = poseidon.createHash(8, 6, 53);

    const storageTree = new SparseMerkleTree(
      Constants.BINARY_TREE_DEPTH_STORAGE / 2
    );
    storageTree.newTree(storageHasher([0, 0, 0, 0, 0, 0, 1]).toString(10));

    const accountLeaf: OnchainAccountLeaf = {
      accountID: account.accountId,
      owner: account.owner,
      pubKeyX: account.publicKeyX,
      pubKeyY: account.publicKeyY,
      nonce: account.nonce
      // storageRoot:
      //   account.storageTree !== undefined
      //     ? account.storageTree.getRoot()
      //     : storageTree.getRoot()
    };
    const balanceLeaf: OnchainBalanceLeaf = {
      tokenID,
      balance: account.getBalance(tokenID).balance.toString(10)
      // storageRoot:
      //   account.getBalance(tokenID).storageTree !== undefined
      //     ? account.getBalance(tokenID).storageTree.getRoot()
      //     : storageTree.getRoot()
    };
    const withdrawFromMerkleTreeData: WithdrawFromMerkleTreeData = {
      accountLeaf,
      balanceLeaf,
      accountMerkleProof,
      balanceMerkleProof
    };

    return withdrawFromMerkleTreeData;
  }

  /// Blocks

  /**
   * The total number of blocks committed on-chain
   * @return  The total number of blocks
   */
  public getNumBlocks() {
    return this.blocks.length;
  }

  /**
   * Gets the blocks using the blocks's index in the list of all blocks
   * @param   index   The index of the block
   * @return  The block on the given index
   */
  public getBlock(blockIdx: number) {
    return this.blocks[blockIdx];
  }

  /// Tokens

  /**
   * The total number of tokens registered on the exchange
   * @return  The total number of tokens
   */
  public getNumTokens() {
    return this.tokens.length;
  }

  /**
   * Gets the token with the specified token ID
   * @param   tokenID   The ID of the token
   * @return  The token with the given tokenID
   */
  public getToken(tokenID: number) {
    return this.tokens[tokenID];
  }

  /// Accounts

  /**
   * The total number of accounts registered in the Merkle tree
   * (note that this can be less than registered on-chain because the account
   * registration needs to be processed in a block).
   * @return  The total number of accounts
   */
  public getNumAccounts() {
    return this.state.accounts.length;
  }

  /**
   * Gets the account with the specified account ID
   * @param   accountID   The ID of the account
   * @return  The account with the given accountID
   */
  public getAccount(accountID: number) {
    return this.state.accounts[accountID];
  }

  /**
   * Gets the account with the specified owner
   * @param   owner   The owner of the account
   * @return  The account with the given owner
   */
  public getAccountByOwner(owner: string) {
    return this.state.accounts[this.getAccountId(owner)];
  }

  /**
   * Gets the accountID of the specified owner
   * @param   owner   The owner of the account
   * @return  The accountID of the given owner
   */
  public getAccountId(owner: string) {
    return this.state.ownerToAccountId[owner];
  }

  /// Processed requests

  /**
   * The total number of requests processed in blocks on the exchange
   * @return  The total number of processed requests
   */
  public getNumProcessedRequests() {
    return this.state.processedRequests.length;
  }

  /**
   * Gets a processed request with the specified index
   * @return  The processed request
   */
  public getProcessedRequest(requestIdx: number) {
    return this.state.processedRequests[requestIdx];
  }

  /**
   * Gets the processed requests within the specified range.
   * This function will automatically clamp to a valid range.
   * @return  The processed requests
   */
  public getProcessedRequests(startIdx: number, count: number) {
    const requests: any[] = [];
    if (startIdx >= this.state.processedRequests.length) {
      return [];
    }
    const endIdx = Math.min(
      startIdx + count,
      this.state.processedRequests.length
    );
    for (let i = startIdx; i < endIdx; i++) {
      requests.push(this.getProcessedRequest(i));
    }
    return requests;
  }

  /**
   * Gets the processed requests that were processed in the specified block
   * @return  The processed requests in the given block
   */
  public getRequestsInBlock(blockIdx: number) {
    if (blockIdx === 0 || blockIdx >= this.blocks.length) {
      return [];
    }
    const block = this.getBlock(blockIdx);
    return this.getProcessedRequests(
      block.totalNumRequestsProcessed - block.numRequestsProcessed,
      block.numRequestsProcessed
    );
  }

  /// Deposits

  /**
   * Returns the total number of deposits done on-chain
   * @return  The total number of deposits
   */
  public getNumDeposits() {
    return this.state.deposits.length;
  }

  /**
   * Returns the deposit with the specified depositIdx
   * @return  The requested deposit
   */
  public getDeposit(depositIdx: number) {
    return this.state.deposits[depositIdx];
  }

  /**
   * Returns the deposits within the specified range.
   * This function will automatically clamp to a valid range.
   * @return  The requested deposits
   */
  public getDeposits(startIdx: number, count: number) {
    const deposits: Deposit[] = [];
    if (startIdx >= this.state.deposits.length) {
      return [];
    }
    const endIdx = Math.min(startIdx + count, this.state.deposits.length);
    for (let i = startIdx; i < endIdx; i++) {
      deposits.push(this.getDeposit(i));
    }
    return deposits;
  }

  /// On-chain withdrawals

  /**
   * Returns the total number of on-chain withdrawal requests
   * @return  The total number of on-chain withdrawals
   */
  public getNumOnchainWithdrawalRequests() {
    return this.state.onchainWithdrawals.length;
  }

  /**
   * Returns the withdrawal with the specified withdrawalIdx
   * @return  The requested deposit
   */
  public getOnchainWithdrawalRequest(withdrawalIdx: number) {
    return this.state.onchainWithdrawals[withdrawalIdx];
  }

  /**
   * Returns the withdrawals within the specified range.
   * This function will automatically clamp to a valid range.
   * @return  The requested on-chain withdrawals
   */
  public getOnchainWithdrawalRequests(startIdx: number, count: number) {
    const withdrawals: OnchainWithdrawal[] = [];
    if (startIdx >= this.state.onchainWithdrawals.length) {
      return [];
    }
    const endIdx = Math.min(
      startIdx + count,
      this.state.onchainWithdrawals.length
    );
    for (let i = startIdx; i < endIdx; i++) {
      withdrawals.push(this.getOnchainWithdrawalRequest(i));
    }
    return withdrawals;
  }

  /// Meta

  /**
   * Gets the address of the contract
   * @return  The address of the contract
   */
  public getAddress() {
    return this.exchangeAddress;
  }

  /**
   * Gets the exchange owner
   * @return  The owner of the exchange
   */
  public getOwner() {
    return this.owner;
  }

  /**
   * Gets the exchange operator
   * @return  The operator of the exchange
   */
  public getOperator() {
    return this.operator;
  }

  /**
   * Returns the exchange stake amount (in LRC)
   * @return  The amount staked in LRC
   */
  public getExchangeStake() {
    return this.exchange.getExchangeStake();
  }

  /**
   * Returns the protocol fee stake amount (in LRC)
   * @return  The amount staked in LRC
   */
  public getProtocolFeeStake() {
    return this.exchange.getProtocolFeeStake();
  }

  /**
   * Returns whether the exchange is shutdown
   * @return  True if the exchange is shutdown, else false
   */
  public isShutdown() {
    return this.shutdown;
  }

  /**
   * Returns the time when the exchange was shutdown
   * @return  The shutdownn start time.
   */
  public getShutdownStartTime() {
    return this.shutdownStartTime;
  }

  /**
   * Returns whether the exchange is in withdrawal mode
   * @return  True if the exchange is in withdrawal mode, else false
   */
  public isInWithdrawalMode() {
    return this.inWithdrawalMode;
  }

  /**
   * Returns the current protocol fees on this exchange
   * @return  The protocol fees
   */
  public getProtocolFees() {
    return this.protocolFees;
  }

  /// Private

  private async processBlockSubmitted(event: any) {
    // Make sure the blocks are in the right order
    const blockIdx = parseInt(event.returnValues.blockIdx);
    if (blockIdx < this.blocks.length) {
      // Block was already processed, but we still need to get the block fee
      //console.log("skip: " + blockIdx);
      this.blocks[blockIdx].blockFee = new BN(event.returnValues.blockFee);
      return;
    }
    assert.equal(blockIdx, this.blocks.length, "Unexpected blockIdx");
    log.DEBUG("processBlockCommitted event, blockIdx:", blockIdx);

    // Get the timestamp from the block
    const ethereumBlock = await this.web3.eth.getBlock(event.blockNumber);
    const timestamp = Number(ethereumBlock.timestamp);

    // Get the block data from the transaction data
    const submitBlocksFunctionSignature = "0x377bb770"; // submitBlocks

    const transaction = await this.web3.eth.getTransaction(
      event.transactionHash
    );
    //console.log(transaction.input);
    if (transaction.input.startsWith(submitBlocksFunctionSignature)) {
      const decodedInput = this.web3.eth.abi.decodeParameters(
        [
          "bool",
          "bytes"
          // "bytes"
          /*{
            "struct CallbackConfig": {
              "struct BlockCallback[]": {
                "struct TxCallback[]": {
                  txIdx: "uint16",
                  receiverIdx: "uint16",
                  data: "bytes"
                },
                blockIdx: "uint16"
              },
              receivers: "address[]"
            }
          }*/
        ],
        "0x" + transaction.input.slice(2 + 4 * 2)
      );

      const data = decodedInput[0]
        ? decompressZeros(decodedInput[1])
        : decodedInput[1];
      // Get the inputs to commitBlock
      const decodedInputs = this.web3.eth.abi.decodeParameters(
        [
          {
            "struct ExchangeData.Block[]": {
              blockType: "uint8",
              blockSize: "uint16",
              blockVersion: "uint8",
              data: "bytes",
              proof: "uint256[8]",
              storeDataHashOnchain: "bool",
              auxiliaryData: "bytes",
              offchainData: "bytes"
            }
          }
        ],
        "0x" +
          data /*transaction.input*/
            .slice(2 + 4 * 2)
      );
      //console.log(decodedInputs);
      const numBlocks = decodedInputs[0].length;
      //console.log("numBlocks: " + numBlocks);
      for (let i = 0; i < numBlocks; i++) {
        // Get the block data
        const blockType = parseInt(decodedInputs[0][i].blockType);
        const blockSize = parseInt(decodedInputs[0][i].blockSize);
        const blockVersion = parseInt(decodedInputs[0][i].blockVersion);
        const onchainData = decodedInputs[0][i].data;
        const offchainData = decodedInputs[0][i].offchainData;
        const data = decodedInputs[4] === null ? "0x" : onchainData;

        // Get the new Merkle root
        const bs = new Bitstream(data);
        if (bs.length() < 20 + 32 + 32) {
          // console.log("Invalid block data: " + data);
          return;
        }

        const merkleRoot = bs.extractUint(20 + 32).toString(10);
        console.log("processBlockSubmitted merkleRoot: " + merkleRoot);

        const merkleAssetRoot = bs.extractUint(20 + 32 + 32 + 32).toString(10);
        console.log(
          "processBlockSubmitted merkleAssetRoot: " + merkleAssetRoot
        );

        // Get the previous block
        const lastBlock = this.blocks[this.blocks.length - 1];

        // Create the block
        const newBlock: Block = {
          exchange: this.exchangeAddress,
          blockIdx: blockIdx + i,

          blockType,
          blockSize,
          blockVersion,
          data,
          offchainData,

          operator: this.operator,
          origin: transaction.from,

          blockFee: new BN(event.returnValues.blockFee),

          merkleRoot,
          merkleAssetRoot,

          timestamp,

          numRequestsProcessed: 0,
          totalNumRequestsProcessed: lastBlock.totalNumRequestsProcessed,

          transactionHash: event.transactionHash
        };
        this.blocks.push(newBlock);
        this.processBlock(newBlock);

        // TODO: remove (Only done here for debugging)
        this.buildMerkleTree();
        for (let a = 0; a < this.state.accounts.length; a++) {
          this.merkleTree.createProof(a);
        }
      }
    } else {
      console.log(
        "block " +
          blockIdx +
          " was committed with an unsupported function signature"
      );
    }
  }

  private async processDepositRequested(event: any) {
    //console.log(event);
    // Get the timestamp from the block
    const ethereumBlock = await this.web3.eth.getBlock(event.blockNumber);
    const timestamp = Number(ethereumBlock.timestamp);

    const deposit: Deposit = {
      exchange: this.exchangeAddress,
      timestamp,

      owner: event.returnValues.owner,
      token: event.returnValues.token,
      amount: new BN(event.returnValues.amount, 10),
      fee: new BN(event.returnValues.fee, 10),

      transactionHash: event.transactionHash
    };
    this.state.deposits.push(deposit);
  }

  private async processWithdrawalRequested(event: any) {
    console.log(event);

    // Get the timestamp from the block
    const ethereumBlock = await this.web3.eth.getBlock(event.blockNumber);
    const timestamp = Number(ethereumBlock.timestamp);

    const onchainWithdrawal: OnchainWithdrawal = {
      exchange: this.exchangeAddress,
      withdrawalIdx: parseInt(event.returnValues.withdrawalIdx),
      timestamp,

      accountID: parseInt(event.returnValues.accountID),
      tokenID: parseInt(event.returnValues.tokenID),
      amountRequested: new BN(event.returnValues.amount, 10),

      transactionHash: event.transactionHash
    };
    this.state.onchainWithdrawals.push(onchainWithdrawal);
  }

  private async processTokenRegistered(event: any) {
    // Make sure the tokens are in the right order
    assert.equal(
      this.tokens.length,
      parseInt(event.returnValues.tokenId),
      "Unexpected tokenId"
    );
    const token: Token = {
      exchange: this.exchangeAddress,
      tokenID: this.tokens.length,
      address: event.returnValues.token,
      enabled: true
    };
    this.tokens.push(token);
  }

  private async processShutdown(event: any) {
    this.shutdown = true;
    this.shutdownStartTime = parseInt(event.returnValues.timestamp);
  }

  private async processWithdrawalModeActivated(event: any) {
    this.inWithdrawalMode = true;
    this.withdrawalModeStartTime = parseInt(event.returnValues.timestamp);
  }

  private async processOperatorChanged(event: any) {
    assert(
      this.operator === event.returnValues.oldOperator,
      "unexpected operator"
    );
    this.operator = event.returnValues.newOperator;
  }

  private async processProtocolFeesUpdated(event: any) {
    this.protocolFees.protocolFeeBips = parseInt(
      event.returnValues.protocolFeeBips
    );
    this.protocolFees.previousProtocolFeeBips = parseInt(
      event.returnValues.previousProtocolFeeBips
    );
  }

  private async processOwnershipTransferred(event: any) {
    assert(this.owner === event.returnValues.previousOwner, "unexpected owner");
    this.owner = event.returnValues.previousOwner;
  }

  // Apply the block changes to the current state
  private processBlock(block: Block) {
    let requests: any[] = [];

    console.log("processBlock: ", block.data);

    let data = new Bitstream(block.data);
    let offset = 0;

    // General data( exchange + merkleRootBefore + merkleRootAfter + merkleAssetRootBefore + merkleAssetRootAfter + timestamp)
    offset += 20 + 32 + 32 + 32 + 32 + 4;
    const protocolFeeBips = data.extractUint8(offset);
    offset += 1;

    const numConditionalTransactions = data.extractUint32(offset);
    offset += 4;
    const operatorAccountID = data.extractUint32(offset);
    offset += 4;

    const depositSize = data.extractUint16(offset);
    offset += 2; // depositSize
    const accountUpdateSize = data.extractUint16(offset);
    offset += 2; // accountUpdateSize
    const withdrawSize = data.extractUint16(offset);
    offset += 2; // withdrawSize

    const ctx: BlockContext = {
      protocolFeeBips,
      operatorAccountID
    };

    for (let i = 0; i < block.blockSize; i++) {
      const size1 = Constants.TX_DATA_AVAILABILITY_SIZE_PART_1;
      const size2 = Constants.TX_DATA_AVAILABILITY_SIZE_PART_2;

      const txData1 = data.extractData(offset + i * size1, size1);
      const txData2 = data.extractData(
        offset + block.blockSize * size1 + i * size2,
        size2
      );
      const txData = new Bitstream(txData1 + txData2);

      let txType = TransactionType.NOOP;
      if (i < depositSize) {
        txType = TransactionType.DEPOSIT;
      } else if (i < depositSize + accountUpdateSize) {
        txType = TransactionType.ACCOUNT_UPDATE;
      } else if (i < block.blockSize - withdrawSize) {
        // extract tx type
        let dataInString = txData.getData();
        let txTypeString = dataInString.slice(0, 3);
        txType = parseInt(txTypeString, 16);
      } else {
        txType = TransactionType.WITHDRAWAL;
      }

      // const txType = txData.extractUint8(0);
      console.log("txType:", txType);
      console.log("txData", txData.getData());

      let request: any;
      if (txType === TransactionType.NOOP) {
        // Do nothing
      } else if (txType === TransactionType.DEPOSIT) {
        request = DepositProcessor.process(this.state, ctx, txData);
      } else if (txType === TransactionType.SPOT_TRADE) {
        request = SpotTradeProcessor.process(this.state, ctx, txData);
      } else if (txType === TransactionType.BATCH_SPOT_TRADE) {
        request = BatchSpotTradeProcessor.process(this.state, ctx, txData);
      } else if (txType === TransactionType.TRANSFER) {
        request = TransferProcessor.process(this.state, ctx, txData);
      } else if (txType === TransactionType.WITHDRAWAL) {
        request = WithdrawalProcessor.process(this.state, ctx, txData);
      } else if (txType === TransactionType.ACCOUNT_UPDATE) {
        request = AccountUpdateProcessor.process(this.state, ctx, txData);
      } else if (txType == TransactionType.ORDER_CANCEL) {
        // DEG-146:order cancel
        request = OrderCancelProcessor.process(this.state, ctx, txData);
      } else if (txType === TransactionType.APPKEY_UPDATE) {
        request = AppKeyUpdateProcessor.process(this.state, ctx, txData);
      } else {
        assert(false, "unknown transaction type: " + txType);
      }

      requests.push(request);
    }

    // Update operator nonce
    this.state.getAccount(ctx.operatorAccountID).nonce++;

    block.numRequestsProcessed = requests.length;
    block.totalNumRequestsProcessed += requests.length;
    this.state.processedRequests.push(...requests);
  }
}
