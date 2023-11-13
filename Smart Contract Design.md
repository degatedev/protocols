# Smart Contract Design

# Glossary
| Terms |      Description        |
| :--------:|:-------------------|
|Asset Merkle Tree	|The asset Merkle tree is a Merkle tree that stores all important data of DeGate's accounts and assets.	|
|Asset Private Key	|Asset private key refers to the private key in the asset key pair.	|
|Asset Public Key	|Asset public key refers to the public key in the asset key pair.	|
|Block Submission	|Block submission is a process where the Operator submits the zero-knowledge proof of a zkBlock to the DeGate smart contract via the Postman.	|
|Block-Producing Node	|The block-producing node is responsible for completing the zk-rollup process of DeGate off-chain transactions which includes sequencing transactions, batching, producing blocks, and generating data such as zero-knowledge proofs to be posted on-chain.	|
|Calldata|When a smart contract function is called externally (the call is not initiated within the contract), the binary input data of the transaction call is stored in calldata. All off-chain account and asset data of DeGate protocol are sent to DeGate smart contracts by calldata.	|
|Circuit|A circuit is a system component that defines activities that require zero-knowledge proofs. It also accepts a zkBlock as an input and outputs the respective zero-knowledge proof.	|
|DeGate Account	|DeGate account is used to track portfolio assets, place orders, facilitate deposits, withdrawals, and transfers in DeGate protocol.	|
|Deposit|The deposit function facilitates the transfer of tokens or ETH from wallet address into a DeGate account.	|
|Advanced Deposit	|Users utilize the Deposit function in DeGate's smart contract to perform asset deposits.	|
|Standard Deposit	|Users perform asset deposits by transferring their assets directly to DeGate's smart contract address.	|
|Entire Merkle Tree|The entire Merkle tree is a Merkle tree that stores all data of DeGate's accounts, assets, and orders.	|
|Exodus Mode	|Exodus mode allows users to retrieve assets in their DeGate account without the involvement of any third party. When the exodus mode is activated, DeGate smart contract rejects receiving new zkBlock data (all off-chain activities will cease). DeGate protocol will only support the withdrawal of user assets back to their wallet address.	|
|Force Withdrawal	|Force withdrawal is a smart contract function that allows users to initiate a withdrawal request that forces DeGate node(s) to execute the request within a stipulated timeframe.|
|Gas Fee	|Gas fee is the amount of computational effort gas, required to execute operations on the Ethereum network. In the current phase of the DeGate protocol, ETH, USDC & USDT can be used to pay for the gas fee.	|
|Trading Fee	|A trading fee is charged each time a trade occurs and is fully paid by the taker order. An order can be charged multiple trading fees if it matches with multiple orders. Trading fee is the main source of revenue for the DeGate protocol.	|
|Off-chain Transaction	|When DeGate node processes some user requests, off-chain transactions are generated. These transactions will be submitted on-chain via ZK-rollup.	|
|On-chain Order Cancellation	|On-chain order cancellation is a feature that is designed to increase trustlessness in the DeGate protocol. It submits the order cancellation instruction for canceled order that has an active order validity period to the blockchain which ensures the order can never be executed again.	|
|Operator|The operator has the core role in the block-producing node that is responsible for sequencing transactions, performing batching, producing blocks, and calling other modules to complete the zk-rollup process.	|
|Postman|Postman is responsible for calling DeGate smart contract and passing calldata to the smart contract which uploads data on-chain.	|
|Processing Fee for Deposit	|Processing fee for deposit only occurs and is needed when users have used up the number of free deposits (subsidized by DeGate protocol) and will be required to pay for the gas fees to process the deposit request.	|
|Shutdown Mode	|Shutdown mode can be activated by the DeGate protocol which returns all assets to users.	|
|Trading Fee	|A trading fee is charged each time a trade occurs and is fully paid by the taker order. An order can be charged multiple trading fees if it matches with multiple orders. Trading fee is the main source of revenue for the DeGate protocol.	|
|Transfer|The transfer function facilitates the transfer of assets between 2 DeGate accounts.	|
|Withdraw|The withdrawal function facilitates the transfer of assets from a user's DeGate account back to their wallet address.	|
|ZKP-Worker|ZKP-Worker generates zero-knowledge proofs through the circuit based on the data passed by the operator.|


# Introduction

This documentation briefly explains the design and implementation of DeGate smart contracts.

# Contract Overview

DeGate smart contract receives zkBlock from the operator and validates the block using zk-SNARKs. It then modifies the contract state if the block is valid.

Users interact with DeGate contracts through interfaces such as `deposit`, `withdraw`, and `registerToken`.

## zkBlock Validation

The operator invokes the `submitBlocks` function through the Postman account, this function will validate blocks by the following rules:

### Entire Merkle Tree And Asset Merkle Tree Validation

