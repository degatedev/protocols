#!/bin/sh

echo sol-preprocess $1

if [ "$1" = "test" ]
then
    npx solpp --no-flatten contracts-solpp/IBlockVerifier.solpp  -o contracts/core/iface/IBlockVerifier.sol -D Enable_Test_Functions
    npx solpp --no-flatten contracts-solpp/BlockVerifier.solpp  -o contracts/core/impl/BlockVerifier.sol -D Enable_Test_Functions

    npx solpp --no-flatten contracts-solpp/IExchangeV3.solpp  -o contracts/core/iface/IExchangeV3.sol -D Enable_Test_Functions
    npx solpp --no-flatten contracts-solpp/ExchangeV3.solpp  -o contracts/core/impl/ExchangeV3.sol -D Enable_Test_Functions

    npx solpp --no-flatten contracts-solpp/ExchangeData.solpp  -o contracts/core/iface/ExchangeData.sol -D Enable_Test_Functions
else
    npx solpp --no-flatten contracts-solpp/IBlockVerifier.solpp  -o contracts/core/iface/IBlockVerifier.sol
    npx solpp --no-flatten contracts-solpp/BlockVerifier.solpp  -o contracts/core/impl/BlockVerifier.sol

    npx solpp --no-flatten contracts-solpp/IExchangeV3.solpp  -o contracts/core/iface/IExchangeV3.sol
    npx solpp --no-flatten contracts-solpp/ExchangeV3.solpp  -o contracts/core/impl/ExchangeV3.sol

    npx solpp --no-flatten contracts-solpp/ExchangeData.solpp  -o contracts/core/iface/ExchangeData.sol
fi
