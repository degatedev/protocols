import sys
sys.path.insert(0, 'ethsnarks')
sys.path.insert(0, 'operator')
import os.path
import subprocess
import json
import pathlib
# DEG-358 BatchSpotTrade - opetator
from state import Account, Context, State, Order, Ring, copyAccountInfo, AccountUpdateData
from calculate import MAX_BATCH_SPOT_TRADE_USER, MAX_BATCH_SPOT_TRADE_USER_ORDER

class Block(object):
    def __init__(self):
        self.blockType = 0
        self.transactions = []

    def toJSON(self):
        self.blockSize = len(self.transactions)
        data = json.dumps(self, default=lambda o: o.__dict__, sort_keys=False, indent=4)
        # Work around the reserved keyword "from" in python
        data = data.replace('"from_"','"from"')
        return data

class GeneralObject(object):
    pass

def autoMarketStartOrderFromJSON(jOrder, state):
    storageID = int(jOrder["storageID"])
    accountID = int(jOrder["accountID"])
    tokenS = int(jOrder["tokenIdS"])
    tokenB = int(jOrder["tokenIdB"])
    amountS = int(jOrder["amountS"])
    amountB = int(jOrder["amountB"])
    validUntil = int(jOrder["validUntil"])
    fillAmountBorS = int(jOrder["fillAmountBorS"])
    taker = str(jOrder["taker"])
    feeBips = int(jOrder["feeBips"])
    tradingFee = int(jOrder["tradingFee"])

    feeTokenID = int(jOrder["feeTokenID"])
    maxFee = int(jOrder["maxFee"])

    # AutoMarket
    # DEG-265 auto market
    type = int(jOrder["type"])
    gridOffset = int(jOrder["gridOffset"])
    orderOffset = int(jOrder["orderOffset"])
    maxLevel = int(jOrder["maxLevel"])

    useAppKey = int(jOrder["useAppKey"])

    account = state.getAccount(accountID)

    order = Order(account.publicKeyX, account.publicKeyY,
                  storageID, accountID,
                  tokenS, tokenB,
                  amountS, amountB,
                  # DEG-358 BatchSpotTrade - opetator
                  0, 0,
                  validUntil, fillAmountBorS, taker,
                  # split TradingFee and GasFee - ProtocolFeeBips as the max TradingFee
                  feeBips, #maxFeeBips, feeBips,
                  tradingFee,
                  # split trading fee and gas fee
                  feeTokenID, 0, maxFee,
                # DEG-265 auto market
                  type, 0, None, gridOffset, orderOffset, maxLevel,
                  useAppKey)

    order.signature = None
    if "signature" in jOrder:
        order.signature = jOrder["signature"]

    return order
def orderFromJSON(jOrder, state):
    storageID = int(jOrder["storageID"])
    accountID = int(jOrder["accountID"])
    tokenS = int(jOrder["tokenIdS"])
    tokenB = int(jOrder["tokenIdB"])
    amountS = int(jOrder["amountS"])
    amountB = int(jOrder["amountB"])
    # DEG-358 BatchSpotTrade - opetator
    deltaFilledS = int(jOrder["deltaFilledS"])
    deltaFilledB = int(jOrder["deltaFilledB"])
    validUntil = int(jOrder["validUntil"])
    fillAmountBorS = int(jOrder["fillAmountBorS"])
    taker = str(jOrder["taker"])

    feeBips = int(jOrder["feeBips"])
    tradingFee = int(jOrder["tradingFee"])

    # split trading fee and gas fee
    feeTokenID = int(jOrder["feeTokenID"])
    fee = int(jOrder["fee"])
    maxFee = int(jOrder["maxFee"])

    # AutoMarket
    # DEG-265 auto market
    type = int(jOrder["type"])
    level = int(jOrder["level"])
    gridOffset = int(jOrder["gridOffset"])
    orderOffset = int(jOrder["orderOffset"])
    maxLevel = int(jOrder["maxLevel"])

    useAppKey = int(jOrder["useAppKey"])

    startOrder = None
    if type == 6 or type == 7:
        startOrder = autoMarketStartOrderFromJSON(jOrder["startOrder"], state)

    account = state.getAccount(accountID)

    order = Order(account.publicKeyX, account.publicKeyY,
                  storageID, accountID,
                  tokenS, tokenB,
                  amountS, amountB,
                  # DEG-358 BatchSpotTrade - opetator
                  deltaFilledS, deltaFilledB,
                  validUntil, fillAmountBorS, taker,
                  # split TradingFee and GasFee - ProtocolFeeBips as the max TradingFee
                  feeBips, #maxFeeBips, feeBips,
                  tradingFee,
                  # split trading fee and gas fee
                  feeTokenID, fee, maxFee,
                  type, level, startOrder, gridOffset, orderOffset, maxLevel,
                  useAppKey)

    order.signature = None
    if "signature" in jOrder:
        order.signature = jOrder["signature"]

    return order