The state of users in DeGate is composed of two parts: one is the root of the Entire Merkle Tree that keeps user account info such as public key; and the other is the root of the Asset Merkle Tree that keeps user asset info. Both the old roots and the new roots of the two trees are included in the header of zkBlock. Before the tree root in the smart contract is updated to the new one carried in current block, the smart contract will verify that the current tree root stored in the contract is the same as the root in parent block. 

Another role of the Asset Merkle Tree is to guarantee trustlessness: When DeGate shifts into Exodus Mode, the user could submit merkle proof of Asset Merkle Tree to withdraw his assets directly from Layer 1.

### Timestamp Validation

The header of zkBlock also embeds a timestamp as an input to the circuit, which is used by the smart contract to prevent an outdated block from being accepted. Operators deliver zkBlock attached with this timestamp to the smart contract, as the circuit is not able to verify the timestamp itself. The smart contract in DeGate will compare the timestamp in zkBlock with the timestamp on Layer 1 (also known as `TimestampL1`), thus ensuring the zkBlock is in an effective time range.

The timestamp range is defined as follows(`TIMESTAMP_HALF_WINDOW_SIZE_IN_SECONDS` is configured as 7 days):
```
[TimestampL1 - TIMESTAMP_HALF_WINDOW_SIZE_IN_SECONDS,TimestampL1 + TIMESTAMP_HALF_WINDOW_SIZE_IN_SECONDS ]
```

### Transactions Validation & Execution

Specific types of transactions in zkBlock would be processed in the smart contract, like `Deposit`, `Withdrawal`, and `AccountUpdate`. These transactions are also referred to as conditional transactions.

The validaty of conditional transactions is enforced by both zk-SNARKs and the smart contract. The smart contract will process these transactions in a specific order; If any transaction fails, the whole zkBlock reverts. Refer to subsequent sections for more detail on this process.

### Zk-Proofs Validation

A zk-SNARKs key pair is composed of a proving key and a verifying key. The proving key is used to generate proofs, while the verifying key is used for verifying proofs. Refer to "trusted setup" for more details about how these keys are generated.

On the off-chain side, the operators generate the proof through the circuit and proving key; On the on-chain side, the smart contract selects the corresponding verifying key by zkBlock size to verify proof. Different zkBlock sizes correspond to different verifying keys, and the keys are hardcoded into the smart contracts. One zk-SNARKs key pair is unique to one circuit, meaning that changing keys will result in changing circuits. 


Zk proofs are generated by operators, embedded inside zkBlock, then verified by smart contract using a hardcoded verifying key. To generate zero-knowledge proofs, DeGate takes Groth16 for zk-SNARKs, which is based on the ALT_BN128 curve.  There are several precompiled opcodes ( ecAdd/ecMul/ecPairing, added in EIP-196 and EIP-197) for this curve that make verification efficient.


The verification process takes the following arguments as input: the verifying key, the zk proof, and the PublicInputDataHash which is the hash of the zkBlock. During verification, a "pair" is constructed from inputs, and an ecPairing process is executed to verify the constructed pair.

DeGate uses the verifier contract from ethsnarks to verify zk proofs.

## Deposit

Users could deposit their funds by transferring to DeGate, or by invoking the related smart contract directly.

### Standard Deposit

For those whitelisted tokens, the user can deposit funds by transferring assets to DeGate. This is also referred to as `standard deposit`. In this way, the user doesn't have to approve DeGate beforehand, compared with the smart contract way.

To verify such transfers, DeGate requires that the unconfirmed token balance is larger or equal to the balance deposited. For this fund, operator can deposit to any account, so users need to trust that operator will do the right thing. Anyone could view the unconfirmed balance by calling the `getUnconfirmedBalance` function.


### Advanced Deposit

Users could also deposit any of their assets to the smart contract by invoking the `deposit` function, known as `advanced deposit`. The user should grant DeGate authority to operate their funds first if the deposited token conforms to the ERC20 standard.

The Operator should process these deposits within `MAX_AGE_DEPOSIT_UNTIL_WITHDRAWABLE`(15 days), otherwise, users could withdraw their funds by invoking the `withdrawFromDepositRequest` function.
 
### User Deposit Process
Users can initiate a deposit transaction on the chain. 

After the transaction is entered into a block and confirmed by the chain, the DeGate node will top up the deposit amount into the account's available balance, and the user can immediately use this asset. At the same time, the Operator will initiate an off-chain transaction to confirm the deposit. After a series of operations, including packaging, block generation, generation of proofs, and updating Merkle tree assets, the Operator will finally rollup this proof to the chain to complete the deposit.

For deposit process and method comparison, please refer to DeGate product documentation for details https://docs.degate.com/v/product_en/main-features/deposit

### Fees

In normal cases, there is no fee charged for depositing. However, if there are more deposit transactions than the threshold within a specific time window, fees will be charged.

The maximum number of transactions without fees can be seen from the `getFreeDepositRemained` function. By default, the threshold is set to 5000, with 2 transactions recovered for each block.

