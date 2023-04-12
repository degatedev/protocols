# Circuit documentation

For a full overview of the Loopring protocol, see https://github.com/Loopring/protocols/blob/master/packages/loopring_v3/DESIGN.md.

## Constants

- TREE_DEPTH_STORAGE = 7
- TREE_DEPTH_ACCOUNTS = 16
- TREE_DEPTH_TOKENS = 16

- TX_DATA_AVAILABILITY_SIZE = 83

- NUM_BITS_MAX_VALUE = 254
- NUM_BITS_FIELD_CAPACITY = 253
- NUM_BITS_AMOUNT = 96
- NUM_BITS_AMOUNT_DEPOSIT = 248
- NUM_BITS_AMOUNT_WITHDRAW = 248
- NUM_BITS_STORAGE_ADDRESS = TREE_DEPTH_STORAGE\*2
- NUM_BITS_ACCOUNT = TREE_DEPTH_ACCOUNTS\*2
- NUM_BITS_TOKEN = TREE_DEPTH_TOKENS\*2
- NUM_BITS_AUTOMARKET_LEVEL = 8
- NUM_BITS_STORAGEID = 32
- NUM_BITS_TIMESTAMP = 32
- NUM_BITS_NONCE = 32
- NUM_BITS_BIPS = 12
- NUM_BITS_BIPS_DA = 6
- NUM_BITS_PROTOCOL_FEE_BIPS = 8
- NUM_BITS_TYPE = 8
- NUM_STORAGE_SLOTS = 16384
- NUM_MARKETS_PER_BLOCK = 16

- NUM_BITS_TX_TYPE = 3
- NUM_BITS_TX_TYPE_FOR_SELECT = 5
- NUM_BITS_BATCH_SPOTRADE_TOKEN_TYPE = 2
- NUM_BITS_TX_SIZE = 16
- NUM_BITS_BIND_TOKEN_ID_SIZE = 5
- NUM_BITS_ADDRESS = 160
- NUM_BITS_HASH = 160
- NUM_BITS_BOOL = 8
- NUM_BITS_BIT = 1
- NUM_BITS_BYTE = 8
- NUM_BITS_FLOAT_31 = 31
- NUM_BITS_FLOAT_30 = 30
- NUM_BITS_MIN_GAS = 248

- EMPTY_STORAGE_ROOT = 6592749167578234498153410564243369229486412054742481069049239297514590357090
- MAX_AMOUNT = 79228162514264337593543950335 // 2^96 - 1
- FIXED_BASE = 1000000000000000000 // 10^18
- NUM_BITS_FIXED_BASE = 60 // ceil(log2(10^18))
- FEE_MULTIPLIER = 50

- BATCH_SPOT_TRADE_MAX_USER = 6
- BATCH_SPOT_TRADE_MAX_TOKENS = 3
- ORDER_SIZE_USER_MAX = 4
- ORDER_SIZE_USER_A = 4
- ORDER_SIZE_USER_B = 2
- ORDER_SIZE_USER_C = 1
- ORDER_SIZE_USER_D = 1
- ORDER_SIZE_USER_E = 1
- ORDER_SIZE_USER_F = 1

- LogDebug = "Debug"
- LogInfo = "Info"
- LogError = "Error"

- Float32Encoding: Accuracy = {7, 24} The remaining 1 bit is used for the symbols of positive and negative numbers
- Float30Encoding: Accuracy = {5, 25}
- Float29Encoding: Accuracy = {5, 24}
- Float24Encoding: Accuracy = {5, 19}
- Float16Encoding: Accuracy = {5, 11}

- JubJub.a := 168700
- JubJub.d := 168696
- JubJub.A := 168698

- TransactionType.Noop := 0 (4 bits)
- TransactionType.Transfer := 1 (4 bits)
- TransactionType.SpotTrade := 2 (4 bits)
- TransactionType.OrderCancel := 3 (4 bits)
- TransactionType.AppKeyUpdate := 4 (4 bits)
- TransactionType.BatchSpotTrade := 5 (4 bits)
- TransactionType.Deposit := 6 (4 bits)
- TransactionType.AccountUpdate := 7 (4 bits)
- TransactionType.Withdrawal := 8 (4 bits)

## Data types

- F := Snark field element
- Storage := (tokenSID: F, tokenBID: F, data: F, storageID: F, gasFee: F, cancelled: F, forward: F)
- Balance := (balance: F)
- Account := (owner: F, publicKeyX: F, publicKeyY: F, appKeyPublicKeyX: F, appKeyPublicKeyY: F, nonce: F, disableAppKeySpotTrade:F, disableAppKeyWithdraw: F, disableAppKeyTransferToOther: F, balancesRoot: F, storageRoot: F)
- SignedF := (sign: {0..2}, value: F),
  sign == 1 -> positive value,
  sign == 0 -> negative value,
  value == 0 can have sign 0 or 1.

- AccountState := (
  storage: Storage,
  storageArray: StorageGadget[],
  balanceS: Balance,
  balanceB: Balance,
  balanceFee: Balance,
  account: Account
  )
- AccountOperator := (
  balanceA: Balance,
  balanceB: Balance,
  balanceC: Balance,
  balanceD: Balance,
  account: Account
  )
- AccountBalances := (
  balanceA: Balance,
  balanceB: Balance,
  balanceC: Balance
  )
- State := (
  exchange: {0..2^NUM_BITS_ADDRESS},
  timestamp: {0..2^NUM_BITS_TIMESTAMP},
  protocolFeeBips: {0..2^NUM_BITS_PROTOCOL_FEE_BIPS},
  numConditionalTransactions: F,
  txType: {0..2^NUM_BITS_TX_TYPE},

  accountA: AccountState,
  accountB: AccountState,
  accountC: AccountState,
  accountD: AccountState,
  accountE: AccountState,
  accountF: AccountState,
  operator: AccountOperatorState
  )

- Accuracy := (N: unsigned int, D: unsigned int)

- TxOutput := (
  STORAGE_A_ADDRESS: F[NUM_BITS_STORAGE_ADDRESS],
  STORAGE_A_TOKENSID: F,
  STORAGE_A_TOKENBID: F,
  STORAGE_A_DATA: F,
  STORAGE_A_STORAGEID: F,
  STORAGE_A_GASFEE: F,
  STORAGE_A_CANCELLED: F,
  STORAGE_A_FORWARD: F,

  BALANCE_A_S_ADDRESS: F[NUM_BITS_TOKEN],
  BALANCE_A_S_BALANCE: F,

  BALANCE_A_B_ADDRESS: F[NUM_BITS_TOKEN],
  BALANCE_A_B_BALANCE: F,

  ACCOUNT_A_ADDRESS: F[NUM_BITS_ACCOUNT],
  ACCOUNT_A_OWNER: F,
  ACCOUNT_A_PUBKEY_X: F,
  ACCOUNT_A_PUBKEY_Y: F,
  ACCOUNT_A_NONCE: F,

  STORAGE_A_ADDRESS_ARRAY_0: F[NUM_BITS_ACCOUNT],
  STORAGE_A_TOKENSID_ARRAY_0: F,
  STORAGE_A_TOKENBID_ARRAY_0: F,
  STORAGE_A_DATA_ARRAY_0: F,
  STORAGE_A_STORAGEID_ARRAY_0: F,
  STORAGE_A_GASFEE_ARRAY_0: F,
  STORAGE_A_CANCELLED_ARRAY_0: F,
  STORAGE_A_FORWARD_ARRAY_0: F,

  STORAGE_A_ADDRESS_ARRAY_1: F[NUM_BITS_ACCOUNT],
  STORAGE_A_TOKENSID_ARRAY_1: F,
  STORAGE_A_TOKENBID_ARRAY_1: F,
  STORAGE_A_DATA_ARRAY_1: F,
  STORAGE_A_STORAGEID_ARRAY_1: F,
  STORAGE_A_GASFEE_ARRAY_1: F,
  STORAGE_A_CANCELLED_ARRAY_1: F,
  STORAGE_A_FORWARD_ARRAY_1: F,

  STORAGE_A_ADDRESS_ARRAY_2: F[NUM_BITS_ACCOUNT],
  STORAGE_A_TOKENSID_ARRAY_2: F,
  STORAGE_A_TOKENBID_ARRAY_2: F,
  STORAGE_A_DATA_ARRAY_2: F,
  STORAGE_A_STORAGEID_ARRAY_2: F,
  STORAGE_A_GASFEE_ARRAY_2: F,
  STORAGE_A_CANCELLED_ARRAY_2: F,
  STORAGE_A_FORWARD_ARRAY_2: F,

  STORAGE_A_ADDRESS_ARRAY_3: F[NUM_BITS_ACCOUNT],
  STORAGE_A_TOKENSID_ARRAY_3: F,
  STORAGE_A_TOKENBID_ARRAY_3: F,
  STORAGE_A_DATA_ARRAY_3: F,
  STORAGE_A_STORAGEID_ARRAY_3: F,
  STORAGE_A_GASFEE_ARRAY_3: F,
  STORAGE_A_CANCELLED_ARRAY_3: F,
  STORAGE_A_FORWARD_ARRAY_3: F,

  STORAGE_B_ADDRESS: F[NUM_BITS_STORAGE_ADDRESS],
  STORAGE_B_TOKENSID: F,
  STORAGE_B_TOKENBID: F,
  STORAGE_B_DATA: F,
  STORAGE_B_STORAGEID: F,
  STORAGE_B_GASFEE: F,
  STORAGE_B_FORWARD: F,

  BALANCE_B_S_ADDRESS: F[NUM_BITS_TOKEN],
  BALANCE_B_S_BALANCE: F,

  BALANCE_B_B_ADDRESS: F,
  BALANCE_B_B_BALANCE: F,

  ACCOUNT_B_ADDRESS: F[NUM_BITS_ACCOUNT],
  ACCOUNT_B_OWNER: F,
  ACCOUNT_B_PUBKEY_X: F,
  ACCOUNT_B_PUBKEY_Y: F,
  ACCOUNT_B_NONCE: F,

  STORAGE_B_ADDRESS_ARRAY_0: F[NUM_BITS_ACCOUNT],
  STORAGE_B_TOKENSID_ARRAY_0: F,
  STORAGE_B_TOKENBID_ARRAY_0: F,
  STORAGE_B_DATA_ARRAY_0: F,
  STORAGE_B_STORAGEID_ARRAY_0: F,
  STORAGE_B_GASFEE_ARRAY_0: F,
  STORAGE_B_CANCELLED_ARRAY_0: F,
  STORAGE_B_FORWARD_ARRAY_0: F,

  BALANCE_C_S_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_C_S_BALANCE: F,

  BALANCE_C_B_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_C_B_BALANCE: F,

  ACCOUNT_C_ADDRESS: F[NUM_BITS_ACCOUNT],
  ACCOUNT_C_OWNER: F,
  ACCOUNT_C_PUBKEY_X: F,
  ACCOUNT_C_PUBKEY_Y: F,
  ACCOUNT_C_NONCE: F,

  STORAGE_C_ADDRESS_ARRAY_0: F[NUM_BITS_ACCOUNT],
  STORAGE_C_TOKENSID_ARRAY_0: F,
  STORAGE_C_TOKENBID_ARRAY_0: F,
  STORAGE_C_DATA_ARRAY_0: F,
  STORAGE_C_STORAGEID_ARRAY_0: F,
  STORAGE_C_GASFEE_ARRAY_0: F,
  STORAGE_C_CANCELLED_ARRAY_0: F,
  STORAGE_C_FORWARD_ARRAY_0: F,

  BALANCE_D_S_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_D_S_BALANCE: F,

  BALANCE_D_B_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_D_B_BALANCE: F,

  ACCOUNT_D_ADDRESS: F[NUM_BITS_ACCOUNT],
  ACCOUNT_D_OWNER: F,
  ACCOUNT_D_PUBKEY_X: F,
  ACCOUNT_D_PUBKEY_Y: F,
  ACCOUNT_D_NONCE: F,

  STORAGE_D_ADDRESS_ARRAY_0: F[NUM_BITS_ACCOUNT],
  STORAGE_D_TOKENSID_ARRAY_0: F,
  STORAGE_D_TOKENBID_ARRAY_0: F,
  STORAGE_D_DATA_ARRAY_0: F,
  STORAGE_D_STORAGEID_ARRAY_0: F,
  STORAGE_D_GASFEE_ARRAY_0: F,
  STORAGE_D_CANCELLED_ARRAY_0: F,
  STORAGE_D_FORWARD_ARRAY_0: F,

  BALANCE_E_S_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_E_S_BALANCE: F,

  BALANCE_E_B_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_E_B_BALANCE: F,

  ACCOUNT_E_ADDRESS: F[NUM_BITS_ACCOUNT],
  ACCOUNT_E_OWNER: F,
  ACCOUNT_E_PUBKEY_X: F,
  ACCOUNT_E_PUBKEY_Y: F,
  ACCOUNT_E_NONCE: F,

  STORAGE_E_ADDRESS_ARRAY_0: F[NUM_BITS_ACCOUNT],
  STORAGE_E_TOKENSID_ARRAY_0: F,
  STORAGE_E_TOKENBID_ARRAY_0: F,
  STORAGE_E_DATA_ARRAY_0: F,
  STORAGE_E_STORAGEID_ARRAY_0: F,
  STORAGE_E_GASFEE_ARRAY_0: F,
  STORAGE_E_CANCELLED_ARRAY_0: F,
  STORAGE_E_FORWARD_ARRAY_0: F,

  BALANCE_F_S_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_F_S_BALANCE: F,

  BALANCE_F_B_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_F_B_BALANCE: F,

  ACCOUNT_F_ADDRESS: F[NUM_BITS_ACCOUNT],
  ACCOUNT_F_OWNER: F,
  ACCOUNT_F_PUBKEY_X: F,
  ACCOUNT_F_PUBKEY_Y: F,
  ACCOUNT_F_NONCE: F,

  STORAGE_F_ADDRESS_ARRAY_0: F[NUM_BITS_ACCOUNT],
  STORAGE_F_TOKENSID_ARRAY_0: F,
  STORAGE_F_TOKENBID_ARRAY_0: F,
  STORAGE_F_DATA_ARRAY_0: F,
  STORAGE_F_STORAGEID_ARRAY_0: F,
  STORAGE_F_GASFEE_ARRAY_0: F,
  STORAGE_F_CANCELLED_ARRAY_0: F,
  STORAGE_F_FORWARD_ARRAY_0: F,

  BALANCE_O_A_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_O_B_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_O_C_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_O_D_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_O_A_BALANCE: F,
  BALANCE_O_B_BALANCE: F,
  BALANCE_O_C_BALANCE: F,
  BALANCE_O_D_BALANCE: F,

  BALANCE_A_FEE_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_B_FEE_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_C_FEE_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_D_FEE_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_E_FEE_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_F_FEE_ADDRESS: F[NUM_BITS_ACCOUNT],
  BALANCE_A_FEE_BALANCE: F,
  BALANCE_B_FEE_BALANCE: F,
  BALANCE_C_FEE_BALANCE: F,
  BALANCE_D_FEE_BALANCE: F,
  BALANCE_E_FEE_BALANCE: F,
  BALANCE_F_FEE_BALANCE: F,

  HASH_A: F,
  PUBKEY_X_A: F,
  PUBKEY_Y_A: F,
  SIGNATURE_REQUIRED_A: F,

  HASH_B: F,
  PUBKEY_X_B: F,
  PUBKEY_Y_B: F,
  SIGNATURE_REQUIRED_B: F,

  HASH_A_ARRAY: F[3],
  PUBKEY_X_A_ARRAY: F[3],
  PUBKEY_Y_A_ARRAY: F[3],
  SIGNATURE_REQUIRED_A_ARRAY: F[3],

  HASH_B_ARRAY: F[1],
  PUBKEY_X_B_ARRAY: F[1],
  PUBKEY_Y_B_ARRAY: F[1],
  SIGNATURE_REQUIRED_B_ARRAY: F[1],

  HASH_C_ARRAY: F[1],
  PUBKEY_X_C_ARRAY: F[1],
  PUBKEY_Y_C_ARRAY: F[1],
  SIGNATURE_REQUIRED_C_ARRAY: F[1],

  HASH_D_ARRAY: F[1],
  PUBKEY_X_D_ARRAY: F[1],
  PUBKEY_Y_D_ARRAY: F[1],
  SIGNATURE_REQUIRED_D_ARRAY: F[1],

  HASH_E_ARRAY: F[1],
  PUBKEY_X_E_ARRAY: F[1],
  PUBKEY_Y_E_ARRAY: F[1],
  SIGNATURE_REQUIRED_E_ARRAY: F[1],

  HASH_F_ARRAY: F[1],
  PUBKEY_X_F_ARRAY: F[1],
  PUBKEY_Y_F_ARRAY: F[1],
  SIGNATURE_REQUIRED_F_ARRAY: F[1],

  NUM_CONDITIONAL_TXS: F,

  DA: F[TX_DATA_AVAILABILITY_SIZE\*8]
)