def transferFromJSON(jTransfer):
    transfer = GeneralObject()
    transfer.fromAccountID = int(jTransfer["fromAccountID"])
    transfer.toAccountID = int(jTransfer["toAccountID"])
    transfer.tokenID = int(jTransfer["tokenID"])
    transfer.amount = str(jTransfer["amount"])
    transfer.feeTokenID = int(jTransfer["feeTokenID"])
    transfer.fee = str(jTransfer["fee"])
    transfer.type = int(jTransfer["type"])
    transfer.storageID = str(jTransfer["storageID"])
    transfer.from_ = str(jTransfer["from"])
    transfer.to = str(jTransfer["to"])
    transfer.validUntil = int(jTransfer["validUntil"])
    transfer.dualAuthorX = str(jTransfer["dualAuthorX"])
    transfer.dualAuthorY = str(jTransfer["dualAuthorY"])
    transfer.payerToAccountID = int(jTransfer["payerToAccountID"])
    transfer.payerTo = str(jTransfer["payerTo"])
    transfer.payeeToAccountID = int(jTransfer["payeeToAccountID"])
    transfer.maxFee = str(jTransfer["maxFee"])
    transfer.putAddressesInDA = bool(jTransfer["putAddressesInDA"])
    transfer.useAppKey = int(jTransfer["useAppKey"])
    transfer.signature = None
    transfer.dualSignature = None
    transfer.onchainSignature = None
    if "signature" in jTransfer:
        transfer.signature = jTransfer["signature"]
    if "dualSignature" in jTransfer:
        transfer.dualSignature = jTransfer["dualSignature"]
    return transfer

def withdrawFromJSON(jWithdraw):
    withdraw = GeneralObject()
    withdraw.owner = str(jWithdraw["owner"])
    withdraw.accountID = int(jWithdraw["accountID"])
    withdraw.storageID = str(jWithdraw["storageID"])
    withdraw.tokenID = int(jWithdraw["tokenID"])
    withdraw.amount = str(jWithdraw["amount"])
    withdraw.feeTokenID = int(jWithdraw["feeTokenID"])
    withdraw.fee = str(jWithdraw["fee"])
    withdraw.onchainDataHash = str(jWithdraw["onchainDataHash"])
    withdraw.type = int(jWithdraw["type"])
    withdraw.validUntil = int(jWithdraw["validUntil"])
    withdraw.maxFee = str(jWithdraw["maxFee"])
    withdraw.useAppKey = int(jWithdraw["useAppKey"])

    withdraw.minGas = str(jWithdraw["minGas"])
    withdraw.to = str(jWithdraw["to"])

    withdraw.signature = None
    if "signature" in jWithdraw:
        withdraw.signature = jWithdraw["signature"]
    return withdraw

def depositFromJSON(jDeposit):
    deposit = GeneralObject()
    deposit.owner = str(jDeposit["owner"])
    deposit.accountID = int(jDeposit["accountID"])
    deposit.tokenID = int(jDeposit["tokenID"])
    deposit.amount = str(jDeposit["amount"])
    deposit.type = int(jDeposit["type"])
    return deposit