For those deposits with fees charged, users should have a specific amount of ETH attached to their deposit transactions, 0.01ETH by default. The transaction will be declined without this value.

## Withdrawal

Users can withdraw funds in one of the following ways:

### Normal Withdrawal

The user usually does not have to interact with the smart contract on Layer 1, instead, the user withdraws funds by sending a dedicated withdrawal transaction to a DeGate server. The Operator will process these transactions when submitting zkBlock.


### Force Withdrawal

Users could also request the operator to return the fund by invoking the `forceWithdraw` interface. The operator must process the requests within `MAX_AGE_FORCED_REQUEST_UNTIL_WITHDRAW_MODE`(15 days), otherwise, anyone could shift DeGate into Exodus Mode by invoking the `notifyForcedRequestTooOld` interface.

Force withdrawal requires the user to pay an extra fee that by default is 0.01 ETH and does not exceed 0.25 ETH. The user should also specify the recipient address, token address, and the correct accountID. Since the parameters can be provided by malicious users, and the smart contract does not have the necessary user info like user accounts and balances, the operator has to submit the parameters with the zk proof, and the smart contract will remove the invalid withdrawal requests.

To prevent users from sending too many force withdrawal requests, DeGate does not allow more than `MAX_OPEN_FORCED_REQUESTS`(1000000, set in the smart contract) requests to be sent. The value is configured as 1000000 since DeGate takes into account both the operator capability and the lowest force withdrawal fees.

If there are too many force withdrawal transactions, the DeGate system will not be able to handle them and will be forced to enter the exodus mode. The setting of 1,000,000 requests is reasonable after evaluating the processing capability of DeGate.

Since malicious users consume too many transactions, resulting in ordinary users being unable to use DeGate, the DeGate system will adjust reasonable force withdrawal fees according to the actual situation to increase the attacker's cost, making the attack economically unfeasible.


### Exodus Mode
DeGate will pause its services when it enters into Exodus Mode, where the operator cannot submit zk blocks. Users can only withdraw their assets based on the last block. To withdraw funds in Exodus Mode, you can invoke `withdrawFromMerkleTree` with a merkle proof from Asset Merkle Tree. Each call will return the remaining balance of the token to the user. 

### User Withdrawal Process
Users can initiate withdrawal requests off-chain anytime to retrieve assets from the DeGate smart contract. 

Users sign and initiate a withdrawal request. After the node verifies the signature, it first locks the part of the amount that needs to be withdrawn from the available balance of the asset and then hands it to the Operator for the rollup operation. When the rollup transaction containing the withdrawal request is included in the block, users will receive the withdrawn assets on the chain. Users can retrieve the assets through forced withdrawal and exodus mode if the node refuses to process the withdrawal request.

For withdrawal process, please refer to DeGate product documentation for details https://docs.degate.com/v/product_en/main-features/withdrawal

## Token Registration
A token should be registered on DeGate before being transacted. Anyone can call the `registerToken` function to register. The smart contract ensures that each token address can be registered only once; For ETH and DG, they will be registered on deployment.

A token can also be registered automatically by the smart contract on deposit if it was not registered before. 

Each token address will be assigned to a token ID. IDs between 0 and 31, named `BindToken`, are reserved by DeGate and can only be registered by the admin account in DeGate. BindTokens benefit from lower fees when used in aggregated transactions.

## Protocol Fees
When using DeGate, users may need to pay different fees depending on the operation of different scenarios.

For scenarios and fees，please refer to DeGate product documentation for details https://docs.degate.com/v/product_en/concepts/protocol-fees

## Delayed Upgradable of Smart Contracts 
In order to enhance product functionality and safety, products can evolve and improve sustainably, DeGate smart contracts deployment adopts a delayed upgradable scheme. In the scheme, the admin rights to upgrade the contract are jointly managed by the Timelock contract and HomeDAO multi-signature account. When an upgrade occurs, DeGate users have at least 45 days to review the upcoming transaction and take action to decide whether to continue using DeGate, thus ensuring the highest level of trustless.

# Contract Implementation
## Overview
Contracts in DeGate  are made up of the following modules:

- Exchange: Handles zkBlock, as well as deposit and withdrawal requests.
- Deposit: Manages users' funds and transfers.
- Loopring: Manages configurations in Exchange.
- BlockVerifier: Manages zk-SNARKs keys and verifies zk proof.

## Exchange Smart Contract

### zkBlock Data Definition

The Operator uses the Postman account to submit zkBlock by invoking the `submitBlocks` interface.

```
function submitBlocks(Block[] zkBlocks)
```

The data structure of zkBlock:

```
struct Block
{
    uint8      blockType;
    uint16     blockSize;
    uint8      blockVersion;
    bytes      data;
    uint256[8] proof;
     
    bool storeBlockInfoOnchain;  // not used
    bytes auxiliaryData;
    bytes offchainData; // not used
}
```

