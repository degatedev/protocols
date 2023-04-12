import sys
import json
import copy
from collections import namedtuple
from math import *

from sparse_merkle_tree import SparseMerkleTree
from float import *

from ethsnarks.eddsa import PureEdDSA
from ethsnarks.jubjub import Point
from ethsnarks.field import FQ
from ethsnarks.merkletree import MerkleTree
from ethsnarks.poseidon import poseidon, poseidon_params
from ethsnarks.field import SNARK_SCALAR_FIELD

from calculate import Calculate, Fill, BINARY_TREE_DEPTH_STORAGE, BINARY_TREE_DEPTH_ACCOUNTS, BINARY_TREE_DEPTH_TOKENS, MAX_BATCH_SPOT_TRADE_USER_ORDER, MAX_BATCH_SPOT_TRADE_ALL_USER_ORDER_MAX

# DEG-347 Storage location change
# poseidonParamsAccount = poseidon_params(SNARK_SCALAR_FIELD, 7, 6, 52, b'poseidon', 5, security_target=128)
# poseidonParamsAccount = poseidon_params(SNARK_SCALAR_FIELD, 9, 6, 53, b'poseidon', 5, security_target=128)
poseidonParamsAccount = poseidon_params(SNARK_SCALAR_FIELD, 12, 6, 53, b'poseidon', 5, security_target=128)
# poseidonParamsAccountAsset = poseidon_params(SNARK_SCALAR_FIELD, 6, 6, 52, b'poseidon', 5, security_target=128)
poseidonParamsAccountAsset = poseidon_params(SNARK_SCALAR_FIELD, 6, 6, 52, b'poseidon', 5, security_target=128)
# poseidonParamsAccount = poseidon_params(SNARK_SCALAR_FIELD, 8, 6, 53, b'poseidon', 5, security_target=128)
poseidonParamsBalance = poseidon_params(SNARK_SCALAR_FIELD, 5, 6, 52, b'poseidon', 5, security_target=128)
# DEG-347 Storage move
# poseidonParamsStorage = poseidon_params(SNARK_SCALAR_FIELD, 5, 6, 52, b'poseidon', 5, security_target=128)
poseidonParamsStorage = poseidon_params(SNARK_SCALAR_FIELD, 8, 6, 53, b'poseidon', 5, security_target=128)


# MAX_AMOUNT = 2 ** 96 - 1
MAX_AMOUNT = 2 ** 248 - 1
# DEG-358 BatchSpotTrade - opetator

class GeneralObject(object):
    pass

def setValue(value, default):
    return default if value is None else value

def copyBalanceInfo(leaf):
    c = copy.deepcopy(leaf)
    return c

def copyAccountInfo(account):
    c = copy.deepcopy(account)
    c.balancesRoot = str(account._balancesTree._root)
    c._balancesTree = None
    c._balancesLeafs = None
    # DEG-347 Storage location change
    # Storage
    c.storageRoot = str(account._storageTree._root)
    c._storageTree = None
    c._storageLeafs = None
    return c

def getDefaultAccount():
    return Account(0, Point(0, 0), Point(0, 0))

class Context(object):
    def __init__(self, operatorAccountID, timestamp, protocolFeeBips):
        self.operatorAccountID = int(operatorAccountID)
        self.timestamp = int(timestamp)
        self.protocolFeeBips = int(protocolFeeBips)
        self.numConditionalTransactions = int(0)

class Signature(object):
    def __init__(self, sig):
        if sig != None:
            self.Rx = str(sig.R.x)
            self.Ry = str(sig.R.y)
            self.s = str(sig.s)
        else:
            self.Rx = "0"
            self.Ry = "0"
            self.s = "0"

class BalanceLeaf(object):
    def __init__(self, balance = 0):
        self.balance = str(balance)

    def hash(self):
        #print("balance: " + self.balance)
        temp = [int(self.balance)]
        #print(temp)
        return poseidon(temp, poseidonParamsBalance)

    def fromJSON(self, jBalance):
        self.balance = jBalance["balance"]

class StorageLeaf(object):
    # split trading fee and gas fee - add up gas
    # DEG-146:order cancel
    # DEG-347 Storage move
    def __init__(self, tokenSID = 0, tokenBID = 0, data = 0, storageID = 0, gasFee = 0, cancelled = 0, forward = 1):
        self.tokenSID = str(tokenSID)
        self.tokenBID = str(tokenBID)
        self.data = str(data)
        self.storageID = str(storageID)
        self.gasFee = str(gasFee)
        self.cancelled = str(cancelled)
        self.forward = int(forward)

    def hash(self):
        # split trading fee and gas fee - add up gas
        # There is no need to modify the Poseidon parameter here because there is surplus in the pit position. Reference balance leaf
        # DEG-146:order cancel
        # DEG-347 Storage move
        return poseidon([int(self.tokenSID), int(self.tokenBID), int(self.data), int(self.storageID), int(self.gasFee), int(self.cancelled), int(self.forward)], poseidonParamsStorage)

    def fromJSON(self, jBalance):
        # DEG-347 Storage move
        self.tokenSID = jBalance["tokenSID"]
        self.tokenBID = jBalance["tokenBID"]
        self.data = jBalance["data"]
        self.storageID = jBalance["storageID"]
        self.gasFee = jBalance["gasFee"]
        # DEG-146:order cancel
        self.cancelled = jBalance["cancelled"]
        # DEG-347 Storage move
        self.forward = int(jBalance["forward"])