def accountUpdateFromJSON(jUpdate):
    update = GeneralObject()
    update.owner = str(jUpdate["owner"])
    update.accountID = int(jUpdate["accountID"])
    update.nonce = str(jUpdate["nonce"])
    update.validUntil = int(jUpdate["validUntil"])
    update.publicKeyX = str(jUpdate["publicKeyX"])
    update.publicKeyY = str(jUpdate["publicKeyY"])
    update.feeTokenID = int(jUpdate["feeTokenID"])
    update.fee = str(jUpdate["fee"])
    update.maxFee = str(jUpdate["maxFee"])
    update.type = int(jUpdate["type"])
    update.signature = None
    if "signature" in jUpdate:
        update.signature = jUpdate["signature"]
    return update

def appKeyUpdateFromJSON(jUpdate):
    update = GeneralObject()
    # update.owner = str(jUpdate["owner"])
    update.accountID = int(jUpdate["accountID"])
    update.nonce = str(jUpdate["nonce"])
    update.validUntil = int(jUpdate["validUntil"])
    update.appKeyPublicKeyX = str(jUpdate["appKeyPublicKeyX"])
    update.appKeyPublicKeyY = str(jUpdate["appKeyPublicKeyY"])
    update.feeTokenID = int(jUpdate["feeTokenID"])
    update.fee = str(jUpdate["fee"])
    update.maxFee = str(jUpdate["maxFee"])
    update.disableAppKeySpotTrade = int(jUpdate["disableAppKeySpotTrade"])
    update.disableAppKeyWithdraw = int(jUpdate["disableAppKeyWithdraw"])
    update.disableAppKeyTransferToOther = int(jUpdate["disableAppKeyTransferToOther"])

    update.signature = None
    if "signature" in jUpdate:
        update.signature = jUpdate["signature"]
    return update

# DEG-146:order cancel
def orderCancelFromJSON(jUpdate):
    update = GeneralObject()
    # update.owner = str(jUpdate["owner"])
    update.accountID = int(jUpdate["accountID"])
    # update.tokenID = int(jUpdate["tokenID"])

    update.storageID = str(jUpdate["storageID"])
    update.fee = str(jUpdate["fee"])
    update.maxFee = str(jUpdate["maxFee"])
    update.feeTokenID = int(jUpdate["feeTokenID"])
    update.useAppKey = int(jUpdate["useAppKey"])
    
    update.signature = None
    if "signature" in jUpdate:
        update.signature = jUpdate["signature"]
    return update

# DEG-358 BatchSpotTrade - opetator
def batchSpotTradeUserFromJSON(jUser, state, maxOrders):
    user = GeneralObject()
    user.accountID = int(jUser["accountID"])
    user.isNoop = 0
    user.orders = []
    index = 0
    for order in jUser["orders"]:
        user.orders.append(orderFromJSON(order, state))
        print("==============batchSpotTradeUserFromJSON order tokenSID:" + str(user.orders[index].tokenS))
        index = index + 1
    return user