- blockType: Type of zkBlock, currently set as 0 since there is only one type.
- blockSize: Number of transactions contained inside the block, ranging in [355, 300, 250, 200, 150, 100, 50, 25, 10, 5]
- blockVersion: Version of zkBlock, 0 by default.
- data: This field is the main body in zkBlock, also named as PublicInputData. It contains two components: PublicInputData carrying transactions, and the BlockHeader. It is also associated with a zk proof, it will be hashed by sha256 to generate PublicInputDataHash as input to the circuit.
- proof: The zk-SNARKs proof to validate zkBlock.
- auxiliaryData: The smart contract also needs to process this data. It is not used as input to the circuit but is used for the conditional transaction. Please refer to Auxdata in conditional transactions for more details.

### BlockHeader In the Data Field Of zkBlock

BlockHeader is serialized to 167 bytes, structured as follows:

```
struct BlockHeader
{
    address exchange;
    bytes32 merkleRootBefore;
    bytes32 merkleRootAfter;
    bytes32 merkleAssetRootBefore;
    bytes32 merkleAssetRootAfter;
    uint32  timestamp;
    uint8   protocolFeeBips;
 
    uint32  numConditionalTransactions;
    uint32  operatorAccountID;
 
    uint16  depositSize;
    uint16  accountUpdateSize;
    uint16  withdrawSize;
}
```
- exchange: Exchange contract address.
- merkleRootBefore: Merkle root of Entire Merkle Tree before the zkBlock is accepted.
- merkleRootAfter: Merkle root of Entire Merkle Tree after the zkBlock is accepted.
- merkleAssetRootBefore: Merkle root of Asset Merkle Tree before the zkBlock is accepted.
- merkleAssetRootAfter: Merkle root of Asset Merkle Tree after the zkBlock is accepted.
- timestamp: Timestamp passed to the circuit.
- protocolFeeBips: Maximum amount of fee allowed in spot trades. The denominator is 10,000, so if the fee is set as 18, the actual fee is 18/10000.
- numConditionalTransactions: The total number of transactions the contract should deal with, that is, depositSize + accountUpdateSize + withdrawSize. 
- operatorAccountID: The AccountID of operator.
- depositSize: Number of Deposit requests in this zkBlock.
- accountUpdateSize: Number of Account Update requests in this zkBlock.
- withdrawSize: Number of Withdrawal requests in this zkBlock.

### zkBlock PublicInputData Definition

All conditional transactions in zkBlock are serialized into PublicInputData. Deserializing the binary should respect to specific number and order:

- The count of each type of transaction is defined in BlockHeader, as depositSize/accountUpdateSize/withdrawSize implies above. 
- Transactions are put in the order that Deposit transactions come first, then comes AccountUpdate and Withdraw. The smart contract can infer each transaction type based on the above information. 
[Deposit transactions][AccountUpdate transactions][…others…][Withdraw transactions]
- Each transaction in PublicInputData occupies 83 bytes, the concrete structure of which is defined in the next section.

### Conditional Transactions
The transactions processed in the smart contract including deposit, withdrawal, and accountUpdate are named conditional transactions, each of which is made up of PublicInputData and AuxData.


#### PublicInputData
The PublicInputData part of a transaction occupies 83 bytes with zero bytes padding if needed. To save gas, a transaction's PublicInputData splits into two parts: 
- part1: 80 bytes
- part2: 3 bytes

The data field of Block is arranged such that all of the part 1 of transactions are put together before part 2. Hence, parsing a transaction means reading part 1 and part 2 separately and putting the pieces together into 83 bytes.

#### AuxData

Each AuxData in each transaction is an AuxiliaryData object. All AuxDatas are stored in the AuxiliaryData array, which is serialized into auxiliaryData of zkBlock.

```
struct AuxiliaryData
{
    bytes data;
}
```

To read AuxData, process the data in the corresponding order. Also note that deposit transactions do not have AuxData, to make it easier to parse, AuxData in deposit transaction is set to empty data ("0x").


The smart contract will also verify that length of the AuxiliaryData array must be equal to the number of conditional transactions.


### Deposit Transaction
Deposit transactions handle users' deposit requests.

PublicInputData of deposit transaction is defined as:

```
- Type: 1 byte (0 for `Advanced Deposit`, 1 for `Standard Deposit`)
- Owner: 20 bytes
- Account ID: 4 bytes
- Token ID: 4 bytes
- Amount: 31 bytes
```

