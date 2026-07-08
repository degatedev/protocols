> **:warning: Historical repository.** These are the smart contracts of DeGate's zkRollup
> orderbook DEX, which was sunset in 2025 and is no longer in operation.
> DeGate is now a self-custodial multichain crypto wallet: [degate.com](https://degate.com).

# DeGate Protocol using zkSNARKs

## About
Smart contracts of DeGate’s sunset zkRollup orderbook DEX (historical). DeGate is now a self-custodial multichain crypto wallet: degate.com

## Top Features

- Onchain data-availability (DEXes can opt out for even greater throughput & lower cost)
- ERC20 tokens and Ether are supported by default
- Onchain deposit + on-chain & offchain withdrawal support
- Support onchain and offchain order cancellation and order time-to-live settings
- Allow partial order matching (aka partial fill) and offchain order-scaling
- Multiple circuit permutations for different request batch sizes
- Use tokens/ether traded as trading fee
- A built-in mechanism to force DEX operators to fulfill duties in time
- Support a unique feature called Order Aliasing
- 100% secure for end users, even when DEX operators are evil
- Grid trading capabilities with automatic/customizable strategies through parameters control
- Batch spot trading significantly boosts protocol TPS and reduces transactional cost & fees
- Gas fees reduction for deposits via the transfer deposit feature
- and more...

## Etherscan.io Code Verification

You can run `truffle-flattener <solidity-files>` to flatten a file. For contracts with constructor arguments, use https://abi.hashex.org/ to generated the argument string. To verify on etherscan.io, you also need to learn which libraries are linked to each contract from the migration scripts.