# DEG-358 BatchSpotTrade - opetator
def batchSpotTradeFromJSON(jBatch, state):
    batchSpotTrade = GeneralObject()
    batchSpotTrade.users = []
    index = 0
    for user in jBatch["users"]:
        print("in users loop==============:" + str(user["accountID"]))
        batchSpotTrade.users.append(batchSpotTradeUserFromJSON(user, state, MAX_BATCH_SPOT_TRADE_USER_ORDER[index]))
        index = index + 1
    batchSpotTrade.tokens = []
    index = 0
    for token in jBatch["tokens"]:
        batchSpotTrade.tokens.append(int(jBatch["tokens"][index]))
        index = index + 1
    
    batchSpotTrade.bindTokenID = int(jBatch["bindTokenID"])

    batchSpotTrade.enableDataCheck = bool(jBatch["enableDataCheck"])
    batchSpotTrade.userAFirstEstimateExchange = int(jBatch["userAFirstEstimateExchange"])
    batchSpotTrade.userASecondEstimateExchange = int(jBatch["userASecondEstimateExchange"])
    batchSpotTrade.userAThirdEstimateExchange = int(jBatch["userAThirdEstimateExchange"])

    batchSpotTrade.userBFirstEstimateExchange = int(jBatch["userBFirstEstimateExchange"])
    batchSpotTrade.userBSecondEstimateExchange = int(jBatch["userBSecondEstimateExchange"])
    batchSpotTrade.userBThirdEstimateExchange = int(jBatch["userBThirdEstimateExchange"])

    batchSpotTrade.userCFirstEstimateExchange = int(jBatch["userCFirstEstimateExchange"])
    batchSpotTrade.userCSecondEstimateExchange = int(jBatch["userCSecondEstimateExchange"])
    batchSpotTrade.userCThirdEstimateExchange = int(jBatch["userCThirdEstimateExchange"])

    batchSpotTrade.userDFirstEstimateExchange = int(jBatch["userDFirstEstimateExchange"])
    batchSpotTrade.userDSecondEstimateExchange = int(jBatch["userDSecondEstimateExchange"])
    batchSpotTrade.userDThirdEstimateExchange = int(jBatch["userDThirdEstimateExchange"])

    batchSpotTrade.userEFirstEstimateExchange = int(jBatch["userEFirstEstimateExchange"])
    batchSpotTrade.userESecondEstimateExchange = int(jBatch["userESecondEstimateExchange"])
    batchSpotTrade.userEThirdEstimateExchange = int(jBatch["userEThirdEstimateExchange"])

    batchSpotTrade.userFFirstEstimateExchange = int(jBatch["userFFirstEstimateExchange"])
    batchSpotTrade.userFSecondEstimateExchange = int(jBatch["userFSecondEstimateExchange"])
    batchSpotTrade.userFThirdEstimateExchange = int(jBatch["userFThirdEstimateExchange"])

    batchSpotTrade.operatorFirstEstimateExchange = int(jBatch["operatorFirstEstimateExchange"])
    batchSpotTrade.operatorSecondEstimateExchange = int(jBatch["operatorSecondEstimateExchange"])
    batchSpotTrade.operatorThirdEstimateExchange = int(jBatch["operatorThirdEstimateExchange"])
    # batchSpotTrade.stableToken = int(jBatch["stableToken"])
    return batchSpotTrade

def ringFromJSON(jRing, state):
    orderA = orderFromJSON(jRing["orderA"], state)
    orderB = orderFromJSON(jRing["orderB"], state)

    ring = Ring(orderA, orderB)

    return ring