class Account(object):
    def __init__(self, owner, publicKey, appKeyPublicKey):
        self.owner = str(owner)
        self.publicKeyX = str(publicKey.x)
        self.publicKeyY = str(publicKey.y)
        self.appKeyPublicKeyX = str(appKeyPublicKey.x)
        self.appKeyPublicKeyY = str(appKeyPublicKey.y)
        self.nonce = 0
        self.disableAppKeySpotTrade = 0
        self.disableAppKeyWithdraw = 0
        self.disableAppKeyTransferToOther = 0
        # Balances
        self._balancesTree = SparseMerkleTree(BINARY_TREE_DEPTH_TOKENS // 2, 4)
        self._balancesTree.newTree(BalanceLeaf().hash())
        self._balancesLeafs = {}
        # DEG-347 Storage location change
        # Storage
        self._storageTree = SparseMerkleTree(BINARY_TREE_DEPTH_STORAGE // 2, 4)
        self._storageTree.newTree(StorageLeaf().hash())
        self._storageLeafs = {}
        print("Empty storage tree: " + str(self._storageTree._root))

    def hash(self):
        # DEG-347 Storage location change
        # return poseidon([int(self.owner), int(self.publicKeyX), int(self.publicKeyY), int(self.nonce), int(self._balancesTree._root), int(self._autoMarketTree._root), int(self._storageTree._root)], poseidonParamsAccount)
        return poseidon([int(self.owner), int(self.publicKeyX), int(self.publicKeyY), int(self.appKeyPublicKeyX), int(self.appKeyPublicKeyY), int(self.nonce), int(self.disableAppKeySpotTrade), int(self.disableAppKeyWithdraw), int(self.disableAppKeyTransferToOther), int(self._balancesTree._root), int(self._storageTree._root)], poseidonParamsAccount)
    def hashAsset(self):
        # DEG-347 Storage location change
        # return poseidon([int(self.owner), int(self.publicKeyX), int(self.publicKeyY), int(self.nonce), int(self._balancesTree._root), int(self._autoMarketTree._root), int(self._storageTree._root)], poseidonParamsAccount)
        return poseidon([int(self.owner), int(self.publicKeyX), int(self.publicKeyY), int(self.nonce), int(self._balancesTree._root)], poseidonParamsAccountAsset)

    def fromJSON(self, jAccount):
        self.owner = jAccount["owner"]
        self.publicKeyX = jAccount["publicKeyX"]
        self.publicKeyY = jAccount["publicKeyY"]
        self.appKeyPublicKeyX = jAccount["appKeyPublicKeyX"]
        self.appKeyPublicKeyY = jAccount["appKeyPublicKeyY"]
        self.nonce = int(jAccount["nonce"])
        self.disableAppKeySpotTrade = int(jAccount["disableAppKeySpotTrade"])
        self.disableAppKeyWithdraw = int(jAccount["disableAppKeyWithdraw"])
        self.disableAppKeyTransferToOther = int(jAccount["disableAppKeyTransferToOther"])

        # Balances
        balancesLeafsDict = jAccount["_balancesLeafs"]
        for key, val in balancesLeafsDict.items():
            balanceLeaf = BalanceLeaf()
            balanceLeaf.fromJSON(val)
            self._balancesLeafs[key] = balanceLeaf
        self._balancesTree._root = jAccount["_balancesTree"]["_root"]
        self._balancesTree._db.kv = jAccount["_balancesTree"]["_db"]["kv"]
        # DEG-347 Storage location change
        # Storage
        storageLeafsDict = jAccount["_storageLeafs"]
        for key, val in storageLeafsDict.items():
            # split trading fee and gas fee - add up gas
            # DEG-146:order cancel
            self._storageLeafs[key] = StorageLeaf(val["tokenSID"], val["tokenBID"], val["data"], val["storageID"], val["gasFee"], val["cancelled"], val["forward"])
        self._storageTree._root = jAccount["_storageTree"]["_root"]
        self._storageTree._db.kv = jAccount["_storageTree"]["_db"]["kv"]

    def getBalanceLeaf(self, address):
        # Make sure the leaf exist in our map
        if not(str(address) in self._balancesLeafs):
            return BalanceLeaf()
        else:
            return self._balancesLeafs[str(address)]
    # DEG-347 Storage location change
    def getStorage(self, storageID):
        address = int(storageID) % (2 ** BINARY_TREE_DEPTH_STORAGE)
        # Make sure the leaf exist in our map
        if not(str(address) in self._storageLeafs):
            return StorageLeaf()
        else:
            return self._storageLeafs[str(address)]

    def getBalance(self, address):
        return self.getBalanceLeaf(address).balance

    def updateBalance(self, tokenID, deltaBalance):
        # Make sure the leaf exists in our map
        if not(str(tokenID) in self._balancesLeafs):
            self._balancesLeafs[str(tokenID)] = BalanceLeaf()

        balancesBefore = copyBalanceInfo(self._balancesLeafs[str(tokenID)])
        rootBefore = self._balancesTree._root
        print("in updateBalance before:" + str(self._balancesLeafs[str(tokenID)].balance))
        self._balancesLeafs[str(tokenID)].balance = str(int(self._balancesLeafs[str(tokenID)].balance) + int(deltaBalance))

        balancesAfter = copyBalanceInfo(self._balancesLeafs[str(tokenID)])
        proof = self._balancesTree.createProof(tokenID)
        self._balancesTree.update(tokenID, self._balancesLeafs[str(tokenID)].hash())
        rootAfter = self._balancesTree._root

        print("in updateBalance after:" + str(self._balancesLeafs[str(tokenID)].balance))
        return BalanceUpdateData(tokenID, proof,
                                 rootBefore, rootAfter,
                                 balancesBefore, balancesAfter)
    # split trading fee and gas fee - add up gas
    # DEG-146:order cancel
    # DEG-347 Storage move
    def updateBalanceAndStorage(self, tokenID, tokenSID, tokenBID, storageID, filled, storageGasFee, storageCancelled, delta_balance, forward):
        # Make sure the leaf exist in our map
        if not(str(tokenID) in self._balancesLeafs):
            self._balancesLeafs[str(tokenID)] = BalanceLeaf()

        balancesBefore = copyBalanceInfo(self._balancesLeafs[str(tokenID)])
        rootBefore = self._balancesTree._root

        # Update filled amounts
        # split trading fee and gas fee - add up gas
        # DEG-146:order cancel
        # DEG-347 Storage location change
        # DEG-347 Storage move
        storageUpdate = self.updateStorage(storageID, tokenSID, tokenBID, filled, storageGasFee, storageCancelled, forward)

        self._balancesLeafs[str(tokenID)].balance = str(int(self._balancesLeafs[str(tokenID)].balance) + int(delta_balance))

        #print("str(delta_balance): " + str(delta_balance))
        #print("endBalance: " + self._balancesLeafs[str(tokenID)].balance)

        balancesAfter = copyBalanceInfo(self._balancesLeafs[str(tokenID)])
        proof = self._balancesTree.createProof(tokenID)
        self._balancesTree.update(tokenID, self._balancesLeafs[str(tokenID)].hash())
        rootAfter = self._balancesTree._root

        print("tokenID:" + str(tokenID))
        print("balance:" + str(self._balancesLeafs[str(tokenID)].balance))
        print("balancesLeaf:" + str(self._balancesLeafs[str(tokenID)].hash()))

        return (BalanceUpdateData(tokenID, proof,
                                 rootBefore, rootAfter,
                                 balancesBefore, balancesAfter),
                # DEG-358 BatchSpotTrade - opetator
                storageUpdate)

    def updateBatchStorage(self, storageIDArray, tokenSIDArray, tokenBIDArray, filledArray, storageExtraArray, storageCancelledArray, forwardArray, length):
        storageUpdateArray = []
        index = 0
        for storageIDTemp in storageIDArray:
            storageUpdateTemp = self.updateStorage(storageIDTemp, tokenSIDArray[index], tokenBIDArray[index], filledArray[index], storageExtraArray[index], storageCancelledArray[index], forwardArray[index])
            storageUpdateArray.append(storageUpdateTemp)
            index = index + 1
        # If the storage update quantity is insufficient, the default data is used instead
        if index < length:
            for i in range(length - index):
                storage = self.getStorage(0)
                storageUpdateTemp = self.updateStorage(storage.storageID, storage.tokenSID, storage.tokenBID, storage.data, storage.gasFee, storage.cancelled, storage.forward)
                storageUpdateArray.append(storageUpdateTemp)
        return storageUpdateArray
    # DEG-146:order cancel
    # At present, gasfee is used for gas accumulation, and the order cancelled is marked from 0 to greater than 0
    # DEG-347 Storage move
    def updateStorage(self, storageID, tokenSID, tokenBID, data, gasFee, cancelled, forward):
        address = int(storageID) % (2 ** BINARY_TREE_DEPTH_STORAGE)
        # Make sure the leaf exist in our map
        if not(str(address) in self._storageLeafs):
            # split trading fee and gas fee - add up gas
            self._storageLeafs[str(address)] = StorageLeaf(0, 0, 0, 0, 0, 0, 1)

        leafBefore = copy.deepcopy(self._storageLeafs[str(address)])
        rootBefore = self._storageTree._root
        # print("leafBefore ID==========: " + str(leafBefore.storageID))
        # print("leafBefore Data==========: " + str(leafBefore.data))
        self._storageLeafs[str(address)].tokenSID = str(tokenSID)
        self._storageLeafs[str(address)].tokenBID = str(tokenBID)
        self._storageLeafs[str(address)].data = str(data)
        self._storageLeafs[str(address)].storageID = str(storageID)
        self._storageLeafs[str(address)].gasFee = str(gasFee)
        
        self._storageLeafs[str(address)].cancelled = str(int(cancelled))
        self._storageLeafs[str(address)].forward = int(forward)
        leafAfter = copy.deepcopy(self._storageLeafs[str(address)])
        # print("leafAfter ID: " + str(leafAfter.storageID))
        # print("leafAfter Data: " + str(leafAfter.data))
        proof = self._storageTree.createProof(address)
        self._storageTree.update(address, leafAfter.hash())
        rootAfter = self._storageTree._root

        return StorageUpdateData(storageID, proof,
                                 rootBefore, rootAfter,
                                 leafBefore, leafAfter)

def write_proof(proof):
    # return [[str(_) for _ in proof_level] for proof_level in proof]
    return [str(_) for _ in proof]

class StorageUpdateData(object):
    def __init__(self,
                 storageID, proof,
                 rootBefore, rootAfter,
                 before, after):
        self.storageID = str(storageID)
        self.proof = write_proof(proof)
        self.rootBefore = str(rootBefore)
        self.rootAfter = str(rootAfter)
        self.before = before
        self.after = after
class BalanceUpdateData(object):
    def __init__(self,
                 tokenID, proof,
                 rootBefore, rootAfter,
                 before, after):
        self.tokenID = int(tokenID)
        self.proof = write_proof(proof)
        self.rootBefore = str(rootBefore)
        self.rootAfter = str(rootAfter)
        self.before = before
        self.after = after

class AccountUpdateData(object):
    def __init__(self,
                 accountID, proof, proofAsset,
                 rootBefore, rootAfter,
                 rootAssetBefore, rootAssetAfter,
                 before, after):
        self.accountID = int(accountID)
        self.proof = write_proof(proof)
        self.assetProof = write_proof(proofAsset)
        self.rootBefore = str(rootBefore)
        self.rootAfter = str(rootAfter)
        self.assetRootBefore = str(rootAssetBefore)
        self.assetRootAfter = str(rootAssetAfter)
        self.before = before
        self.after = after

class Order(object):
    def __init__(self,
                 publicKeyX, publicKeyY,
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
                 useAppKey):
        self.publicKeyX = str(publicKeyX)
        self.publicKeyY = str(publicKeyY)

        self.storageID = str(storageID)
        self.accountID = int(accountID)

        self.amountS = str(amountS)
        self.amountB = str(amountB)
        # DEG-358 BatchSpotTrade - opetator
        self.deltaFilledS = str(deltaFilledS)
        self.deltaFilledB = str(deltaFilledB)

        self.tokenS = tokenS
        self.tokenB = tokenB

        self.validUntil = validUntil
        self.fillAmountBorS = bool(fillAmountBorS)
        self.taker = str(taker)
        # split TradingFee and GasFee - ProtocolFeeBips as the max TradingFee
        # self.maxFeeBips = maxFeeBips

        self.feeBips = feeBips
        self.tradingFee = str(tradingFee)
        # split trading fee and gas fee
        self.feeTokenID = feeTokenID
        self.fee = str(fee)
        self.maxFee = str(maxFee)
        # DEG-265 auto market
        self.type = int(type)
        # self.autoMarketID = int(autoMarketID)
        self.level = int(level)
        self.startOrder = startOrder
        self.gridOffset = str(gridOffset)
        self.orderOffset = str(orderOffset)
        self.maxLevel = int(maxLevel)

        self.useAppKey = int(useAppKey)

        # DEG-358 BatchSpotTrade - opetator
        self.isNoop = 0

    def checkValid(self, context, order, fillAmountS, fillAmountB):
        valid = True
        valid = valid and (context.timestamp <= self.validUntil)
        valid = valid and self.checkFillRate(int(order.amountS), int(order.amountB), fillAmountS, fillAmountB)
        self.valid = valid

    def checkFillRate(self, amountS, amountB, fillAmountS, fillAmountB):
        # Return true if the fill rate <= 0.1% worse than the target rate
        # (fillAmountS/fillAmountB) * 1000 <= (amountS/amountB) * 1001
        return (fillAmountS * amountB * 1000) <= (fillAmountB * amountS * 1001)

class Ring(object):
    def __init__(self, orderA, orderB):
        self.orderA = orderA
        self.orderB = orderB
# DEG-358 BatchSpotTrade - opetator
class BatchSpotTradeUser(object):
    def __init__(self):
        self.accountID = 0
        self.order = []

class TxWitness(object):
    def __init__(self, witness, input):
        self.witness = witness
        self.input = input

class Witness(object):
    def __init__(self,
                 # DEG-358 BatchSpotTrade - opetator
                 signatureA, signatureB, signatures,
                 accountsMerkleRoot, accountsMerkleAssetRoot,
                 storageUpdate_A, storageUpdate_B,
                 # DEG-358 BatchSpotTrade - opetator
                 storageUpdate_A_array, storageUpdate_B_array,
                 storageUpdate_C_array, storageUpdate_D_array,
                 storageUpdate_E_array, storageUpdate_F_array,
                 balanceUpdateS_A, balanceUpdateB_A, balanceUpdateFee_A, accountUpdate_A,
                 balanceUpdateS_B, balanceUpdateB_B, balanceUpdateFee_B, accountUpdate_B,
                 # DEG-358 BatchSpotTrade - opetator
                 balanceUpdateS_C, balanceUpdateB_C, balanceUpdateFee_C, accountUpdate_C,
                 balanceUpdateS_D, balanceUpdateB_D, balanceUpdateFee_D, accountUpdate_D,
                 balanceUpdateS_E, balanceUpdateB_E, balanceUpdateFee_E, accountUpdate_E,
                 balanceUpdateS_F, balanceUpdateB_F, balanceUpdateFee_F, accountUpdate_F,

                 balanceUpdateA_O, balanceUpdateB_O, balanceUpdateC_O, balanceUpdateD_O, accountUpdate_O
                 ):
        if signatureA is not None:
            self.signatureA = signatureA
        if signatureB is not None:
            self.signatureB = signatureB
        # DEG-358 BatchSpotTrade - opetator
        if signatures is not None:
            self.signatures = signatures

        self.accountsMerkleRoot = str(accountsMerkleRoot)
        self.accountsMerkleAssetRoot = str(accountsMerkleAssetRoot)

        self.storageUpdate_A = storageUpdate_A
        self.storageUpdate_B = storageUpdate_B
        # DEG-358 BatchSpotTrade - opetator
        self.storageUpdate_A_array = storageUpdate_A_array
        self.storageUpdate_B_array = storageUpdate_B_array
        self.storageUpdate_C_array = storageUpdate_C_array
        self.storageUpdate_D_array = storageUpdate_D_array
        self.storageUpdate_E_array = storageUpdate_E_array
        self.storageUpdate_F_array = storageUpdate_F_array

        self.balanceUpdateS_A = balanceUpdateS_A
        self.balanceUpdateB_A = balanceUpdateB_A
        # split trading fee and gas fee
        self.balanceUpdateFee_A = balanceUpdateFee_A
        self.accountUpdate_A = accountUpdate_A

        self.balanceUpdateS_B = balanceUpdateS_B
        self.balanceUpdateB_B = balanceUpdateB_B
        # split trading fee and gas fee
        self.balanceUpdateFee_B = balanceUpdateFee_B
        self.accountUpdate_B = accountUpdate_B
        # DEG-358 BatchSpotTrade - opetator
        self.balanceUpdateS_C = balanceUpdateS_C
        self.balanceUpdateB_C = balanceUpdateB_C
        self.balanceUpdateFee_C = balanceUpdateFee_C
        self.accountUpdate_C = accountUpdate_C


        self.balanceUpdateS_D = balanceUpdateS_D
        self.balanceUpdateB_D = balanceUpdateB_D
        self.balanceUpdateFee_D = balanceUpdateFee_D
        self.accountUpdate_D = accountUpdate_D

        self.balanceUpdateS_E = balanceUpdateS_E
        self.balanceUpdateB_E = balanceUpdateB_E
        self.balanceUpdateFee_E = balanceUpdateFee_E
        self.accountUpdate_E = accountUpdate_E

        self.balanceUpdateS_F = balanceUpdateS_F
        self.balanceUpdateB_F = balanceUpdateB_F
        self.balanceUpdateFee_F = balanceUpdateFee_F
        self.accountUpdate_F = accountUpdate_F


        self.balanceUpdateD_O = balanceUpdateD_O
        self.balanceUpdateC_O = balanceUpdateC_O
        self.balanceUpdateB_O = balanceUpdateB_O
        self.balanceUpdateA_O = balanceUpdateA_O
        self.accountUpdate_O = accountUpdate_O

class State(object):
    def __init__(self, exchangeID):
        self.exchangeID = int(exchangeID)
        # Accounts
        self._accountsTree = SparseMerkleTree(BINARY_TREE_DEPTH_ACCOUNTS // 2, 4)
        self._accountsTree.newTree(getDefaultAccount().hash())
        # AssetTree
        self._accountsAssetTree = SparseMerkleTree(BINARY_TREE_DEPTH_ACCOUNTS // 2, 4)
        self._accountsAssetTree.newTree(getDefaultAccount().hashAsset())
        self._accounts = {}
        self._accounts[str(0)] = getDefaultAccount()
        self._accounts[str(1)] = getDefaultAccount()
        self.calculate = Calculate()
        print("Empty accounts tree: " + str(hex(self._accountsTree._root)))
        print("Empty accounts asset tree: " + str(hex(self._accountsAssetTree._root)))

    def load(self, filename):
        with open(filename) as f:
            data = json.load(f)
            self.exchangeID = int(data["exchangeID"])
            # Accounts
            accountLeafsDict = data["accounts_values"]
            for key, val in accountLeafsDict.items():
                account = getDefaultAccount()
                account.fromJSON(val)
                self._accounts[key] = account
            self._accountsTree._root = data["accounts_root"]
            self._accountsTree._db.kv = data["accounts_tree"]
            self._accountsAssetTree._root = data["accounts_asset_root"]
            self._accountsAssetTree._db.kv = data["accounts_asset_tree"]

    def save(self, filename):
        with open(filename, "w") as file:
            file.write(json.dumps(
                {
                    "exchangeID": self.exchangeID,
                    "accounts_values": self._accounts,
                    "accounts_root": self._accountsTree._root,
                    "accounts_tree": self._accountsTree._db.kv,
                    "accounts_asset_root": self._accountsAssetTree._root,
                    "accounts_asset_tree": self._accountsAssetTree._db.kv,
                }, default=lambda o: o.__dict__, sort_keys=True, indent=4))
    
    def batchSpotTradeDataCheck(self, batchSpotTrade):
        if not batchSpotTrade.enableDataCheck:
            print("in batchSpotTradeBalanceCheck not enable ddata check")
            return

        assert batchSpotTrade.userAFirstEstimateExchange == batchSpotTrade.userAFirstTokenExchange, "userAFirstEstimateExchange != userAFirstTokenExchange;userAFirstEstimateExchange" + str(batchSpotTrade.userAFirstEstimateExchange) + ";userAFirstTokenExchange:" + str(batchSpotTrade.userAFirstTokenExchange)
        assert batchSpotTrade.userASecondEstimateExchange == batchSpotTrade.userASecondTokenExchange, "userASecondEstimateExchange != userASecondTokenExchange;userAFirstEstimateExchange" + str(batchSpotTrade.userASecondEstimateExchange) + ";userASecondTokenExchange:" + str(batchSpotTrade.userASecondTokenExchange)
        assert batchSpotTrade.userAThirdEstimateExchange == batchSpotTrade.userAThirdTokenExchange, "userAThirdEstimateExchange != userAThirdTokenExchange;userAFirstEstimateExchange" + str(batchSpotTrade.userAThirdEstimateExchange) + ";userAThirdTokenExchange:" + str(batchSpotTrade.userAThirdTokenExchange)

        assert batchSpotTrade.userBFirstEstimateExchange == batchSpotTrade.userBFirstTokenExchange, "userBFirstEstimateExchange != userBFirstTokenExchange;userBFirstEstimateExchange" + str(batchSpotTrade.userBFirstEstimateExchange) + ";userBFirstTokenExchange:" + str(batchSpotTrade.userBFirstTokenExchange)
        assert batchSpotTrade.userBSecondEstimateExchange == batchSpotTrade.userBSecondTokenExchange, "userBSecondEstimateExchange != userBSecondTokenExchange;userBSecondEstimateExchange" + str(batchSpotTrade.userBSecondEstimateExchange) + ";userBSecondTokenExchange:" + str(batchSpotTrade.userBSecondTokenExchange)
        assert batchSpotTrade.userBThirdEstimateExchange == batchSpotTrade.userBThirdTokenExchange, "userBThirdEstimateExchange != userBThirdTokenExchange;userBThirdEstimateExchange" + str(batchSpotTrade.userBThirdEstimateExchange) + ";userBThirdTokenExchange:" + str(batchSpotTrade.userBThirdTokenExchange)

        assert batchSpotTrade.userCFirstEstimateExchange == batchSpotTrade.userCFirstTokenExchange, "userCFirstEstimateExchange != userCFirstTokenExchange;userCFirstEstimateExchange" + str(batchSpotTrade.userCFirstEstimateExchange) + ";userCFirstTokenExchange:" + str(batchSpotTrade.userCFirstTokenExchange)
        assert batchSpotTrade.userCSecondEstimateExchange == batchSpotTrade.userCSecondTokenExchange, "userCSecondEstimateExchange != userCSecondTokenExchange;userCSecondEstimateExchange" + str(batchSpotTrade.userCSecondEstimateExchange) + ";userCSecondTokenExchange:" + str(batchSpotTrade.userCSecondTokenExchange)
        assert batchSpotTrade.userCThirdEstimateExchange == batchSpotTrade.userCThirdTokenExchange, "userCThirdEstimateExchange != userCThirdTokenExchange;userCThirdEstimateExchange" + str(batchSpotTrade.userCThirdEstimateExchange) + ";userCThirdTokenExchange:" + str(batchSpotTrade.userCThirdTokenExchange)

        assert batchSpotTrade.userDFirstEstimateExchange == batchSpotTrade.userDFirstTokenExchange, "userDFirstEstimateExchange != userDFirstTokenExchange;userDFirstEstimateExchange" + str(batchSpotTrade.userDFirstEstimateExchange) + ";userDFirstTokenExchange:" + str(batchSpotTrade.userDFirstTokenExchange)
        assert batchSpotTrade.userDSecondEstimateExchange == batchSpotTrade.userDSecondTokenExchange, "userDSecondEstimateExchange != userDSecondTokenExchange;userDSecondEstimateExchange" + str(batchSpotTrade.userDSecondEstimateExchange) + ";userDSecondTokenExchange:" + str(batchSpotTrade.userDSecondTokenExchange)
        assert batchSpotTrade.userDThirdEstimateExchange == batchSpotTrade.userDThirdTokenExchange, "userDThirdEstimateExchange != userDThirdTokenExchange;userDThirdEstimateExchange" + str(batchSpotTrade.userDThirdEstimateExchange) + ";userDThirdTokenExchange:" + str(batchSpotTrade.userDThirdTokenExchange)

        assert batchSpotTrade.userEFirstEstimateExchange == batchSpotTrade.userEFirstTokenExchange, "userEFirstEstimateExchange != userEFirstTokenExchange;userEFirstEstimateExchange" + str(batchSpotTrade.userEFirstEstimateExchange) + ";userEFirstTokenExchange:" + str(batchSpotTrade.userEFirstTokenExchange)
        assert batchSpotTrade.userESecondEstimateExchange == batchSpotTrade.userESecondTokenExchange, "userESecondEstimateExchange != userESecondTokenExchange;userESecondEstimateExchange" + str(batchSpotTrade.userESecondEstimateExchange) + ";userESecondTokenExchange:" + str(batchSpotTrade.userESecondTokenExchange)
        assert batchSpotTrade.userEThirdEstimateExchange == batchSpotTrade.userEThirdTokenExchange, "userEThirdEstimateExchange != userEThirdTokenExchange;userEThirdEstimateExchange" + str(batchSpotTrade.userEThirdEstimateExchange) + ";userEThirdTokenExchange:" + str(batchSpotTrade.userEThirdTokenExchange)

        assert batchSpotTrade.userFFirstEstimateExchange == batchSpotTrade.userFFirstTokenExchange, "userFFirstEstimateExchange != userFFirstTokenExchange;userFFirstEstimateExchange" + str(batchSpotTrade.userFFirstEstimateExchange) + ";userFFirstTokenExchange:" + str(batchSpotTrade.userFFirstTokenExchange)
        assert batchSpotTrade.userFSecondEstimateExchange == batchSpotTrade.userFSecondTokenExchange, "userFSecondEstimateExchange != userFSecondTokenExchange;userFSecondEstimateExchange" + str(batchSpotTrade.userFSecondEstimateExchange) + ";userFSecondTokenExchange:" + str(batchSpotTrade.userFSecondTokenExchange)
        assert batchSpotTrade.userFThirdEstimateExchange == batchSpotTrade.userFThirdTokenExchange, "userFThirdEstimateExchange != userFThirdTokenExchange;userFThirdEstimateExchange" + str(batchSpotTrade.userFThirdEstimateExchange) + ";userFThirdTokenExchange:" + str(batchSpotTrade.userFThirdTokenExchange)

        assert batchSpotTrade.operatorFirstEstimateExchange == batchSpotTrade.operatorFirstTokenExchange, "operatorFirstEstimateExchange != operatorFirstTokenExchange;operatorFirstEstimateExchange" + str(batchSpotTrade.operatorFirstEstimateExchange) + ";operatorFirstTokenExchange:" + str(batchSpotTrade.operatorFirstTokenExchange)
        assert batchSpotTrade.operatorSecondEstimateExchange == batchSpotTrade.operatorSecondTokenExchange, "operatorSecondEstimateExchange != operatorSecondTokenExchange;operatorSecondEstimateExchange" + str(batchSpotTrade.operatorSecondEstimateExchange) + ";operatorSecondTokenExchange:" + str(batchSpotTrade.operatorSecondTokenExchange)
        assert batchSpotTrade.operatorThirdEstimateExchange == batchSpotTrade.operatorThirdTokenExchange, "operatorThirdEstimateExchange != operatorThirdTokenExchange;operatorThirdEstimateExchange" + str(batchSpotTrade.operatorThirdEstimateExchange) + ";operatorThirdTokenExchange:" + str(batchSpotTrade.operatorThirdTokenExchange)
        
    def executeTransaction(self, context, txInput):
        newState = GeneralObject()
        newState.signatureA = None
        newState.signatureB = None
        newState.signatures = None
        newState.signaturesTemp = None
        # =========== User A
        # Tokens
        newState.TXV_BALANCE_A_S_ADDRESS = None
        newState.TXV_BALANCE_A_S_ADDRESS = None
        # A
        newState.TXV_ACCOUNT_A_ADDRESS = None
        newState.TXV_ACCOUNT_A_OWNER = None
        newState.TXV_ACCOUNT_A_PUBKEY_X = None
        newState.TXV_ACCOUNT_A_PUBKEY_Y = None
        newState.TXV_ACCOUNT_A_APPKEY_PUBKEY_X = None
        newState.TXV_ACCOUNT_A_APPKEY_PUBKEY_Y = None
        newState.TXV_ACCOUNT_A_NONCE = None
        newState.TXV_ACCOUNT_A_DISABLE_APPKEY_SPOTTRADE = None
        newState.TXV_ACCOUNT_A_DISABLE_APPKEY_WITHDRAW = None
        newState.TXV_ACCOUNT_A_DISABLE_APPKEY_TRANSFER_TO_OTHER = None

        newState.TXV_BALANCE_A_S_ADDRESS = None
        newState.TXV_BALANCE_A_S_BALANCE = None
        newState.TXV_BALANCE_A_B_ADDRESS = None
        newState.TXV_BALANCE_A_B_BALANCE = None
        # split trading fee and gas fee
        # A fee
        newState.TXV_BALANCE_A_FEE_BALANCE = None
        # DEG-127 split trading fee and gas fee
        newState.TXV_ACCOUNT_A_FEE_ADDRESS = None

        newState.TXV_STORAGE_A_ADDRESS = None
        # DEG-347 Storage move
        newState.TXV_STORAGE_A_TOKENSID = None
        newState.TXV_STORAGE_A_TOKENBID = None
        newState.TXV_STORAGE_A_DATA = None
        newState.TXV_STORAGE_A_STORAGEID = None        
        newState.TXV_STORAGE_A_GASFEE = None
        # DEG-146:order cancel
        newState.TXV_STORAGE_A_CANCELLED = None
        # DEG-347 Storage move
        newState.TXV_STORAGE_A_FORWARD = None
        # DEG-358 BatchSpotTrade - opetator
        newState.TXV_STORAGE_A_ADDRESS_ARRAY = []
        newState.TXV_STORAGE_A_TOKENSID_ARRAY = []
        newState.TXV_STORAGE_A_TOKENBID_ARRAY = []
        newState.TXV_STORAGE_A_DATA_ARRAY = []
        newState.TXV_STORAGE_A_STORAGEID_ARRAY = []
        newState.TXV_STORAGE_A_GASFEE_ARRAY = []
        newState.TXV_STORAGE_A_CANCELLED_ARRAY = []
        newState.TXV_STORAGE_A_FORWARD_ARRAY = []

        # ==========User B
        # B
        newState.TXV_ACCOUNT_B_ADDRESS = None
        newState.TXV_ACCOUNT_B_OWNER = None
        newState.TXV_ACCOUNT_B_PUBKEY_X = None
        newState.TXV_ACCOUNT_B_PUBKEY_Y = None
        newState.TXV_ACCOUNT_B_NONCE = None
        newState.TXV_BALANCE_B_S_ADDRESS = None
        newState.TXV_BALANCE_B_S_BALANCE = None
        newState.TXV_BALANCE_B_B_ADDRESS = None
        newState.TXV_BALANCE_B_B_BALANCE = None
        # split trading fee and gas fee
        # B fee
        newState.TXV_BALANCE_B_FEE_BALANCE = None

        # DEG-127 split trading fee and gas fee
        newState.TXV_ACCOUNT_B_FEE_ADDRESS = None

        newState.TXV_STORAGE_B_ADDRESS = None
        # DEG-347 Storage move
        newState.TXV_STORAGE_B_TOKENSID = None
        newState.TXV_STORAGE_B_TOKENBID = None
        newState.TXV_STORAGE_B_DATA = None
        newState.TXV_STORAGE_B_STORAGEID = None
        newState.TXV_STORAGE_B_GASFEE = None
        # DEG-347 Storage move
        newState.TXV_STORAGE_B_FORWARD = None
        # DEG-358 BatchSpotTrade - opetator
        newState.TXV_STORAGE_B_ADDRESS_ARRAY = []
        newState.TXV_STORAGE_B_TOKENSID_ARRAY = []
        newState.TXV_STORAGE_B_TOKENBID_ARRAY = []
        newState.TXV_STORAGE_B_DATA_ARRAY = []
        newState.TXV_STORAGE_B_STORAGEID_ARRAY = []
        newState.TXV_STORAGE_B_GASFEE_ARRAY = []
        newState.TXV_STORAGE_B_CANCELLED_ARRAY = []
        newState.TXV_STORAGE_B_FORWARD_ARRAY = []
        # DEG-358 BatchSpotTrade - opetator
        # =============User C
        newState.TXV_ACCOUNT_C_ADDRESS = None
        newState.TXV_ACCOUNT_C_OWNER = None
        newState.TXV_ACCOUNT_C_PUBKEY_X = None
        newState.TXV_ACCOUNT_C_PUBKEY_Y = None
        newState.TXV_ACCOUNT_C_NONCE = None
        newState.TXV_BALANCE_C_S_ADDRESS = None
        newState.TXV_BALANCE_C_S_BALANCE = None
        newState.TXV_BALANCE_C_B_ADDRESS = None
        newState.TXV_BALANCE_C_B_BALANCE = None
        # C fee
        newState.TXV_BALANCE_C_FEE_BALANCE = None

        newState.TXV_ACCOUNT_C_FEE_ADDRESS = None

        newState.TXV_STORAGE_C_ADDRESS_ARRAY = []
        newState.TXV_STORAGE_C_TOKENSID_ARRAY = []
        newState.TXV_STORAGE_C_TOKENBID_ARRAY = []
        newState.TXV_STORAGE_C_DATA_ARRAY = []
        newState.TXV_STORAGE_C_STORAGEID_ARRAY = []
        newState.TXV_STORAGE_C_GASFEE_ARRAY = []
        newState.TXV_STORAGE_C_CANCELLED_ARRAY = []
        newState.TXV_STORAGE_C_FORWARD_ARRAY = []
        # DEG-358 BatchSpotTrade - opetator
        # =============User D
        newState.TXV_ACCOUNT_D_ADDRESS = None
        newState.TXV_ACCOUNT_D_OWNER = None
        newState.TXV_ACCOUNT_D_PUBKEY_X = None
        newState.TXV_ACCOUNT_D_PUBKEY_Y = None
        newState.TXV_ACCOUNT_D_NONCE = None
        newState.TXV_BALANCE_D_S_ADDRESS = None
        newState.TXV_BALANCE_D_S_BALANCE = None
        newState.TXV_BALANCE_D_B_ADDRESS = None
        newState.TXV_BALANCE_D_B_BALANCE = None
        # D fee
        newState.TXV_BALANCE_D_FEE_BALANCE = None

        newState.TXV_ACCOUNT_D_FEE_ADDRESS = None

        newState.TXV_STORAGE_D_ADDRESS_ARRAY = []
        newState.TXV_STORAGE_D_TOKENSID_ARRAY = []
        newState.TXV_STORAGE_D_TOKENBID_ARRAY = []
        newState.TXV_STORAGE_D_DATA_ARRAY = []
        newState.TXV_STORAGE_D_STORAGEID_ARRAY = []
        newState.TXV_STORAGE_D_GASFEE_ARRAY = []
        newState.TXV_STORAGE_D_CANCELLED_ARRAY = []
        newState.TXV_STORAGE_D_FORWARD_ARRAY = []

        # =============User E
        newState.TXV_ACCOUNT_E_ADDRESS = None
        newState.TXV_ACCOUNT_E_OWNER = None
        newState.TXV_ACCOUNT_E_PUBKEY_X = None
        newState.TXV_ACCOUNT_E_PUBKEY_Y = None
        newState.TXV_ACCOUNT_E_NONCE = None
        newState.TXV_BALANCE_E_S_ADDRESS = None
        newState.TXV_BALANCE_E_S_BALANCE = None
        newState.TXV_BALANCE_E_B_ADDRESS = None
        newState.TXV_BALANCE_E_B_BALANCE = None
        # E fee
        newState.TXV_BALANCE_E_FEE_BALANCE = None

        newState.TXV_ACCOUNT_E_FEE_ADDRESS = None

        newState.TXV_STORAGE_E_ADDRESS_ARRAY = []
        newState.TXV_STORAGE_E_TOKENSID_ARRAY = []
        newState.TXV_STORAGE_E_TOKENBID_ARRAY = []
        newState.TXV_STORAGE_E_DATA_ARRAY = []
        newState.TXV_STORAGE_E_STORAGEID_ARRAY = []
        newState.TXV_STORAGE_E_GASFEE_ARRAY = []
        newState.TXV_STORAGE_E_CANCELLED_ARRAY = []
        newState.TXV_STORAGE_E_FORWARD_ARRAY = []

        # =============User F
        newState.TXV_ACCOUNT_F_ADDRESS = None
        newState.TXV_ACCOUNT_F_OWNER = None
        newState.TXV_ACCOUNT_F_PUBKEY_X = None
        newState.TXV_ACCOUNT_F_PUBKEY_Y = None
        newState.TXV_ACCOUNT_F_NONCE = None
        newState.TXV_BALANCE_F_S_ADDRESS = None
        newState.TXV_BALANCE_F_S_BALANCE = None
        newState.TXV_BALANCE_F_B_ADDRESS = None
        newState.TXV_BALANCE_F_B_BALANCE = None
        # F fee
        newState.TXV_BALANCE_F_FEE_BALANCE = None

        newState.TXV_ACCOUNT_F_FEE_ADDRESS = None

        newState.TXV_STORAGE_F_ADDRESS_ARRAY = []
        newState.TXV_STORAGE_F_TOKENSID_ARRAY = []
        newState.TXV_STORAGE_F_TOKENBID_ARRAY = []
        newState.TXV_STORAGE_F_DATA_ARRAY = []
        newState.TXV_STORAGE_F_STORAGEID_ARRAY = []
        newState.TXV_STORAGE_F_GASFEE_ARRAY = []
        newState.TXV_STORAGE_F_CANCELLED_ARRAY = []
        newState.TXV_STORAGE_F_FORWARD_ARRAY = []

        # Operator
        newState.balanceDeltaA_O = None
        newState.balanceDeltaB_O = None
        newState.balanceDeltaC_O = None
        newState.balanceDeltaD_O = None
        # split trading fee and gas fee
        newState.balanceDeltaA_O_Address = None
        newState.balanceDeltaB_O_Address = None
        newState.balanceDeltaC_O_Address = None
        newState.balanceDeltaD_O_Address = None

        typeAutoMarketSell = 6
        typeAutoMarketBuy = 7

        if txInput.txType == "Noop":

            # Nothing to do
            pass

        elif txInput.txType == "SpotTrade":

            ring = txInput

            # Amount filled in the trade history
            # filled_A = self.getData(ring.orderA.accountID, ring.orderA.tokenS, ring.orderA.storageID)
            # filled_B = self.getData(ring.orderB.accountID, ring.orderB.tokenS, ring.orderB.storageID)
            tokenSID_A, tokenBID_A, filled_A, gasFee_A, cancelled_A, forward_A = self.calculate.getData(ring.orderA.accountID, ring.orderA.storageID)
            tokenSID_B, tokenBID_B, filled_B, gasFee_B, cancelled_B, forward_B = self.calculate.getData(ring.orderB.accountID, ring.orderB.storageID)
            
            if ring.orderA.type == 6 or ring.orderA.type == 7:
                newState.TXV_STORAGE_A_FORWARD = self.calculate.calculateAutoMarketForward(forward_A, ring.orderA)
                if newState.TXV_STORAGE_A_FORWARD != forward_A :
                    filled_A = 0
            if ring.orderB.type == 6 or ring.orderB.type == 7:
                newState.TXV_STORAGE_B_FORWARD = self.calculate.calculateAutoMarketForward(forward_B, ring.orderB)
                if newState.TXV_STORAGE_B_FORWARD != forward_B :
                    filled_B = 0
            # Simple matching logic
            fillA = self.calculate.getMaxFill(ring.orderA, filled_A, True, self)
            fillB = self.calculate.getMaxFill(ring.orderB, filled_B, True, self)
            '''
            print("fillA.S: " + str(fillA.S))
            print("fillA.B: " + str(fillA.B))
            print("fillB.S: " + str(fillB.S))
            print("fillB.B: " + str(fillB.B))
            print("-------------")
            '''
            if ring.orderA.fillAmountBorS:
                (spread, matchable) = self.calculate.match(ring.orderA, fillA, ring.orderB, fillB)
                fillA.S = fillB.B
            else:
                (spread, matchable) = self.calculate.match(ring.orderB, fillB, ring.orderA, fillA)
                fillA.B = fillB.S

            # Check valid
            ring.orderA.checkValid(context, ring.orderA, fillA.S, fillA.B)
            ring.orderB.checkValid(context, ring.orderB, fillB.S, fillB.B)
            ring.valid = matchable and ring.orderA.valid and ring.orderB.valid
            #print("ring.orderA.valid " + str(ring.orderA.valid))
            #print("ring.orderB.valid " + str(ring.orderB.valid))

            # Saved in ring for tests
            # ring.fFillS_A = toFloat(fillA.S, Float24Encoding)
            # ring.fFillS_B = toFloat(fillB.S, Float24Encoding)
            ring.fFillS_A = toFloat(fillA.S, Float32Encoding)
            ring.fFillS_B = toFloat(fillB.S, Float32Encoding)

            # fillA.S = roundToFloatValue(fillA.S, Float24Encoding)
            # fillB.S = roundToFloatValue(fillB.S, Float24Encoding)
            fillA.S = roundToFloatValue(fillA.S, Float32Encoding)
            fillB.S = roundToFloatValue(fillB.S, Float32Encoding)
            fillA.B = fillB.S
            fillB.B = fillA.S
            ring.orderA.fee = str(roundToFloatValue(int(ring.orderA.fee), Float16Encoding))
            ring.orderB.fee = str(roundToFloatValue(int(ring.orderB.fee), Float16Encoding))

            '''
            print("fillA.S: " + str(fillA.S))
            print("fillA.B: " + str(fillA.B))
            print("fillB.S: " + str(fillB.S))
            print("fillB.B: " + str(fillB.B))
            print("spread: " + str(spread))
            '''
            '''
            protocolFee = (amountB * protocolFeeBips) // 100000
            fee = (amountB * feeBips) // 10000
            '''
            (fee_A) = self.calculate.calculateFees(
                fillA.B,
                ring.orderA.feeBips
            )
            # , referFee_B, uiReferFee_B
            (fee_B) = self.calculate.calculateFees(
                fillB.B,
                ring.orderB.feeBips
            )

            '''
            print("fee_A: " + str(fee_A))
            print("protocolFee_A: " + str(protocolFee_A))

            print("fee_B: " + str(fee_B))
            print("protocolFee_B: " + str(protocolFee_B))
            '''

            newState.signatureA = ring.orderA.signature
            newState.signatureB = ring.orderB.signature

            newState.TXV_ACCOUNT_A_ADDRESS = ring.orderA.accountID
            accountA = self.getAccount(ring.orderA.accountID)

            newState.TXV_BALANCE_A_S_ADDRESS = ring.orderA.tokenS
            newState.TXV_BALANCE_A_S_BALANCE = -fillA.S

            newState.TXV_BALANCE_A_B_ADDRESS = ring.orderA.tokenB
            newState.TXV_BALANCE_A_B_BALANCE = fillA.B - fee_A

            newState.TXV_STORAGE_A_ADDRESS = ring.orderA.storageID
            # DEG-347 Storage move
            newState.TXV_STORAGE_A_TOKENSID = ring.orderA.tokenS
            newState.TXV_STORAGE_A_TOKENBID = ring.orderA.tokenB
            newState.TXV_STORAGE_A_DATA = filled_A + (fillA.B if ring.orderA.fillAmountBorS else fillA.S)
            newState.TXV_STORAGE_A_STORAGEID = ring.orderA.storageID
            newState.TXV_STORAGE_A_GASFEE = gasFee_A + int(ring.orderA.fee)
            newState.TXV_STORAGE_A_CANCELLED = cancelled_A
            # DEG-347 Storage move
            newState.TXV_STORAGE_A_FORWARD = forward_A


            newState.TXV_ACCOUNT_B_ADDRESS = ring.orderB.accountID
            accountB = self.getAccount(ring.orderB.accountID)

            newState.TXV_BALANCE_B_S_ADDRESS = ring.orderB.tokenS
            newState.TXV_BALANCE_B_S_BALANCE = -fillB.S

            newState.TXV_BALANCE_B_B_ADDRESS = ring.orderB.tokenB
            newState.TXV_BALANCE_B_B_BALANCE = fillB.B - fee_B

            newState.TXV_STORAGE_B_ADDRESS = ring.orderB.storageID
            # DEG-347 Storage move
            newState.TXV_STORAGE_B_TOKENSID = ring.orderB.tokenS
            newState.TXV_STORAGE_B_TOKENBID = ring.orderB.tokenB
            newState.TXV_STORAGE_B_DATA = filled_B + (fillB.B if ring.orderB.fillAmountBorS else fillB.S)
            newState.TXV_STORAGE_B_STORAGEID = ring.orderB.storageID
            newState.TXV_STORAGE_B_GASFEE = gasFee_B + int(ring.orderB.fee)
            newState.TXV_STORAGE_B_CANCELLED = cancelled_B
            # DEG-347 Storage move
            newState.TXV_STORAGE_B_FORWARD = forward_B

            '''
            In the requirement of splitting taking fee and protocol fee, the operator receives the fee token
            '''
            newState.balanceDeltaA_O = ring.orderA.fee
            newState.balanceDeltaB_O = ring.orderB.fee
            newState.balanceDeltaA_O_Address = ring.orderA.feeTokenID
            newState.balanceDeltaB_O_Address = ring.orderB.feeTokenID
            # DEG-127 split trading fee and gas fee
            newState.TXV_ACCOUNT_A_FEE_ADDRESS = newState.balanceDeltaA_O_Address
            newState.TXV_ACCOUNT_B_FEE_ADDRESS = newState.balanceDeltaB_O_Address

            # newState.TXV_BALANCE_A_FEE_BALANCE = -roundToFloatValue(ring.orderA.fee, Float24Encoding)
            # newState.TXV_BALANCE_B_FEE_BALANCE = -roundToFloatValue(ring.orderB.fee, Float24Encoding)
            newState.TXV_BALANCE_A_FEE_BALANCE = -roundToFloatValue(ring.orderA.fee, Float32Encoding)
            newState.TXV_BALANCE_B_FEE_BALANCE = -roundToFloatValue(ring.orderB.fee, Float32Encoding)
            '''
            In the requirement of splitting tading fee and protocol fee, you can directly give all the fee to operator
            '''
            # '''
            newState.balanceDeltaC_O = fee_A
            newState.balanceDeltaD_O = fee_B
            # For the changes involved after adding batchspottrade, the field needs to clearly indicate the currency update address of the protocol account. 
            # The addresses of account a and account B cannot be used
            newState.balanceDeltaC_O_Address = newState.TXV_BALANCE_B_S_ADDRESS
            newState.balanceDeltaD_O_Address = newState.TXV_BALANCE_A_S_ADDRESS


            if ring.orderA.type == typeAutoMarketSell or ring.orderA.type == typeAutoMarketBuy:
                newState.TXV_STORAGE_A_FORWARD = self.calculate.calculateAutoMarketForward(forward_A, ring.orderA)
                # DEG-265 auto market
                # If it is a grid order, the tokensid and tokenbid stored in the storage must be consistent with the startorder, even if it is a reverse order
                newState.TXV_STORAGE_A_TOKENSID = ring.orderA.startOrder.tokenS
                newState.TXV_STORAGE_A_TOKENBID = ring.orderA.startOrder.tokenB
                # If it is a reverse order, the data needs to be reset
                if newState.TXV_STORAGE_A_FORWARD != forward_A :
                    newState.TXV_STORAGE_A_DATA = fillA.B if ring.orderA.fillAmountBorS else fillA.S
                    newState.TXV_STORAGE_A_GASFEE = ring.orderA.fee
            
            if ring.orderB.type == typeAutoMarketSell or ring.orderB.type == typeAutoMarketBuy:
                # DEG-347 Storage move
                newState.TXV_STORAGE_B_FORWARD = self.calculate.calculateAutoMarketForward(forward_B, ring.orderB)
                # DEG-265 auto market
                # If it is a grid order, the tokensid and tokenbid stored in the storage must be consistent with the startorder, even if it is a reverse order
                newState.TXV_STORAGE_B_TOKENSID = ring.orderB.startOrder.tokenS
                newState.TXV_STORAGE_B_TOKENBID = ring.orderB.startOrder.tokenB
                # If it is a reverse order, the data needs to be reset
                if newState.TXV_STORAGE_B_FORWARD != forward_B :
                    newState.TXV_STORAGE_B_DATA = fillB.B if ring.orderB.fillAmountBorS else fillB.S
                    newState.TXV_STORAGE_B_GASFEE = ring.orderB.fee
            
        # DEG-358 BatchSpotTrade - opetator
        elif (txInput.txType == "BatchSpotTrade"):
            # A lot of arrays will be generated in batch transactions. Based on the principle of not changing the original logic, 
            # the processing here will be strange. First, the original data settings of account a and account B will be retained, 
            # and three storage updates will be added on the basis of account a (if a user has 4 orders)
            batchSpotTrade = txInput
            # StorageDataMap, Two dimensional array, key1: accountid, key2: storageid
            storageDataMap = dict()
            storageGasFeeMap = dict()
            storageForwardMap = dict()
            # bindTokenId must be the third
            firstTokenID = batchSpotTrade.tokens[0]
            secondTokenID = batchSpotTrade.tokens[1]
            thirdTokenID = batchSpotTrade.tokens[2]

            # Token income, in the form of float, is the real change value of the balance on the tree
            firstTokenFloatForward = 0
            secondTokenFloatForward = 0
            thirdTokenFloatForward = 0

            firstTokenFloatReverse = 0
            secondTokenFloatReverse = 0
            thirdTokenFloatReverse = 0
            (tokenSIDArray, tokenBIDArray, filledArray, gasFeeArray, cancelledArray, forwardArray, 
                fill, tradingFee, tradingFeeUser, gasFeeOrder, gasFeeUser, newState.signaturesTemp, newState.signatures, 
                firstTokenTradingFeeSum, secondTokenTradingFeeSum, thirdTokenTradingFeeSum, 
                firstTokenGasFeeSum, secondTokenGasFeeSum, thirdTokenGasFeeSum) = self.calculate.loopAllBacthSpotTradeOrders(batchSpotTrade.users, batchSpotTrade.tokens, self)
            
            # When processing signature data, the first signature of account a and account B should be placed in the original pit
            newState.signatureA = newState.signaturesTemp[0][0]
            newState.signatureB = newState.signaturesTemp[1][0]

            # In order not to destroy the data update of other transactions, it is necessary to be compatible with the old data setting method
            orderA = batchSpotTrade.users[0].orders[0]
            orderB = batchSpotTrade.users[1].orders[0]
            filled_A = filledArray[0][0]
            filled_B = filledArray[1][0]
            fillA = fill[0][0]
            fillB = fill[1][0]
            
            # The balance of all users is updated. Follow this rule,
            # First user tokens = = firsttokenid, tokenb = = secondtokenid, feetokenid = = thirdtokenid
            # Second user tokens = = secondtokenid, tokenb = = firsttokenid, feetokenid = = thirdtokenid
            # The third user tokens = = firsttokenid, tokenb = = secondtokenid, feetokenid = = thirdtokenid
            # The fourth user tokens = = firsttokenid, tokenb = = secondtokenid, feetokenid = = thirdtokenid
            # First and second, why is tokens opposite to tokenb? In order to take care of the previous transaction, otherwise it will be greatly changed
            # ==========UserA Order
            newState.TXV_ACCOUNT_A_ADDRESS = orderA.accountID
            accountA = self.getAccount(orderA.accountID)
            batchSpotTrade.userAAccountID = int(newState.TXV_ACCOUNT_A_ADDRESS)

            # Update token balance at one time
            # Balance of a token = calculated value of revenue and expenditure - tradingfee - gasfee
            newState.TXV_BALANCE_A_S_ADDRESS = firstTokenID
            newState.TXV_BALANCE_A_S_BALANCE = roundToFloatValueWithNegative(self.calculate.getBatchOrderBalance(batchSpotTrade.users[0].orders, firstTokenID) - tradingFeeUser[0].first - gasFeeUser[0].first, Float29Encoding)

            batchSpotTrade.userAFirstTokenExchange = int(newState.TXV_BALANCE_A_S_BALANCE)
            firstTokenFloatReverse = firstTokenFloatReverse + int(-batchSpotTrade.userAFirstTokenExchange if batchSpotTrade.userAFirstTokenExchange < 0 else 0)
            firstTokenFloatForward = firstTokenFloatForward + int(batchSpotTrade.userAFirstTokenExchange if batchSpotTrade.userAFirstTokenExchange > 0 else 0)

            # print("firstTokenFloatForward:" + str(firstTokenFloatForward))
            # print("self.getBatchOrderBalance(batchSpotTrade.users[0].orders, firstTokenID):" + str(self.getBatchOrderBalance(batchSpotTrade.users[0].orders, firstTokenID)))
            # print("tradingFeeUser[0].first:" + str(tradingFeeUser[0].first))
            # print("gasFeeUser[0].first:" + str(gasFeeUser[0].first))
            # assert(1==2)

            newState.TXV_BALANCE_A_B_ADDRESS = secondTokenID
            newState.TXV_BALANCE_A_B_BALANCE = roundToFloatValueWithNegative(self.calculate.getBatchOrderBalance(batchSpotTrade.users[0].orders, secondTokenID) - tradingFeeUser[0].second - gasFeeUser[0].second, Float29Encoding)
            batchSpotTrade.userASecondTokenExchange = int(newState.TXV_BALANCE_A_B_BALANCE)
            secondTokenFloatReverse = secondTokenFloatReverse + int(-batchSpotTrade.userASecondTokenExchange if batchSpotTrade.userASecondTokenExchange < 0 else 0)
            secondTokenFloatForward = secondTokenFloatForward + int(batchSpotTrade.userASecondTokenExchange if batchSpotTrade.userASecondTokenExchange > 0 else 0)

            # Although it's called fee here, it's not fee, but the third token
            # If thirdtoken is the same as firsttoken or secondtoken, you don't need to subtract again, just 0
            newState.TXV_BALANCE_A_FEE_BALANCE = roundToFloatValueWithNegative(self.calculate.getThirdBalance(firstTokenID, secondTokenID, thirdTokenID, batchSpotTrade.users[0].orders, tradingFeeUser[0].third, gasFeeUser[0].third), Float29Encoding)
            newState.TXV_ACCOUNT_A_FEE_ADDRESS = thirdTokenID
            batchSpotTrade.userAThirdTokenExchange = int(newState.TXV_BALANCE_A_FEE_BALANCE)
            thirdTokenFloatReverse = thirdTokenFloatReverse + int(-batchSpotTrade.userAThirdTokenExchange if batchSpotTrade.userAThirdTokenExchange < 0 else 0)
            thirdTokenFloatForward = thirdTokenFloatForward + int(batchSpotTrade.userAThirdTokenExchange if batchSpotTrade.userAThirdTokenExchange > 0 else 0)

            # The orderone of user a is recorded here
            newState.TXV_STORAGE_A_ADDRESS = orderA.storageID
            newState.TXV_STORAGE_A_TOKENSID = orderA.tokenS
            newState.TXV_STORAGE_A_TOKENBID = orderA.tokenB
            newState.TXV_STORAGE_A_DATA = filled_A + (fillA.B if orderA.fillAmountBorS else fillA.S)
            self.calculate.addTwoDimDict(storageDataMap, orderA.accountID, orderA.storageID, int(newState.TXV_STORAGE_A_DATA))

            newState.TXV_STORAGE_A_STORAGEID = orderA.storageID
            newState.TXV_STORAGE_A_GASFEE = gasFeeArray[0][0] + gasFeeOrder[0][0]
            self.calculate.addTwoDimDict(storageGasFeeMap, orderA.accountID, orderA.storageID, int(newState.TXV_STORAGE_A_GASFEE))
            newState.TXV_STORAGE_A_CANCELLED = cancelledArray[0][0]
            newState.TXV_STORAGE_A_FORWARD = forwardArray[0][0]

            # UserA AutoMarket, Order0
            if orderA.type == typeAutoMarketSell or orderA.type == typeAutoMarketBuy:
                # storageA = accountA.getStorage(orderA.storageID)
                newState.TXV_STORAGE_A_FORWARD = self.calculate.calculateAutoMarketForward(forwardArray[0][0], orderA)
                self.calculate.addTwoDimDict(storageForwardMap, orderA.accountID, orderA.storageID, int(newState.TXV_STORAGE_A_FORWARD))
                # DEG-265 auto market
                # If it is a grid order, the tokensid and tokenbid stored in the storage must be consistent with the startorder, even if it is a reverse order
                newState.TXV_STORAGE_A_TOKENSID = orderA.startOrder.tokenS
                newState.TXV_STORAGE_A_TOKENBID = orderA.startOrder.tokenB
                # If it is a reverse order, the data needs to be reset
                if newState.TXV_STORAGE_A_FORWARD != forwardArray[0][0] :
                    newState.TXV_STORAGE_A_DATA = fillA.B if orderA.fillAmountBorS else fillA.S
                    self.calculate.addTwoDimDict(storageDataMap, orderA.accountID, orderA.storageID, int(newState.TXV_STORAGE_A_DATA))
                    newState.TXV_STORAGE_A_GASFEE = gasFeeOrder[0][0]
                    self.calculate.addTwoDimDict(storageGasFeeMap, orderA.accountID, orderA.storageID, int(newState.TXV_STORAGE_A_GASFEE))
            
            # ==========UserA Remaining storage updates
            orderIndex = 0
            for order in batchSpotTrade.users[0].orders:
                if orderIndex == 0 or orderA == order or order.isNoop == 1:
                    # The first skip has been handled
                    orderIndex = orderIndex + 1
                    continue
                newState.TXV_STORAGE_A_ADDRESS_ARRAY.append(order.storageID)
                newState.TXV_STORAGE_A_TOKENSID_ARRAY.append(order.tokenS)
                newState.TXV_STORAGE_A_TOKENBID_ARRAY.append(order.tokenB)
                newState.TXV_STORAGE_A_DATA_ARRAY.append(self.calculate.getBatchSpotTradeStorageData(storageDataMap, order.accountID, 
                    order.storageID, order.fillAmountBorS, filledArray[0][orderIndex], fill[0][orderIndex]))
                newState.TXV_STORAGE_A_STORAGEID_ARRAY.append(order.storageID)
                newState.TXV_STORAGE_A_GASFEE_ARRAY.append(self.calculate.getBatchSpotTradeStorageGasFee(storageGasFeeMap, order.accountID, 
                    order.storageID, gasFeeArray[0][orderIndex], gasFeeOrder[0][orderIndex]))
                newState.TXV_STORAGE_A_CANCELLED_ARRAY.append(cancelledArray[0][orderIndex])
                newState.TXV_STORAGE_A_FORWARD_ARRAY.append(forwardArray[0][orderIndex])

                #### Process grid orders
                if order.isNoop == 1 or (order.type != typeAutoMarketSell and order.type != typeAutoMarketBuy):
                    orderIndex = orderIndex + 1
                    continue
                # DEG-347 Storage move
                # storageA = accountA.getStorage(order.storageID)
                # OrderIndex-1 here means that the data that should have been the first OrderIndex is ignored because it has been specially processed above
                # If OrderIndex starts from 1 and the array starts from 0, the current array tag should be index - 1
                (newState.TXV_STORAGE_A_FORWARD_ARRAY[orderIndex - 1], forward) = self.calculate.getBatchSpotTradeStorageForward(storageForwardMap, order, forwardArray[0][orderIndex])
                # DEG-265 auto market
                # If it is a grid order, the TokenSID and TokenBID stored in Storage need to be consistent with startOrder, and the reverse order is immediately.
                newState.TXV_STORAGE_A_TOKENSID_ARRAY[orderIndex - 1] = order.startOrder.tokenS
                newState.TXV_STORAGE_A_TOKENBID_ARRAY[orderIndex - 1] = order.startOrder.tokenB
                # If it is a reverse order, the data needs to be reset
                if newState.TXV_STORAGE_A_FORWARD_ARRAY[orderIndex - 1] != forward :
                    newState.TXV_STORAGE_A_DATA_ARRAY[orderIndex - 1] = fill[0][orderIndex].B if order.fillAmountBorS else fill[0][orderIndex].S
                    self.calculate.addTwoDimDict(storageDataMap, order.accountID, order.storageID, int(newState.TXV_STORAGE_A_DATA_ARRAY[orderIndex - 1]))
                    newState.TXV_STORAGE_A_GASFEE_ARRAY[orderIndex - 1] = gasFeeOrder[0][orderIndex]
                    self.calculate.addTwoDimDict(storageGasFeeMap, order.accountID, order.storageID, int(newState.TXV_STORAGE_A_GASFEE_ARRAY[orderIndex - 1]))

                orderIndex = orderIndex + 1
            
            # ==========UserB Order
            newState.TXV_ACCOUNT_B_ADDRESS = orderB.accountID
            accountB = self.getAccount(orderB.accountID)
            batchSpotTrade.userBAccountID = newState.TXV_ACCOUNT_B_ADDRESS

            newState.TXV_BALANCE_B_S_ADDRESS = firstTokenID
            newState.TXV_BALANCE_B_S_BALANCE = roundToFloatValueWithNegative(self.calculate.getBatchOrderBalance(batchSpotTrade.users[1].orders, firstTokenID) - tradingFeeUser[1].first - gasFeeUser[1].first, Float29Encoding)
            batchSpotTrade.userBFirstTokenExchange = int(newState.TXV_BALANCE_B_S_BALANCE)
            firstTokenFloatReverse = firstTokenFloatReverse + int(-batchSpotTrade.userBFirstTokenExchange if batchSpotTrade.userBFirstTokenExchange < 0 else 0)
            firstTokenFloatForward = firstTokenFloatForward + int(batchSpotTrade.userBFirstTokenExchange if batchSpotTrade.userBFirstTokenExchange > 0 else 0)

            newState.TXV_BALANCE_B_B_ADDRESS = secondTokenID
            newState.TXV_BALANCE_B_B_BALANCE = roundToFloatValueWithNegative(self.calculate.getBatchOrderBalance(batchSpotTrade.users[1].orders, secondTokenID) - tradingFeeUser[1].second - gasFeeUser[1].second, Float29Encoding)
            batchSpotTrade.userBSecondTokenExchange = int(newState.TXV_BALANCE_B_B_BALANCE)
            secondTokenFloatReverse = secondTokenFloatReverse + int(-batchSpotTrade.userBSecondTokenExchange if batchSpotTrade.userBSecondTokenExchange < 0 else 0)
            secondTokenFloatForward = secondTokenFloatForward + int(batchSpotTrade.userBSecondTokenExchange if batchSpotTrade.userBSecondTokenExchange > 0 else 0)

            newState.TXV_BALANCE_B_FEE_BALANCE = roundToFloatValueWithNegative(self.calculate.getThirdBalance(firstTokenID, secondTokenID, thirdTokenID, batchSpotTrade.users[1].orders, tradingFeeUser[1].third, gasFeeUser[1].third), Float29Encoding)
            newState.TXV_ACCOUNT_B_FEE_ADDRESS = thirdTokenID
            batchSpotTrade.userBThirdTokenExchange = int(newState.TXV_BALANCE_B_FEE_BALANCE)
            thirdTokenFloatReverse = thirdTokenFloatReverse + int(-batchSpotTrade.userBThirdTokenExchange if batchSpotTrade.userBThirdTokenExchange < 0 else 0)
            thirdTokenFloatForward = thirdTokenFloatForward + int(batchSpotTrade.userBThirdTokenExchange if batchSpotTrade.userBThirdTokenExchange > 0 else 0)

            newState.TXV_STORAGE_B_ADDRESS = orderB.storageID
            newState.TXV_STORAGE_B_TOKENSID = orderB.tokenS
            newState.TXV_STORAGE_B_TOKENBID = orderB.tokenB
            newState.TXV_STORAGE_B_DATA = self.calculate.getBatchSpotTradeStorageData(storageDataMap, orderB.accountID, 
                orderB.storageID, orderB.fillAmountBorS, filled_B, fillB)

            newState.TXV_STORAGE_B_STORAGEID = orderB.storageID

            newState.TXV_STORAGE_B_GASFEE = self.calculate.getBatchSpotTradeStorageGasFee(storageGasFeeMap, orderB.accountID, 
                orderB.storageID, gasFeeArray[1][0], gasFeeOrder[1][0])
            
            newState.TXV_STORAGE_B_CANCELLED = cancelledArray[1][0]
            newState.TXV_STORAGE_B_FORWARD = forwardArray[1][0]
            # UserB AutoMarket order 0
            if orderB.type == typeAutoMarketSell or orderB.type == typeAutoMarketBuy:
                # DEG-347 Storage move
                # storageB = accountB.getStorage(orderB.storageID)
                (newState.TXV_STORAGE_B_FORWARD, forward) = self.calculate.getBatchSpotTradeStorageForward(storageForwardMap, orderB, forwardArray[1][0])
                # DEG-265 auto market
                # If it is a grid order, the tokensid and tokenbid stored in the storage must be consistent with the startorder, even if it is a reverse order
                newState.TXV_STORAGE_B_TOKENSID = orderB.startOrder.tokenS
                newState.TXV_STORAGE_B_TOKENBID = orderB.startOrder.tokenB
                # If it is a reverse order, the data needs to be reset
                if newState.TXV_STORAGE_B_FORWARD != forward :
                    newState.TXV_STORAGE_B_DATA = fillB.B if orderB.fillAmountBorS else fillB.S
                    self.calculate.addTwoDimDict(storageDataMap, orderB.accountID, orderB.storageID, int(newState.TXV_STORAGE_B_DATA))
                    newState.TXV_STORAGE_B_GASFEE = gasFeeOrder[1][0]
                    self.calculate.addTwoDimDict(storageGasFeeMap, orderB.accountID, orderB.storageID, int(newState.TXV_STORAGE_B_GASFEE))

            # ==========UserB Remaining storage updates
            orderIndex = 0
            for order in batchSpotTrade.users[1].orders:
                if orderIndex == 0 or orderB == order or order.isNoop == 1:
                    # The first skip has been handled
                    orderIndex = orderIndex + 1
                    continue
                newState.TXV_STORAGE_B_ADDRESS_ARRAY.append(order.storageID)
                newState.TXV_STORAGE_B_TOKENSID_ARRAY.append(order.tokenS)
                newState.TXV_STORAGE_B_TOKENBID_ARRAY.append(order.tokenB)
                newState.TXV_STORAGE_B_DATA_ARRAY.append(self.calculate.getBatchSpotTradeStorageData(storageDataMap, order.accountID, 
                    order.storageID, order.fillAmountBorS, filledArray[1][orderIndex], fill[1][orderIndex]))
                newState.TXV_STORAGE_B_STORAGEID_ARRAY.append(order.storageID)
                newState.TXV_STORAGE_B_GASFEE_ARRAY.append(self.calculate.getBatchSpotTradeStorageGasFee(storageGasFeeMap, order.accountID, 
                    order.storageID, gasFeeArray[1][orderIndex], gasFeeOrder[1][orderIndex]))
                newState.TXV_STORAGE_B_CANCELLED_ARRAY.append(cancelledArray[1][orderIndex])
                newState.TXV_STORAGE_B_FORWARD_ARRAY.append(forwardArray[1][orderIndex])

                # Process grid orders
                if order.isNoop == 1 or (order.type != typeAutoMarketSell and order.type != typeAutoMarketBuy):
                    orderIndex = orderIndex + 1
                    continue
                # DEG-347 Storage move
                # storageB = accountB.getStorage(order.storageID)

                (newState.TXV_STORAGE_B_FORWARD_ARRAY[orderIndex - 1], forward) = self.calculate.getBatchSpotTradeStorageForward(storageForwardMap, order, forwardArray[1][orderIndex])
                
                # DEG-265 auto market
                # If it is a grid order, the tokensid and tokenbid stored in the storage must be consistent with the startorder, even if it is a reverse order
                newState.TXV_STORAGE_B_TOKENSID_ARRAY[orderIndex - 1] = (order.startOrder.tokenS)
                newState.TXV_STORAGE_B_TOKENBID_ARRAY[orderIndex - 1] = (order.startOrder.tokenB)
                # If it is a reverse order, the data needs to be reset
                if newState.TXV_STORAGE_B_FORWARD_ARRAY[orderIndex - 1] != forward :
                    newState.TXV_STORAGE_B_DATA_ARRAY[orderIndex - 1] = (fill[1][orderIndex].B if order.fillAmountBorS else fill[1][orderIndex].S)
                    self.calculate.addTwoDimDict(storageDataMap, order.accountID, order.storageID, int(newState.TXV_STORAGE_B_DATA_ARRAY[orderIndex - 1]))
                    newState.TXV_STORAGE_B_GASFEE_ARRAY[orderIndex - 1] = gasFeeOrder[1][orderIndex]
                    self.calculate.addTwoDimDict(storageGasFeeMap, order.accountID, order.storageID, int(newState.TXV_STORAGE_B_GASFEE_ARRAY[orderIndex - 1]))

                orderIndex = orderIndex + 1
            
            # ==========UserC
            newState.TXV_BALANCE_C_S_ADDRESS = firstTokenID
            newState.TXV_BALANCE_C_B_ADDRESS = secondTokenID
            newState.TXV_ACCOUNT_C_FEE_ADDRESS = thirdTokenID
            batchSpotTrade.userCFirstTokenExchange = 0
            batchSpotTrade.userCSecondTokenExchange = 0
            batchSpotTrade.userCThirdTokenExchange = 0
            if len(batchSpotTrade.users) >= 3 and batchSpotTrade.users[2].isNoop == 0:
                orderC = batchSpotTrade.users[2].orders[0]
                newState.TXV_ACCOUNT_C_ADDRESS = orderC.accountID
                accountC = self.getAccount(orderC.accountID)
                batchSpotTrade.userCAccountID = newState.TXV_ACCOUNT_C_ADDRESS

                newState.TXV_BALANCE_C_S_BALANCE = roundToFloatValueWithNegative(self.calculate.getBatchOrderBalance(batchSpotTrade.users[2].orders, firstTokenID) - tradingFeeUser[2].first - gasFeeUser[2].first, Float29Encoding)
                batchSpotTrade.userCFirstTokenExchange = int(newState.TXV_BALANCE_C_S_BALANCE)
                firstTokenFloatReverse = firstTokenFloatReverse + int(-batchSpotTrade.userCFirstTokenExchange if batchSpotTrade.userCFirstTokenExchange < 0 else 0)
                firstTokenFloatForward = firstTokenFloatForward + int(batchSpotTrade.userCFirstTokenExchange if batchSpotTrade.userCFirstTokenExchange > 0 else 0)

                newState.TXV_BALANCE_C_B_BALANCE = roundToFloatValueWithNegative(self.calculate.getBatchOrderBalance(batchSpotTrade.users[2].orders, secondTokenID) - tradingFeeUser[2].second - gasFeeUser[2].second, Float29Encoding)
                batchSpotTrade.userCSecondTokenExchange = int(newState.TXV_BALANCE_C_B_BALANCE)
                secondTokenFloatReverse = secondTokenFloatReverse + int(-batchSpotTrade.userCSecondTokenExchange if batchSpotTrade.userCSecondTokenExchange < 0 else 0)
                secondTokenFloatForward = secondTokenFloatForward + int(batchSpotTrade.userCSecondTokenExchange if batchSpotTrade.userCSecondTokenExchange > 0 else 0)

                newState.TXV_BALANCE_C_FEE_BALANCE = roundToFloatValueWithNegative(self.calculate.getThirdBalance(firstTokenID, secondTokenID, thirdTokenID, batchSpotTrade.users[2].orders, tradingFeeUser[2].third, gasFeeUser[2].third), Float29Encoding)
                batchSpotTrade.userCThirdTokenExchange = int(newState.TXV_BALANCE_C_FEE_BALANCE)
                thirdTokenFloatReverse = thirdTokenFloatReverse + int(-batchSpotTrade.userCThirdTokenExchange if batchSpotTrade.userCThirdTokenExchange < 0 else 0)
                thirdTokenFloatForward = thirdTokenFloatForward + int(batchSpotTrade.userCThirdTokenExchange if batchSpotTrade.userCThirdTokenExchange > 0 else 0)

                # Storage update
                orderIndex = 0
                for order in batchSpotTrade.users[2].orders:
                    if order.isNoop == 1:
                        orderIndex = orderIndex + 1
                        continue
                    newState.TXV_STORAGE_C_ADDRESS_ARRAY.append(order.storageID)
                    newState.TXV_STORAGE_C_TOKENSID_ARRAY.append(order.tokenS)
                    newState.TXV_STORAGE_C_TOKENBID_ARRAY.append(order.tokenB)
                    newState.TXV_STORAGE_C_DATA_ARRAY.append(self.calculate.getBatchSpotTradeStorageData(storageDataMap, order.accountID, 
                        order.storageID, order.fillAmountBorS, filledArray[2][orderIndex], fill[2][orderIndex]))
                    newState.TXV_STORAGE_C_STORAGEID_ARRAY.append(order.storageID)
                    newState.TXV_STORAGE_C_GASFEE_ARRAY.append(self.calculate.getBatchSpotTradeStorageGasFee(storageGasFeeMap, order.accountID, 
                    order.storageID, gasFeeArray[2][orderIndex], gasFeeOrder[2][orderIndex]))
                    newState.TXV_STORAGE_C_CANCELLED_ARRAY.append(cancelledArray[2][orderIndex])
                    newState.TXV_STORAGE_C_FORWARD_ARRAY.append(forwardArray[2][orderIndex])

                    # Process grid orders
                    if order.isNoop == 1 or (order.type != typeAutoMarketSell and order.type != typeAutoMarketBuy):
                        orderIndex = orderIndex + 1
                        continue
                    # storageC = accountC.getStorage(order.storageID)

                    (newState.TXV_STORAGE_C_FORWARD_ARRAY[orderIndex], forward) = self.calculate.getBatchSpotTradeStorageForward(storageForwardMap, order, forwardArray[2][orderIndex])
                    # DEG-265 auto market
                    # If it is a grid order, the tokensid and tokenbid stored in the storage must be consistent with the startorder, even if it is a reverse order
                    newState.TXV_STORAGE_C_TOKENSID_ARRAY[orderIndex] = (order.startOrder.tokenS)
                    newState.TXV_STORAGE_C_TOKENBID_ARRAY[orderIndex] = (order.startOrder.tokenB)
                    # If it is a reverse order, the data needs to be reset
                    if newState.TXV_STORAGE_C_FORWARD_ARRAY[orderIndex] != forward :
                        newState.TXV_STORAGE_C_DATA_ARRAY[orderIndex] = (fill[2][orderIndex].B if order.fillAmountBorS else fill[2][orderIndex].S)
                        self.calculate.addTwoDimDict(storageDataMap, order.accountID, order.storageID, int(newState.TXV_STORAGE_C_DATA_ARRAY[orderIndex]))
                        newState.TXV_STORAGE_C_GASFEE_ARRAY[orderIndex] = gasFeeOrder[2][orderIndex]
                        self.calculate.addTwoDimDict(storageGasFeeMap, order.accountID, order.storageID, int(newState.TXV_STORAGE_C_GASFEE_ARRAY[orderIndex]))

                    orderIndex = orderIndex + 1
                
            # ==========UserD
            newState.TXV_BALANCE_D_S_ADDRESS = firstTokenID
            newState.TXV_BALANCE_D_B_ADDRESS = secondTokenID
            newState.TXV_ACCOUNT_D_FEE_ADDRESS = thirdTokenID
            batchSpotTrade.userDFirstTokenExchange = 0
            batchSpotTrade.userDSecondTokenExchange = 0
            batchSpotTrade.userDThirdTokenExchange = 0
            if len(batchSpotTrade.users) >= 4 and batchSpotTrade.users[3].isNoop == 0:
                orderD = batchSpotTrade.users[3].orders[0]
                newState.TXV_ACCOUNT_D_ADDRESS = orderD.accountID
                accountD = self.getAccount(orderD.accountID)
                batchSpotTrade.userDAccountID = newState.TXV_ACCOUNT_D_ADDRESS

                newState.TXV_BALANCE_D_S_BALANCE = roundToFloatValueWithNegative(self.calculate.getBatchOrderBalance(batchSpotTrade.users[3].orders, firstTokenID) - tradingFeeUser[3].first - gasFeeUser[3].first, Float29Encoding)
                batchSpotTrade.userDFirstTokenExchange = int(newState.TXV_BALANCE_D_S_BALANCE)
                firstTokenFloatReverse = firstTokenFloatReverse + int(-batchSpotTrade.userDFirstTokenExchange if batchSpotTrade.userDFirstTokenExchange < 0 else 0)
                firstTokenFloatForward = firstTokenFloatForward + int(batchSpotTrade.userDFirstTokenExchange if batchSpotTrade.userDFirstTokenExchange > 0 else 0)

                newState.TXV_BALANCE_D_B_BALANCE = roundToFloatValueWithNegative(self.calculate.getBatchOrderBalance(batchSpotTrade.users[3].orders, secondTokenID) - tradingFeeUser[3].second - gasFeeUser[3].second, Float29Encoding)
                batchSpotTrade.userDSecondTokenExchange = int(newState.TXV_BALANCE_D_B_BALANCE)
                secondTokenFloatReverse = secondTokenFloatReverse + int(-batchSpotTrade.userDSecondTokenExchange if batchSpotTrade.userDSecondTokenExchange < 0 else 0)
                secondTokenFloatForward = secondTokenFloatForward + int(batchSpotTrade.userDSecondTokenExchange if batchSpotTrade.userDSecondTokenExchange > 0 else 0)

                newState.TXV_BALANCE_D_FEE_BALANCE = roundToFloatValueWithNegative(self.calculate.getThirdBalance(firstTokenID, secondTokenID, thirdTokenID, batchSpotTrade.users[3].orders, tradingFeeUser[3].third, gasFeeUser[3].third), Float29Encoding)
                batchSpotTrade.userDThirdTokenExchange = int(newState.TXV_BALANCE_D_FEE_BALANCE)
                thirdTokenFloatReverse = thirdTokenFloatReverse + int(-batchSpotTrade.userDThirdTokenExchange if batchSpotTrade.userDThirdTokenExchange < 0 else 0)
                thirdTokenFloatForward = thirdTokenFloatForward + int(batchSpotTrade.userDThirdTokenExchange if batchSpotTrade.userDThirdTokenExchange > 0 else 0)

                # Storage update
                orderIndex = 0
                for order in batchSpotTrade.users[3].orders:
                    if order.isNoop == 1:
                        orderIndex = orderIndex + 1
                        continue
                    newState.TXV_STORAGE_D_ADDRESS_ARRAY.append(order.storageID)
                    newState.TXV_STORAGE_D_TOKENSID_ARRAY.append(order.tokenS)
                    newState.TXV_STORAGE_D_TOKENBID_ARRAY.append(order.tokenB)
                    newState.TXV_STORAGE_D_DATA_ARRAY.append(self.calculate.getBatchSpotTradeStorageData(storageDataMap, order.accountID, 
                        order.storageID, order.fillAmountBorS, filledArray[3][orderIndex], fill[3][orderIndex]))
                    newState.TXV_STORAGE_D_STORAGEID_ARRAY.append(order.storageID)
                    newState.TXV_STORAGE_D_GASFEE_ARRAY.append(self.calculate.getBatchSpotTradeStorageGasFee(storageGasFeeMap, order.accountID, 
                    order.storageID, gasFeeArray[3][orderIndex], gasFeeOrder[3][orderIndex]))
                    newState.TXV_STORAGE_D_CANCELLED_ARRAY.append(cancelledArray[3][orderIndex])
                    newState.TXV_STORAGE_D_FORWARD_ARRAY.append(forwardArray[3][orderIndex])

                    # Process grid orders
                    if order.isNoop == 1 or (order.type != typeAutoMarketSell and order.type != typeAutoMarketBuy):
                        orderIndex = orderIndex + 1
                        continue
                    # DEG-347 Storage move
                    # storageD = accountD.getStorage(order.storageID)

                    (newState.TXV_STORAGE_D_FORWARD_ARRAY[orderIndex], forward) = self.calculate.getBatchSpotTradeStorageForward(storageForwardMap, order, forwardArray[3][orderIndex])
                    
                    # DEG-265 auto market
                    # If it is a grid order, the tokensid and tokenbid stored in the storage must be consistent with the startorder, even if it is a reverse order
                    newState.TXV_STORAGE_D_TOKENSID_ARRAY[orderIndex] = (order.startOrder.tokenS)
                    newState.TXV_STORAGE_D_TOKENBID_ARRAY[orderIndex] = (order.startOrder.tokenB)
                    # If it is a reverse order, the data needs to be reset
                    if newState.TXV_STORAGE_D_FORWARD_ARRAY[orderIndex] != forward :
                        newState.TXV_STORAGE_D_DATA_ARRAY[orderIndex] = (fill[3][orderIndex].B if order.fillAmountBorS else fill[3][orderIndex].S)
                        self.calculate.addTwoDimDict(storageDataMap, order.accountID, order.storageID, int(newState.TXV_STORAGE_D_DATA_ARRAY[orderIndex]))
                        newState.TXV_STORAGE_D_GASFEE_ARRAY[orderIndex] = gasFeeOrder[3][orderIndex]
                        self.calculate.addTwoDimDict(storageGasFeeMap, order.accountID, order.storageID, int(newState.TXV_STORAGE_D_GASFEE_ARRAY[orderIndex]))

                    orderIndex = orderIndex + 1
            
            # ==========UserE
            newState.TXV_BALANCE_E_S_ADDRESS = firstTokenID
            newState.TXV_BALANCE_E_B_ADDRESS = secondTokenID
            newState.TXV_ACCOUNT_E_FEE_ADDRESS = thirdTokenID
            batchSpotTrade.userEFirstTokenExchange = 0
            batchSpotTrade.userESecondTokenExchange = 0
            batchSpotTrade.userEThirdTokenExchange = 0
            if len(batchSpotTrade.users) >= 5 and batchSpotTrade.users[4].isNoop == 0:
                orderE = batchSpotTrade.users[4].orders[0]
                newState.TXV_ACCOUNT_E_ADDRESS = orderE.accountID
                accountE = self.getAccount(orderE.accountID)
                batchSpotTrade.userEAccountID = newState.TXV_ACCOUNT_E_ADDRESS

                newState.TXV_BALANCE_E_S_BALANCE = roundToFloatValueWithNegative(self.calculate.getBatchOrderBalance(batchSpotTrade.users[4].orders, firstTokenID) - tradingFeeUser[4].first - gasFeeUser[4].first, Float29Encoding)
                batchSpotTrade.userEFirstTokenExchange = int(newState.TXV_BALANCE_E_S_BALANCE)
                firstTokenFloatReverse = firstTokenFloatReverse + int(-batchSpotTrade.userEFirstTokenExchange if batchSpotTrade.userEFirstTokenExchange < 0 else 0)
                firstTokenFloatForward = firstTokenFloatForward + int(batchSpotTrade.userEFirstTokenExchange if batchSpotTrade.userEFirstTokenExchange > 0 else 0)

                newState.TXV_BALANCE_E_B_BALANCE = roundToFloatValueWithNegative(self.calculate.getBatchOrderBalance(batchSpotTrade.users[4].orders, secondTokenID) - tradingFeeUser[4].second - gasFeeUser[4].second, Float29Encoding)
                batchSpotTrade.userESecondTokenExchange = int(newState.TXV_BALANCE_E_B_BALANCE)
                secondTokenFloatReverse = secondTokenFloatReverse + int(-batchSpotTrade.userESecondTokenExchange if batchSpotTrade.userESecondTokenExchange < 0 else 0)
                secondTokenFloatForward = secondTokenFloatForward + int(batchSpotTrade.userESecondTokenExchange if batchSpotTrade.userESecondTokenExchange > 0 else 0)

                newState.TXV_BALANCE_E_FEE_BALANCE = roundToFloatValueWithNegative(self.calculate.getThirdBalance(firstTokenID, secondTokenID, thirdTokenID, batchSpotTrade.users[4].orders, tradingFeeUser[4].third, gasFeeUser[4].third), Float29Encoding)
                batchSpotTrade.userEThirdTokenExchange = int(newState.TXV_BALANCE_E_FEE_BALANCE)
                thirdTokenFloatReverse = thirdTokenFloatReverse + int(-batchSpotTrade.userEThirdTokenExchange if batchSpotTrade.userEThirdTokenExchange < 0 else 0)
                thirdTokenFloatForward = thirdTokenFloatForward + int(batchSpotTrade.userEThirdTokenExchange if batchSpotTrade.userEThirdTokenExchange > 0 else 0)

                # Storage update
                orderIndex = 0
                for order in batchSpotTrade.users[4].orders:
                    if order.isNoop == 1:
                        orderIndex = orderIndex + 1
                        continue
                    newState.TXV_STORAGE_E_ADDRESS_ARRAY.append(order.storageID)
                    newState.TXV_STORAGE_E_TOKENSID_ARRAY.append(order.tokenS)
                    newState.TXV_STORAGE_E_TOKENBID_ARRAY.append(order.tokenB)
                    newState.TXV_STORAGE_E_DATA_ARRAY.append(self.calculate.getBatchSpotTradeStorageData(storageDataMap, order.accountID, 
                        order.storageID, order.fillAmountBorS, filledArray[4][orderIndex], fill[4][orderIndex]))
                    newState.TXV_STORAGE_E_STORAGEID_ARRAY.append(order.storageID)
                    newState.TXV_STORAGE_E_GASFEE_ARRAY.append(self.calculate.getBatchSpotTradeStorageGasFee(storageGasFeeMap, order.accountID, 
                    order.storageID, gasFeeArray[4][orderIndex], gasFeeOrder[4][orderIndex]))
                    newState.TXV_STORAGE_E_CANCELLED_ARRAY.append(cancelledArray[4][orderIndex])
                    newState.TXV_STORAGE_E_FORWARD_ARRAY.append(forwardArray[4][orderIndex])

                    # Process grid orders
                    if order.isNoop == 1 or (order.type != typeAutoMarketSell and order.type != typeAutoMarketBuy):
                        orderIndex = orderIndex + 1
                        continue
                    # DEG-347 Storage move
                    # storageE = accountE.getStorage(order.storageID)

                    (newState.TXV_STORAGE_E_FORWARD_ARRAY[orderIndex], forward) = self.calculate.getBatchSpotTradeStorageForward(storageForwardMap, order, forwardArray[4][orderIndex])
                    
                    # DEG-265 auto market
                    # If it is a grid order, the tokensid and tokenbid stored in the storage must be consistent with the startorder, even if it is a reverse order
                    newState.TXV_STORAGE_E_TOKENSID_ARRAY[orderIndex] = (order.startOrder.tokenS)
                    newState.TXV_STORAGE_E_TOKENBID_ARRAY[orderIndex] = (order.startOrder.tokenB)
                    # If it is a reverse order, the data needs to be reset
                    if newState.TXV_STORAGE_E_FORWARD_ARRAY[orderIndex] != forward :
                        newState.TXV_STORAGE_E_DATA_ARRAY[orderIndex] = (fill[4][orderIndex].B if order.fillAmountBorS else fill[4][orderIndex].S)
                        self.calculate.addTwoDimDict(storageDataMap, order.accountID, order.storageID, int(newState.TXV_STORAGE_E_DATA_ARRAY[orderIndex]))
                        newState.TXV_STORAGE_E_GASFEE_ARRAY[orderIndex] = gasFeeOrder[4][orderIndex]
                        self.calculate.addTwoDimDict(storageGasFeeMap, order.accountID, order.storageID, int(newState.TXV_STORAGE_E_GASFEE_ARRAY[orderIndex]))

                    orderIndex = orderIndex + 1
            
            # ==========UserF
            newState.TXV_BALANCE_F_S_ADDRESS = firstTokenID
            newState.TXV_BALANCE_F_B_ADDRESS = secondTokenID
            newState.TXV_ACCOUNT_F_FEE_ADDRESS = thirdTokenID
            batchSpotTrade.userFFirstTokenExchange = 0
            batchSpotTrade.userFSecondTokenExchange = 0
            batchSpotTrade.userFThirdTokenExchange = 0
            if len(batchSpotTrade.users) >= 6 and batchSpotTrade.users[5].isNoop == 0:
                orderF = batchSpotTrade.users[5].orders[0]
                newState.TXV_ACCOUNT_F_ADDRESS = orderF.accountID
                accountF = self.getAccount(orderF.accountID)
                batchSpotTrade.userFAccountID = newState.TXV_ACCOUNT_F_ADDRESS

                newState.TXV_BALANCE_F_S_BALANCE = roundToFloatValueWithNegative(self.calculate.getBatchOrderBalance(batchSpotTrade.users[5].orders, firstTokenID) - tradingFeeUser[5].first - gasFeeUser[5].first, Float29Encoding)
                batchSpotTrade.userFFirstTokenExchange = int(newState.TXV_BALANCE_F_S_BALANCE)
                firstTokenFloatReverse = firstTokenFloatReverse + int(-batchSpotTrade.userFFirstTokenExchange if batchSpotTrade.userFFirstTokenExchange < 0 else 0)
                firstTokenFloatForward = firstTokenFloatForward + int(batchSpotTrade.userFFirstTokenExchange if batchSpotTrade.userFFirstTokenExchange > 0 else 0)

                newState.TXV_BALANCE_F_B_BALANCE = roundToFloatValueWithNegative(self.calculate.getBatchOrderBalance(batchSpotTrade.users[5].orders, secondTokenID) - tradingFeeUser[5].second - gasFeeUser[5].second, Float29Encoding)
                batchSpotTrade.userFSecondTokenExchange = int(newState.TXV_BALANCE_F_B_BALANCE)
                secondTokenFloatReverse = secondTokenFloatReverse + int(-batchSpotTrade.userFSecondTokenExchange if batchSpotTrade.userFSecondTokenExchange < 0 else 0)
                secondTokenFloatForward = secondTokenFloatForward + int(batchSpotTrade.userFSecondTokenExchange if batchSpotTrade.userFSecondTokenExchange > 0 else 0)
                
                newState.TXV_BALANCE_F_FEE_BALANCE = roundToFloatValueWithNegative(self.calculate.getThirdBalance(firstTokenID, secondTokenID, thirdTokenID, batchSpotTrade.users[5].orders, tradingFeeUser[5].third, gasFeeUser[5].third), Float29Encoding)
                batchSpotTrade.userFThirdTokenExchange = int(newState.TXV_BALANCE_F_FEE_BALANCE)
                thirdTokenFloatReverse = thirdTokenFloatReverse + int(-batchSpotTrade.userFThirdTokenExchange if batchSpotTrade.userFThirdTokenExchange < 0 else 0)
                thirdTokenFloatForward = thirdTokenFloatForward + int(batchSpotTrade.userFThirdTokenExchange if batchSpotTrade.userFThirdTokenExchange > 0 else 0)

                # Storage update
                orderIndex = 0
                for order in batchSpotTrade.users[5].orders:
                    if order.isNoop == 1:
                        orderIndex = orderIndex + 1
                        continue
                    newState.TXV_STORAGE_F_ADDRESS_ARRAY.append(order.storageID)
                    newState.TXV_STORAGE_F_TOKENSID_ARRAY.append(order.tokenS)
                    newState.TXV_STORAGE_F_TOKENBID_ARRAY.append(order.tokenB)
                    newState.TXV_STORAGE_F_DATA_ARRAY.append(self.calculate.getBatchSpotTradeStorageData(storageDataMap, order.accountID, 
                        order.storageID, order.fillAmountBorS, filledArray[5][orderIndex], fill[5][orderIndex]))
                    newState.TXV_STORAGE_F_STORAGEID_ARRAY.append(order.storageID)
                    newState.TXV_STORAGE_F_GASFEE_ARRAY.append(self.calculate.getBatchSpotTradeStorageGasFee(storageGasFeeMap, order.accountID, 
                    order.storageID, gasFeeArray[5][orderIndex], gasFeeOrder[5][orderIndex]))
                    newState.TXV_STORAGE_F_CANCELLED_ARRAY.append(cancelledArray[5][orderIndex])
                    newState.TXV_STORAGE_F_FORWARD_ARRAY.append(forwardArray[5][orderIndex])

                    # Process grid orders
                    if order.isNoop == 1 or (order.type != typeAutoMarketSell and order.type != typeAutoMarketBuy):
                        orderIndex = orderIndex + 1
                        continue
                    # storageF = accountF.getStorage(order.storageID)

                    (newState.TXV_STORAGE_F_FORWARD_ARRAY[orderIndex], forward) = self.calculate.getBatchSpotTradeStorageForward(storageForwardMap, order, forwardArray[5][orderIndex])
                    
                    # DEG-265 auto market
                    # If it is a grid order, the tokensid and tokenbid stored in the storage must be consistent with the startorder, even if it is a reverse order
                    newState.TXV_STORAGE_F_TOKENSID_ARRAY[orderIndex] = (order.startOrder.tokenS)
                    newState.TXV_STORAGE_F_TOKENBID_ARRAY[orderIndex] = (order.startOrder.tokenB)
                    # If it is a reverse order, the data needs to be reset
                    if newState.TXV_STORAGE_F_FORWARD_ARRAY[orderIndex] != forward :
                        newState.TXV_STORAGE_F_DATA_ARRAY[orderIndex] = (fill[5][orderIndex].B if order.fillAmountBorS else fill[5][orderIndex].S)
                        self.calculate.addTwoDimDict(storageDataMap, order.accountID, order.storageID, int(newState.TXV_STORAGE_F_DATA_ARRAY[orderIndex]))
                        newState.TXV_STORAGE_F_GASFEE_ARRAY[orderIndex] = gasFeeOrder[5][orderIndex]
                        self.calculate.addTwoDimDict(storageGasFeeMap, order.accountID, order.storageID, int(newState.TXV_STORAGE_F_GASFEE_ARRAY[orderIndex]))

                    orderIndex = orderIndex + 1
            # ==========Operator
            # Three tokens are required for the charge of the operator, because all three tokens may be feeToken
            # Operator fee collection total revenue - actual revenue
            # Token (deltaFilledB of all orders + gas of all orders) - token (revenue of all orders)
            print("batchSpotTrade.userAFirstTokenExchange:" + str(batchSpotTrade.userAFirstTokenExchange))
            print("batchSpotTrade.userASecondTokenExchange:" + str(batchSpotTrade.userASecondTokenExchange))
            print("batchSpotTrade.userAThirdTokenExchange:" + str(batchSpotTrade.userAThirdTokenExchange))

            print("batchSpotTrade.userBFirstTokenExchange:" + str(batchSpotTrade.userBFirstTokenExchange))
            print("batchSpotTrade.userBSecondTokenExchange:" + str(batchSpotTrade.userBSecondTokenExchange))
            print("batchSpotTrade.userBThirdTokenExchange:" + str(batchSpotTrade.userBThirdTokenExchange))

            newState.balanceDeltaC_O = int(firstTokenFloatReverse - firstTokenFloatForward)
            newState.balanceDeltaC_O_Address = firstTokenID

            newState.balanceDeltaB_O = int(secondTokenFloatReverse - secondTokenFloatForward)
            newState.balanceDeltaB_O_Address = secondTokenID

            newState.balanceDeltaA_O = int(thirdTokenFloatReverse - thirdTokenFloatForward)
            newState.balanceDeltaA_O_Address = thirdTokenID

            print("operator first:" + str(newState.balanceDeltaC_O))
            print("operator second:" + str(newState.balanceDeltaB_O))
            print("operator third:" + str(newState.balanceDeltaA_O))
            batchSpotTrade.operatorFirstTokenExchange = int(newState.balanceDeltaC_O)
            batchSpotTrade.operatorSecondTokenExchange = int(newState.balanceDeltaB_O)
            batchSpotTrade.operatorThirdTokenExchange = int(newState.balanceDeltaA_O)
            self.batchSpotTradeDataCheck(batchSpotTrade)
            # # ==========Protocol
            # newState.balanceDeltaA_P = firstTokenTradingFeeSum
            # newState.balanceDeltaB_P = secondTokenTradingFeeSum
            # newState.balanceDeltaC_P = thirdTokenTradingFeeSum
            # newState.balanceDeltaA_P_Address = firstTokenID
            # newState.balanceDeltaB_P_Address = secondTokenID
            # newState.balanceDeltaC_P_Address = thirdTokenID
        elif txInput.txType == "Transfer":

            # storageData = self.getData(txInput.fromAccountID, txInput.tokenID, txInput.storageID)
            tokenSID, tokenBID, storageData, gasFee, cancelled, forward = self.calculate.getData(txInput.fromAccountID, txInput.storageID)

            # transferAmount = roundToFloatValue(int(txInput.amount), Float24Encoding)
            transferAmount = roundToFloatValue(int(txInput.amount), Float32Encoding)
            feeValue = roundToFloatValue(int(txInput.fee), Float16Encoding)

            newState.signatureA = txInput.signature
            newState.signatureB = txInput.dualSignature

            newState.TXV_ACCOUNT_A_ADDRESS = txInput.fromAccountID
            accountA = self.getAccount(newState.TXV_ACCOUNT_A_ADDRESS)

            newState.TXV_BALANCE_A_S_ADDRESS = txInput.tokenID
            newState.TXV_BALANCE_A_S_BALANCE = -transferAmount

            # newState.TXV_BALANCE_B_S_ADDRESS = txInput.feeTokenID
            newState.TXV_BALANCE_A_B_ADDRESS = txInput.feeTokenID
            newState.TXV_BALANCE_A_B_BALANCE = -feeValue

            newState.TXV_ACCOUNT_B_ADDRESS = txInput.toAccountID
            accountB = self.getAccount(newState.TXV_ACCOUNT_B_ADDRESS)
            newState.TXV_ACCOUNT_B_OWNER = txInput.to

            # newState.TXV_BALANCE_A_S_ADDRESS = txInput.tokenID
            newState.TXV_BALANCE_B_B_ADDRESS = txInput.tokenID
            newState.TXV_BALANCE_B_B_BALANCE = transferAmount

            newState.TXV_STORAGE_A_ADDRESS = txInput.storageID
            # DEG-347 Storage move
            newState.TXV_STORAGE_A_TOKENSID = txInput.tokenID
            newState.TXV_STORAGE_A_TOKENBID = tokenBID
            newState.TXV_STORAGE_A_DATA = 1
            newState.TXV_STORAGE_A_STORAGEID = txInput.storageID
            newState.TXV_STORAGE_A_GASFEE = gasFee
            newState.TXV_STORAGE_A_CANCELLED = cancelled
            newState.TXV_STORAGE_A_FORWARD = forward

            if txInput.type != 0:
                context.numConditionalTransactions = context.numConditionalTransactions + 1

            newState.balanceDeltaA_O = feeValue
            # newState.TXV_STORAGE_A_GASFEE = feeValue
            # split trading fee and gas fee
            newState.balanceDeltaA_O_Address = txInput.feeTokenID

            # For tests (used to set the DA data)
            txInput.toNewAccount = True if accountB.owner == str(0) else False

        elif txInput.txType == "Withdraw":

            ## calculate how much can be withdrawn
            account = self.getAccount(txInput.accountID)
            if int(txInput.type) == 2:
                # Full balance with intrest
                balanceLeaf = account.getBalanceLeaf(txInput.tokenID)
                txInput.amount = str(balanceLeaf.balance)
            elif int(txInput.type) == 3:
                txInput.amount = str(0)

            tokenSID, tokenBID, storageData, gasFee, cancelled, forward = self.calculate.getData(txInput.accountID, txInput.storageID)

            # Protocol fee withdrawals are handled a bit differently
            # as the balance needs to be withdrawn from the already opened protocol pool account
            # no protocol account
            # isProtocolfeeWithdrawal = int(txInput.accountID) == 0
            isProtocolfeeWithdrawal = False

            feeValue = roundToFloatValue(int(txInput.fee), Float16Encoding)

            newState.signatureA = txInput.signature

            newState.TXV_ACCOUNT_A_ADDRESS = 1 if isProtocolfeeWithdrawal else txInput.accountID
            accountA = self.getAccount(newState.TXV_ACCOUNT_A_ADDRESS)

            newState.TXV_BALANCE_A_S_ADDRESS = txInput.tokenID
            newState.TXV_BALANCE_A_S_BALANCE = 0 if isProtocolfeeWithdrawal else -int(txInput.amount)

            # newState.TXV_BALANCE_B_S_ADDRESS = txInput.feeTokenID
            newState.TXV_BALANCE_A_B_ADDRESS = txInput.feeTokenID
            newState.TXV_BALANCE_A_B_BALANCE = -feeValue

            newState.TXV_STORAGE_A_ADDRESS = txInput.storageID
            # Forced withdrawal does not need to modify storage
            if int(txInput.type) == 0 or int(txInput.type) == 1:
                # DEG-347 Storage move
                newState.TXV_STORAGE_A_TOKENSID = txInput.tokenID
                newState.TXV_STORAGE_A_TOKENBID = tokenBID
                newState.TXV_STORAGE_A_DATA = 1
                newState.TXV_STORAGE_A_STORAGEID = txInput.storageID
                newState.TXV_STORAGE_A_GASFEE = gasFee
                newState.TXV_STORAGE_A_CANCELLED = cancelled
                newState.TXV_STORAGE_A_FORWARD = forward

            newState.balanceDeltaA_O = feeValue
            # split trading fee and gas fee
            newState.balanceDeltaA_O_Address = txInput.feeTokenID

            # no protocol account
            # newState.balanceDeltaD_O = -int(txInput.amount) if isProtocolfeeWithdrawal else 0
            newState.balanceDeltaD_O = 0
            newState.balanceDeltaD_O_Address = newState.TXV_BALANCE_A_S_ADDRESS

            context.numConditionalTransactions = context.numConditionalTransactions + 1

        elif txInput.txType == "Deposit":

            newState.TXV_ACCOUNT_A_ADDRESS = txInput.accountID
            newState.TXV_ACCOUNT_A_OWNER = txInput.owner

            newState.TXV_BALANCE_A_S_ADDRESS = txInput.tokenID
            newState.TXV_BALANCE_A_S_BALANCE = txInput.amount

            context.numConditionalTransactions = context.numConditionalTransactions + 1

        elif txInput.txType == "AccountUpdate":

            feeValue = roundToFloatValue(int(txInput.fee), Float16Encoding)

            newState.TXV_ACCOUNT_A_ADDRESS = txInput.accountID
            accountA = self.getAccount(newState.TXV_ACCOUNT_A_ADDRESS)

            newState.TXV_ACCOUNT_A_OWNER = txInput.owner
            newState.TXV_ACCOUNT_A_PUBKEY_X = txInput.publicKeyX
            newState.TXV_ACCOUNT_A_PUBKEY_Y = txInput.publicKeyY
            newState.TXV_ACCOUNT_A_NONCE = 1

            newState.TXV_BALANCE_A_S_ADDRESS = txInput.feeTokenID
            newState.TXV_BALANCE_A_S_BALANCE = -feeValue

            newState.balanceDeltaB_O = feeValue
            # split trading fee and gas fee
            # DEG-127 split trading fee and gas fee
            newState.balanceDeltaB_O_Address = txInput.feeTokenID

            newState.signatureA = txInput.signature

            if txInput.type != 0:
                context.numConditionalTransactions = context.numConditionalTransactions + 1
        # DEG-146:order cancel
        elif txInput.txType == "OrderCancel":
            feeValue = roundToFloatValue(int(txInput.fee), Float16Encoding)

            accountA = self.getAccount(txInput.accountID)

            tokenSID, tokenBID, storageData, gasFee, cancelled, forward = self.calculate.getData(txInput.accountID, txInput.storageID)

            newState.TXV_ACCOUNT_A_ADDRESS = txInput.accountID

            newState.TXV_STORAGE_A_ADDRESS = txInput.storageID
            newState.TXV_STORAGE_A_STORAGEID = txInput.storageID

            newState.TXV_BALANCE_A_S_ADDRESS = txInput.feeTokenID
            newState.TXV_BALANCE_A_S_BALANCE = -feeValue

            newState.balanceDeltaB_O = feeValue
            # DEG-127 split trading fee and gas fee
            newState.balanceDeltaB_O_Address = txInput.feeTokenID
            newState.TXV_STORAGE_B_GASSFEE = feeValue
            
            newState.TXV_STORAGE_A_TOKENSID = tokenSID
            newState.TXV_STORAGE_A_TOKENBID = tokenBID
            newState.TXV_STORAGE_A_DATA = storageData
            newState.TXV_STORAGE_A_STORAGEID = txInput.storageID
            newState.TXV_STORAGE_A_GASFEE = gasFee
            newState.TXV_STORAGE_A_CANCELLED = 1
            newState.TXV_STORAGE_A_FORWARD = forward

            newState.signatureA = txInput.signature
        
        elif txInput.txType == "AppKeyUpdate":

            feeValue = roundToFloatValue(int(txInput.fee), Float16Encoding)

            newState.TXV_ACCOUNT_A_ADDRESS = txInput.accountID
            accountA = self.getAccount(newState.TXV_ACCOUNT_A_ADDRESS)

            newState.TXV_ACCOUNT_A_APPKEY_PUBKEY_X = txInput.appKeyPublicKeyX
            newState.TXV_ACCOUNT_A_APPKEY_PUBKEY_Y = txInput.appKeyPublicKeyY
            newState.TXV_ACCOUNT_A_NONCE = 1

            newState.TXV_ACCOUNT_A_DISABLE_APPKEY_SPOTTRADE = txInput.disableAppKeySpotTrade
            newState.TXV_ACCOUNT_A_DISABLE_APPKEY_WITHDRAW = txInput.disableAppKeyWithdraw
            newState.TXV_ACCOUNT_A_DISABLE_APPKEY_TRANSFER_TO_OTHER = txInput.disableAppKeyTransferToOther

            newState.TXV_BALANCE_A_S_ADDRESS = txInput.feeTokenID
            newState.TXV_BALANCE_A_S_BALANCE = -feeValue

            newState.balanceDeltaB_O = feeValue
            newState.balanceDeltaB_O_Address = txInput.feeTokenID

            newState.signatureA = txInput.signature

        # Tokens default values
        newState.TXV_BALANCE_A_S_ADDRESS = setValue(newState.TXV_BALANCE_A_S_ADDRESS, 0)
        newState.TXV_BALANCE_A_B_ADDRESS = setValue(newState.TXV_BALANCE_A_B_ADDRESS, 0)
        newState.TXV_BALANCE_B_S_ADDRESS = setValue(newState.TXV_BALANCE_B_S_ADDRESS, 0)
        newState.TXV_BALANCE_B_B_ADDRESS = setValue(newState.TXV_BALANCE_B_B_ADDRESS, 0)

        newState.balanceDeltaA_O_Address = setValue(newState.balanceDeltaA_O_Address, 0)
        newState.balanceDeltaB_O_Address = setValue(newState.balanceDeltaB_O_Address, 0)
        newState.balanceDeltaC_O_Address = setValue(newState.balanceDeltaC_O_Address, 0)
        newState.balanceDeltaD_O_Address = setValue(newState.balanceDeltaD_O_Address, 0)
        # DEG-127 split trading fee and gas fee
        newState.TXV_ACCOUNT_A_FEE_ADDRESS = setValue(newState.TXV_ACCOUNT_A_FEE_ADDRESS, 0)
        newState.TXV_ACCOUNT_B_FEE_ADDRESS = setValue(newState.TXV_ACCOUNT_B_FEE_ADDRESS, 0)

        newState.TXV_BALANCE_A_FEE_BALANCE = setValue(newState.TXV_BALANCE_A_FEE_BALANCE, 0)
        newState.TXV_BALANCE_B_FEE_BALANCE = setValue(newState.TXV_BALANCE_B_FEE_BALANCE, 0)

        # A default values
        # newState.TXV_ACCOUNT_A_ADDRESS = setValue(newState.TXV_ACCOUNT_A_ADDRESS, 1)
        newState.TXV_ACCOUNT_A_ADDRESS = setValue(newState.TXV_ACCOUNT_A_ADDRESS, 0)
        accountA = self.getAccount(newState.TXV_ACCOUNT_A_ADDRESS)
        newState.TXV_ACCOUNT_A_OWNER = setValue(newState.TXV_ACCOUNT_A_OWNER, accountA.owner)
        newState.TXV_ACCOUNT_A_PUBKEY_X = setValue(newState.TXV_ACCOUNT_A_PUBKEY_X, accountA.publicKeyX)
        newState.TXV_ACCOUNT_A_PUBKEY_Y = setValue(newState.TXV_ACCOUNT_A_PUBKEY_Y, accountA.publicKeyY)
        newState.TXV_ACCOUNT_A_APPKEY_PUBKEY_X = setValue(newState.TXV_ACCOUNT_A_APPKEY_PUBKEY_X, accountA.appKeyPublicKeyX)
        newState.TXV_ACCOUNT_A_APPKEY_PUBKEY_Y = setValue(newState.TXV_ACCOUNT_A_APPKEY_PUBKEY_Y, accountA.appKeyPublicKeyY)
        newState.TXV_ACCOUNT_A_NONCE = setValue(newState.TXV_ACCOUNT_A_NONCE, 0)

        newState.TXV_ACCOUNT_A_DISABLE_APPKEY_SPOTTRADE = setValue(newState.TXV_ACCOUNT_A_DISABLE_APPKEY_SPOTTRADE, accountA.disableAppKeySpotTrade)
        newState.TXV_ACCOUNT_A_DISABLE_APPKEY_WITHDRAW = setValue(newState.TXV_ACCOUNT_A_DISABLE_APPKEY_WITHDRAW, accountA.disableAppKeyWithdraw)
        newState.TXV_ACCOUNT_A_DISABLE_APPKEY_TRANSFER_TO_OTHER = setValue(newState.TXV_ACCOUNT_A_DISABLE_APPKEY_TRANSFER_TO_OTHER, accountA.disableAppKeyTransferToOther)
        
        # newState.TXV_ACCOUNT_A_REFERID = setValue(newState.TXV_ACCOUNT_A_REFERID, accountA.referID)

        balanceLeafA_S = accountA.getBalanceLeaf(newState.TXV_BALANCE_A_S_ADDRESS)
        newState.TXV_BALANCE_A_S_BALANCE = setValue(newState.TXV_BALANCE_A_S_BALANCE, 0)

        newState.TXV_BALANCE_A_B_BALANCE = setValue(newState.TXV_BALANCE_A_B_BALANCE, 0)

        newState.TXV_STORAGE_A_ADDRESS = setValue(newState.TXV_STORAGE_A_ADDRESS, 0)
        # DEG-347 Storage location change
        storageA = accountA.getStorage(newState.TXV_STORAGE_A_ADDRESS)
        # DEG-347 Storage move
        newState.TXV_STORAGE_A_TOKENSID = setValue(newState.TXV_STORAGE_A_TOKENSID, storageA.tokenSID)
        newState.TXV_STORAGE_A_TOKENBID = setValue(newState.TXV_STORAGE_A_TOKENBID, storageA.tokenBID)
        newState.TXV_STORAGE_A_DATA = setValue(newState.TXV_STORAGE_A_DATA, storageA.data)
        newState.TXV_STORAGE_A_STORAGEID = setValue(newState.TXV_STORAGE_A_STORAGEID, storageA.storageID)
        newState.TXV_STORAGE_A_FORWARD = setValue(newState.TXV_STORAGE_A_FORWARD, storageA.forward)

        newState.TXV_STORAGE_A_GASFEE = setValue(newState.TXV_STORAGE_A_GASFEE, storageA.gasFee)
        

        # Operator default values
        newState.balanceDeltaA_O = setValue(newState.balanceDeltaA_O, 0)
        newState.balanceDeltaB_O = setValue(newState.balanceDeltaB_O, 0)
        newState.balanceDeltaC_O = setValue(newState.balanceDeltaC_O, 0)
        newState.balanceDeltaD_O = setValue(newState.balanceDeltaD_O, 0)

        # Copy the initial merkle root
        accountsMerkleRoot = self._accountsTree._root
        accountsMerkleAssetRoot = self._accountsAssetTree._root

        # Update A
        accountA = self.getAccount(newState.TXV_ACCOUNT_A_ADDRESS)

        rootBefore = self._accountsTree._root
        rootAssetBefore = self._accountsAssetTree._root
        accountBefore = copyAccountInfo(self.getAccount(newState.TXV_ACCOUNT_A_ADDRESS))
        proof = self._accountsTree.createProof(newState.TXV_ACCOUNT_A_ADDRESS)
        proofAsset = self._accountsAssetTree.createProof(newState.TXV_ACCOUNT_A_ADDRESS)

        print("=======================================")
        # DEG-347 Storage move
        (balanceUpdateS_A, storageUpdate_A) = accountA.updateBalanceAndStorage(
            newState.TXV_BALANCE_A_S_ADDRESS,
            newState.TXV_STORAGE_A_TOKENSID,
            newState.TXV_STORAGE_A_TOKENBID,
            newState.TXV_STORAGE_A_STORAGEID,
            newState.TXV_STORAGE_A_DATA,
            # split trading fee and gas fee - add up gas
            # newState.balanceDeltaA_O,
            newState.TXV_STORAGE_A_GASFEE,
            setValue(newState.TXV_STORAGE_A_CANCELLED, storageA.cancelled),
            newState.TXV_BALANCE_A_S_BALANCE,
            newState.TXV_STORAGE_A_FORWARD
        )

        (newState.TXV_STORAGE_A_ADDRESS_ARRAY, 
        newState.TXV_STORAGE_A_TOKENSID_ARRAY, 
        newState.TXV_STORAGE_A_TOKENBID_ARRAY, 
        newState.TXV_STORAGE_A_DATA_ARRAY, 
        newState.TXV_STORAGE_A_STORAGEID_ARRAY, 
        newState.TXV_STORAGE_A_GASFEE_ARRAY, 
        newState.TXV_STORAGE_A_CANCELLED_ARRAY, 
        newState.TXV_STORAGE_A_FORWARD_ARRAY) = self.calculate.storageDefaultSet(
            accountA, 
            newState.TXV_STORAGE_A_ADDRESS_ARRAY, 
            newState.TXV_STORAGE_A_TOKENSID_ARRAY, 
            newState.TXV_STORAGE_A_TOKENBID_ARRAY, 
            newState.TXV_STORAGE_A_DATA_ARRAY, 
            newState.TXV_STORAGE_A_STORAGEID_ARRAY, 
            newState.TXV_STORAGE_A_GASFEE_ARRAY, 
            newState.TXV_STORAGE_A_CANCELLED_ARRAY,
            newState.TXV_STORAGE_A_FORWARD_ARRAY, 
            MAX_BATCH_SPOT_TRADE_USER_ORDER[0] - 1)
        storageUpdate_A_array = accountA.updateBatchStorage(
            newState.TXV_STORAGE_A_STORAGEID_ARRAY, 
            newState.TXV_STORAGE_A_TOKENSID_ARRAY, 
            newState.TXV_STORAGE_A_TOKENBID_ARRAY, 
            newState.TXV_STORAGE_A_DATA_ARRAY, 
            newState.TXV_STORAGE_A_GASFEE_ARRAY, 
            newState.TXV_STORAGE_A_CANCELLED_ARRAY,
            newState.TXV_STORAGE_A_FORWARD_ARRAY,
            MAX_BATCH_SPOT_TRADE_USER_ORDER[0] - 1
        )

        balanceUpdateB_A = accountA.updateBalance(
            newState.TXV_BALANCE_A_B_ADDRESS,
            newState.TXV_BALANCE_A_B_BALANCE
        )
        # split trading fee and gas fee
        # account fee token
        balanceUpdateFee_A = accountA.updateBalance(
            # DEG-127 split trading fee and gas fee
            newState.TXV_ACCOUNT_A_FEE_ADDRESS,
            newState.TXV_BALANCE_A_FEE_BALANCE
        )

        accountA.owner = newState.TXV_ACCOUNT_A_OWNER
        accountA.publicKeyX = newState.TXV_ACCOUNT_A_PUBKEY_X
        accountA.publicKeyY = newState.TXV_ACCOUNT_A_PUBKEY_Y
        accountA.appKeyPublicKeyX = newState.TXV_ACCOUNT_A_APPKEY_PUBKEY_X
        accountA.appKeyPublicKeyY = newState.TXV_ACCOUNT_A_APPKEY_PUBKEY_Y
        accountA.nonce = accountA.nonce + newState.TXV_ACCOUNT_A_NONCE

        accountA.disableAppKeySpotTrade = newState.TXV_ACCOUNT_A_DISABLE_APPKEY_SPOTTRADE
        accountA.disableAppKeyWithdraw = newState.TXV_ACCOUNT_A_DISABLE_APPKEY_WITHDRAW
        accountA.disableAppKeyTransferToOther = newState.TXV_ACCOUNT_A_DISABLE_APPKEY_TRANSFER_TO_OTHER

        self.updateAccountTree(newState.TXV_ACCOUNT_A_ADDRESS)
        accountAfter = copyAccountInfo(self.getAccount(newState.TXV_ACCOUNT_A_ADDRESS))
        rootAfter = self._accountsTree._root
        rootAssetAfter = self._accountsAssetTree._root
        accountUpdate_A = AccountUpdateData(newState.TXV_ACCOUNT_A_ADDRESS, proof, proofAsset, rootBefore, rootAfter, rootAssetBefore, rootAssetAfter, accountBefore, accountAfter)
        ###

        # B default values
        newState.TXV_ACCOUNT_B_ADDRESS = setValue(newState.TXV_ACCOUNT_B_ADDRESS, 0)
        accountB = self.getAccount(newState.TXV_ACCOUNT_B_ADDRESS)
        newState.TXV_ACCOUNT_B_OWNER = setValue(newState.TXV_ACCOUNT_B_OWNER, accountB.owner)
        newState.TXV_ACCOUNT_B_PUBKEY_X = setValue(newState.TXV_ACCOUNT_B_PUBKEY_X, accountB.publicKeyX)
        newState.TXV_ACCOUNT_B_PUBKEY_Y = setValue(newState.TXV_ACCOUNT_B_PUBKEY_Y, accountB.publicKeyY)
        newState.TXV_ACCOUNT_B_NONCE = setValue(newState.TXV_ACCOUNT_B_NONCE, 0)

        balanceLeafB_S = accountB.getBalanceLeaf(newState.TXV_BALANCE_B_S_ADDRESS)
        newState.TXV_BALANCE_B_S_BALANCE = setValue(newState.TXV_BALANCE_B_S_BALANCE, 0)

        newState.TXV_BALANCE_B_B_BALANCE = setValue(newState.TXV_BALANCE_B_B_BALANCE, 0)

        newState.TXV_STORAGE_B_ADDRESS = setValue(newState.TXV_STORAGE_B_ADDRESS, 0)
        # DEG-347 Storage location change
        storageB = accountB.getStorage(newState.TXV_STORAGE_B_ADDRESS)
        # DEG-347 Storage move
        newState.TXV_STORAGE_B_TOKENSID = setValue(newState.TXV_STORAGE_B_TOKENSID, storageB.tokenSID)
        newState.TXV_STORAGE_B_TOKENBID = setValue(newState.TXV_STORAGE_B_TOKENBID, storageB.tokenBID)
        newState.TXV_STORAGE_B_DATA = setValue(newState.TXV_STORAGE_B_DATA, storageB.data)
        newState.TXV_STORAGE_B_STORAGEID = setValue(newState.TXV_STORAGE_B_STORAGEID, storageB.storageID)
        newState.TXV_STORAGE_B_FORWARD = setValue(newState.TXV_STORAGE_B_FORWARD, storageB.forward)

        newState.TXV_STORAGE_B_GASFEE = setValue(newState.TXV_STORAGE_B_GASFEE, storageB.gasFee)

        # Update B
        accountB = self.getAccount(newState.TXV_ACCOUNT_B_ADDRESS)

        rootBefore = self._accountsTree._root
        rootAssetBefore = self._accountsAssetTree._root
        accountBefore = copyAccountInfo(self.getAccount(newState.TXV_ACCOUNT_B_ADDRESS))
        proof = self._accountsTree.createProof(newState.TXV_ACCOUNT_B_ADDRESS)
        proofAsset = self._accountsAssetTree.createProof(newState.TXV_ACCOUNT_B_ADDRESS)
        # DEG-347 Storage move
        (balanceUpdateS_B, storageUpdate_B) = accountB.updateBalanceAndStorage(
            newState.TXV_BALANCE_B_S_ADDRESS,
            newState.TXV_STORAGE_B_TOKENSID,
            newState.TXV_STORAGE_B_TOKENBID,
            newState.TXV_STORAGE_B_STORAGEID,
            newState.TXV_STORAGE_B_DATA,
            # split trading fee and gas fee - add up gas
            # newState.balanceDeltaB_O,
            newState.TXV_STORAGE_B_GASFEE,
            storageB.cancelled,
            newState.TXV_BALANCE_B_S_BALANCE,
            newState.TXV_STORAGE_B_FORWARD
        )

        (newState.TXV_STORAGE_B_ADDRESS_ARRAY, 
        newState.TXV_STORAGE_B_TOKENSID_ARRAY, 
        newState.TXV_STORAGE_B_TOKENBID_ARRAY, 
        newState.TXV_STORAGE_B_DATA_ARRAY, 
        newState.TXV_STORAGE_B_STORAGEID_ARRAY, 
        newState.TXV_STORAGE_B_GASFEE_ARRAY, 
        newState.TXV_STORAGE_B_CANCELLED_ARRAY, 
        newState.TXV_STORAGE_B_FORWARD_ARRAY) = self.calculate.storageDefaultSet(
            accountB, 
            newState.TXV_STORAGE_B_ADDRESS_ARRAY, 
            newState.TXV_STORAGE_B_TOKENSID_ARRAY, 
            newState.TXV_STORAGE_B_TOKENBID_ARRAY, 
            newState.TXV_STORAGE_B_DATA_ARRAY, 
            newState.TXV_STORAGE_B_STORAGEID_ARRAY, 
            newState.TXV_STORAGE_B_GASFEE_ARRAY, 
            newState.TXV_STORAGE_B_CANCELLED_ARRAY, 
            newState.TXV_STORAGE_B_FORWARD_ARRAY, 
            MAX_BATCH_SPOT_TRADE_USER_ORDER[1] - 1)
        storageUpdate_B_array = accountB.updateBatchStorage(
            newState.TXV_STORAGE_B_STORAGEID_ARRAY,
            newState.TXV_STORAGE_B_TOKENSID_ARRAY, 
            newState.TXV_STORAGE_B_TOKENBID_ARRAY, 
            newState.TXV_STORAGE_B_DATA_ARRAY, 
            newState.TXV_STORAGE_B_GASFEE_ARRAY, 
            newState.TXV_STORAGE_B_CANCELLED_ARRAY, 
            newState.TXV_STORAGE_B_FORWARD_ARRAY,
            MAX_BATCH_SPOT_TRADE_USER_ORDER[1] - 1
        )

        balanceUpdateB_B = accountB.updateBalance(
            newState.TXV_BALANCE_B_B_ADDRESS,
            newState.TXV_BALANCE_B_B_BALANCE
        )
        # split trading fee and gas fee
        # account fee token
        balanceUpdateFee_B = accountB.updateBalance(
            # DEG-127 split trading fee and gas fee
            newState.TXV_ACCOUNT_B_FEE_ADDRESS,
            newState.TXV_BALANCE_B_FEE_BALANCE
        )

        accountB.owner = newState.TXV_ACCOUNT_B_OWNER
        accountB.publicKeyX = newState.TXV_ACCOUNT_B_PUBKEY_X
        accountB.publicKeyY = newState.TXV_ACCOUNT_B_PUBKEY_Y
        accountB.nonce = accountB.nonce + newState.TXV_ACCOUNT_B_NONCE

        self.updateAccountTree(newState.TXV_ACCOUNT_B_ADDRESS)
        accountAfter = copyAccountInfo(self.getAccount(newState.TXV_ACCOUNT_B_ADDRESS))
        rootAfter = self._accountsTree._root
        rootAssetAfter = self._accountsAssetTree._root
        accountUpdate_B = AccountUpdateData(newState.TXV_ACCOUNT_B_ADDRESS, proof, proofAsset, rootBefore, rootAfter, rootAssetBefore, rootAssetAfter, accountBefore, accountAfter)
        ###
        # ======== UserC
        newState.TXV_ACCOUNT_C_ADDRESS = setValue(newState.TXV_ACCOUNT_C_ADDRESS, 0)
        accountC = self.getAccount(newState.TXV_ACCOUNT_C_ADDRESS)
        newState.TXV_ACCOUNT_C_OWNER = setValue(newState.TXV_ACCOUNT_C_OWNER, accountC.owner)
        newState.TXV_ACCOUNT_C_PUBKEY_X = setValue(newState.TXV_ACCOUNT_C_PUBKEY_X, accountC.publicKeyX)
        newState.TXV_ACCOUNT_C_PUBKEY_Y = setValue(newState.TXV_ACCOUNT_C_PUBKEY_Y, accountC.publicKeyY)
        newState.TXV_ACCOUNT_C_NONCE = setValue(newState.TXV_ACCOUNT_C_NONCE, 0)

        newState.TXV_BALANCE_C_S_ADDRESS = setValue(newState.TXV_BALANCE_C_S_ADDRESS, 0)
        balanceLeafC_S = accountC.getBalanceLeaf(newState.TXV_BALANCE_C_S_ADDRESS)
        newState.TXV_BALANCE_C_S_BALANCE = setValue(newState.TXV_BALANCE_C_S_BALANCE, 0)

        newState.TXV_BALANCE_C_B_ADDRESS = setValue(newState.TXV_BALANCE_C_B_ADDRESS, 0)
        newState.TXV_BALANCE_C_B_BALANCE = setValue(newState.TXV_BALANCE_C_B_BALANCE, 0)

        newState.TXV_ACCOUNT_C_FEE_ADDRESS = setValue(newState.TXV_ACCOUNT_C_FEE_ADDRESS, 0)
        newState.TXV_BALANCE_C_FEE_BALANCE = setValue(newState.TXV_BALANCE_C_FEE_BALANCE, 0)
        
        # Update C
        rootBefore = self._accountsTree._root
        rootAssetBefore = self._accountsAssetTree._root
        accountBefore = copyAccountInfo(self.getAccount(newState.TXV_ACCOUNT_C_ADDRESS))
        proof = self._accountsTree.createProof(newState.TXV_ACCOUNT_C_ADDRESS)
        proofAsset = self._accountsAssetTree.createProof(newState.TXV_ACCOUNT_C_ADDRESS)

        (balanceUpdateS_C) = accountC.updateBalance(
            newState.TXV_BALANCE_C_S_ADDRESS,
            newState.TXV_BALANCE_C_S_BALANCE
        )

        (newState.TXV_STORAGE_C_ADDRESS_ARRAY,  
        newState.TXV_STORAGE_C_TOKENSID_ARRAY, 
        newState.TXV_STORAGE_C_TOKENBID_ARRAY, 
        newState.TXV_STORAGE_C_DATA_ARRAY, 
        newState.TXV_STORAGE_C_STORAGEID_ARRAY, 
        newState.TXV_STORAGE_C_GASFEE_ARRAY, 
        newState.TXV_STORAGE_C_CANCELLED_ARRAY, 
        newState.TXV_STORAGE_C_FORWARD_ARRAY) = self.calculate.storageDefaultSet(
            accountC, 
            newState.TXV_STORAGE_C_ADDRESS_ARRAY, 
            newState.TXV_STORAGE_C_TOKENSID_ARRAY, 
            newState.TXV_STORAGE_C_TOKENBID_ARRAY, 
            newState.TXV_STORAGE_C_DATA_ARRAY, 
            newState.TXV_STORAGE_C_STORAGEID_ARRAY, 
            newState.TXV_STORAGE_C_GASFEE_ARRAY, 
            newState.TXV_STORAGE_C_CANCELLED_ARRAY, 
            newState.TXV_STORAGE_C_FORWARD_ARRAY, 
            MAX_BATCH_SPOT_TRADE_USER_ORDER[2])
        storageUpdate_C_array = accountC.updateBatchStorage(
            newState.TXV_STORAGE_C_STORAGEID_ARRAY,
            newState.TXV_STORAGE_C_TOKENSID_ARRAY, 
            newState.TXV_STORAGE_C_TOKENBID_ARRAY, 
            newState.TXV_STORAGE_C_DATA_ARRAY, 
            newState.TXV_STORAGE_C_GASFEE_ARRAY, 
            newState.TXV_STORAGE_C_CANCELLED_ARRAY, 
            newState.TXV_STORAGE_C_FORWARD_ARRAY,
            MAX_BATCH_SPOT_TRADE_USER_ORDER[2]
        )
        balanceUpdateB_C = accountC.updateBalance(
            newState.TXV_BALANCE_C_B_ADDRESS,
            newState.TXV_BALANCE_C_B_BALANCE
        )
        balanceUpdateFee_C = accountC.updateBalance(
            newState.TXV_ACCOUNT_C_FEE_ADDRESS,
            newState.TXV_BALANCE_C_FEE_BALANCE
        )

        accountC.owner = newState.TXV_ACCOUNT_C_OWNER
        accountC.publicKeyX = newState.TXV_ACCOUNT_C_PUBKEY_X
        accountC.publicKeyY = newState.TXV_ACCOUNT_C_PUBKEY_Y
        accountC.nonce = accountC.nonce + newState.TXV_ACCOUNT_C_NONCE

        self.updateAccountTree(newState.TXV_ACCOUNT_C_ADDRESS)
        accountAfter = copyAccountInfo(self.getAccount(newState.TXV_ACCOUNT_C_ADDRESS))
        rootAfter = self._accountsTree._root
        rootAssetAfter = self._accountsAssetTree._root
        accountUpdate_C = AccountUpdateData(newState.TXV_ACCOUNT_C_ADDRESS, proof, proofAsset, rootBefore, rootAfter, rootAssetBefore, rootAssetAfter, accountBefore, accountAfter)

        # ======== UserD
        newState.TXV_ACCOUNT_D_ADDRESS = setValue(newState.TXV_ACCOUNT_D_ADDRESS, 0)
        accountD = self.getAccount(newState.TXV_ACCOUNT_D_ADDRESS)
        newState.TXV_ACCOUNT_D_OWNER = setValue(newState.TXV_ACCOUNT_D_OWNER, accountD.owner)
        newState.TXV_ACCOUNT_D_PUBKEY_X = setValue(newState.TXV_ACCOUNT_D_PUBKEY_X, accountD.publicKeyX)
        newState.TXV_ACCOUNT_D_PUBKEY_Y = setValue(newState.TXV_ACCOUNT_D_PUBKEY_Y, accountD.publicKeyY)
        newState.TXV_ACCOUNT_D_NONCE = setValue(newState.TXV_ACCOUNT_D_NONCE, 0)

        newState.TXV_BALANCE_D_S_ADDRESS = setValue(newState.TXV_BALANCE_D_S_ADDRESS, 0)
        balanceLeafD_S = accountD.getBalanceLeaf(newState.TXV_BALANCE_D_S_ADDRESS)
        newState.TXV_BALANCE_D_S_BALANCE = setValue(newState.TXV_BALANCE_D_S_BALANCE, 0)

        newState.TXV_BALANCE_D_B_ADDRESS = setValue(newState.TXV_BALANCE_D_B_ADDRESS, 0)
        newState.TXV_BALANCE_D_B_BALANCE = setValue(newState.TXV_BALANCE_D_B_BALANCE, 0)

        newState.TXV_ACCOUNT_D_FEE_ADDRESS = setValue(newState.TXV_ACCOUNT_D_FEE_ADDRESS, 0)
        newState.TXV_BALANCE_D_FEE_BALANCE = setValue(newState.TXV_BALANCE_D_FEE_BALANCE, 0)
        
        # Update D
        rootBefore = self._accountsTree._root
        rootAssetBefore = self._accountsAssetTree._root
        accountBefore = copyAccountInfo(self.getAccount(newState.TXV_ACCOUNT_D_ADDRESS))
        proof = self._accountsTree.createProof(newState.TXV_ACCOUNT_D_ADDRESS)
        proofAsset = self._accountsAssetTree.createProof(newState.TXV_ACCOUNT_D_ADDRESS)

        (balanceUpdateS_D) = accountD.updateBalance(
            newState.TXV_BALANCE_D_S_ADDRESS,
            newState.TXV_BALANCE_D_S_BALANCE
        )

        (newState.TXV_STORAGE_D_ADDRESS_ARRAY,  
        newState.TXV_STORAGE_D_TOKENSID_ARRAY, 
        newState.TXV_STORAGE_D_TOKENBID_ARRAY, 
        newState.TXV_STORAGE_D_DATA_ARRAY, 
        newState.TXV_STORAGE_D_STORAGEID_ARRAY, 
        newState.TXV_STORAGE_D_GASFEE_ARRAY, 
        newState.TXV_STORAGE_D_CANCELLED_ARRAY, 
        newState.TXV_STORAGE_D_FORWARD_ARRAY) = self.calculate.storageDefaultSet(
            accountD, 
            newState.TXV_STORAGE_D_ADDRESS_ARRAY, 
            newState.TXV_STORAGE_D_TOKENSID_ARRAY, 
            newState.TXV_STORAGE_D_TOKENBID_ARRAY, 
            newState.TXV_STORAGE_D_DATA_ARRAY, 
            newState.TXV_STORAGE_D_STORAGEID_ARRAY, 
            newState.TXV_STORAGE_D_GASFEE_ARRAY, 
            newState.TXV_STORAGE_D_CANCELLED_ARRAY, 
            newState.TXV_STORAGE_D_FORWARD_ARRAY, 
            MAX_BATCH_SPOT_TRADE_USER_ORDER[3])
        storageUpdate_D_array = accountD.updateBatchStorage(
            newState.TXV_STORAGE_D_STORAGEID_ARRAY,
            newState.TXV_STORAGE_D_TOKENSID_ARRAY, 
            newState.TXV_STORAGE_D_TOKENBID_ARRAY, 
            newState.TXV_STORAGE_D_DATA_ARRAY, 
            newState.TXV_STORAGE_D_GASFEE_ARRAY, 
            newState.TXV_STORAGE_D_CANCELLED_ARRAY, 
            newState.TXV_STORAGE_D_FORWARD_ARRAY,
            MAX_BATCH_SPOT_TRADE_USER_ORDER[3]
        )
        balanceUpdateB_D = accountD.updateBalance(
            newState.TXV_BALANCE_D_B_ADDRESS,
            newState.TXV_BALANCE_D_B_BALANCE
        )
        balanceUpdateFee_D = accountD.updateBalance(
            newState.TXV_ACCOUNT_D_FEE_ADDRESS,
            newState.TXV_BALANCE_D_FEE_BALANCE
        )

        accountD.owner = newState.TXV_ACCOUNT_D_OWNER
        accountD.publicKeyX = newState.TXV_ACCOUNT_D_PUBKEY_X
        accountD.publicKeyY = newState.TXV_ACCOUNT_D_PUBKEY_Y
        accountD.nonce = accountD.nonce + newState.TXV_ACCOUNT_D_NONCE

        self.updateAccountTree(newState.TXV_ACCOUNT_D_ADDRESS)
        accountAfter = copyAccountInfo(self.getAccount(newState.TXV_ACCOUNT_D_ADDRESS))
        rootAfter = self._accountsTree._root
        rootAssetAfter = self._accountsAssetTree._root
        accountUpdate_D = AccountUpdateData(newState.TXV_ACCOUNT_D_ADDRESS, proof, proofAsset, rootBefore, rootAfter, rootAssetBefore, rootAssetAfter, accountBefore, accountAfter)


        # ======== UserE
        newState.TXV_ACCOUNT_E_ADDRESS = setValue(newState.TXV_ACCOUNT_E_ADDRESS, 0)
        accountE = self.getAccount(newState.TXV_ACCOUNT_E_ADDRESS)
        newState.TXV_ACCOUNT_E_OWNER = setValue(newState.TXV_ACCOUNT_E_OWNER, accountE.owner)
        newState.TXV_ACCOUNT_E_PUBKEY_X = setValue(newState.TXV_ACCOUNT_E_PUBKEY_X, accountE.publicKeyX)
        newState.TXV_ACCOUNT_E_PUBKEY_Y = setValue(newState.TXV_ACCOUNT_E_PUBKEY_Y, accountE.publicKeyY)
        newState.TXV_ACCOUNT_E_NONCE = setValue(newState.TXV_ACCOUNT_E_NONCE, 0)

        newState.TXV_BALANCE_E_S_ADDRESS = setValue(newState.TXV_BALANCE_E_S_ADDRESS, 0)
        balanceLeafE_S = accountE.getBalanceLeaf(newState.TXV_BALANCE_E_S_ADDRESS)
        newState.TXV_BALANCE_E_S_BALANCE = setValue(newState.TXV_BALANCE_E_S_BALANCE, 0)

        newState.TXV_BALANCE_E_B_ADDRESS = setValue(newState.TXV_BALANCE_E_B_ADDRESS, 0)
        newState.TXV_BALANCE_E_B_BALANCE = setValue(newState.TXV_BALANCE_E_B_BALANCE, 0)

        newState.TXV_ACCOUNT_E_FEE_ADDRESS = setValue(newState.TXV_ACCOUNT_E_FEE_ADDRESS, 0)
        newState.TXV_BALANCE_E_FEE_BALANCE = setValue(newState.TXV_BALANCE_E_FEE_BALANCE, 0)
        
        # Update E
        rootBefore = self._accountsTree._root
        rootAssetBefore = self._accountsAssetTree._root
        accountBefore = copyAccountInfo(self.getAccount(newState.TXV_ACCOUNT_E_ADDRESS))
        proof = self._accountsTree.createProof(newState.TXV_ACCOUNT_E_ADDRESS)
        proofAsset = self._accountsAssetTree.createProof(newState.TXV_ACCOUNT_E_ADDRESS)

        (balanceUpdateS_E) = accountE.updateBalance(
            newState.TXV_BALANCE_E_S_ADDRESS,
            newState.TXV_BALANCE_E_S_BALANCE
        )

        (newState.TXV_STORAGE_E_ADDRESS_ARRAY,  
        newState.TXV_STORAGE_E_TOKENSID_ARRAY, 
        newState.TXV_STORAGE_E_TOKENBID_ARRAY, 
        newState.TXV_STORAGE_E_DATA_ARRAY, 
        newState.TXV_STORAGE_E_STORAGEID_ARRAY, 
        newState.TXV_STORAGE_E_GASFEE_ARRAY, 
        newState.TXV_STORAGE_E_CANCELLED_ARRAY, 
        newState.TXV_STORAGE_E_FORWARD_ARRAY) = self.calculate.storageDefaultSet(
            accountE, 
            newState.TXV_STORAGE_E_ADDRESS_ARRAY, 
            newState.TXV_STORAGE_E_TOKENSID_ARRAY, 
            newState.TXV_STORAGE_E_TOKENBID_ARRAY, 
            newState.TXV_STORAGE_E_DATA_ARRAY, 
            newState.TXV_STORAGE_E_STORAGEID_ARRAY, 
            newState.TXV_STORAGE_E_GASFEE_ARRAY, 
            newState.TXV_STORAGE_E_CANCELLED_ARRAY, 
            newState.TXV_STORAGE_E_FORWARD_ARRAY, 
            MAX_BATCH_SPOT_TRADE_USER_ORDER[4])
        storageUpdate_E_array = accountE.updateBatchStorage(
            newState.TXV_STORAGE_E_STORAGEID_ARRAY,
            newState.TXV_STORAGE_E_TOKENSID_ARRAY, 
            newState.TXV_STORAGE_E_TOKENBID_ARRAY, 
            newState.TXV_STORAGE_E_DATA_ARRAY, 
            newState.TXV_STORAGE_E_GASFEE_ARRAY, 
            newState.TXV_STORAGE_E_CANCELLED_ARRAY, 
            newState.TXV_STORAGE_E_FORWARD_ARRAY,
            MAX_BATCH_SPOT_TRADE_USER_ORDER[4]
        )
        balanceUpdateB_E = accountE.updateBalance(
            newState.TXV_BALANCE_E_B_ADDRESS,
            newState.TXV_BALANCE_E_B_BALANCE
        )
        balanceUpdateFee_E = accountE.updateBalance(
            newState.TXV_ACCOUNT_E_FEE_ADDRESS,
            newState.TXV_BALANCE_E_FEE_BALANCE
        )

        accountE.owner = newState.TXV_ACCOUNT_E_OWNER
        accountE.publicKeyX = newState.TXV_ACCOUNT_E_PUBKEY_X
        accountE.publicKeyY = newState.TXV_ACCOUNT_E_PUBKEY_Y
        accountE.nonce = accountE.nonce + newState.TXV_ACCOUNT_E_NONCE

        self.updateAccountTree(newState.TXV_ACCOUNT_E_ADDRESS)
        accountAfter = copyAccountInfo(self.getAccount(newState.TXV_ACCOUNT_E_ADDRESS))
        rootAfter = self._accountsTree._root
        rootAssetAfter = self._accountsAssetTree._root
        accountUpdate_E = AccountUpdateData(newState.TXV_ACCOUNT_E_ADDRESS, proof, proofAsset, rootBefore, rootAfter, rootAssetBefore, rootAssetAfter, accountBefore, accountAfter)

        # ======== UserF
        newState.TXV_ACCOUNT_F_ADDRESS = setValue(newState.TXV_ACCOUNT_F_ADDRESS, 0)
        accountF = self.getAccount(newState.TXV_ACCOUNT_F_ADDRESS)
        newState.TXV_ACCOUNT_F_OWNER = setValue(newState.TXV_ACCOUNT_F_OWNER, accountF.owner)
        newState.TXV_ACCOUNT_F_PUBKEY_X = setValue(newState.TXV_ACCOUNT_F_PUBKEY_X, accountF.publicKeyX)
        newState.TXV_ACCOUNT_F_PUBKEY_Y = setValue(newState.TXV_ACCOUNT_F_PUBKEY_Y, accountF.publicKeyY)
        newState.TXV_ACCOUNT_F_NONCE = setValue(newState.TXV_ACCOUNT_F_NONCE, 0)

        newState.TXV_BALANCE_F_S_ADDRESS = setValue(newState.TXV_BALANCE_F_S_ADDRESS, 0)
        balanceLeafF_S = accountF.getBalanceLeaf(newState.TXV_BALANCE_F_S_ADDRESS)
        newState.TXV_BALANCE_F_S_BALANCE = setValue(newState.TXV_BALANCE_F_S_BALANCE, 0)

        newState.TXV_BALANCE_F_B_ADDRESS = setValue(newState.TXV_BALANCE_F_B_ADDRESS, 0)
        newState.TXV_BALANCE_F_B_BALANCE = setValue(newState.TXV_BALANCE_F_B_BALANCE, 0)

        newState.TXV_ACCOUNT_F_FEE_ADDRESS = setValue(newState.TXV_ACCOUNT_F_FEE_ADDRESS, 0)
        newState.TXV_BALANCE_F_FEE_BALANCE = setValue(newState.TXV_BALANCE_F_FEE_BALANCE, 0)
        
        # Update F
        rootBefore = self._accountsTree._root
        rootAssetBefore = self._accountsAssetTree._root
        accountBefore = copyAccountInfo(self.getAccount(newState.TXV_ACCOUNT_F_ADDRESS))
        proof = self._accountsTree.createProof(newState.TXV_ACCOUNT_F_ADDRESS)
        proofAsset = self._accountsAssetTree.createProof(newState.TXV_ACCOUNT_F_ADDRESS)

        (balanceUpdateS_F) = accountF.updateBalance(
            newState.TXV_BALANCE_F_S_ADDRESS,
            newState.TXV_BALANCE_F_S_BALANCE
        )

        (newState.TXV_STORAGE_F_ADDRESS_ARRAY,  
        newState.TXV_STORAGE_F_TOKENSID_ARRAY, 
        newState.TXV_STORAGE_F_TOKENBID_ARRAY, 
        newState.TXV_STORAGE_F_DATA_ARRAY, 
        newState.TXV_STORAGE_F_STORAGEID_ARRAY, 
        newState.TXV_STORAGE_F_GASFEE_ARRAY, 
        newState.TXV_STORAGE_F_CANCELLED_ARRAY, 
        newState.TXV_STORAGE_F_FORWARD_ARRAY) = self.calculate.storageDefaultSet(
            accountF, 
            newState.TXV_STORAGE_F_ADDRESS_ARRAY, 
            newState.TXV_STORAGE_F_TOKENSID_ARRAY, 
            newState.TXV_STORAGE_F_TOKENBID_ARRAY, 
            newState.TXV_STORAGE_F_DATA_ARRAY, 
            newState.TXV_STORAGE_F_STORAGEID_ARRAY, 
            newState.TXV_STORAGE_F_GASFEE_ARRAY, 
            newState.TXV_STORAGE_F_CANCELLED_ARRAY, 
            newState.TXV_STORAGE_F_FORWARD_ARRAY, 
            MAX_BATCH_SPOT_TRADE_USER_ORDER[5])
        storageUpdate_F_array = accountF.updateBatchStorage(
            newState.TXV_STORAGE_F_STORAGEID_ARRAY,
            newState.TXV_STORAGE_F_TOKENSID_ARRAY, 
            newState.TXV_STORAGE_F_TOKENBID_ARRAY, 
            newState.TXV_STORAGE_F_DATA_ARRAY, 
            newState.TXV_STORAGE_F_GASFEE_ARRAY, 
            newState.TXV_STORAGE_F_CANCELLED_ARRAY, 
            newState.TXV_STORAGE_F_FORWARD_ARRAY,
            MAX_BATCH_SPOT_TRADE_USER_ORDER[5]
        )
        balanceUpdateB_F = accountF.updateBalance(
            newState.TXV_BALANCE_F_B_ADDRESS,
            newState.TXV_BALANCE_F_B_BALANCE
        )
        balanceUpdateFee_F = accountF.updateBalance(
            newState.TXV_ACCOUNT_F_FEE_ADDRESS,
            newState.TXV_BALANCE_F_FEE_BALANCE
        )

        accountF.owner = newState.TXV_ACCOUNT_F_OWNER
        accountF.publicKeyX = newState.TXV_ACCOUNT_F_PUBKEY_X
        accountF.publicKeyY = newState.TXV_ACCOUNT_F_PUBKEY_Y
        accountF.nonce = accountF.nonce + newState.TXV_ACCOUNT_F_NONCE

        self.updateAccountTree(newState.TXV_ACCOUNT_F_ADDRESS)
        accountAfter = copyAccountInfo(self.getAccount(newState.TXV_ACCOUNT_F_ADDRESS))
        rootAfter = self._accountsTree._root
        rootAssetAfter = self._accountsAssetTree._root
        accountUpdate_F = AccountUpdateData(newState.TXV_ACCOUNT_F_ADDRESS, proof, proofAsset, rootBefore, rootAfter, rootAssetBefore, rootAssetAfter, accountBefore, accountAfter)

        # Update balances Operator
        accountO = self.getAccount(context.operatorAccountID)

        rootBefore = self._accountsTree._root
        rootAssetBefore = self._accountsAssetTree._root
        accountBefore = copyAccountInfo(self.getAccount(context.operatorAccountID))
        proof = self._accountsTree.createProof(context.operatorAccountID)
        proofAsset = self._accountsAssetTree.createProof(context.operatorAccountID)

        # split trading fee and gas fee
        balanceUpdateD_O = accountO.updateBalance(
            newState.balanceDeltaD_O_Address,
            newState.balanceDeltaD_O
        )
        balanceUpdateC_O = accountO.updateBalance(
            newState.balanceDeltaC_O_Address,
            newState.balanceDeltaC_O
        )
        balanceUpdateB_O = accountO.updateBalance(
            newState.balanceDeltaB_O_Address,
            newState.balanceDeltaB_O
        )
        balanceUpdateA_O = accountO.updateBalance(
            newState.balanceDeltaA_O_Address,
            newState.balanceDeltaA_O
        )
        

        self.updateAccountTree(context.operatorAccountID)
        accountAfter = copyAccountInfo(self.getAccount(context.operatorAccountID))
        rootAfter = self._accountsTree._root
        rootAssetAfter = self._accountsAssetTree._root
        accountUpdate_O = AccountUpdateData(context.operatorAccountID, proof, proofAsset, rootBefore, rootAfter, rootAssetBefore, rootAssetAfter, accountBefore, accountAfter)
        ###

        witness = Witness(newState.signatureA, newState.signatureB, newState.signatures,
                          accountsMerkleRoot, accountsMerkleAssetRoot,
                          storageUpdate_A, storageUpdate_B,
                          storageUpdate_A_array, storageUpdate_B_array,
                          storageUpdate_C_array, storageUpdate_D_array,
                          storageUpdate_E_array, storageUpdate_F_array,

                          balanceUpdateS_A, balanceUpdateB_A, balanceUpdateFee_A, accountUpdate_A,
                          balanceUpdateS_B, balanceUpdateB_B, balanceUpdateFee_B, accountUpdate_B,
                          balanceUpdateS_C, balanceUpdateB_C, balanceUpdateFee_C, accountUpdate_C,
                          balanceUpdateS_D, balanceUpdateB_D, balanceUpdateFee_D, accountUpdate_D,
                          balanceUpdateS_E, balanceUpdateB_E, balanceUpdateFee_E, accountUpdate_E,
                          balanceUpdateS_F, balanceUpdateB_F, balanceUpdateFee_F, accountUpdate_F,

                          balanceUpdateA_O, balanceUpdateB_O, balanceUpdateC_O, balanceUpdateD_O, accountUpdate_O
                          )

        return TxWitness(witness, txInput)

    def getAccount(self, accountID):
        # Make sure the leaf exist in our map
        if not(str(accountID) in self._accounts):
            # print("Account doesn't exist: " + str(accountID))
            self._accounts[str(accountID)] = Account(0, Point(0, 0), Point(0, 0))
        return self._accounts[str(accountID)]

    def updateAccountTree(self, accountID):
        self._accountsTree.update(accountID, self.getAccount(accountID).hash())
        self._accountsAssetTree.update(accountID, self.getAccount(accountID).hashAsset())

    def getRoot(self):
        return self._accountsTree._root
    def getAssetRoot(self):
        return self._accountsAssetTree._root

