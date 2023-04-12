// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
pragma solidity ^0.7.0;

import "../../lib/AddressUtil.sol";
import "../../lib/ERC20SafeTransfer.sol";
import "../../lib/MathUint.sol";
import "../../lib/ReentrancyGuard.sol";
import "../iface/IExchangeV3.sol";
import "../iface/ILoopringV3.sol";


/// @title LoopringV3
/// @dev This contract does NOT support proxy.
/// @author Brecht Devos - <brecht@loopring.org>
/// @author Daniel Wang  - <daniel@loopring.org>
contract LoopringV3 is ILoopringV3, ReentrancyGuard
{
    using AddressUtil       for address payable;
    using MathUint          for uint;
    using ERC20SafeTransfer for address;

    address public immutable override lrcAddress;

    // -- Constructor --
    constructor(
        address _lrcAddress,
        address payable _protocolFeeVault,
        address _blockVerifierAddress
        )
        Claimable()
    {
        require(address(0) != _lrcAddress, "ZERO_ADDRESS");

        lrcAddress = _lrcAddress;

        updateSettingsInternal(_protocolFeeVault, _blockVerifierAddress, 0);
    }

    // == Public Functions ==
    function updateSettings(
        address payable _protocolFeeVault,
        address _blockVerifierAddress,
        uint    _forcedWithdrawalFee
        )
        external
        override
        nonReentrant
        onlyOwner
    {
        updateSettingsInternal(
            _protocolFeeVault,
            _blockVerifierAddress,
            _forcedWithdrawalFee
        );
    }

    function updateProtocolFeeSettings(
        uint8 _protocolFeeBips
        )
        external
        override
        nonReentrant
        onlyOwner
    {
        protocolFeeBips = _protocolFeeBips;

        emit SettingsUpdated(block.timestamp);
    }

    function getExchangeStake(
        address exchangeAddr
        )
        public
        override
        view
        returns (uint)
    {
        return exchangeStake[exchangeAddr];
    }

    function burnExchangeStake(
        uint amount
        )
        external
        override
        nonReentrant
        returns (uint burnedLRC)
    {
        require(amount > 0, "ZERO_VALUE");

        burnedLRC = exchangeStake[msg.sender];

        if (amount < burnedLRC) {
            burnedLRC = amount;
        }
        if (burnedLRC > 0) {
            lrcAddress.safeTransferAndVerify(protocolFeeVault, burnedLRC);
            exchangeStake[msg.sender] = exchangeStake[msg.sender].sub(burnedLRC);
            totalStake = totalStake.sub(burnedLRC);
        }
        emit ExchangeStakeBurned(msg.sender, burnedLRC);
    }

    function depositExchangeStake(
        address exchangeAddr,
        uint    amountLRC
        )
        external
        override
        nonReentrant
        returns (uint stakedLRC)
    {
        require(amountLRC > 0, "ZERO_VALUE");

        lrcAddress.safeTransferFromAndVerify(msg.sender, address(this), amountLRC);

        stakedLRC = exchangeStake[exchangeAddr].add(amountLRC);
        exchangeStake[exchangeAddr] = stakedLRC;
        totalStake = totalStake.add(amountLRC);

        emit ExchangeStakeDeposited(exchangeAddr, amountLRC);
    }

    function withdrawExchangeStake(
        address recipient,
        uint    requestedAmount
        )
        external
        override
        nonReentrant
        returns (uint amountLRC)
    {
        require(requestedAmount > 0, "ZERO_VALUE");

        uint stake = exchangeStake[msg.sender];
        amountLRC = (stake > requestedAmount) ? requestedAmount : stake;

        if (amountLRC > 0) {
            lrcAddress.safeTransferAndVerify(recipient, amountLRC);
            exchangeStake[msg.sender] = exchangeStake[msg.sender].sub(amountLRC);
            totalStake = totalStake.sub(amountLRC);
        }

        emit ExchangeStakeWithdrawn(msg.sender, amountLRC);
    }

    function getProtocolFeeValues()
        public
        override
        view
        returns (
            uint8 feeBips
        )
    {
        return protocolFeeBips;
    }

    // == Internal Functions ==
    function updateSettingsInternal(
        address payable  _protocolFeeVault,
        address _blockVerifierAddress,
        uint    _forcedWithdrawalFee
        )
        private
    {
        require(address(0) != _protocolFeeVault, "ZERO_ADDRESS");
        require(address(0) != _blockVerifierAddress, "ZERO_ADDRESS");
        require(_forcedWithdrawalFee <= ExchangeData.MAX_FORCED_WITHDRAWAL_FEE, "INVALID_FORCED_WITHDRAWAL_FEE");

        protocolFeeVault = _protocolFeeVault;
        blockVerifierAddress = _blockVerifierAddress;
        forcedWithdrawalFee = _forcedWithdrawalFee;

        emit SettingsUpdated(block.timestamp);
    }
}
