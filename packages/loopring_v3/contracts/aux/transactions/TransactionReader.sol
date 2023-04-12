// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
pragma solidity ^0.7.0;
pragma experimental ABIEncoderV2;

import "../../core/impl/libtransactions/BlockReader.sol";

import "../../core/impl/libtransactions/DepositTransaction.sol";
import "../../core/impl/libtransactions/WithdrawTransaction.sol";

/// @title TransactionReader
/// @author Brecht Devos - <brecht@loopring.org>
/// @dev Utility library to read transactions.
library TransactionReader {
    using BlockReader       for bytes;
    using TransactionReader for ExchangeData.Block;
    using BytesUtil         for bytes;

    function readDeposit(
        ExchangeData.Block memory _block,
        uint txIdx,
        bytes memory txData
        )
        internal
        pure
        returns (DepositTransaction.Deposit memory deposit)
    {
        _block.readTx(txIdx, txData);
        DepositTransaction.readTx(txData, 0, deposit);
    }

    function readWithdrawal(
        ExchangeData.Block memory _block,
        uint txIdx,
        bytes memory txData
        )
        internal
        pure
        returns (WithdrawTransaction.Withdrawal memory withdrawal)
    {
        _block.readTx(txIdx, txData);
        WithdrawTransaction.readTx(txData, 0, withdrawal);
    }

    function readTx(
        ExchangeData.Block memory _block,
        uint txIdx,
        bytes memory txData
        )
        internal
        pure
    {
        _block.data.readTransactionData(txIdx, _block.blockSize, txData);
    }

    function readTxs(
        ExchangeData.Block memory _block,
        uint                      txIdx,
        uint16                    numTransactions,
        bytes              memory txsData
        )
        internal
        pure
    {
        bytes memory txData = txsData;
        uint TX_DATA_AVAILABILITY_SIZE = ExchangeData.TX_DATA_AVAILABILITY_SIZE;
        for (uint i = 0; i < numTransactions; i++) {
            _block.data.readTransactionData(txIdx + i, _block.blockSize, txData);
            assembly {
                txData := add(txData, TX_DATA_AVAILABILITY_SIZE)
            }
        }
    }
}