* Type: Type = 0 for [Advanced Deposit](#advanced-deposit)，Type =1 for [Standard Deposit](#standard-deposit).
* Owner: User address on Layer 1.
* Account ID: The id representing the user account in DeGate.
* Token ID: The id representing the token in DeGate.
* Amount: An uint248 value representing the amount of token value to deposit.


The smart contract reconstructs the transaction by decoding the PublicInputdata and processes the depositing request according to the `Type` field.

Standard Deposit:

1. UnconfirmedBalance should be not less than the depositing amount in the deposit request.
2. Since the smart contract has no way to know the original user who is depositing, DeGate only needs to confirm the amount is valid.
3. Unconfirmed balance can be queried by the `getUnconfirmedBalance` interface.


Advanced Deposit:

1. Make sure the pending request the `pendingDeposit` exists
2. Make sure the amount in the pending deposit is larger or equal to that being deposited
3. If the ending balance is not fully spent, then the pending deposit would be updated and saved; Otherwise, the pending deposit request is discarded.
4. Balance of pending deposit can be queried by the `getPendingDepositAmount` interface.


### Account Update transaction

The AccountUpdate transaction allows users to update their asset public key.

Definition of PublicInputData of AccountUpdate transaction:
```
- Type: 1 byte (type = 1 for a conditional transaction)
- Account owner: 20 bytes
- Account ID: 4 bytes (for contract verification)
- Fee token ID: 4 bytes
- Fee amount: 2 bytes （16 bits, 11 bits for the mantissa part, and 5 for the exponent part)
- Public key: 32 bytes
- Nonce: 4 bytes
- Account ID: 4 bytes (appointed accountID by DeGate, used for crawling blocks to restore the Asset Merkle Tree)
```

* Type: This field is set to 1 since only ECDSA-based signature is supported.
* Account owner: User address on Layer 1.
* Account ID: The id representing the user account in DeGate.
* Fee token ID: id of the token in DeGate used for charging fees.
* Fee amount: Amount of fees to charge
* PublicKey: The public key of the asset, in EdDSA format.
* Nonce: The nonce value is to prevent replay attacks and it will increase by one each time it is used.
* Account ID: Preallocated accountID to recover Asset Merkle Tree.


Definition of AuxData:

```
struct AccountUpdateAuxiliaryData
{
    bytes  signature;
    uint96 maxFee;
    uint32 validUntil;
}
```
* signature: This ECDSA signature is signed by the user using the private key on Layer 1 following the EIP712 standard.

Data to sign:
    ```
    AccountUpdate {
      owner (address)
      accountID (uint32)
      feeTokenID (uint32)
      maxFee (uint96)
      publicKeyX (uint256)
      validUntil (uint32)
      nonce (uint32)
    }
    ```
* maxFee: The max amount of fee to pay. It is part of the signature.
* validUntil: The timestamp that the transaction will expire on.

The smart contract decodes specific transaction fields based on PublicInputData and AuxData. Here is how validation works:

1. Timestamp passed to the circuit is less than validUntil.
2. Fee paid should be less than maxFee
3. The signature is valid.

### Withdraw Transaction
Withdraw transactions allow the user to withdraw the fund from L2 to L1.

Definition of PublicInputData for Withdraw Transaction:

```
- Type: 1 bytes (type > 0 for conditional withdrawals, type == 2 for a valid force withdrawal, type == 3 when invalid)
- Owner: 20 bytes
- Account ID: 4 bytes
- Token ID: 4 bytes
- Fee token ID: 4 bytes
- Fee amount: 2 bytes （16 bits, 11 bits for the mantissa part, and 5 for the exponent part)
- StorageID: 4 bytes
- OnchainDataHash: 20 bytes
```

* Type: There are four types  ( 0/1/2/3 ) of withdrawal transactions. Only types 1/2/3 are processed in the smart contract, as explained later.
* Owner: The receiver address in L1 specified by the user.
* Account ID: The id in DeGate representing the creator of the withdrawal transaction.
* Token ID: The id in DeGate representing the token to withdraw.
* Fee token ID: The id in DeGate representing the fee to pay.
* Fee amount: The amount of fee to be paid.
* StorageID: This field is to prevent replaying attacks as part of data to sign. It is also used to record that the withdrawal is completed.
* OnchainDataHash: It is the hash of (minGas, to, amount). These three fields are passed to the contract by auxData, rather than publicInput, due to the limit on the size of 83 bytes.

Definition of AuxData:

```
struct WithdrawalAuxiliaryData
{
    bool  storeRecipient;
    uint  gasLimit;
    bytes signature;
    
    uint248 minGas;
    address to;
    uint96  maxFee;
    uint32  validUntil;
    uint248 amount;
}
```
* storeRecipient: The withdrawals recorded on the blockchain.
* gasLimit: The upper bound of gas to pay in withdrawal operation.
* signature: The ECDSA signature from the user.
* minGas: The lower bound of gas to pay in withdrawal operation.
* to: The address the fund being returned to.
* maxFee: The upper bound of the fee to pay. The actual fee paid must be no larger than maxFee.
* amount: The amount of token value to withdraw.

The smart contract decodes specific transaction fields based on PublicInputData and AuxData. 

How it works:
1. Verify that onchainDataHash is correct.

```
bytes20 onchainDataHash = bytes20(
            abi.encodePacked(
                minGas,
                to,
                amount
            ).fastSHA256()
        );
```
2. Verify transactions by type.
    - type=0: The transaction is using the EdDSA signature scheme, and no validation is needed.
    - type=1: The transaction is using the ECDSA signature scheme, and the contract verifies that:
        - The timestamp passed to the circuit should be less than "validUntil".
        - "fee" is less or equal to "maxFee".
        - The signature is correct.
    - type=2: The transaction is a valid force withdrawal, and the contract verifies that:
        - "from" is equal to "to". The "from" address  initiates the forced withdrawal transaction on chain, and the "to" address is the address in Auxdata. Forced withdrawal transactions can only be withdrawn to the "from" address, so the "from" and "to" addresses need to be consistent.
        - "fee" must be zero.
        - If the forceWithdrawal request exists, then the owner of the request is consistent with the "from" address.
        - If the forceWithdrawal request does not exist, then DeGate must be in shutdown mode, and the operator will engage the force withdrawal on behalf of the user.
    - type=3: the transaction is an invalid force withdrawal, invalid means that the user has entered the wrong account ID for force withdrawal,  and the contract verifies that:
        - "from" is equal to "to". The "from" address is that which initiates the forced withdrawal transaction on chain, and the "to" address is the address in Auxdata. Forced withdrawal transactions can only be withdrawn to the "from" address, so the "from" and "to" addresses need to be consistent.
        - "fee" must be zero.
        - If the forceWithdrawal request exists, then the owner of the request must not be equal to the "from" address.
        - "amount" must be zero. In this case, no withdrawal request is processed, instead, the contract only clears the invalid force withdrawal requests.


3. Process withdrawal requests
The contract process withdrawal requests in the following steps:

    - The contract verifies that "gasLimit" in AuxData is larger or equal to "minGas".
    - call withdraw function in the deposit contract, which will transfer funds to the user.
    - Sometimes the withdrawal function of the deposit contract could fail to execute while the withdrawal transaction is successful. In this situation, the withdrawal request will be recorded to table the "amoutWithdrawable", and the transaction in submitBlocks will still be sent to Layer 1. Users can call the `withdrawFromApprovedWithdrawals` function to withdraw the fund from Layer 1 manually. 

The `withdrawFromApprovedWithdrawals` interface allows the user to withdraw funds manually as a backup strategy for withdrawal transaction failure.

```
function withdrawFromApprovedWithdrawals(address[] owners, address[] tokens)
```

The contract processes in the following steps:
1. Anyone can call this function, with a list of owner addresses and a list of token addresses passed to the function.
2. Process the request in according to parameters. The contract will check that "amountWithdrawable" table have a corresponding withdrawal request by the owner and token.
3. According to the balance recorded in table "amountWithdrawable", transfer funds to the user address.
4. Remove the withdrawal record in the table "amountWithdrawable".

### Validating And Updating Of Trading Fees
Different trading pairs have different trading fees, the validation of which is executed by the smart contract, rather than the circuit.

The max trading fees are verified and updated at the time when the operator submits zkBlock using the Postman account.

- How Validation Works
The "protocolFeeBips" field, included in BlockHeader of each submitted zkBlock, is the maximum trading fees allowed in spot trades. The circuit will constrain that all fees for spot trades do not exceed this value, and the contract will also check that the protocolFeeBips field included in BlockHeader is consistent with that configured in the contract, otherwise, the transaction is reverted.

- Delayed Updating
The fee rate in smart contracts can be modified with a delay of `MIN_AGE_PROTOCOL_FEES_UNTIL_UPDATED`(7 days). To update, the admin should call the `updateProtocolFeeSettings` function in the Loopring contract with a specified value passed to it. The modification will take effect after calling `validateAndSyncProtocolFees` function if there is a zkBlock to submit after the delay period expires.


### Function "submitBlocks"

The Postman account in the operator invokes the `submitBlocks` function to submit a zkBlock to Layer 1.

```
function submitBlocks(Block[] zkBlocks)
```

The contract processes this request in the following steps:

1. Verify that the merkle roots in zkBlock, including Entire Merkle Tree and Asset Merkle Tree, are correct 
2. Verify that the timestamp passed to the circuit is correct
3. Process conditional transactions
4. Verify and update max trading fee
5. Verify the zero knowledge proof by calling the `verifyProofs` interface in the BlockVerifier contract

### Function "deposit"

The `deposit` function allows users to deposit their funds.

```
function deposit(address from,address to,address tokenAddress,uint248 amount, bytes extraData)
```
How it works:
1. Verify that the token is registered and register the token if not.
2. Check whether the fee is required. If so, some fee(depositFeeETH)  will be charged from msg.value, and the transaction is reverted if there is not enough eth to deduct in this case.
3. Invoke the `deposit` function in the Deposit contract to transfer funds
4. Record the depositing request to the "pendingDeposits" table, and multiple depositing requests would be combined into one if they share the same token and user.
 
The depositing operation will not take effect until the Deposit Transaction is submitted into the blockchain by DeGate. If the operator is not responding, then the user could withdraw the fund from the contract directly by calling the `withdrawFromDepositRequest` function.

### Function "withdrawFromDepositRequest"

The `withdrawFromDepositRequest` function is defined as follows:

```
function withdrawFromDepositRequest(address owner, address token)
```

How it works:

1. Anyone can call it, with the user address as "owner" and token address as "token" as input arguments
2. Check that there is a corresponding deposit request in the "pendingDeposits" table.
3. Check that the deposit request is not processed for a configured time(`MAX_AGE_DEPOSIT_UNTIL_WITHDRAWABLE`), or DeGate enters into Exodus Mode.
4. Transfer the balance to the user according to the balance in the "pendingDeposits" table.
5. Delete the depositing request in the "pendingDeposits" table.

### Function "forceWithdraw"

The `forceWithdraw` function allows the user to force DeGate to return funds. 


```
function forceWithdraw(address from, address token, uint32 accountID)
```

Notes:
1. Check that there are available slots.
2. Check that the eth attached is sufficient to pay for the fee.
3. Save the force withdrawal request to the "pendingForcedWithdrawals" table.

The actual withdrawal would not finish until the withdrawal transaction built by DeGate is submitted. If the operator does not process the request for a configured time(`MAX_AGE_FORCED_REQUEST_UNTIL_WITHDRAW_MODE`), then the user could trigger the exchange into Exodus Mode by calling `notifyForcedRequestTooOld`.


### Exodus Mode
#### Function "notifyForcedRequestTooOld"

Function `notifyForcedRequestTooOld` is triggered by the user whose forced withdrawal request has not been processed for a long time, causing the exchange to change to Exodus Mode.

```
function notifyForcedRequestTooOld(uint32 accountID, address token)
```
How it works:

1. Check that there exists a forced withdrawal request that matches (accountID and token).
2. Check that the withdrawal request already expired on `MAX_AGE_FORCED_REQUEST_UNTIL_WITHDRAW_MODE` (15 days).
3. Set the current timestamp as the start time of Exodus Mode.
4. There is a function `isInWithdrawalMode` to check whether the exchange is in Exodus Mode((internally, checks withdrawalModeStartTime > 0)



#### Function "withdrawFromMerkleTree"

The function `withdrawFromMerkleTree` is used to withdraw funds in Exodus Mode.

```
function withdrawFromMerkleTree(MerkleProof merkleProof)
```
1. Check that the exchange is in Exodus Mode.
2. Each invoke corresponds to each pair of (user, token), returning all remaining balance to the user.
3. Verifies the merkle proof.
4. Execute the withdrawal operation. All the remaining balance of the token to withdraw will be transferred to the user and marked as withdrawn.
5. The cost of this function is about 300w gas.

#### Validation Of Merkle Proof

The MerkleProof is defined as follows:

```
struct AccountLeaf
{
    uint32   accountID;
    address  owner;
    uint     pubKeyX;
    uint     pubKeyY;
    uint32   nonce;
}
struct BalanceLeaf
{
    uint32  tokenID;
    uint248 balance;
}
struct MerkleProof
{
    AccountLeaf accountLeaf;
    BalanceLeaf balanceLeaf;
    uint[48]                 accountMerkleProof;
    uint[48]                 balanceMerkleProof;
}
```
- accountLeaf: Account leaf node of Asset Merkle Tree
- balanceLeaf: Token leaf node of Balance Subtree.
- accountMerkleProof: The path from the leaf to the root of the Asset Merkle Tree. Since the tree is a quadtree with 16 levels, there are 3 * 16=48 nodes included.
- balanceMerkleProof: The path from the leaf to the root of the Balance Subtree. Since the tree is a quadtree with 16 levels, there are 3 * 16=48 nodes included.


Verification of MerkleProof:
```
function verifyAccountBalance(uint merkleRoot, MerkleProof merkleProof)
```

1. Get the hash of the leaf node of Asset Merkle Tree as hash (balance).
2. Split balanceMerkleProof to multiple groups, where each group representing one level has three nodes. Recursively calculate parent node by hashing current node with 3 other nodes in the group until the root of Balance Subtree (marked as balancesRoot) is reached.
3. Get the hash of the leaf node in the Account Tree as hash (owner, pubKeyX, pubKeyY, nonce, balancesRoot).
4. Split accountMerkleProof to multiple groups, where each group representing one level has three nodes. Recursively calculate the parent node by hashing the current node with 3 other nodes in the group until the root of Asset Merkle Tree (marked as accountsRoot) is reached.
5. Check that the accountsRoot rebuilt from merkle proof is equal to the root stored in the contract.

### Shutdown Mode
At any time, the admin can shift the exchange to shutdown mode by invoking the `shutdown` function. In this mode, users can still withdraw their funds as per normal, and the operator can also submit blocks.

The operator can return the user's fund on Layer 1 if the withdrawal operation has not been called by the user. In this mode, the operator only process withdrawal transaction, other types of the transaction will be ignored.


The shutdown mode can be triggered by the following function:

```
function shutdown()
```

How it works:
1. The exchange can not shift into shutdown mode if it is in Exodus Mode.
2. Set the start time of shutdown mode as the current timestamp.
3. Anyone can call the "isShutdown" query to check whether the exchange is in shutdown mode, which internally checks shutdownModeStartTime > 0.

## Deposit Contract

The Deposit contract, which is used by the Exchange contract, manages user funds. To make the fund safe, the Deposit contract only supports ETH and ERC20 tokens.


### Function "deposit"

```
function deposit(address from, address token, uint248 amount)
```

Notes:
1. It can only be called by Exchange contract.
2. If ETH is being deposited, the contract will check that msg.value >= amount and would refund the exceeding part.
3. If ERC20 is being deposited, the contract will call `transferFrom` to move funds from the user to the Deposit contract, before which the user should authorize the Deposit contract first.
4. If the user deposits by transfer rather than invoking the `deposit` function, the fund will be transferred to the Deposit contract directly.

### Function "withdraw"

```
function withdraw(to, token, amount)
```

Notes:
1. It can only be called by Exchange contract.
2. Move the fund to the address specified by the parameter "to".


### Non-Standard Tokens
DeGate does not support rebasing tokens like ampl whose balance will automatically inflate or deflate. DeGate also does not support those tokens that automatically burn on transfer(like cult DAO), but you can add it to the whitelist manually.


#### Function "setCheckBalance"
This function puts a token on the whitelist.
```
function setCheckBalance(address token, bool checkBalance)
```

How it works:

1. Add a token to the whitelist.
2. The amount to deposit is calculated based on the actual balance changes.

## Loopring Contract

Loopring contract is used to configure fees in forced withdrawal and max trading fee. This contract is based on the original Loopring protocol, which manages multiple Exchanges and the corresponding configurations, but in DeGate there is only one Exchange instance.


### Function "updateSettings"

This function updates the fee in forced withdrawal.
```
function updateSettings (uint forcedWithdrawalFee)
```
The function checks the argument by the following steps:

1. it will check that the fee passed(forcedWithdrawalFee) does not exceed `MAX_FORCED_WITHDRAWAL_FEE`（0.25 eth）

### Function "updateProtocolFeeSettings"

This function sets the max trading fee.
```
function updateProtocolFeeSettings(uint8 protocolFeeBips)
```
Notes:

1. This operation will take effect after a specific delay(currently 7 days). 



## BlockVerifier Contract

This contract is responsible for holding verifying keys and verifying zk proofs.


### Registration Of Verifying Keys
A group of verifying keys are hardcoded into VerificationKeys.sol. Each key corresponds to a specific block size, so the group of keys are associated with sizes [355, 300, 250, 200, 150, 100, 50, 25, 10, 5].

DeGate does not allow modification to these verifying keys once the contract is deployed.

### Function "verifyProofs"

function `verifyProofs` is used to verify zk proofs.

```
function verifyProofs(uint8 blockType, uint16 blockSize, uint8 blockVersion, uint[] publicInputs, uint[] proofs)
```
* blockType: Type of zkBlock. Currently set it to 0 as there is only one type.
* blockSize: The number of transactions contained, which is associated with the size of the corresponding circuit.
* blockVersion: Version of zkBlock. Currently set it to 0.
* publicInputs: Hash of data in zkBlock.
* proofs: Zk proof of zkBlock.

The function process the request in the following steps:
1. Choose the verifying key according to blockType/blockSize/blockVersion.
2. According to the number of proofs, call the `verify` interface in the third-party library.
    - For a single proof, verify the proof using [Verifier Contract](https://github.com/HarryR/ethsnarks/blob/master/contracts/Verifier.sol).
    - For a bunch of proofs, verify the proofs using [BatchVerifier Contract](https://github.com/matter-labs-archive/Groth16BatchVerifier/blob/master/BatchedSnarkVerifier/contracts/BatchVerifier.sol)

About gas cost for verification :
- For a single proof, since batch verification has a constant gas cost, it would be more efficient to use the Verifier contract to verify a single zk proof.
- For multiple proofs, it is more efficient to verify by batch.
- It would take 18.7w gas to verify a single proof, while the cost of verifying a batch of proofs would grow linearly with the number of proofs(n), costing 4.6w* n+ 15.3w gas.

## About Trustless
User doesn't have to make any trust assumptions when using the DeGate contract, as "trustless" is guaranteed by the following factors:

- Contracts and circuits are upgradable with a delay. DeGate users have at least 45 days to review upcoming transactions and take action in advance. 
- Forced withdrawal can be made by users to force the operator to process the withdrawal request, otherwise, the exchange will shift into Exodus Mode.
- Users can rebuild the whole Asset Merkle Tree by crawling Calldata in blocks.
- In Exodus Mode, users can submit merkle proof of Asset Merkle Tree to withdraw funds from Layer 1 directly.