- OrderMatchingData := (
  orderFeeBips: {0..2^8},
  fillS: {0..2^NUM_BITS_AMOUNT},
  balanceBeforeS: {0..2^NUM_BITS_AMOUNT},
  balanceBeforeB: {0..2^NUM_BITS_AMOUNT},
  balanceAfterS: {0..2^NUM_BITS_AMOUNT},
  balanceAfterB: {0..2^NUM_BITS_AMOUNT},
  )

## Poseidon

Most of the hashing is done using the Poseidon (https://eprint.iacr.org/2019/458.pdf) hash function. This hash function works directly on field elements and is very efficient.

Poseidon can be instantiated with 3 parameters: t, f, and p. In all cases we use the following method to choose these:

1. Set t equal to the number of inputs + 1 (this extra input of 0 is the capacity).
2. With the above t, choose f and p so that the number of constraints necessary is minimized, while ensuring a minimum security level of 128 bit. The rules that need to be followed to achieve this are described in the paper. In practice, the script available at https://github.com/Loopring/protocols/blob/master/packages/loopring_v3/util/find_optimal_poseidon.py.

Because Poseidon is more efficient for four inputs compared to two inputs we use quad Merkle trees instead of binary Merkle trees.

## DefaultTxOutput

A valid instance of a DefaultTxOutput statement assures that given an input of:

- state: State

the prover knows an auxiliary input:

- output: TxOutput

such that the following conditions hold:

- output.STORAGE_A_ADDRESS = 0
- output.STORAGE_A_DATA = state.accountA.storage.data
- output.STORAGE_A_TOKENSID = state.accountA.storage.tokenSID
- output.STORAGE_A_TOKENBID = state.accountA.storage.tokenBID
- output.TORAGE_A_STORAGEID = state.accountA.storage.storageID
- output.STORAGE_A_GASFEE = state.accountA.storage.gasFee
- output.STORAGE_A_CANCELLED = state.accountA.storage.cancelled
- output.STORAGE_A_FORWARD = state.accountA.storage.forward

- output.BALANCE_A_S_ADDRESS = 0
- output.BALANCE_A_S_BALANCE = state.accountA.balanceS.balance

- output.BALANCE_A_B_ADDRESS = 0
- output.BALANCE_A_B_BALANCE = state.accountA.balanceB.balance

- output.BALANCE_A_FEE_ADDRESS = 0
- output.BALANCE_A_FEE_BALANCE = state.accountA.balanceFee.balance

- output.ACCOUNT_A_ADDRESS = 1
- output.ACCOUNT_A_OWNER = state.accountA.account.owner
- output.ACCOUNT_A_PUBKEY_X = state.accountA.account.publicKeyX
- output.ACCOUNT_A_PUBKEY_Y = state.accountA.account.publicKeyY
- output.ACCOUNT_A_NONCE = state.accountA.account.nonce

- output.STORAGE_A_ADDRESS_ARRAY_0 = 0
- output.STORAGE_A_TOKENSID_ARRAY_0 = state.accountA.storageArray[0].tokenSID
- output.STORAGE_A_TOKENBID_ARRAY_0 = state.accountA.storageArray[0].tokenBID
- output.STORAGE_A_DATA_ARRAY_0 = state.accountA.storageArray[0].data
- output.STORAGE_A_STORAGEID_ARRAY_0 = state.accountA.storageArray[0].storageID
- output.STORAGE_A_GASFEE_ARRAY_0 = state.accountA.storageArray[0].gasFee
- output.STORAGE_A_CANCELLED_ARRAY_0 = state.accountA.storageArray[0].cancelled
- output.STORAGE_A_FORWARD_ARRAY_0 = state.accountA.storageArray[0].forward

- output.STORAGE_A_ADDRESS_ARRAY_1 = 0
- output.STORAGE_A_TOKENSID_ARRAY_1 = state.accountA.storageArray[1].tokenSID
- output.STORAGE_A_TOKENBID_ARRAY_1 = state.accountA.storageArray[1].tokenBID
- output.STORAGE_A_DATA_ARRAY_1 = state.accountA.storageArray[1].data
- output.STORAGE_A_STORAGEID_ARRAY_1 = state.accountA.storageArray[1].storageID
- output.STORAGE_A_GASFEE_ARRAY_1 = state.accountA.storageArray[1].gasFee
- output.STORAGE_A_CANCELLED_ARRAY_1 = state.accountA.storageArray[1].cancelled
- output.STORAGE_A_FORWARD_ARRAY_1 = state.accountA.storageArray[1].forward

- output.STORAGE_A_ADDRESS_ARRAY_2 = 0
- output.STORAGE_A_TOKENSID_ARRAY_2 = state.accountA.storageArray[2].tokenSID
- output.STORAGE_A_TOKENBID_ARRAY_2 = state.accountA.storageArray[2].tokenBID
- output.STORAGE_A_DATA_ARRAY_2 = state.accountA.storageArray[2].data
- output.STORAGE_A_STORAGEID_ARRAY_2 = state.accountA.storageArray[2].storageID
- output.STORAGE_A_GASFEE_ARRAY_2 = state.accountA.storageArray[2].gasFee
- output.STORAGE_A_CANCELLED_ARRAY_2 = state.accountA.storageArray[2].cancelled
- output.STORAGE_A_FORWARD_ARRAY_2 = state.accountA.storageArray[2].forward

- output.STORAGE_B_ADDRESS] = 0
- output.STORAGE_B_TOKENSID = state.accountB.storage.tokenSID
- output.STORAGE_B_TOKENBID = state.accountB.storage.tokenBID
- output.STORAGE_B_DATA = state.accountB.storage.data
- output.STORAGE_B_STORAGEID = state.accountB.storage.storageID
- output.TSTORAGE_B_GASFEE = state.accountB.storage.gasFee
- output.STORAGE_B_FORWARD = state.accountB.storage.forward

- output.BALANCE_B_S_ADDRESS = 0
- output.BALANCE_B_S_BALANCE = state.accountB.balanceS.balance
- output.BALANCE_B_B_ADDRESS = 0
- output.BALANCE_B_B_BALANCE = state.accountB.balanceB.balance
- output.BALANCE_B_FEE_ADDRESS = 0
- output.BALANCE_B_FEE_BALANCE = state.accountB.balanceFee.balance

- output.ACCOUNT_B_ADDRESS = 1
- output.ACCOUNT_B_OWNER = state.accountB.account.owner
- output.ACCOUNT_B_PUBKEY_X = state.accountB.account.publicKeyX
- output.ACCOUNT_B_PUBKEY_Y = state.accountB.account.publicKeyY
- output.ACCOUNT_B_NONCE = state.accountB.account.nonce

- output.STORAGE_B_ADDRESS_ARRAY_0 = 0
- output.STORAGE_B_TOKENSID_ARRAY_0 = state.accountB.storageArray[0].tokenSID
- output.STORAGE_B_TOKENBID_ARRAY_0 = state.accountB.storageArray[0].tokenBID
- output.STORAGE_B_DATA_ARRAY_0 = state.accountB.storageArray[0].data
- output.STORAGE_B_STORAGEID_ARRAY_0 = state.accountB.storageArray[0].storageID
- output.STORAGE_B_GASFEE_ARRAY_0 = state.accountB.storageArray[0].gasFee
- output.STORAGE_B_CANCELLED_ARRAY_0 = state.accountB.storageArray[0].cancelled
- output.STORAGE_B_FORWARD_ARRAY_0 = state.accountB.storageArray[0].forward

- output.BALANCE_C_S_ADDRESS = 0
- output.BALANCE_C_S_BALANCE = state.accountC.balanceS.balance
- output.BALANCE_C_B_ADDRESS = 0
- output.BALANCE_C_B_BALANCE = state.accountC.balanceB.balance
- output.BALANCE_C_FEE_ADDRESS = 0
- output.BALANCE_C_FEE_BALANCE = state.accountC.balanceFee.balance

- output.ACCOUNT_C_ADDRESS = 1
- output.ACCOUNT_C_OWNER = state.accountC.account.owner
- output.ACCOUNT_C_PUBKEY_X = state.accountC.account.publicKeyX
- output.ACCOUNT_C_PUBKEY_Y = state.accountC.account.publicKeyY
- output.ACCOUNT_C_NONCE = state.accountC.account.nonce

- output.STORAGE_C_ADDRESS_ARRAY_0 = 0
- output.STORAGE_C_TOKENSID_ARRAY_0 = state.accountC.storageArray[0].tokenSID
- output.STORAGE_C_TOKENBID_ARRAY_0 = state.accountC.storageArray[0].tokenBID
- output.STORAGE_C_DATA_ARRAY_0 = state.accountC.storageArray[0].data
- output.STORAGE_C_STORAGEID_ARRAY_0 = state.accountC.storageArray[0].storageID
- output.STORAGE_C_GASFEE_ARRAY_0 = state.accountC.storageArray[0].gasFee
- output.STORAGE_C_CANCELLED_ARRAY_0 = state.accountC.storageArray[0].cancelled
- output.STORAGE_C_FORWARD_ARRAY_0 = state.accountC.storageArray[0].forward

- output.BALANCE_D_S_ADDRESS = 0
- output.BALANCE_D_S_BALANCE = state.accountD.balanceS.balance
- output.BALANCE_D_B_ADDRESS = 0
- output.BALANCE_D_B_BALANCE = state.accountD.balanceB.balance
- output.BALANCE_D_FEE_ADDRESS = 0
- output.BALANCE_D_FEE_BALANCE = state.accountD.balanceFee.balance

- output.ACCOUNT_D_ADDRESS = 1
- output.ACCOUNT_D_OWNER = state.accountD.account.owner
- output.ACCOUNT_D_PUBKEY_X = state.accountD.account.publicKeyX
- output.ACCOUNT_D_PUBKEY_Y = state.accountD.account.publicKeyY
- output.ACCOUNT_D_NONCE = state.accountD.account.nonce

- output.STORAGE_D_ADDRESS_ARRAY_0 = 0
- output.STORAGE_D_TOKENSID_ARRAY_0 = state.accountD.storageArray[0].tokenSID
- output.STORAGE_D_TOKENBID_ARRAY_0 = state.accountD.storageArray[0].tokenBID
- output.STORAGE_D_DATA_ARRAY_0 = state.accountD.storageArray[0].data
- output.STORAGE_D_STORAGEID_ARRAY_0 = state.accountD.storageArray[0].storageID
- output.STORAGE_D_GASFEE_ARRAY_0 = state.accountD.storageArray[0].gasFee
- output.STORAGE_D_CANCELLED_ARRAY_0 = state.accountD.storageArray[0].cancelled
- output.STORAGE_D_FORWARD_ARRAY_0 = state.accountD.storageArray[0].forward

- output.BALANCE_E_S_ADDRESS = 0
- output.BALANCE_E_S_BALANCE = state.accountE.balanceS.balance
- output.BALANCE_E_B_ADDRESS = 0
- output.BALANCE_E_B_BALANCE = state.accountE.balanceB.balance
- output.BALANCE_E_FEE_ADDRESS = 0
- output.BALANCE_E_FEE_BALANCE = state.accountE.balanceFee.balance

- output.ACCOUNT_E_ADDRESS = 1
- output.ACCOUNT_E_OWNER = state.accountE.account.owner
- output.ACCOUNT_E_PUBKEY_X = state.accountE.account.publicKeyX
- output.ACCOUNT_E_PUBKEY_Y = state.accountE.account.publicKeyY
- output.ACCOUNT_E_NONCE = state.accountE.account.nonce

- output.STORAGE_E_ADDRESS_ARRAY_0 = 0
- output.STORAGE_E_TOKENSID_ARRAY_0 = state.accountE.storageArray[0].tokenSID
- output.STORAGE_E_TOKENBID_ARRAY_0 = state.accountE.storageArray[0].tokenBID
- output.STORAGE_E_DATA_ARRAY_0 = state.accountE.storageArray[0].data
- output.STORAGE_E_STORAGEID_ARRAY_0 = state.accountE.storageArray[0].storageID
- output.STORAGE_E_GASFEE_ARRAY_0 = state.accountE.storageArray[0].gasFee
- output.STORAGE_E_CANCELLED_ARRAY_0 = state.accountE.storageArray[0].cancelled
- output.STORAGE_E_FORWARD_ARRAY_0 = state.accountE.storageArray[0].forward

- output.BALANCE_F_S_ADDRESS = 0
- output.BALANCE_F_S_BALANCE = state.accountF.balanceS.balance
- output.BALANCE_F_B_ADDRESS = 0
- output.BALANCE_F_B_BALANCE = state.accountF.balanceB.balance
- output.BALANCE_F_FEE_ADDRESS = 0
- output.BALANCE_F_FEE_BALANCE = state.accountF.balanceFee.balance

- output.ACCOUNT_F_ADDRESS = 1
- output.ACCOUNT_F_OWNER = state.accountF.account.owner
- output.ACCOUNT_F_PUBKEY_X = state.accountF.account.publicKeyX
- output.ACCOUNT_F_PUBKEY_Y = state.accountF.account.publicKeyY
- output.ACCOUNT_F_NONCE = state.accountF.account.nonce

- output.STORAGE_F_ADDRESS_ARRAY_0 = 0
- output.STORAGE_F_TOKENSID_ARRAY_0 = state.accountF.storageArray[0].tokenSID
- output.STORAGE_F_TOKENBID_ARRAY_0 = state.accountF.storageArray[0].tokenBID
- output.STORAGE_F_DATA_ARRAY_0 = state.accountF.storageArray[0].data
- output.STORAGE_F_STORAGEID_ARRAY_0 = state.accountF.storageArray[0].storageID
- output.STORAGE_F_GASFEE_ARRAY_0 = state.accountF.storageArray[0].gasFee
- output.STORAGE_F_CANCELLED_ARRAY_0 = state.accountF.storageArray[0].cancelled
- output.STORAGE_F_FORWARD_ARRAY_0 = state.accountF.storageArray[0].forward

- output.BALANCE_O_A_ADDRESS = 0
- output.BALANCE_O_D_ADDRESS = 0
- output.BALANCE_O_C_ADDRESS = 0
- output.BALANCE_O_D_ADDRESS = 0

- output.BALANCE_O_A_BALANCE = state.operator.balanceA.balance
- output.BALANCE_O_B_BALANCE = state.operator.balanceB.balance
- output.BALANCE_O_C_BALANCE = state.operator.balanceC.balance
- output.BALANCE_O_D_BALANCE = state.operator.balanceD.balance

- output.HASH_A = 0
- output.PUBKEY_X_A = state.accountA.account.publicKeyX
- output.PUBKEY_Y_A = state.accountA.account.publicKeyY
- output.SIGNATURE_REQUIRED_A = 1

- output.HASH_B = 0
- output.PUBKEY_X_B = state.accountB.account.publicKeyX
- output.PUBKEY_Y_B = state.accountB.account.publicKeyY
- output.SIGNATURE_REQUIRED_B = 1

- output.HASH_A_ARRAY = VariableArrayT(ORDER_SIZE_USER_A - 1, state.constants._0)
- output.PUBKEY_X_A_ARRAY = VariableArrayT(ORDER_SIZE_USER_A - 1, state.accountA.account.publicKey.x)
- output.PUBKEY_Y_A_ARRAY = VariableArrayT(ORDER_SIZE_USER_A - 1, state.accountA.account.publicKey.y)
- output.SIGNATURE_REQUIRED_A_ARRAY = VariableArrayT(ORDER_SIZE_USER_A - 1, state.constants._0)