def createBlock(state, data):
    block = Block()
    block.exchange = str(data["exchange"])
    block.merkleRootBefore = str(state.getRoot())
    block.merkleAssetRootBefore = str(state.getAssetRoot())
    block.timestamp = int(data["timestamp"])
    block.protocolFeeBips = int(data["protocolFeeBips"])
    block.operatorAccountID = int(data["operatorAccountID"])
    context = Context(block.operatorAccountID, block.timestamp, block.protocolFeeBips)

    # Protocol fee payment
    accountBefore_P = copyAccountInfo(state.getAccount(0))

    for transactionInfo in data["transactions"]:
        txType = transactionInfo["txType"]
        if txType == "Noop":
            transaction = GeneralObject()
        if txType == "SpotTrade":
            transaction = ringFromJSON(transactionInfo, state)
        # DEG-358 BatchSpotTrade - opetator
        if txType == "BatchSpotTrade":
            transaction = batchSpotTradeFromJSON(transactionInfo, state)
        if txType == "Transfer":
            transaction = transferFromJSON(transactionInfo)
        if txType == "Withdraw":
            transaction = withdrawFromJSON(transactionInfo)
        if txType == "Deposit":
            transaction = depositFromJSON(transactionInfo)
        if txType == "AccountUpdate":
            transaction = accountUpdateFromJSON(transactionInfo)
        # DEG-146:order cancel
        if txType == "OrderCancel":
            transaction = orderCancelFromJSON(transactionInfo)
        if txType == "AppKeyUpdate":
            transaction = appKeyUpdateFromJSON(transactionInfo)

        transaction.txType = txType
        tx = state.executeTransaction(context, transaction)
        txWitness = GeneralObject()
        txWitness.witness = tx.witness
        if txType == "Noop":
            txWitness.noop = tx.input
        if txType == "SpotTrade":
            txWitness.spotTrade = tx.input
        # DEG-358 BatchSpotTrade - opetator
        if txType == "BatchSpotTrade":
            txWitness.batchSpotTrade = tx.input
        if txType == "Transfer":
            txWitness.transfer = tx.input
        if txType == "Withdraw":
            txWitness.withdraw = tx.input
        if txType == "Deposit":
            txWitness.deposit = tx.input
        if txType == "AccountUpdate":
            txWitness.accountUpdate = tx.input
        # DEG-146:order cancel
        if txType == "OrderCancel":
            txWitness.orderCancel = tx.input
        # DEG-170 auto market - config add
        if txType == "AutoMarketUpdate":
            txWitness.autoMarketUpdate = tx.input
        if txType == "AppKeyUpdate":
            txWitness.appKeyUpdate = tx.input
        txWitness.witness.numConditionalTransactionsAfter = context.numConditionalTransactions
        block.transactions.append(txWitness)


    print("stateMerkleRoot after transactions : " + str(state.getRoot()))
    print("stateMerkleAssetRoot after transactions : " + str(state.getAssetRoot()))

    # Protocol fees
    rootBefore = state._accountsTree._root
    rootAssetBefore = state._accountsAssetTree._root
    proof = state._accountsTree.createProof(0)
    proofAsset = state._accountsAssetTree.createProof(0)
    state.updateAccountTree(0)
    accountAfter = copyAccountInfo(state.getAccount(0))
    rootAfter = state._accountsTree._root
    rootAssetAfter = state._accountsAssetTree._root
    block.accountUpdate_P = AccountUpdateData(0, proof, proofAsset, rootBefore, rootAfter, rootAssetBefore, rootAssetAfter, accountBefore_P, accountAfter)

    print("stateMerkleRoot after Protocol fees : " + str(state.getRoot()))
    print("stateMerkleAssetRoot after Protocol fees : " + str(state.getAssetRoot()))

    # Operator
    account = state.getAccount(context.operatorAccountID)
    rootBefore = state._accountsTree._root
    rootAssetBefore = state._accountsAssetTree._root
    accountBefore = copyAccountInfo(state.getAccount(context.operatorAccountID))
    proof = state._accountsTree.createProof(context.operatorAccountID)
    proofAsset = state._accountsAssetTree.createProof(context.operatorAccountID)
    account.nonce += 1
    state.updateAccountTree(context.operatorAccountID)
    accountAfter = copyAccountInfo(state.getAccount(context.operatorAccountID))
    rootAfter = state._accountsTree._root
    rootAssetAfter = state._accountsAssetTree._root
    block.accountUpdate_O = AccountUpdateData(context.operatorAccountID, proof, proofAsset, rootBefore, rootAfter, rootAssetBefore, rootAssetAfter, accountBefore, accountAfter)

    block.merkleRootAfter = str(state.getRoot())
    block.merkleAssetRootAfter = str(state.getAssetRoot())

    print("stateMerkleRoot after Operator : " + str(state.getRoot()))
    print("stateMerkleAssetRoot after Operator : " + str(state.getAssetRoot()))

    return block

def main(exchangeID, blockIdx, blockType, inputFilename, outputFilename):
    previousBlockIdx = int(blockIdx) - 1
    previous_state_filename = "./states/state_" + str(exchangeID) + "_" + str(previousBlockIdx) + ".json"
    print("main previous_state_filename: " + previous_state_filename)

    state = State(exchangeID)
    if os.path.exists(previous_state_filename):
        state.load(previous_state_filename)

    with open(inputFilename) as f:
        data = json.load(f)

    stateMerkleRoot = state.getRoot()
    stateMerkleAssetRoot = state.getAssetRoot()
    print("empty newTree stateMerkleRoot : " + str(stateMerkleRoot))
    print("empty newTree stateMerkleAssetRoot : " + str(stateMerkleAssetRoot))
    print("stateMerkleRoot in str: " + str(hex(state.getRoot())))
    print("stateMerkleAssetRoot in str: " + str(hex(state.getAssetRoot())))

    block = createBlock(state, data)

    f = open(outputFilename,"w+")
    f.write(block.toJSON())
    f.close()

    pathlib.Path("./states").mkdir(parents=True, exist_ok=True)
    state_filename = "./states/state_" + str(exchangeID) + "_" + str(blockIdx) + ".json"
    state.save(state_filename)


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))