- output.HASH_B_ARRAY = VariableArrayT(ORDER_SIZE_USER_B - 1, state.constants._0)
- output.PUBKEY_X_B_ARRAY = VariableArrayT(ORDER_SIZE_USER_B - 1, state.accountA.account.publicKey.x)
- output.PUBKEY_Y_B_ARRAY = VariableArrayT(ORDER_SIZE_USER_B - 1, state.accountA.account.publicKey.y)
- output.SIGNATURE_REQUIRED_B_ARRAY = VariableArrayT(ORDER_SIZE_USER_B - 1, state.constants._0)

- output.HASH_C_ARRAY = VariableArrayT(ORDER_SIZE_USER_C - 1, state.constants._0)
- output.PUBKEY_X_C_ARRAY = VariableArrayT(ORDER_SIZE_USER_C - 1, state.accountA.account.publicKey.x)
- output.PUBKEY_Y_C_ARRAY = VariableArrayT(ORDER_SIZE_USER_C - 1, state.accountA.account.publicKey.y)
- output.SIGNATURE_REQUIRED_C_ARRAY = VariableArrayT(ORDER_SIZE_USER_C - 1, state.constants._0)

- output.HASH_D_ARRAY = VariableArrayT(ORDER_SIZE_USER_D - 1, state.constants._0)
- output.PUBKEY_X_D_ARRAY = VariableArrayT(ORDER_SIZE_USER_D - 1, state.accountA.account.publicKey.x)
- output.PUBKEY_Y_D_ARRAY = VariableArrayT(ORDER_SIZE_USER_D - 1, state.accountA.account.publicKey.y)
- output.SIGNATURE_REQUIRED_D_ARRAY = VariableArrayT(ORDER_SIZE_USER_D - 1, state.constants._0)

- output.HASH_E_ARRAY = VariableArrayT(ORDER_SIZE_USER_E - 1, state.constants._0)
- output.PUBKEY_X_E_ARRAY = VariableArrayT(ORDER_SIZE_USER_E - 1, state.accountA.account.publicKey.x)
- output.PUBKEY_Y_E_ARRAY = VariableArrayT(ORDER_SIZE_USER_E - 1, state.accountA.account.publicKey.y)
- output.SIGNATURE_REQUIRED_E_ARRAY = VariableArrayT(ORDER_SIZE_USER_E - 1, state.constants._0)

- output.HASH_F_ARRAY = VariableArrayT(ORDER_SIZE_USER_F - 1, state.constants._0)
- output.PUBKEY_X_F_ARRAY = VariableArrayT(ORDER_SIZE_USER_F - 1, state.accountA.account.publicKey.x)
- output.PUBKEY_Y_F_ARRAY = VariableArrayT(ORDER_SIZE_USER_F - 1, state.accountA.account.publicKey.y)
- output.SIGNATURE_REQUIRED_F_ARRAY = VariableArrayT(ORDER_SIZE_USER_F - 1, state.constants._0)

- output.NUM_CONDITIONAL_TXS = state.numConditionalTransactions
- output.DA = 0

### Description

Simply sets the default values for a transaction output. If a transaction doesn't explicitly set an output, these values are used.

# Math

## DualVariableGadget

This gadget is a simple wrapper around `libsnark::dual_variable_gadget`.

The gadget is used in two different ways:

- To ensure a value matches its bit representation using a specified number of bits
- As a range check: value < 2^n with n the number of bits

## DynamicBalanceGadget and DynamicVariableGadget

This gadget contains a stack of `VariableT` variables.

The gadget is used to make writing circuits easier. A `VariableT` can only have a single value at all times, so using this to represent a mutable value isn't possible.

A single instance of a DynamicVariableGadget can be created which internally contains a list of `VariableT` members. When the value needs to be updated a new `VariableT` is pushed on top of the stack. This way using the latest value is just looking at the `VariableT` at the top of the list.

## UnsafeSub statement

A valid instance of an UnsafeSub statement assures that given an input of:

- value: F
- sub: F

the prover knows an auxiliary input:

- result: F

such that the following conditions hold:

- result = value - sub

Notes:

- Does _not_ check for underflow, not because any circuit depends on underflow, but because this gadget should only be used in cases where it should never happen.

## UnsafeAdd statement

A valid instance of an UnsafeAdd statement assures that given an input of:

- value: F
- add: F

the prover knows an auxiliary input:

- result: F

such that the following conditions hold:

- result = value + add

Notes:

- Does _not_ check for overflow, not because any circuit depends on overflow, but because this gadget should only be used in cases where it should never happen.

## UnsafeMul statement

A valid instance of an UnsafeMul statement assures that given an input of:

- valueA: F
- valueB: F

the prover knows an auxiliary input:

- result: F

such that the following conditions hold:

- result = valueA \* valueB

Notes:

- Does _not_ check for overflow, not because any circuit depends on overflow, but because this gadget should only be used in cases where it should never happen.

## Add statement

A valid instance of an Add statement assures that given an input of:

- A: {0..2^n}
- B: {0..2^n}

with circuit parameters:

- n: unsigned int

the prover knows an auxiliary input:

- result: {0..2^n}

such that the following conditions hold:

- result = UnsafeAdd(A, B)
- result < 2^n

Notes:

- Calculates A + B with overflow checking
- A and B are limited to n + 1 <= NUM_BITS_FIELD_CAPACITY, so we can be sure to detect overflow with a simple range check on the result.

## Sub statement

A valid instance of a Sub statement assures that given an input of:

- A: {0..2^n}
- B: {0..2^n}

with circuit parameters:

- n: unsigned int

the prover knows an auxiliary input:

- result: {0..2^n}

such that the following conditions hold:

- result = UnsafeSub(A, B)
- result < 2^n && result >= 0

Notes:

- Calculates A - B with underflow checking
- A and B are limited to n + 1 <= NUM_BITS_FIELD_CAPACITY, so we can be sure to detect underflow with a simple range check on the result.
- Underflow check is thus detected when the result is a value taking up more than n bits.

## SafeMul statement

A valid instance of an SafeMul statement assures that given an input of:

- valueA: {0..2^n}
- valueB: {0..2^n}

with circuit parameters:

- n: unsigned int

the prover knows an auxiliary input:

- result: {0..2^n}

such that the following conditions hold:

- result = valueA \* valueB
- result < 2^n && result >= 0

## Transfer statement

A valid instance of a Transfer statement assures that given an input of:

- from: DynamicVariableGadget
- to: DynamicVariableGadget
- value: F

the prover knows an auxiliary input:

-

such that the following conditions hold:

- from = Sub(from, value, NUM_BITS_AMOUNT)
- to = Add(to, value, NUM_BITS_AMOUNT)

### Description

Simple reusable gadget to transfer a certain amount of funds from one balance to another.

## Ternary statement

A valid instance of a Ternary statement assures that given an input of:

- b: {0..2}
- x: F
- y: F

with circuit parameters:

- enforceBitness: bool

the prover knows an auxiliary input:

- result: F

such that the following conditions hold:

- result = (b == 1) ? x : y
- if enforceBitness then generate_boolean_r1cs_constraint(b)

Notes:

- Implements the ternary operator.
- Constraint logic from https://github.com/daira/r1cs/blob/master/zkproofs.pdf.

## ArrayTernary statement

A valid instance of an ArrayTernary statement assures that given an input of:

- b: {0..2}
- x: F[N]
- y: F[N]

with circuit parameters:

- enforceBitness: bool

the prover knows an auxiliary input:

- result: F[N]

such that the following conditions hold:

- for i in {0..N}: result[i] = (b == 1) ? x[i] : y[i]
- if enforceBitness then generate_boolean_r1cs_constraint(b)

Notes:

- Implements the ternary operator.
- Constraint logic from https://github.com/daira/r1cs/blob/master/zkproofs.pdf.

## And statement

A valid instance of an And statement assures that given an input of:

- inputs: {0..2}[N]

the prover knows an auxiliary input:

- result: {0..2}

such that the following conditions hold:

- result = inputs[0] && inputs[1] && ... && inputs[N-1]

Notes:

- All inputs are expected to be boolean
- Implements the AND operator. Constraint logic from https://github.com/daira/r1cs/blob/master/zkproofs.pdf.

## Or statement

A valid instance of an Or statement assures that given an input of:

- inputs: {0..2}[N]

the prover knows an auxiliary input:

- result: {0..2}

such that the following conditions hold:

- result = inputs[0] || inputs[1] || ... || inputs[N-1]

Notes:

- All inputs are expected to be boolean
- Implements the OR operator. Constraint logic from https://github.com/daira/r1cs/blob/master/zkproofs.pdf

## Not statement

A valid instance of a Not statement assures that given an input of:

- A: {0..2}

with circuit parameters:

- enforceBitness: bool

the prover knows an auxiliary input:

- result: {0..2}

such that the following conditions hold:

- result = 1 - A
- if enforceBitness then generate_boolean_r1cs_constraint(b)

Notes:

- Implements the NOT operator.
- Constraint logic from https://github.com/daira/r1cs/blob/master/zkproofs.pdf

## XorArray statement

A valid instance of an XorArray statement assures that given an input of:

- A: {0..2}[N]
- B: {0..2}[N]

the prover knows an auxiliary input:

- result: {0..2}[N]

such that the following conditions hold:

- for i in {0..N}: result[i] = A[i] ^ B[i]

Notes:

- All inputs are expected to be boolean
- Implements the XOR operator.
- Constraint logic from https://github.com/daira/r1cs/blob/master/zkproofs.pdf

## Equal statement

A valid instance of an Equal statement assures that given an input of:

- A: F
- B: F

the prover knows an auxiliary input:

- result: {0..2}

such that the following conditions hold:

- result = (A - B == 0) ? 1 : 0

Notes:

- Checks for equality between any two field elements

## RequireEqual statement

A valid instance of a RequireEqual statement assures that given an input of:

- A: F
- B: F

the prover knows an auxiliary input:

-

such that the following conditions hold:

- A == B

Notes:

- Enforces equality between any two field elements

## RequireZeroAorB statement

A valid instance of a RequireZeroAorB statement assures that given an input of:

- A: F
- B: F

the prover knows an auxiliary input:

-

such that the following conditions hold:

- A \* B = 0

Notes:

- Checks (A == 0) || (B == 0).
- Constraint logic from https://github.com/daira/r1cs/blob/master/zkproofs.pdf.

## RequireNotZero statement

A valid instance of a RequireNotZero statement assures that given an input of:

- A: F

the prover knows an auxiliary input:

- inv: F

such that the following conditions hold:

- inv = 1/A
- A \* inv = 1

Notes:

- Enforces A != 0 by using the trick that the inverse exists for all values except 0.
- Constraint logic from https://github.com/daira/r1cs/blob/master/zkproofs.pdf

## RequireNotEqual statement

A valid instance of a RequireNotEqual statement assures that given an input of:

- A: F
- B: F

the prover knows an auxiliary input:

-

such that the following conditions hold:

- A - B != 0

Notes:

- Enforces inequality between any two field elements

## LeqGadget

This gadget is a wrapper around `libsnark::comparison_gadget`, exposing `<`, `<=`, `==`, `>=` and `>` for simplicity (and sometimes efficiensy if the same comparison result can be reused e.g. when both `<` and `<=` are needed).

One important limitation of `libsnark::comparison_gadget` is that it does not work for values close to the max field element value. This is an implementation detail as the gadget depends on there being an extra bit at MSB of the valules to be available. As the max field element is ~254 bits, only 253 bits can be used. And because the implementation needs an extra bit we can only compare values that take up at most 252 bits.

This is _not_ checked in the gadget itself, and it depends on the caller to specify a valid `n` which needs to be the max number of bits of the value passed into the gadget.

## LtField statement

A valid instance of a LtField statement assures that given an input of:

- A: F
- B: F

the prover knows an auxiliary input:

- result: {0..2}

such that the following conditions hold:

- result = A[NUM_BITS_MAX_VALUE/2..NUM_BITS_MAX_VALUE] == B[NUM_BITS_MAX_VALUE/2..NUM_BITS_MAX_VALUE] ?
  A[0..NUM_BITS_MAX_VALUE/2] < B[0..NUM_BITS_MAX_VALUE] :
  A[NUM_BITS_MAX_VALUE/2..NUM_BITS_MAX_VALUE] < B[NUM_BITS_MAX_VALUE/2..NUM_BITS_MAX_VALUE]

Notes:

- Calculates A < B
- Unlike LeqGadget, this works for ALL field element values
- Because LeqGadget does not work for certain very large values (values taking up more than 252 bits), we split up the values in two smaller values and do the comparison like that.

## Min statement

A valid instance of a Min statement assures that given an input of:

- A: {0..2^n}
- B: {0..2^n}

with circuit parameters:

- n: unsigned int

the prover knows an auxiliary input:

- result: {0..2^n}

such that the following conditions hold:

- (A < B) ? A : B

Notes:

- Implements the common min operation.

## Max statement

A valid instance of a Max statement assures that given an input of:

- A: {0..2^n}
- B: {0..2^n}

with circuit parameters:

- n: unsigned int

the prover knows an auxiliary input:

- result: {0..2^n}

such that the following conditions hold:

- (A < B) ? B : A

Notes:

- Implements the common max operation.

## RequireLeq statement

A valid instance of a RequireLeq statement assures that given an input of:

- A: {0..2^n}
- B: {0..2^n}

with circuit parameters:

- n: unsigned int

the prover knows an auxiliary input:

-

such that the following conditions hold:

- A <= B

Notes:

- Enforces A <= B.

## RequireLt statement

A valid instance of a RequireLt statement assures that given an input of:

- A: {0..2^n}
- B: {0..2^n}

with circuit parameters:

- n: unsigned int

the prover knows an auxiliary input:

-

such that the following conditions hold:

- A < B

Notes:

- Enforces A < B.

## IfThenRequire statement

A valid instance of an IfThenRequire statement assures that given an input of:

- C: {0..2}
- A: {0..2}

the prover knows an auxiliary input:

-

such that the following conditions hold:

- !C || A

Notes:

- Enforces !C || A.

## IfThenRequireEqual statement

A valid instance of an IfThenRequireEqual statement assures that given an input of:

- C: {0..2}
- A: F
- B: F

the prover knows an auxiliary input:

-

such that the following conditions hold:

- IfThenRequire(C, (A == B) ? 1 : 0)

Notes:

- Enforces !C || (A == B).

## IfThenRequireNotEqual statement

A valid instance of an IfThenRequireNotEqual statement assures that given an input of:

- C: {0..2}
- A: F
- B: F

the prover knows an auxiliary input:

-

such that the following conditions hold:

- IfThenRequire(C, (A != B) ? 1 : 0)

Notes:

- Enforces !C || (A != B).

## MulDivGadget statement

A valid instance of a MulDivGadget statement assures that given an input of:

- value: {0..2^numBitsValue}
- numerator: {0..2^numBitsNumerator}
- denominator: {0..2^numBitsDenominator}

the prover knows an auxiliary input:

- quotient: F
- remainder: {0..2^numBitsDenominator}

such that the following conditions hold:

- denominator != 0
- remainder < 2^numBitsDenominator (range check)
- remainder < denominator
- value \* numerator = denominator \* quotient + remainder

Notes:

- Calculates floor((value \* denominator) / denominator)

## RequireAccuracy statement

A valid instance of a RequireAccuracy statement assures that the prover knows the auxiliary inputs of:

- value: F
- original: F

with circuit parameters:

- accuracy: Accuracy
- maxNumBits: unsigned int

the prover knows an auxiliary input:

-

such that the following conditions hold:

- value < 2^maxNumBits (range check)
- value <= original (RequireLeqGadget)
- original \* accuracy.N <= value \* accuracy.D (RequireLeqGadget)

Notes:

- value is first range checked to ensure value can be used in LeqGadget

### Description

This is a simple gadget that ensures the valid specified are approximately the same. The main reason this gadget is used is because the amounts that are compressed by using decimal floats can have small rounding errors.

We always force the value to be smaller (or equal) to the original value. This is to ensure that we never spend more than the user either expected or even has.

## PublicData statement

A valid instance of a PublicData statement assures that given an input of:

- data: bits[N]

the prover knows an auxiliary input:

- publicInput: F

such that the following conditions hold:

- publicInput = sha256(data) >> 3

### Description

Hashes all public data to a single field element, which is much more efficient on-chain to verify the proof.

3 LBS are stripped from the 256-bit hash so that the packed value always fits inside a single field element (NUM_BITS_FIELD_CAPACITY).

sha256 is used here because we also need to hash the data onchain. sha256 is very cheap to calculate onchain but quite expensive to calculate in the circuits, while something like Poseidon is extremely expensive to calculate onchain, but cheap to calculate in the circuits. Because we aim for scalability, we want onchain costs to be as low as possible, while proving costs are pretty cheap regardless.

## Float statement

A valid instance of a Float statement assures that given an input of:

- floatValue_bits: {0..2^(numBitsExponent+numBitsMantissa)}

the prover knows an auxiliary input:

- decodedValue: F

The following conditions hold:

- decodedValue = floatValue[0..numBitsMantissa[ \* (10^floatValue[numBitsMantissa, numBitsExponent+numBitsMantissa[)

### Description

Floats are used to reduce the amount of data we have to put on-chain for amounts. We can only decode floats in the circuits, we never encode floats (which is a heavier operation normally).

## Selector statement

A valid instance of a Selector statement assures that given an input of:

- type: F

with circuit parameters:

- n: unsigned int

the prover knows an auxiliary input:

- result: {0..2}[n]
- sum: F

The following conditions hold:

- for i in {0..n}: result[i] = (i == type) ? 1 : 0
- for i in {0..n}: sum += result[i]
- sum == 1

### Description

Sets the variable at position type to 1, all other variables are 0

## Select statement

A valid instance of a Select statement assures that given an input of:

- selector: {0..2}[N]
- values: F[N]

the prover knows an auxiliary input:

- result: F

The following conditions hold:

- for i in {0..n}: result = (selector[i] == 1) ? values[i] : result

Notes:

- selector can be assumed to contain exactly a single 1 bit

### Description

Selects the value in values which has a bit set to 1 in selector.

## ArraySelect statement

A valid instance of an ArraySelect statement assures that given an input of:

- selector: {0..2}[N]
- values: F[][n]

the prover knows an auxiliary input:

- result: F[]

The following conditions hold:

- for i in {0..n}: result = (selector[i] == 1) ? values[i] : result

Notes:

- selector can be assumed to contain exactly a single 1 bit

### Description

Selects the value in values which has a bit set to 1 in selector.

## OwnerValid statement

A valid instance of an OwnerValid statement assures that given an input of:

- oldOwner: F
- newOwner: F

such that the following conditions hold:

- (oldOwner == newOwner) || (oldOwner == 0)

### Description

A valid owner is when the account currently has no owner or it matches the current owner of the account.

## SignedAdd statement

A valid instance of a SignedAdd statement assures that given an input of:

- A: SignedF
- B: SignedF

the prover knows an auxiliary input:

- result: SignedF

The following conditions hold:

- result.value = (A.sign == B.sign) ? A.value + B.value : ((A.value < B.value) ? B.value - A.value : A.value - B.value)
- result.sign = result.value == 0 ? 0 : (B.sign == 1 && A.value <= B.value) || (A.sign == 1 && A.value > B.value)

Notes:

- Calculates A + B with overflow/underflow checking with both values being signed field elements.

## SignedSub statement

A valid instance of a SignedSub statement assures that given an input of:

- A: SignedF
- B: SignedF

the prover knows an auxiliary input:

- result: SignedF

The following conditions hold:

- result = SignedAdd(A, -B)

Notes:

- Calculates A - B with overflow/underflow checking with both values being signed field elements.

## SignedMulDiv statement

A valid instance of a SignedMulDiv statement assures that given an input of:

- value: SignedF{0..2^numBitsValue}
- numerator: SignedF{0..2^numBitsNumerator}
- denominator: {0..2^numBitsDenominator}

the prover knows an auxiliary input:

- res: SignedF
- quotient: F
- sign: {0..2}

such that the following conditions hold:

- quotient = MulDiv(value.value, numerator.value, denominator)
- sign = (quotient == 0) ? 0 : ((value.sign == numerator.sign) ? 1 : 0)

Notes:

- Calculates floor((value \* denominator) / denominator) with both the value and the numerator being signed field elements.
- floor rounds always towards 0.

## Power statement

A valid instance of a Power statement assures that given an input of:

- \_x: {0..BASE_FIXED+1}
- y: F

with circuit parameters:

- numIterations: unsigned int

the prover knows an auxiliary input:

- result: {0..2^NUM_BITS_AMOUNT}
- x: F
- sum0: F

such that the following conditions hold:

- x = BASE_FIXED - \_x
- sum[0] = BASE_FIXED \* BASE_FIXED
- sum[1] = sum[0] + (x \* y)
- bn[1] = BASE_FIXED
- xn[1] = x
- cn[1] = y
- for i in {2..numIterations}:
  - bn[i] = bn[i-1] + BASE_FIXED
  - vn[i] = y - bn[i-1]
  - xn[i] = (xn[i-1] \* x) / BASE_FIXED
  - cn[i] = (cn[i-1] \* vn[i]) / bn[i]
  - tn[i] = SignedF((i+1)%2, xn[i]) \* cn[i]
  - sum[i] = sum[i-1] + tn[i]
  - cn[i] < 2^NUM_BITS_AMOUNT
- result = sum[numIterations-1] / BASE_FIXED
- result < 2^NUM_BITS_AMOUNT
- result.sign == 1

Notes:

- \_x will always be in [0, 1] (in fixed point representation)
- Results should never be able to overflow or underflow
- Power approximation formule as found here: https://docs.balancer.finance/protocol/index/approxing

### Description

Calculates [0, 1]\*\*[0, inf) using an approximation. The closer the base is to 1, the higher the accuracy. Values and calculations are done with fixed points.
The result is enforced to be containable in NUM_BITS_AMOUNT bits.
The higher the number of iterations, the higher the accuracy (and the greater the cost).

# Merkle tree

## MerklePathSelector statement

A valid instance of a MerklePathSelector statement assures that given an input of:

- input: F
- sideNodes: F[3]
- bit0: {0..2}
- bit1: {0..2}

the prover knows an auxiliary input:

- children: F[4]

such that the following conditions hold:

- if bit1 == 0 && bit0 == 0: children = [input, sideNodes[0], sideNodes[1], sideNodes[2]]
- if bit1 == 0 && bit0 == 1: children = [sideNodes[0], input, sideNodes[1], sideNodes[2]]
- if bit1 == 1 && bit0 == 0: children = [sideNodes[0], sideNodes[1], input, sideNodes[2]]
- if bit1 == 1 && bit0 == 1: children = [sideNodes[0], sideNodes[1], sideNodes[2], input]

### Description

Using the address bits specified, orders the children as expected in a quad Merkle tree.

## MerklePath statement

A valid instance of a MerklePath statement assures that given an input of:

- address: {0..2^NUM_BITS_ACCOUNT}
- leaf: F
- proof: F[3 * depth]

with circuit parameters:

- depth: unsigned int

the prover knows an auxiliary input:

- result: F
- hashes: F[depth]
- children: F[depth][4]

such that the following conditions hold:

- for i in {0..depth}:
  children[i] = MerklePathSelector(
  (i == 0) ? leaf : hashes[i-1],
  {proof[3*i + 0], proof[3*i + 1], proof[3*i + 2]},
  address[2*i + 0],
  address[2*i + 1]
  )
  hashes[i] = PoseidonHash_t5f6p52(children[i])
- result = hashes[depth-1]

### Description

Calculates the Merkle root in a quad tree along the specified path.

## MerklePathCheck statement

A valid instance of a MerklePathCheck statement assures that given an input of:

- address: {0..2^NUM_BITS_ACCOUNT}
- leaf: F
- root: F
- proof: F[3 * depth]

with circuit parameters:

- depth: unsigned int

the prover knows an auxiliary input:

- expectedRoot: F

such that the following conditions hold:

- expectedRoot = MerklePath(depth, address, leaf, proof)
- root = expectedRoot

### Description

Verifies the Merkle root for the specified path.

## UpdateAccount statement

A valid instance of an UpdateAccount statement assures that given an input of:

- root_before: F
- asset_root_before: F
- address: {0..2^NUM_BITS_ACCOUNT}
- before: Account
- asset_before: Account
- after: Account
- asset_after: Account

the prover knows an auxiliary input:

- root_after: F
- asset_root_after: F
- proof: F[3 * TREE_DEPTH_ACCOUNTS]
- asset_proof: F[3 * TREE_DEPTH_ACCOUNTS]

such that the following conditions hold:

- hash_before = PoseidonHash_t12f6p53(
  before.owner,
  before.publicKeyX,
  before.publicKeyY,
  before.appKeyPublicKeyX,
  before.appKeyPublicKeyY,
  before.nonce,
  before.disableAppKeySpotTrade,
  before.disableAppKeyWithdraw,
  before.disableAppKeyTransferToOther,
  before.balancesRoot,
  before.storageRoot
  )
- hash_after = PoseidonHash_t12f6p53(
  after.owner,
  after.publicKeyX,
  after.publicKeyY,
  after.appKeyPublicKeyX,
  after.appKeyPublicKeyY,
  after.nonce,
  after.disableAppKeySpotTrade,
  after.disableAppKeyWithdraw,
  after.disableAppKeyTransferToOther,
  after.balancesRoot,
  after.storageRoot
  )
- asset_hash_before = PoseidonHash_t6f6p52(
  before.owner,
  before.publicKeyX,
  before.publicKeyY,
  before.nonce,
  before.balancesRoot
  )
- asset_hash_after = PoseidonHash_t6f6p52(
  after.owner,
  after.publicKeyX,
  after.publicKeyY,
  after.nonce,
  after.balancesRoot
  )
- MerklePathCheck(TREE_DEPTH_ACCOUNTS, address, hash_before, root_before, proof)
- root_after = MerklePath(TREE_DEPTH_ACCOUNTS, address, hash_after, proof)
- MerklePathCheck(TREE_DEPTH_ACCOUNTS, address, asset_hash_before, asset_root_before, asset_proof)
- asset_root_after = MerklePath(TREE_DEPTH_ACCOUNTS, address, asset_hash_after, asset_proof)

### Description

Updates an Account leaf in the accounts Merkle tree:

- First check if the data provided for the current state is valid.
- Then calculate the new Merkle root with the new leaf data.

## UpdateBalance statement

A valid instance of an UpdateBalance statement assures that given an input of:

- root_before: F
- address: {0..2^NUM_BITS_TOKEN}
- before: Balance
- after: Balance

the prover knows an auxiliary input:

- root_after: F
- proof: F[3 * TREE_DEPTH_TOKENS]

such that the following conditions hold:

- hash_before = PoseidonHash_t5f6p52(
  before.balance
  )
- hash_after = PoseidonHash_t5f6p52(
  after.balance
  )
- MerklePathCheck(TREE_DEPTH_TOKENS, address, hash_before, root_before, proof)
- root_after = MerklePath(TREE_DEPTH_TOKENS, address, hash_after, proof)

Notes:

- Even though the leaf has only three values, we still use PoseidonHash_t5f6p52 (which hashes up to 4 inputs) so we need less Poseidon implementations in our smart contracts.

### Description

Updates an Balance leaf in the balances Merkle tree:

- First check if the data provided for the current state is valid.
- Then calculate the new Merkle root with the new leaf data.

## UpdateStorage statement

A valid instance of an UpdateStorage statement assures that given an input of:

- root_before: F
- address: {0..2^NUM_BITS_STORAGE_ADDRESS}
- before: Storage
- after: Storage

the prover knows an auxiliary input:

- root_after: F
- proof: F[3 * TREE_DEPTH_STORAGE]

such that the following conditions hold:

- hash_before = PoseidonHash_t8f6p53(
  before.tokenSID,
  before.tokenBID,
  before.data,
  before.storageID,
  before.gasFee,
  before.cancelled,
  before.forward
  )
- hash_after = PoseidonHash_t8f6p53(
  after.tokenSID,
  after.tokenBID,
  after.data,
  after.storageID,
  after.gasFee,
  after.cancelled,
  after.forward
  )
- MerklePathCheck(TREE_DEPTH_STORAGE, address, hash_before, root_before, proof)
- root_after = MerklePath(TREE_DEPTH_STORAGE, address, hash_after, proof)

Notes:

- Even though the leaf has only two values, we still use PoseidonHash_t8f6p53 (which hashes up to 4 inputs) so we need less Poseidon implementations in our smart contracts.

### Description

Updates an Storage leaf in the storage Merkle tree:

- First check if the data provided for the current state is valid.
- Then calculate the new Merkle root with the new leaf data.

## StorageReader statement

A valid instance of a StorageReader statement assures that given an input of:

- storage: Storage
- storageID: {0..2^NUM_BITS_STORAGEID}
- verify: {0..2}

the prover knows an auxiliary input:

- data: F

such that the following conditions hold:

- if verify == 1 then storageID >= storage.storageID
- tokenSID = (storageID == storage.storageID) ? storage.tokenSID : 0
- tokenBID = (storageID == storage.storageID) ? storage.tokenBID : 0
- data = (storageID == storage.storageID) ? storage.data : 0
- gasFee = (storageID == storage.storageID) ? storage.gasFee : 0
- cancelled = (storageID == storage.storageID) ? storage.cancelled : 0
- forward = (storageID == storage.storageID) ? storage.forward : 1

### Description

Reads data at storageID in the storage tree of the account, but allows the data to be overwritten by increasing the storageID in delta's of 2^TREE_DEPTH_STORAGE and reading the tree at storageID % 2^TREE_DEPTH_STORAGE.

## Nonce statement

A valid instance of a Nonce statement assures that given an input of:

- storage: Storage
- storageID: {0..2^NUM_BITS_STORAGEID}
- verify: {0..2}

the prover knows an auxiliary input:

- tokenSID: F
- tokenBID: F
- data: F
- gasFee: F
- cancelled: F
- forward: F

such that the following conditions hold:

- data = StorageReader(storage, storageID, verify)
- if verify == 1 then tokenSID == 0
- if verify == 1 then tokenBID == 0
- if verify == 1 then data == 0
- if verify == 1 then gasFee == 0
- if verify == 1 then cancelled == 0
- if verify == 1 then forward == 1

### Description

Builds a simple parallel nonce system on top of the storage tree. Transactions can use any storage slot that contains 0 as data (after overwriting logic). A 1 will be written to the storage after the transaction is used, making it impossible to re-use the transaction multiple times.

To make is easier to ignore this check, verify is added to make the statement always valid if necessary.

# Signature

## CompressPublicKey statement

A valid instance of a CompressPublicKey statement assures that given an input of:

- publicKeyX: F
- publicKeyY: F

the prover knows an auxiliary input:

- compressedPublicKey_bits: {0..2^256}

The following conditions hold:

If publicKeyY != 0:

- publicKeyY = compressedPublicKey_bits[0..254[
- compressedPublicKey_bits[254] = 0
- publicKeyX = (compressedPublicKey_bits[255] == 1 ? -1 : 1) \* sqrt((y\*y - 1) / ((JubJub.D \* y\*y) - JubJub.A)

If publicKeyY == 0:

- compressedPublicKey_bits[0..256[ = 0

Notes:

- sqrt always needs to return the positive root, which is defined by root < 0 - root. Otherwise the prover can supply either the negative root or the positive root as a valid result of sqrt when the constraint is defined as x == y \* y == -y \* -y.
- A special case is to allow publicKeyX == publicKeyY == 0, which isn't a valid point. This allows disabling the ability to sign with EdDSA with the account).

### Description

Compresses a point on the curve (two field elements) to a single field element (254 bits) + 1 bit. See https://ed25519.cr.yp.to/eddsa-20150704.pdf for the mathematical background.

## EdDSA_HashRAM_Poseidon statement

A valid instance of an EdDSA_HashRAM_Poseidon statement assures that given an input of:

- rX: F
- rY: F
- aX: F
- aY: F
- message: F

the prover knows an auxiliary input:

- hash: F

The following conditions hold:

- hash_bits = hash_packed
- hash = PoseidonHash_t6f6p52(
  rX,
  rY,
  aX,
  aY,
  message
  )

Notes:

- Based on `EdDSA_HashRAM_gadget` in ethsnarks (https://github.com/yueawang/ethsnarks/blob/042ad35a8a67a1844e51eac441b310371eba1fe8/src/jubjub/eddsa.cpp#L11), modified to use Poseidon.

### Description

For use in EdDSA signatures. Hashes the message together with the public key and the signature R point.

## EdDSA_Poseidon statement

A valid instance of an EdDSA_Poseidon statement assures that given an input of:

- aX: F
- aY: F
- rX: F
- rY: F
- s: F[]
- message: F

the prover knows an auxiliary input:

- result: {0..2}
- hash: F
- hashRam: F[]
- atX: F
- atY: F

The following conditions hold:

- PointValidator(aX, aY)
- hashRAM = EdDSA_HashRAM_Poseidon(rX, rY, aX, aY, message)
- (atX, atY) = ScalarMult(aX, aY, hashRAM)
- result = (fixed_base_mul(s) == PointAdder(rX, rY, atX, atY))

Notes:

- Based on `PureEdDSA` in ethsnarks (https://github.com/yueawang/ethsnarks/blob/042ad35a8a67a1844e51eac441b310371eba1fe8/src/jubjub/eddsa.cpp#L63), modified to use Poseidon.

# Matching

## Order statement

A valid instance of an Order statement assures that given an input of:

- exchange: {0..2^NUM_BITS_ADDRESS}
- storageID: {0..2^NUM_BITS_STORAGEID}
- accountID: {0..2^NUM_BITS_ACCOUNT}
- tokenS: {0..2^NUM_BITS_TOKEN}
- tokenB: {0..2^NUM_BITS_TOKEN}
- amountS: {0..2^NUM_BITS_AMOUNT}
- amountB: {0..2^NUM_BITS_AMOUNT}
- validUntil: {0..2^NUM_BITS_TIMESTAMP}
- fee: {0..2^NUM_BITS_FEE_BIPS}
- maxFee: {0..2^NUM_BITS_FEE_BIPS}
- fillAmountBorS: {0..2}
- taker: F
- feeTokenID: {0..2^NUM_BITS_TOKEN}
- feeBips: {0..2^NUM_BITS_FEE_BIPS}
- tradingFee: {0..2^NUM_BITS_AMOUNT}
- type: {0..2^NUM_BITS_TYPE}
- gridOffset: {0..2^NUM_BITS_AMOUNT}
- orderOffset: {0..2^NUM_BITS_AMOUNT}
- maxLevel: {0..2^NUM_BITS_AUTOMARKET_LEVEL}
- useAppKey: {0..2^NUM_BITS_BYTE}

the prover knows an auxiliary input:

- hash: F

such that the following conditions hold:

- storageID_bits = storageID_packed
- accountID_bits = accountID_packed
- tokenS_bits = tokenS_packed
- tokenB_bits = tokenB_packed
- amountS_bits = amountS_packed
- amountB_bits = amountB_packed
- fee_bits = fee_packed
- trading_fee = tradingFee_packed
- maxFee_bits = maxFee_packed
- feeBips_bits = feeBips_packed
- validUntil_bits = validUntil_packed
- fillAmountBorS_bits = fillAmountBorS_packed
- feeTokenID_bits = feeTokenID_packed
- type_bits = type_packed
- gridOffset_bits = gridOffset_packed
- orderOffset_bits = orderOffset_packed
- maxLevel_bits = maxLevel_packed

- hash = PoseidonHash_t18f6p53(
  exchange,
  storageID,
  accountID,
  tokenS,
  tokenB,
  amountS,
  amountB,
  validUntil,
  maxFeeBips,
  fillAmountBorS,
  taker,
  feeTokenID,
  type,
  gridOffset,
  orderOffset,
  maxLevel,
  useAppKey
  )
- feeBips <= protocolFeeBips (RequireLeqGadget)
- tradingFee <= calculateTradingFee (RequireLeqGadget)
- fee <= maxFee (RequireLeqGadget)
- tokenS != tokenB
- amountS != 0
- amountB != 0

### Description

Bundles the order data and does some basic order data validation. Also calculates the order hash.

We do not allow the token bought to be equal to the token sold.

The operator can choose any fee lower or equal than the maxFeeBips specified by the user.

## RequireFillRate statement

A valid instance of a RequireFillRate statement assures that given an input of:

- amountS: {0..2^NUM_BITS_AMOUNT}
- amountB: {0..2^NUM_BITS_AMOUNT}
- fillAmountS: {0..2^NUM_BITS_AMOUNT}
- fillAmountB: {0..2^NUM_BITS_AMOUNT}

the prover knows an auxiliary input:

-

such that the following conditions hold:

- (fillAmountS \* amountB \* 1000) <= (fillAmountB \* amountS \* 1001) (RequireLeqGadget)
- (fillAmountS == 0 && fillAmountB == 0) || (fillAmountS != 0 && fillAmountB != 0)

### Description

The fill rate can be up to 0.1% higher than the max fill rate defined in the order to be more lenient to rounding errors.

The additional requirement for the fill amounts is to make sure rounding errors don't make it possible to only do a token transfer in a single direction (only receiving tokens or only sending tokens). This could allow an order to be used to drain an account as is could be used an unlimited number of times.

## FeeCalculator statement

A valid instance of a FeeCalculator statement assures that given an input of:

- amount: {0..2^NUM_BITS_AMOUNT}
- feeBips: {0..2^NUM_BITS_FEE_BIPS}

the prover knows an auxiliary input:

- tradingFee: {0..2^NUM_BITS_AMOUNT}

such that the following conditions hold:

SpotTrade

- tradingFee = amount \* feeBips // 10000

BatchSpotTrade

- tradingFee <= amount \* feeBips // 10000

Notes:

- While feeBips is called bips

### Description

Calculates the fee (paid by the user to the operator) . Trading fee is a percentage of the amount of tokens bought by the user.

## RequireValidOrder statement

A valid instance of a RequireValidOrder statement assures that given an input of:

- timestamp: {0..2^NUM_BITS_TIMESTAMP}
- order: Order

the prover knows an auxiliary input:

-

such that the following conditions hold:

- timestamp < order.validUntil (RequireLtGadget)

### Description

Validates that the order isn't expired.

## RequireFillLimit statement

A valid instance of a RequireFillLimit statement assures that given an input of:

- order: Order
- filled: {0..2^NUM_BITS_AMOUNT}
- fillS: {0..2^NUM_BITS_AMOUNT}
- fillB: {0..2^NUM_BITS_AMOUNT}

the prover knows an auxiliary input:

- filledAfter: {0..2^NUM_BITS_AMOUNT}

such that the following conditions hold:

- filledAfter = filled + ((order.fillAmountBorS == 1) ? fillB : fillS)
- (order.fillAmountBorS == 1) ? filledAfter <= order.amountB : filledAfter <= order.amountS

### Description

Allows orders to be limited against either the amount bought or the amount sold. This is useful because the price defined in the order is only the worst price the order can be filled.

This also directly limits the use of an order. Once the limit is reached the order cannot be used anymore.

## RequireOrderFills statement

A valid instance of a RequireOrderFills statement assures that given an input of:

- order: Order
- filled: {0..2^NUM_BITS_AMOUNT}
- fillS: {0..2^NUM_BITS_AMOUNT}
- fillB: {0..2^NUM_BITS_AMOUNT}

the prover knows an auxiliary input:

- filledAfter: {0..2^NUM_BITS_AMOUNT}

such that the following conditions hold:

- RequireFillRate(order.amountS, order.amountB, fillS, fillB)
- filledAfter = RequireFillLimit(order, filled, fillS, fillB)

### Description

Verifies the filling of an order i.e. if the rate at which it is being filled is valid and it's not being filled more than allowed.

## RequireValidTaker statement

A valid instance of a RequireValidTaker statement assures that given an input of:

- taker: F
- expectedTaker: F

the prover knows an auxiliary input:

-

such that the following conditions hold:

- (expectedTaker == 0) || (taker == expectedTaker)

### Description

Allows an order to be created that can only be matched against a specific counterparty.

## OrderMatching statement

A valid instance of an OrderMatching statement assures that given an input of:

- timestamp: {0..2^NUM_BITS_TIMESTAMP}
- orderA: Order
- orderB: Order
- ownerA: {0..2^NUM_BITS_ADDRESS}
- ownerB: {0..2^NUM_BITS_ADDRESS}
- cancelledA: {0..1}
- cancelledB: {0..1}
- filledA: {0..2^NUM_BITS_AMOUNT}
- filledB: {0..2^NUM_BITS_AMOUNT}
- fillS_A: {0..2^NUM_BITS_AMOUNT}
- fillS_B: {0..2^NUM_BITS_AMOUNT}

the prover knows an auxiliary input:

- filledAfterA: {0..2^NUM_BITS_AMOUNT}
- filledAfterB: {0..2^NUM_BITS_AMOUNT}

such that the following conditions hold:

- filledAfterA = RequireOrderFills(orderA, filledA, fillS_A, fillS_B)
- filledAfterB = RequireOrderFills(orderB, filledB, fillS_B, fillS_A)
- orderA.tokenS == orderB.tokenB
- orderA.tokenB == orderB.tokenS
- ValidateTaker(ownerB, orderA.taker)
- ValidateTaker(ownerA, orderB.taker)
- RequireValidOrder(timestamp, cancelledA, orderA)
- RequireValidOrder(timestamp, cancelledB, orderB)

### Description

Verifies that the given fill amounts fill both orders in a valid way:

- Valid order fills
- Matching tokens
- Valid taker
- Valid order

## BatchOrderMatching statement

A valid instance of an OrderMatching statement assures that given an input of:

- timestamp: {0..2^NUM_BITS_TIMESTAMP}
- order: Order
- filled: {0..2^NUM_BITS_AMOUNT}
- cancelled: {0..1}
- deltaFilledS: {0..2^NUM_BITS_AMOUNT}
- deltaFilledB: {0..2^NUM_BITS_AMOUNT}
- verify: {0..1}

the prover knows an auxiliary input:

- filledAfter: {0..2^NUM_BITS_AMOUNT}

such that the following conditions hold:

- filledAfter = RequireOrderFills(order, filled, deltaFilledS, deltaFilledB, verify)
- ValidateTaker(order.taker, verify)
- RequireValidOrder(timestamp, cancelled, order, verify)

### Description

Verifies that the given fill amounts fill both orders in a valid way:

- Valid order fills
- Matching tokens
- taker must be zero
- Valid order

# Transactions

## Deposit statement

A valid instance of a Deposit statement assures that given an input of:

- state: State
- owner: {0..2^NUM_BITS_ADDRESS}
- accountID: {0..2^NUM_BITS_ACCOUNT}
- tokenID: {0..2^NUM_BITS_TOKEN}
- amount: {0..2^NUM_BITS_AMOUNT}

the prover knows an auxiliary input:

- output: TxOutput

such that the following conditions hold:

- owner_bits = owner_packed
- accountID_bits = accountID_packed
- tokenID_bits = tokenID_packed
- amount_bits = amount_packed

- OwnerValid(state.accountA.account.owner, owner)

- output = DefaultTxOutput(state)
- output.ACCOUNT_A_ADDRESS = accountID
- output.ACCOUNT_A_OWNER = owner
- output.BALANCE_A_S_ADDRESS = tokenID
- output.BALANCE_A_S_BALANCE = state.accountA.balanceS.balance + amount
- output.SIGNATURE_REQUIRED_A = 0
- output.SIGNATURE_REQUIRED_B = 0
- output.NUM_CONDITIONAL_TXS = state.numConditionalTransactions + 1
- output.DA = {
  TransactionType.Deposit,
  owner,
  accountID,
  tokenID,
  amount
  }

Notes:

- The Merkle tree is allowed to have multiple accounts with the same owner.
- owner cannot be 0, but this is enforced in the smart contracts.

### Description

- https://github.com/Loopring/protocols/blob/master/packages/loopring_v3/DESIGN.md#deposit

This gadgets allows depositing funds to a new or existing account at accountID. The owner of an account can never change, unless state.accountA.account.owner == 0, which means a new account is created for owner.

As deposits are processed and stored on-chain, we have to process this transaction in the smart contract, and so numConditionalTransactions is incremented. No EdDSA signature are ever used, the deposit data is validated on-chain.

## AccountUpdate statement

A valid instance of an AccountUpdate statement assures that given an input of:

- state: State
- owner: {0..2^NUM_BITS_ADDRESS}
- accountID: {0..2^NUM_BITS_ACCOUNT}
- validUntil: {0..2^NUM_BITS_TIMESTAMP}
- publicKeyX: F
- publicKeyY: F
- feeTokenID: {0..2^NUM_BITS_TOKEN}
- fee: {0..2^NUM_BITS_AMOUNT}
- maxFee: {0..2^NUM_BITS_AMOUNT}
- type: {0..2^8}

the prover knows an auxiliary input:

- output: TxOutput
- hash: F
- compressedPublicKey: {0..2^256}
- fFee: {0..2^16}
- uFee: F

such that the following conditions hold:

- owner_bits = owner_packed
- accountID_bits = accountID_packed
- validUntil_bits = validUntil_packed
- feeTokenID_bits = feeTokenID_packed
- fee_bits = fee_packed
- maxFee_bits = maxFee_packed
- type_bits = type_packed
- state.accountA.account.nonce_bits = state.accountA.account.nonce_packed

- hash = PoseidonHash_t9f6p53(
    state.exchange,
    accountID,
    feeTokenID,
    maxFee,
    publicKeyX,
    publicKeyY,
    validUntil,
    nonce
  )
- OwnerValid(state.accountA.account.owner, owner)
- state.timestamp < validUntil
- fee <= maxFee
- compressedPublicKey = CompressPublicKey(publicKeyX, publicKeyY)
- uFee = Float(fFee)
- RequireAccuracy(uFee, fee)

- output = DefaultTxOutput(state)
- output.ACCOUNT_A_ADDRESS = nonce == 0 ? 0 : accountID
- output.ACCOUNT_A_OWNER = owner
- output.ACCOUNT_A_PUBKEY_X = publicKeyX
- output.ACCOUNT_A_PUBKEY_Y = publicKeyY
- output.ACCOUNT_A_NONCE = state.accountA.account.nonce + 1
- output.BALANCE_A_S_ADDRESS = feeTokenID
- output.BALANCE_A_S_BALANCE = state.accountA.balanceS.balance - uFee
- output.BALANCE_O_B_BALANCE = state.operator.balanceB.balance + uFee
- output.HASH_A = hash
- output.SIGNATURE_REQUIRED_A = (type == 0) ? 0 : 1
- output.SIGNATURE_REQUIRED_B = 0
- output.NUM_CONDITIONAL_TXS = state.numConditionalTransactions + ((type == 0) ? 0 : 1)
- output.DA = {
    TransactionType.AccountUpdate,
    owner,
    accountID,
    feeTokenID,
    fFee,
    compressedPublicKey,
    nonce
  }

Notes:

- The Merkle tree is allowed to have multiple accounts with the same owner.
- owner cannot be 0, but this is enforced in the smart contracts.

### Description

- https://github.com/Loopring/protocols/blob/master/packages/loopring_v3/DESIGN.md#account-update

This gadgets allows setting the account EdDSA public key in a new or existing account at accountID. The owner of an account can never change, unless state.accountA.account.owner == 0, which means a new account is created for owner.

The account nonce is used to prevent replay protection.

A fee is paid to the operator in any token. The operator can choose any fee lower or equal than the maxFee specified by the user.

The public key can either be set

- with the help of an on-chain signature. In this case no valid EdDSA signature needs to be provided and numConditionalTransactions is incremented.
- with the help of an EdDSA signature. In this case a valid signature for the _current_ (not the new ones!) EdDSA public keys stored in the account needs to be provided. numConditionalTransactions is not incremented.

## AppKeyUpdate statement

A valid instance of an AccountUpdate statement assures that given an input of:

- state: State
- accountID: {0..2^NUM_BITS_ACCOUNT}
- validUntil: {0..2^NUM_BITS_TIMESTAMP}
- appKeyPublicKeyX: F
- appKeyPublicKeyY: F
- feeTokenID: {0..2^NUM_BITS_TOKEN}
- fee: {0..2^NUM_BITS_AMOUNT}
- maxFee: {0..2^NUM_BITS_AMOUNT}
- disableAppKeySpotTrade: {0..2^NUM_BITS_BIT}
- disableAppKeyWithdraw: {0..2^NUM_BITS_BIT}
- disableAppKeyTransferToOther: {0..2^NUM_BITS_BIT}

the prover knows an auxiliary input:

- output: TxOutput
- hash: F
- compressedPublicKey: {0..2^256}
- fFee: {0..2^16}
- uFee: F

such that the following conditions hold:

- typeTxPad = 5bits zero
- owner_bits = owner_packed
- accountID_bits = accountID_packed
- validUntil_bits = validUntil_packed
- feeTokenID_bits = feeTokenID_packed
- fee_bits = fee_packed
- maxFee_bits = maxFee_packed
- type_bits = type_packed
- state.accountA.account.nonce_bits = state.accountA.account.nonce_packed
- disableAppKeySpotTrade_bits = disableAppKeySpotTrade_packed
- disableAppKeyWithdraw_bits = disableAppKeyWithdraw_packed
- disableAppKeyTransferToOther_bits = disableAppKeyTransferToOther_packed

- hash = PoseidonHash_t9f6p53(
    state.exchange,
    accountID,
    feeTokenID,
    maxFee,
    appKeyPublicKeyX,
    appKeyPublicKeyY,
    validUntil,
    nonce,
    disableAppKeySpotTrade,
    disableAppKeyWithdraw,
    disableAppKeyTransferToOther
  )
- OwnerValid(state.accountA.account.owner, owner)
- state.timestamp < validUntil
- fee <= maxFee
- compressedPublicKey = CompressPublicKey(publicKeyX, publicKeyY)
- uFee = Float(fFee)
- RequireAccuracy(uFee, fee)

- output = DefaultTxOutput(state)
- output.ACCOUNT_A_ADDRESS = nonce == 0 ? 0 : accountID
- output.ACCOUNT_A_OWNER = owner
- output.ACCOUNT_A_PUBKEY_X = publicKeyX
- output.ACCOUNT_A_PUBKEY_Y = publicKeyY
- output.ACCOUNT_A_NONCE = state.accountA.account.nonce + 1
- output.BALANCE_A_S_ADDRESS = feeTokenID
- output.BALANCE_A_S_BALANCE = state.accountA.balanceS.balance - uFee
- output.BALANCE_O_B_BALANCE = state.operator.balanceB.balance + uFee
- output.HASH_A = hash
- output.SIGNATURE_REQUIRED_A = (type == 0) ? 0 : 1
- output.SIGNATURE_REQUIRED_B = 0
- output.NUM_CONDITIONAL_TXS = state.numConditionalTransactions + ((type == 0) ? 0 : 1)
- output.DA = {
    TransactionType.AccountUpdate,
    typeTxPad,
    accountID,
    feeTokenID,
    fFee,
    nonce
  }

### Description

This gadgets allows setting the account EdDSA application public key in a new or existing account at accountID. 

The account nonce is used to prevent replay protection.

A fee is paid to the operator in any token. The operator can choose any fee lower or equal than the maxFee specified by the user.

The application public key can be set

- with the help of an EdDSA signature. In this case a valid signature for the EdDSA public keys stored in the account needs to be provided. numConditionalTransactions is not incremented.

## Noop statement

A valid instance of a Noop statement assures that given an input of:

- state: State

the prover knows an auxiliary input:

-

such that the following conditions hold:

- output = DefaultTxOutput(state)

Notes:

- Should have no side effects

### Description

Can be used to fill up blocks that are not fully filled with actual transactions.

## Withdraw statement

A valid instance of a Withdraw statement assures that given an input of:

- state: State
- accountID: {0..2^NUM_BITS_ACCOUNT}
- tokenID: {0..2^NUM_BITS_TOKEN}
- amount: {0..2^NUM_BITS_AMOUNT}
- feeTokenID: {0..2^NUM_BITS_TOKEN}
- maxFee: {0..2^NUM_BITS_AMOUNT}
- fee: {0..2^NUM_BITS_AMOUNT}
- validUntil: {0..2^NUM_BITS_TIMESTAMP}
- onchainDataHash: {0..2^NUM_BITS_HASH}
- storageID: {0..2^NUM_BITS_STORAGEID}
- type: {0..2^NUM_BITS_TYPE}
- useAppKey: {0..2^NUM_BITS_BYTE}

the prover knows an auxiliary input:

- output: TxOutput
- owner: {0..2^NUM_BITS_ADDRESS}
- hash: F
- fFee: {0..2^16}
- uFee: F

such that the following conditions hold:

- owner_bits = owner_packed
- accountID_bits = accountID_packed
- validUntil_bits = validUntil_packed
- feeTokenID_bits = feeTokenID_packed
- fee_bits = fee_packed
- maxFee_bits = maxFee_packed
- type_bits = type_packed
- useAppKey_bits = useAppKey_packed
- state.accountA.account.nonce_bits = state.accountA.account.nonce_packed

- hash = PoseidonHash_t11f6p53(
    state.exchange,
    accountID,
    tokenID,
    amount,
    feeTokenID,
    maxFee,
    onchainDataHash,
    validUntil,
    storageID,
    useAppKey
  )
- owner = (accountID == 0) ? 0 : state.accountA.account.owner
- state.timestamp < validUntil (RequireLtGadget)
- fee <= maxFee (RequireLeqGadget)
- if type == 2 then amount == (accountID == 0) ? state.pool.balanceB.balance : state.accountA.balanceS.balance
- if type == 3 then amount == 0

- Nonce(state.accountA.storage, storageID, (state.txType == TransactionType.Withdraw && (type == 0 || type == 1)))
- uFee = Float(fFee)
- RequireAccuracy(uFee, fee)

- output = DefaultTxOutput(state)
- output.ACCOUNT_A_ADDRESS = (accountID == 0) ? 1 : accountID
- output.BALANCE_A_S_ADDRESS = tokenID
- output.BALANCE_B_S_ADDRESS = feeTokenID
- output.BALANCE_A_S_WEIGHTAMM = (accountID != 0 && type == 2) ? 0 : state.accountA.balanceS.tokenWeightAMM
- output.BALANCE_A_S_BALANCE = state.accountA.balanceS.balance - ((accountID == 0) ? 0 : amount)
- output.BALANCE_A_B_BALANCE = state.accountA.balanceB.balance - uFee
- output.BALANCE_O_A_BALANCE = state.operator.balanceA.balance + uFee
- output.BALANCE_P_B_BALANCE = state.pool.balanceB.balance - ((accountID == 0) ? amount : 0)
- output.HASH_A = hash
- output.SIGNATURE_REQUIRED_A = (type == 0) ? 1 : 0
- output.SIGNATURE_REQUIRED_B = 0
- output.NUM_CONDITIONAL_TXS = state.numConditionalTransactions + 1
- output.STORAGE_A_ADDRESS = storageID[0..NUM_BITS_STORAGE_ADDRESS]
- output.STORAGE_A_DATA = (type == 0 || type == 1) ? 1 : state.accountA.storage.data
- output.STORAGE_A_STORAGEID = (type == 0 || type == 1) ? storageID : state.accountA.storage.storageID
- output.DA = {
    type
    owner,
    accountID,
    tokenID,
    feeTokenID,
    fFee,
    storageID,
    onchainDataHash
  }

### Description

- https://github.com/Loopring/protocols/blob/master/packages/loopring_v3/DESIGN.md#withdraw

This gadgets allows withdrawing from an account at accountID.

Withdrawing from account == 0 is special because this is where the protocol fees are stored and these balances are not immediately committed to the Merkle tree state. This is why some special logic is needed to make sure we don't do any unexpected state changes on that account.

Some things are only checked when we're actually doing a withdrawal by inspecting txType. This is done because the withdrawal constraints are also part of different transaction types, and so while these constraints aren't needed for a non-Withdrawal transaction, they do need to be valid to be able to create a valid block.

amount is subtracted from the users balance at tokenID. Depending on the type, amount may need to have a specific value:

- type == 0 || type == 1: any amount is allowed as long as amount >= balance
- type == 2: amount == state.accountA.balanceS.balance
- type == 3: amount == 0

These different types are used on-chain to correctly handle withdrawals.

Valid forced withdrawals (type == 0) reset the AMM weight of the token. This is to allow AMMs to be safely disabled in all cases.

A fee is paid to the operator in any token. The operator can choose any fee lower or equal than the maxFee specified by the user.

The storage nonce system is used to prevent replay protection when type == 0 or type == 1. Replay protection for other types are handled on-chain.

Only when type == 0 is a valid EdDSA signature required, for the other types the approval is checked on-chain.

In all cases the withdrawal transaction needs to be processed on-chain, so numConditionalTransactions is always incremented.

## Transfer statement

A valid instance of a Transfer statement assures that given an input of:

- state: State
- fromAccountID: {0..2^NUM_BITS_ACCOUNT}
- toAccountID: {0..2^NUM_BITS_ACCOUNT}
- tokenID: {0..2^NUM_BITS_TOKEN}
- amount: {0..2^NUM_BITS_AMOUNT}
- feeTokenID: {0..2^NUM_BITS_TOKEN}
- maxFee: {0..2^NUM_BITS_AMOUNT}
- fee: {0..2^NUM_BITS_AMOUNT}
- validUntil: {0..2^NUM_BITS_TIMESTAMP}
- onchainDataHash: {0..2^NUM_BITS_HASH}
- storageID: {0..2^NUM_BITS_STORAGEID}
- type: {0..2^NUM_BITS_TYPE}
- to: {0..2^NUM_BITS_ADDRESS}
- dualAuthorX: F
- dualAuthorY: F
- payer_toAccountID: {0..2^NUM_BITS_ACCOUNT}
- payer_to: {0..2^NUM_BITS_ADDRESS}
- payee_toAccountID: {0..2^NUM_BITS_ACCOUNT}
- putAddressesInDA: {0..2}
- useAppKey: {0..2^NUM_BITS_BYTE}

the prover knows an auxiliary input:

- output: TxOutput
- from: {0..2^NUM_BITS_ADDRESS}
- hashPayer: F
- hashDual: F
- fFee: {0..2^16}
- uFee: F
- fAmount: {0..2^24}
- uAmount: F

such that the following conditions hold:

- typeTxPad = 5bits zero
- fromAccountID_bits = fromAccountID_packed
- toAccountID_bits = toAccountID_packed
- tokenID_bits = tokenID_packed
- amount_bits = amount_packed
- feeTokenID_bits = feeTokenID_packed
- fee_bits = fee_packed
- validUntil_bits = validUntil_packed
- type_bits = type_packed
- from_bits = from_packed
- to_bits = to_packed
- storageID_bits = storageID_packed
- payer_toAccountID_bits = payer_toAccountID_packed
- payer_to_bits = payer_to_packed
- payee_toAccountID_bits = payee_toAccountID_packed
- maxFee_bits = maxFee_packed
- putAddressesInDA_bits = putAddressesInDA_packed
- useAppKey_bits = useAppKey_packed


- hashPayer = PoseidonHash_t14f6p53(
  state.exchange,
  fromAccountID,
  payer_toAccountID,
  tokenID,
  amount,
  feeTokenID,
  maxFee,
  payer_to,
  dualAuthorX,
  dualAuthorY,
  validUntil,
  storageID,
  useAppKey
  )
- hashDual = PoseidonHash_t14f6p53(
  exchange,
  fromAccountID,
  payee_toAccountID,
  tokenID,
  amount,
  feeTokenID,
  maxFee,
  to,
  dualAuthorX,
  dualAuthorY,
  validUntil,
  storageID,
  useAppKey
  )
- state.timestamp < validUntil (RequireLtGadget)
- fee <= maxFee (RequireLeqGadget)
- if (payerTo != 0) then payerTo = to
- if (payerTo != 0) then payer_toAccountID = payee_toAccountID
- if (payee_toAccountID != 0) then payee_toAccountID = toAccountID
- if (state.txType == TransactionType.Transfer) then to != 0
- OwnerValid(state.accountB.account.owner, to)
- Nonce(state.accountA.storage, storageID, (state.txType == TransactionType.Transfer))
- uFee = Float(fFee)
- RequireAccuracy(uFee, fee)
- uAmount = Float(fAmount)
- RequireAccuracy(uAmount, amount)

- output = DefaultTxOutput(state)
- output.ACCOUNT_A_ADDRESS = fromAccountID
- output.ACCOUNT_B_ADDRESS = toAccountID
- output.ACCOUNT_B_OWNER = to
- output.BALANCE_A_S_ADDRESS = tokenID
- output.BALANCE_B_S_ADDRESS = feeTokenID
- output.BALANCE_A_S_BALANCE = state.accountA.balanceS.balance - uAmount
- output.BALANCE_B_B_BALANCE = state.accountB.balanceB.balance + uAmount
- output.BALANCE_A_B_BALANCE = state.accountA.balanceB.balance - uFee
- output.BALANCE_O_A_BALANCE = state.operator.balanceA.balance + uFee
- output.HASH_A = hashPayer
- output.HASH_B = hashDual
- output.PUBKEY_X_B = (dualAuthorX == 0 && dualAuthorY == 0) ? state.accountA.account.publicKey.x : dualAuthorX
- output.PUBKEY_Y_B = (dualAuthorX == 0 && dualAuthorY == 0) ? state.accountA.account.publicKey.y : dualAuthorY
- output.SIGNATURE_REQUIRED_A = (type == 0) ? 1 : 0
- output.SIGNATURE_REQUIRED_B = (type == 0) ? 1 : 0
- output.NUM_CONDITIONAL_TXS = state.numConditionalTransactions + (type != 0) ? 1 : 0
- output.STORAGE_A_ADDRESS = storageID[0..NUM_BITS_STORAGE_ADDRESS]
- output.STORAGE_A_DATA = 1
- output.STORAGE_A_STORAGEID = storageID
- output.DA = (
  TransactionType.Transfer,
  typeTxPad,
  fromAccountID,
  toAccountID,
  tokenID,
  fAmount,
  feeTokenID,
  fFee,
  storageID,
  (state.accountA.account.owner == 0 || type == 1 || putAddressesInDA == 1) ? to : 0,
  (type == 1 || putAddressesInDA == 1) ? from : 0
  )

### Description

- https://github.com/Loopring/protocols/blob/master/packages/loopring_v3/DESIGN.md#transfer

This gadgets allows transferring amount tokens of token tokenID from account fromAccountID to accoun toAccountID.

Some things are only checked when we're actually doing a transfer by inspecting txType. This is done because the transfer constraints are also part of different transaction types, and so while these constraints don't need to be valid for a non-Transfer transaction, they do need to be valid to be able to create a valid block.

A fee is paid to the operator in any token. The operator can choose any fee lower or equal than the maxFee specified by the user.

The storage nonce system is used to prevent replay protection.

Only when type == 0 is a valid EdDSA signature required. When type == 1 the transfer transaction needs to be processed on-chain, so numConditionalTransactions is incremented.

Some data is only put in the DA when either required (so that the Merkle tree can be reconstructed) or when requested by the operator (putAddressesInDA == 1). Putting less data on-chain makes the transcation cheaper, but in some cases it may be useful to still put it on-chain.

## SpotTrade statement

A valid instance of a SpotTrade statement assures that given an input of:

- state: State
- orderA: Order
- orderB: Order
- fillS_A: {0..2^24}
- fillS_B: {0..2^24}

the prover knows an auxiliary input:

- output: TxOutput
- storageDataA: F
- storageDataB: F
- uFillS_A: {0..2^NUM_BITS_AMOUNT}
- uFillS_B: {0..2^NUM_BITS_AMOUNT}
- filledAfterA: {0..2^NUM_BITS_AMOUNT}
- filledAfterB: {0..2^NUM_BITS_AMOUNT}

such that the following conditions hold:

- orderA = Order(state.exchange)
- orderB = Order(state.exchange)
- typeTxPad = 5bit zero
- uFillS_A = Float(fillS_A)
- uFillS_B = Float(fillS_B)
- storageDataA = StorageReader(state.accountA.storage, orderA.storageID, (state.txType == TransactionType.SpotTrade))
- storageDataB = StorageReader(state.accountB.storage, orderB.storageID, (state.txType == TransactionType.SpotTrade))
- autoMarketOrderCheckA = AutoMarketOrderCheckA(orderA, storageDataA)
- autoMarketOrderCheckB = AutoMarketOrderCheckA(orderB, storageDataB)
- tradeHistoryWithAutoMarket_A = StorageReaderForAutoMarketGadget(storageDataA, autoMarketOrderCheckA.isNewOrder)
- tradeHistoryWithAutoMarket_B = StorageReaderForAutoMarketGadget(storageDataB, autoMarketOrderCheckB.isNewOrder)
- OrderMatching(state.timestamp, orderA, orderB, state.accountA.account.owner, state.accountB.account.owner, storageDataA, storageDataB, tradeHistoryWithAutoMarket_A, tradeHistoryWithAutoMarket_B, uFillS_A, uFillS_B)
- tradingFeeA = FeeCalculator(uFillS_B, orderA.feeBips)
- tradingFeeB = FeeCalculator(uFillS_A, orderB.feeBips)
- feeMatchA = GasFeeMatchingGadget(orderA.fee, tradeHistoryWithAutoMarket_A.getGasFee(), orderA.maxFee)
- feeMatchB = GasFeeMatchingGadget(orderB.fee, tradeHistoryWithAutoMarket_B.getGasFee(), orderB.maxFee)
- resolvedAAuthorX = orderA.useAppKey ? accountA.appKeyPublicKeyX : accountA.publicKeyX
- resolvedAAuthorY = orderA.useAppKey ? accountA.appKeyPublicKeyY : accountA.publicKeyY
- resolvedBAuthorX = orderB.useAppKey ? accountB.appKeyPublicKeyX : accountB.publicKeyX
- resolvedBAuthorY = orderB.useAppKey ? accountB.appKeyPublicKeyY : accountB.publicKeyY

- output.BALANCE_A_S_ADDRESS = orderA.tokenS
- output.BALANCE_A_B_ADDRESS = orderA.tokenB
- output.BALANCE_B_S_ADDRESS = orderB.tokenS
- output.BALANCE_B_B_ADDRESS = orderB.tokenB

- output.ACCOUNT_A_ADDRESS = orderA.accountID
- output.ACCOUNT_B_ADDRESS = orderB.accountID

- output.BALANCE_A_S_BALANCE = state.accountA.balanceS.balance - uFillS_A
- output.BALANCE_A_B_BALANCE = state.accountB.balanceB.balance + uFillS_B - gasFeeA
- output.BALANCE_B_S_BALANCE = state.accountA.balanceB.balance - uFillS_B
- output.BALANCE_B_B_BALANCE = state.accountB.balanceB.balance + uFillS_A - gasFeeB

- output.BALANCE_O_A_BALANCE = state.operator.balanceA.balance + gasFeeA
- output.BALANCE_O_B_BALANCE = state.operator.balanceB.balance + gasFeeB
- output.BALANCE_O_C_BALANCE = state.operator.balanceC.balance + tradingFeeA
- output.BALANCE_O_D_BALANCE = state.operator.balanceD.balance + tradingFeeB

- output.STORAGE_A_ADDRESS = orderA.storageID[0..NUM_BITS_STORAGE_ADDRESS]
- output.TXV_STORAGE_A_TOKENSID = autoMarketOrderCheckA.getTokenSIDForStorageUpdate()
- output.TXV_STORAGE_A_TOKENBID = autoMarketOrderCheckA.getTokenBIDForStorageUpdate()
- output.STORAGE_A_DATA = filledAfterA
- output.STORAGE_A_STORAGEID = orderA.storageID
- output.TXV_STORAGE_A_GASFEE = feeMatch_A.getFeeSum()
- output.TXV_STORAGE_A_FORWARD = autoMarketOrderCheckA.getNewForwardForStorageUpdate()

- output.STORAGE_B_ADDRESS = orderB.storageID[0..NUM_BITS_STORAGE_ADDRESS]
- output.TXV_STORAGE_B_TOKENSID = autoMarketOrderCheckB.getTokenSIDForStorageUpdate()
- output.TXV_STORAGE_B_TOKENBID = autoMarketOrderCheckB.getTokenBIDForStorageUpdate()
- output.STORAGE_B_DATA = filledAfterB
- output.STORAGE_B_STORAGEID = orderB.storageID
- output.TXV_STORAGE_B_GASFEE = feeMatch_B.getFeeSum()
- output.TXV_STORAGE_B_FORWARD = autoMarketOrderCheckB.getNewForwardForStorageUpdate()

- output.HASH_A = orderA.hash
- output.HASH_B = orderB.hash
- output.TXV_PUBKEY_X_A = resolvedAAuthorX
- output.TXV_PUBKEY_Y_A = resolvedAAuthorY
- output.TXV_PUBKEY_X_B = resolvedBAuthorX
- output.TXV_PUBKEY_Y_B = resolvedBAuthorY
- output.SIGNATURE_REQUIRED_A = (orderA.amm == 0) ? 1 : 0
- output.SIGNATURE_REQUIRED_B = (orderB.amm == 0) ? 1 : 0

- output.DA = (
  TransactionType.SpotTrade,
  typeTxPad,
  orderA.storageID,
  orderB.storageID,
  orderA.accountID,
  orderB.accountID,
  orderA.tokenS,
  orderB.tokenS,
  fillS_A,
  fillS_B,
  orderA.fillAmountBorS, 0, orderA.feeBips,
  orderB.fillAmountBorS, 0, orderB.feeBips
  )

### Description

- https://github.com/Loopring/protocols/blob/master/packages/loopring_v3/DESIGN.md#spot-trade

This gadgets allows trading two tokens (tokenS and tokenB) between two accounts (orderA.accountID and orderB.accountID).

A fee is paid to the operator, and this fee is always paid in the tokens bought by the account. And the trading fee is also paid to the operator.

The operator is free to pass in any fillS_A and fillS_B, as long as all user requirements are met, the most important ones being:

- For limit orders the price is below the maximum price defined in the order as amountS/amountB

Orders need to be signed with EdDSA in all cases, except for AutoMarket orders, which are implicitely authorized by start order.

Trades are never processed on-chain, so numConditionalTransactions is never incremented.


## BatchSpotTrade statement

A valid instance of a BatchSpotTrade statement assures that given an input of:

- state: State
- users: BatchSpotTradeUser[]
  - orders: Order[]

the prover knows an auxiliary input:

- output: TxOutput
- storageDataA: F
- storageDataAArray: F[]
- storageDataB: F
- storageDataBArray: F[]
- storageDataCArray: F[]
- storageDataDArray: F[]
- storageDataEArray: F[]
- storageDataFArray: F[]

such that the following conditions hold:

- userA = BatchUser(state.exchange)
- userB = BatchUser(state.exchange)
- userC = BatchUser(state.exchange)
- userD = BatchUser(state.exchange)
- userE = BatchUser(state.exchange)
- userF = BatchUser(state.exchange)
  - batchOrder = BatchOrder(state.exchange)
    - order = Order(state.exchange)
    - storageData = StorageReader(state.account.storage, order.storageID, (state.txType == TransactionType.BatchSpotTrade))
    - autoMarketOrderCheck = AutoMarketOrderCheckA(order, storageData)
    - tradeHistoryWithAutoMarket = StorageReaderForAutoMarketGadget(storageData, autoMarketOrderCheck.isNewOrder)
    - BatchOrderMatching(state.timestamp, order, storageData, tradeHistoryWithAutoMarket, uFillS_A, uFillS_B)
    - tradingFee = tradingFee_packed <= FeeCalculator(uFillS_B, order.feeBips)
    - feeMatch = GasFeeMatchingGadget(order.fee, tradeHistoryWithAutoMarket.getGasFee(), order.maxFee)
    - resolvedAAuthorX = order.useAppKey ? account.appKeyPublicKeyX : account.publicKeyX
    - resolvedAAuthorY = order.useAppKey ? account.appKeyPublicKeyY : account.publicKeyY

- output.ACCOUNT_A_ADDRESS = userA.accountID
- output.ACCOUNT_B_ADDRESS = userB.accountID
- output.ACCOUNT_C_ADDRESS = userC.accountID
- output.ACCOUNT_D_ADDRESS = userD.accountID
- output.ACCOUNT_E_ADDRESS = userE.accountID
- output.ACCOUNT_F_ADDRESS = userF.accountID

- output.DA = (
  TransactionType.BatchSpotTrade,
  bindToken,
  firstToken,
  secondToken,
  secondTokenType,
  thirdTokenType,
  fourthTokenType,
  fifthTokenType,
  sixthTokenType,
  userB.accountID,
  userB.firstSelectTokenExchange,
  userB.secondSelectTokenExchange,
  userC.accountID,
  userC.firstSelectTokenExchange,
  userC.secondSelectTokenExchange,
  userD.accountID,
  userD.firstSelectTokenExchange,
  userD.secondSelectTokenExchange,
  userE.accountID,
  userE.firstSelectTokenExchange,
  userE.secondSelectTokenExchange,
  userF.accountID,
  userF.firstSelectTokenExchange,
  userF.secondSelectTokenExchange,
  userA.accountID,
  userA.firstTokenExchange,
  userA.secondTokenExchange,
  userA.thirdTokenExchange
  )


## OrderCancel statement

A valid instance of a OrderCancel statement assures that given an input of:

- state: State
- accountID
- tokenID
- storageID

the prover knows an auxiliary input:

- output: TxOutput
- storageDataA: F
- typeTxPad: 5bits zero

such that the following conditions hold:

- accountID_bits = accountID_packed
- tokenID_bits = tokenID_packed
- feeTokenID_bits = feeTokenID_packed
- fee_bits = fee_packed
- storageID_bits = storageID_packed
- nonce = OrderCancelledNonce(storage, storageID)
- hash = PoseidonHash_t8f6p53(
  state.exchange,
  owner,
  accountID,
  tokenID,
  storageID,
  maxFee,
  feeTokenID,
  useAppKey
  )

- output.ACCOUNT_A_ADDRESS = accountID

- output.STORAGE_A_CANCELLED = nonce.getCancelled()
- output.STORAGE_A_STORAGEID = storageID

- output.BALANCE_A_S_ADDRESS = feeTokenID
- output.BALANCE_A_S_BALANCE = state.accountA.balanceS.balance - fee
- output.TXV_BALANCE_O_B_Address = feeTokenID
- output.TXV_BALANCE_O_B_BALANCE = state.oper.balanceB.balance + fee

- output.HASH_A = hash
- output.SIGNATURE_REQUIRED_A = 1
- output.SIGNATURE_REQUIRED_B = 0


- output.DA = (
  TransactionType.OrderCancel,
  typeTxPad,
  owner, 
  accountID, 
  tokenID, 
  storageID, 
  feeTokenID, 
  fee
  )

# Unvisersal Circuit

## SelectTransaction statement

A valid instance of a SelectTransaction statement assures that given an input of:

- selector_bits: {0..2^7}
- outputs[7]: TxOutput

the prover knows an auxiliary input:

- output: TxOutput

such that the following conditions hold:

- for each F var in TxOutput: output.var = Select(selector_bits, outputs[0..7].var)
- for each F_array var in TxOutput: output.var = ArraySelect(selector_bits, outputs[0..7].var)
- output.da = ArraySelect(selector_bits, outputs[0..7].da), with outputs[i].da padded to TX_DATA_AVAILABILITY_SIZE \* 8 bits with zeros

### Description

This gadget selects the correct output for the transaction that's being executed. All transactions types are always executed in the circuit, so we select the output of the required transaction here.

## Transaction statement

A valid instance of a Transaction statement assures that given an input of:

- txType: {0..2^NUM_BITS_TX_TYPE}
- exchange: {0..2^NUM_BITS_ADDRESS}
- timestamp: {0..2^NUM_BITS_TIMESTAMP}
- protocolFeeBips: {0..2^NUM_BITS_PROTOCOL_FEE_BIPS}
- operatorAccountID: {0..2^NUM_BITS_ACCOUNT}
- root_old: F
- protocolBalancesRoot_old: F
- numConditionalTransactions_old: F

the prover knows an auxiliary input:

- root_new: F
- protocolBalancesRoot_new: F
- numConditionalTransactions_new: F
- state: State
- outputs: TxOutput[7]
- output: TxOutput

such that the following conditions hold:

- txType_bits = txType_packed
- selector = Selector(txType)

- state.txType = txType
- state.exchange = exchange
- state.timestamp = timestamp
- state.protocolFeeBips = protocolFeeBips
- state.operatorAccountID = operatorAccountID

- outputs[0] = Noop(state)
- outputs[1] = Transfer(state)
- outputs[2] = SpotTrade(state)
- outputs[3] = OrderCancel(state)
- outputs[4] = AppKeyUpdate(state)
- outputs[5] = BatchSpotTrade(state)
- outputs[6] = Deposit(state)
- outputs[7] = AccountUpdate(state)
- outputs[8] = Withdraw(state)
- output = SelectTransaction(selector, outputs)

- output.ACCOUNT_A_ADDRESS_bits = output.ACCOUNT_A_ADDRESS_packed
- output.ACCOUNT_B_ADDRESS_bits = output.ACCOUNT_B_ADDRESS_packed
- output.ACCOUNT_C_ADDRESS_bits = output.ACCOUNT_C_ADDRESS_packed
- output.ACCOUNT_D_ADDRESS_bits = output.ACCOUNT_D_ADDRESS_packed
- output.ACCOUNT_E_ADDRESS_bits = output.ACCOUNT_E_ADDRESS_packed
- output.ACCOUNT_F_ADDRESS_bits = output.ACCOUNT_F_ADDRESS_packed
- output.ACCOUNT_A_ADDRESS != 0
- output.ACCOUNT_B_ADDRESS != 0
- output.ACCOUNT_C_ADDRESS != 0
- output.ACCOUNT_D_ADDRESS != 0
- output.ACCOUNT_E_ADDRESS != 0
- output.ACCOUNT_F_ADDRESS != 0

- SignatureVerifier(output.PUBKEY_X_A, output.PUBKEY_Y_A, output.HASH_A, output.SIGNATURE_REQUIRED_A)
- SignatureVerifier(output.PUBKEY_X_B, output.PUBKEY_Y_B, output.HASH_B, output.SIGNATURE_REQUIRED_B)
- SignatureVerifier[](output.PUBKEY_X_A_ARRAY, output.PUBKEY_Y_A_ARRAY, output.HASH_A_ARRAY, output.SIGNATURE_REQUIRED_A_ARRAY)
- SignatureVerifier[](output.PUBKEY_X_B_ARRAY, output.PUBKEY_Y_B_ARRAY, output.HASH_B_ARRAY, output.SIGNATURE_REQUIRED_B_ARRAY)
- SignatureVerifier[](output.PUBKEY_X_C_ARRAY, output.PUBKEY_Y_C_ARRAY, output.HASH_C_ARRAY, output.SIGNATURE_REQUIRED_C_ARRAY)
- SignatureVerifier[](output.PUBKEY_X_D_ARRAY, output.PUBKEY_Y_D_ARRAY, output.HASH_D_ARRAY, output.SIGNATURE_REQUIRED_D_ARRAY)
- SignatureVerifier[](output.PUBKEY_X_E_ARRAY, output.PUBKEY_Y_E_ARRAY, output.HASH_E_ARRAY, output.SIGNATURE_REQUIRED_E_ARRAY)
- SignatureVerifier[](output.PUBKEY_X_F_ARRAY, output.PUBKEY_Y_F_ARRAY, output.HASH_F_ARRAY, output.SIGNATURE_REQUIRED_F_ARRAY)
- SignatureVerifiers([{output.PUBKEY_X, output.PUBKEY_Y, output.HASH, output.SIGNATURE_REQUIRED}])

* root_updateStorage_A = StorageUpdate(
  state.accountA.balanceS.storageRoot,
  output.STORAGE_A_ADDRESS,
  state.accountA.storage,
  (output.STORAGE_A_TOKENSID, output.STORAGE_A_TOKENBID, output.STORAGE_A_DATA, output.STORAGE_A_STORAGEID, output.STORAGE_A_GASFEE, output.STORAGE_A_CANCELLED, output.STORAGE_A_FORWARD)
  )
* root_updateStorage_A_batch = BatchStorageAUpdateGadget(
  state.accountA,
  updateStorage_A.result(),
  )
* root_updateBalanceS_A = BalanceUpdate(
  state.accountA.account.balancesRoot,
  output.BALANCE_A_S_ADDRESS,
  state.accountA.balanceS,
  (output.BALANCE_A_S_BALANCE)
  )
* root_updateBalanceB_A = BalanceUpdate(
  root_updateBalanceS_A,
  output.BALANCE_A_B_ADDRESS,
  state.accountA.balanceB,
  (output.BALANCE_A_B_BALANCE)
  )
* root_updateBalanceFee_A = BalanceUpdate(
  root_updateBalanceB_A,
  output.BALANCE_A_FEE_ADDRESS,
  state.accountA.balanceFee,
  (output.BALANCE_A_FEE_BALANCE)
  )
* root_updateAccount_A = AccountUpdate(
  root_old,
  output.ACCOUNT_A_ADDRESS,
  state.accountA.account,
  (output.ACCOUNT_A_OWNER, output.ACCOUNT_A_PUBKEY_X, output.ACCOUNT_A_PUBKEY_Y,  output.ACCOUNT_A_APPKEY_PUBKEY_X, output.ACCOUNT_A_APPKEY_PUBKEY_Y,
  output.ACCOUNT_A_NONCE, 
  output.ACCOUNT_A_DISABLE_APPKEY_SPOTTRADE, output.ACCOUNT_A_DISABLE_APPKEY_WITHDRAW_TO_OTHER, output.ACCOUNT_A_DISABLE_APPKEY_TRANSFER_TO_OTHER,
  updateBalanceFee_A.result(), updateStorage_A_batch.getHashRoot())
  )

* root_updateStorage_B = StorageUpdate(
  state.accountB.balanceS.storageRoot,
  output.STORAGE_B_ADDRESS,
  state.accountB.storage,
  (output.STORAGE_B_TOKENSID, output.STORAGE_B_TOKENBID, output.STORAGE_B_DATA, output.STORAGE_B_STORAGEID, output.STORAGE_B_GASFEE, output.STORAGE_B_CANCELLED, output.STORAGE_B_FORWARD)
  )
* root_updateStorage_B_batch = BatchStorageBUpdateGadget(
  state.accountB,
  updateStorage_B.result(),
  )
* root_updateBalanceS_B = BalanceUpdate(
  state.accountB.account.balancesRoot,
  output.BALANCE_B_S_ADDRESS,
  state.accountB.balanceS,
  (output.BALANCE_B_S_BALANCE)
  )
* root_updateBalanceB_B = BalanceUpdate(
  root_updateBalanceS_B,
  output.BALANCE_B_B_ADDRESS,
  state.accountB.balanceB,
  (output.BALANCE_B_B_BALANCE)
  )
* root_updateBalanceB_Fee = BalanceUpdate(
  root_updateBalanceB_B,
  output.BALANCE_B_Fee_ADDRESS,
  state.accountB.balanceFee,
  (output.BALANCE_B_Fee_BALANCE)
  )
* root_updateAccount_B = AccountUpdate(
  root_updateAccount_A,
  output.ACCOUNT_B_ADDRESS,
  state.accountB.account,
  (output.ACCOUNT_B_OWNER, output.ACCOUNT_B_PUBKEY_X, output.ACCOUNT_B_PUBKEY_Y,  
  state.accountB.account.appKeyPublicKey.x, state.accountB.account.appKeyPublicKey.y,
  output.ACCOUNT_B_NONCE, 
  state.accountB.account.disableAppKeySpotTrade, state.accountB.account.disableAppKeyWithdraw, state.accountB.account.disableAppKeyTransferToOther,
  updateBalanceFee_B.result(), updateStorage_B_batch.getHashRoot())
  )
  )

* root_updateStorage_C_batch = BatchStorageCUpdateGadget(
  state.accountC,
  updateStorage_C.result(),
  )
* root_updateBalanceS_C = BalanceUpdate(
  state.accountC.account.balancesRoot,
  output.BALANCE_C_S_ADDRESS,
  state.accountC.balanceS,
  (output.BALANCE_C_S_BALANCE)
  )
* root_updateBalanceB_C = BalanceUpdate(
  root_updateBalanceS_C,
  output.BALANCE_C_B_ADDRESS,
  state.accountC.balanceB,
  (output.BALANCE_C_B_BALANCE)
  )
* root_updateBalanceC_Fee = BalanceUpdate(
  root_updateBalanceB_C,
  output.BALANCE_C_Fee_ADDRESS,
  state.accountC.balanceFee,
  (output.BALANCE_C_Fee_BALANCE)
  )
* root_updateAccount_C = AccountUpdate(
  root_updateAccount_B,
  output.ACCOUNT_C_ADDRESS,
  state.accountC.account,
  (output.ACCOUNT_C_OWNER, output.ACCOUNT_C_PUBKEY_X, output.ACCOUNT_C_PUBKEY_Y,  
  state.accountC.account.appKeyPublicKey.x, state.accountC.account.appKeyPublicKey.y,
  output.ACCOUNT_C_NONCE, 
  state.accountC.account.disableAppKeySpotTrade, state.accountC.account.disableAppKeyWithdraw, state.accountC.account.disableAppKeyTransferToOther,
  updateBalanceFee_C.result(), updateStorage_C_batch.getHashRoot())
  )
  )

* root_updateStorage_D_batch = BatchStorageDUpdateGadget(
  state.accountD,
  updateStorage_D.result(),
  )
* root_updateBalanceS_D = BalanceUpdate(
  state.accountD.account.balancesRoot,
  output.BALANCE_D_S_ADDRESS,
  state.accountD.balanceS,
  (output.BALANCE_D_S_BALANCE)
  )
* root_updateBalanceB_D = BalanceUpdate(
  root_updateBalanceS_D,
  output.BALANCE_D_B_ADDRESS,
  state.accountD.balanceB,
  (output.BALANCE_D_B_BALANCE)
  )
* root_updateBalanceD_Fee = BalanceUpdate(
  root_updateBalanceB_D,
  output.BALANCE_D_Fee_ADDRESS,
  state.accountD.balanceFee,
  (output.BALANCE_D_Fee_BALANCE)
  )
* root_updateAccount_D = AccountUpdate(
  root_updateAccount_C,
  output.ACCOUNT_D_ADDRESS,
  state.accountD.account,
  (output.ACCOUNT_D_OWNER, output.ACCOUNT_D_PUBKEY_X, output.ACCOUNT_D_PUBKEY_Y,
  state.accountD.account.appKeyPublicKey.x, state.accountD.account.appKeyPublicKey.y,
  output.ACCOUNT_D_NONCE, 
  state.accountD.account.disableAppKeySpotTrade, state.accountD.account.disableAppKeyWithdraw, state.accountD.account.disableAppKeyTransferToOther,
  updateBalanceFee_D.result(), updateStorage_D_batch.getHashRoot())
  )
  )


* root_updateStorage_E_batch = BatchStorageEUpdateGadget(
  state.accountE,
  updateStorage_E.result(),
  )
* root_updateBalanceS_E = BalanceUpdate(
  state.accountE.account.balancesRoot,
  output.BALANCE_E_S_ADDRESS,
  state.accountE.balanceS,
  (output.BALANCE_E_S_BALANCE)
  )
* root_updateBalanceB_E = BalanceUpdate(
  root_updateBalanceS_E,
  output.BALANCE_E_B_ADDRESS,
  state.accountE.balanceB,
  (output.BALANCE_E_B_BALANCE)
  )
* root_updateBalanceE_Fee = BalanceUpdate(
  root_updateBalanceB_E,
  output.BALANCE_E_Fee_ADDRESS,
  state.accountE.balanceFee,
  (output.BALANCE_E_Fee_BALANCE)
  )
* root_updateAccount_E = AccountUpdate(
  root_updateAccount_D,
  output.ACCOUNT_E_ADDRESS,
  state.accountE.account,
  (output.ACCOUNT_E_OWNER, output.ACCOUNT_E_PUBKEY_X, output.ACCOUNT_E_PUBKEY_Y,
  state.accountE.account.appKeyPublicKey.x, state.accountE.account.appKeyPublicKey.y,
  output.ACCOUNT_E_NONCE, 
  state.accountE.account.disableAppKeySpotTrade, state.accountE.account.disableAppKeyWithdraw, state.accountE.account.disableAppKeyTransferToOther,
  updateBalanceFee_E.result(), updateStorage_E_batch.getHashRoot())
  )
  )


* root_updateStorage_F_batch = BatchStorageFUpdateGadget(
  state.accountF,
  updateStorage_F.result(),
  )
* root_updateBalanceS_F = BalanceUpdate(
  state.accountF.account.balancesRoot,
  output.BALANCE_F_S_ADDRESS,
  state.accountF.balanceS,
  (output.BALANCE_F_S_BALANCE)
  )
* root_updateBalanceB_F = BalanceUpdate(
  root_updateBalanceS_F,
  output.BALANCE_F_B_ADDRESS,
  state.accountF.balanceB,
  (output.BALANCE_F_B_BALANCE)
  )
* root_updateBalanceF_Fee = BalanceUpdate(
  root_updateBalanceB_F,
  output.BALANCE_F_Fee_ADDRESS,
  state.accountF.balanceFee,
  (output.BALANCE_F_Fee_BALANCE)
  )
* root_updateAccount_F = AccountUpdate(
  root_updateAccount_E,
  output.ACCOUNT_F_ADDRESS,
  state.accountF.account,
  (output.ACCOUNT_F_OWNER, output.ACCOUNT_F_PUBKEY_X, output.ACCOUNT_F_PUBKEY_Y,
  state.accountF.account.appKeyPublicKey.x, state.accountF.account.appKeyPublicKey.y,
  output.ACCOUNT_F_NONCE, 
  state.accountF.account.disableAppKeySpotTrade, state.accountF.account.disableAppKeyWithdraw, state.accountF.account.disableAppKeyTransferToOther,
  updateBalanceFee_F.result(), updateStorage_F_batch.getHashRoot())
  )
  )

* root_updateBalanceD_O = BalanceUpdate(
  state.operator.account.balancesRoot,
  output.BALANCE_O_D_ADDRESS,
  state.operator.balanceD,
  (output.BALANCE_O_D_BALANCE)
  )
* root_updateBalanceC_O = BalanceUpdate(
  root_updateBalanceD_O,
  output.BALANCE_O_C_ADDRESS,
  state.operator.balanceC,
  (output.BALANCE_O_C_BALANCE)
  )
* root_updateBalanceB_O = BalanceUpdate(
  root_updateBalanceC_O,
  output.BALANCE_O_B_ADDRESS,
  state.operator.balanceB,
  (output.BALANCE_O_B_BALANCE)
  )
* root_updateBalanceA_O = BalanceUpdate(
  root_updateBalanceB_O,
  output.BALANCE_B_S_ADDRESS,
  state.operator.balanceS,
  (output.BALANCE_O_A_BALANCE)
  )
* root_new = AccountUpdate(
  root_updateAccount_D,
  operatorAccountID,
  state.operator.account,
  (state.operator.account.owner, state.operator.account.publicKeyX, state.operator.account.publicKeyY, state.operator.account.appKeyPublicKeyX, state.operator.account.appKeyPublicKeyY, state.operator.account.nonce, state.operator.account.disableAppKeySpotTrade, state.operator.account.disableAppKeyWithdraw, state.operator.account.disableAppKeyTransferToOther, root_updateBalanceA_O)
  )

### Description

This gadget executes the required logic for the transaction (by executing the logic for each transactions type and then selecting the right output) and using the output of the transaction to do all shared and heavy operations: signature checking and Merkle tree updates. By sharing these operations between all transaction types the resulting circuit is much more efficient than if we would simply do these operations for all transactions types at all times (as the number of constraints produced would simply stack on top of each other).

To do this, all data that could be updated in any of the transactions is stored in a shared output data interface. We then always update all output data, even if it remains the same for a specific transaction.

## Universal statement

A valid instance of a Universal statement assures that given an input of:

- exchange: {0..2^NUM_BITS_ADDRESS}
- merkleRootBefore: {0..2^256}
- merkleRootAfter: {0..2^256}
- merkleAssetRootBefore: {0..2^256}
- merkleAssetRootAfter: {0..2^256}
- timestamp: {0..2^NUM_BITS_TIMESTAMP}
- protocolFeeBips: {0..2^NUM_BITS_PROTOCOL_FEE_BIPS}
- operatorAccountID: {0..2^NUM_BITS_ACCOUNT}
- depositSize: {0..2^NUM_BITS_TX_SIZE}
- accountUpdateSize: {0..2^NUM_BITS_TX_SIZE}
- withdrawSize: {0..2^NUM_BITS_TX_SIZE}

the prover knows an auxiliary input:

- transactions: Transaction[N]
- accountP: Account
- accountO: Account
- numConditionalTransactions: {0..2^32}

such that the following conditions hold:

- exchange_bits = exchange_packed
- merkleRootBefore_bits = merkleRootBefore_packed
- merkleRootAfter_bits = merkleRootAfter_packed
- merkleAssetRootBefore_bits = merkleAssetRootBefore_packed
- merkleAssetRootAfter_bits = merkleAssetRootAfter_packed
- timestamp_bits = timestamp_packed
- protocolFeeBips_bits = protocolFeeBips_packed
- operatorAccountID_bits = operatorAccountID_packed
- numConditionalTransactions_bits = numConditionalTransactions_packed
- depositSize = all deposit size
- accountUpdateSize = all account update size
- withdrawSize = all withdraw size

- for i in {0..N}: transactions[i] = Transaction(
  exchange,
  (i == 0) ? merkleRootBefore : transactions[i-1].root_new,
  timestamp,
  protocolFeeBips,
  operatorAccountID,
  (i == 0) ? 0 : transactions[i-1].output.NUM_CONDITIONAL_TXS
  )

- numConditionalTransactions = transactions[N-1].output.NUM_CONDITIONAL_TXS

- publicData = (
  exchange,
  merkleRootBefore,
  merkleRootAfter,
  merkleAssetRootBefore,
  merkleAssetRootAfter,
  timestamp,
  protocolFeeBips,
  numConditionalTransactions,
  operatorAccountID,
  depositSize,
  accountUpdateSize,
  withdrawSize,
  concat(for i in {0..N}: transactions[i].output.DA[0..29\*8], for i in {0..N}: transactions[i].output.DA[29\*8..68\*8])
  )
- publicInput = PublicData(publicData)

- hash = PoseidonHash_t3f6p51(
  publicInput,
  accountO.nonce
  )
- SignatureVerifier(accountO.publicKeyX, accountO.publicKeyY, hash, 1)

- root_P = UpdateAccount(
  transactions[N-1].root_new,
  0,
  (accountP.owner, accountP.publicKey.x, accountP.publicKey.y, accountP.appKeyPublicKey.x, accountP.appKeyPublicKey.y, accountP.nonce, accountP.disableAppKeySpotTrade, accountP.disableAppKeyWithdraw, accountP.disableAppKeyTransferToOther, accountP.balancesRoot, accountP.storageRoot),
  (accountP.owner, accountP.publicKey.x, accountP.publicKey.y, accountP.appKeyPublicKey.x, accountP.appKeyPublicKey.y, accountP.nonce, accountP.disableAppKeySpotTrade, accountP.disableAppKeyWithdraw, accountP.disableAppKeyTransferToOther, accountP.storageRoot)
  )
- root_O = UpdateAccount(
  root_P,
  operatorAccountID,
  (accountO.owner, accountO.publicKey.x, accountO.publicKey.y, accountO.appKeyPublicKey.x, accountO.appKeyPublicKey.y, accountO.nonce, accountO.disableAppKeySpotTrade, accountO.disableAppKeyWithdraw, accountO.disableAppKeyTransferToOther, accountO.balancesRoot, accountO.storageRoot),
  (accountO.owner, accountO.publicKey.x, accountO.publicKey.y, accountO.appKeyPublicKey.x, accountO.appKeyPublicKey.y, accountO.nonce + 1, accountO.disableAppKeySpotTrade, accountO.disableAppKeyWithdraw, accountO.disableAppKeyTransferToOther, accountO.balancesRoot, accountO.storageRoot)
  )
- merkleRootAfter = root_O.result()
- merkleAssetRootAfter = root_O.assetResult()

### Description

Batches multiple transactions together in a block. All public input is hashed to the single field element publicInput, this makes verifying the proof more efficient.

The operator needs to sign the block. This is required because the operator pays the protocol fees directly from his own account.

Here we finally apply the new balance root of the pool acccount (at accountID = 0) to the Merkle tree. All balance updates done while running over the transactions only updated the balance Merkle tree of the account, and so here we set it in the account so the main Merkle tree is updated.
