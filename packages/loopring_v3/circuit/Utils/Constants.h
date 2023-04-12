// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

namespace Loopring
{
    static const unsigned int TREE_DEPTH_STORAGE = 7;
    static const unsigned int TREE_DEPTH_ACCOUNTS = 16;
    // The number of tokens is expanded from 8 layers to 16 layers. 
    // The value here represents the number of layers of the tree, which is a 4-fork number
    // static const unsigned int TREE_DEPTH_TOKENS = 8;
    static const unsigned int TREE_DEPTH_TOKENS = 16;

    // After the number of tokens is expanded from layer 8 to layer 16, the maximum value of tokenid in calldata will become larger, 
    // so the original 68bytes cannot put down the existing data,
    // The maximum transaction will contain two tokenid values, so it will increase by 4 bytes. 
    // Due to the introduction of aggregation transaction, it is necessary to add another 2bytes, and finally 74bytes
    // static const unsigned int TX_DATA_AVAILABILITY_SIZE = 68;
    static const unsigned int TX_DATA_AVAILABILITY_SIZE = 83;

    static const unsigned int NUM_BITS_MAX_VALUE = 254;
    static const unsigned int NUM_BITS_FIELD_CAPACITY = 253;
    static const unsigned int NUM_BITS_AMOUNT = 96;
    // Degate is an open currency mode, and there will be many currencies with different circulation. 96bits is too small, 
    // so it needs to be expanded. Due to the calculation characteristics of the elliptic curve of the circuit, 
    // it only supports 252bits at most, taking the whole as 248
    static const unsigned int NUM_BITS_AMOUNT_MAX = 248;
    // At present, only recharge and withdrawal are modified, and spot transactions involve multiplication, which is not supported by 248
    static const unsigned int NUM_BITS_AMOUNT_DEPOSIT = 248;
    static const unsigned int NUM_BITS_AMOUNT_WITHDRAW = 248;
    static const unsigned int NUM_BITS_STORAGE_ADDRESS = TREE_DEPTH_STORAGE * 2;
    static const unsigned int NUM_BITS_ACCOUNT = TREE_DEPTH_ACCOUNTS * 2;
    // DEG-265 auto market
    // // DEG-170 auto market - config add
    // static const unsigned int NUM_BITS_AUTOMARKET_ADDRESS = TREE_DEPTH_AUTOMARKET * 2;
    // static const unsigned int NUM_BITS_AUTOMARKET_STORAGE_ADDRESS = TREE_DEPTH_AUTOMARKET_STORAGE * 2;
    static const unsigned int NUM_BITS_AUTOMARKET_LEVEL = 8;
    static const unsigned int NUM_BITS_TOKEN = TREE_DEPTH_TOKENS * 2;
    static const unsigned int NUM_BITS_STORAGEID = 32;
    // DEG-265 auto market
    // static const unsigned int NUM_BITS_AUTOMARKET_STORAGE_LEVEL = 8;
    static const unsigned int NUM_BITS_TIMESTAMP = 32;
    static const unsigned int NUM_BITS_NONCE = 32;
    static const unsigned int NUM_BITS_BIPS = 12; // ceil(log2(2**NUM_BITS_BIPS_DA * FEE_MULTIPLIER))
    static const unsigned int NUM_BITS_BIPS_DA = 6;
    static const unsigned int NUM_BITS_PROTOCOL_FEE_BIPS = 8;
    static const unsigned int NUM_BITS_TYPE = 8;
    static const unsigned int NUM_STORAGE_SLOTS = 16384; // 2**NUM_BITS_STORAGE_ADDRESS
    static const unsigned int NUM_MARKETS_PER_BLOCK = 16;
    // static const unsigned int NUM_BITS_TX_TYPE = 8;
    static const unsigned int NUM_BITS_TX_TYPE = 3;
    static const unsigned int NUM_BITS_TX_TYPE_FOR_SELECT = 5;
    static const unsigned int NUM_BITS_BATCH_SPOTRADE_TOKEN_TYPE = 2;
    static const unsigned int NUM_BITS_BATCH_SPOTRADE_TOKEN_TYPE_PAD = 6;
    static const unsigned int NUM_BITS_TX_SIZE = 16;
    static const unsigned int NUM_BITS_BIND_TOKEN_ID_SIZE = 5;
    static const unsigned int NUM_BITS_ADDRESS = 160;
    static const unsigned int NUM_BITS_HASH = 160;
    static const unsigned int NUM_BITS_BOOL = 8;
    static const unsigned int NUM_BITS_BIT = 1;
    static const unsigned int NUM_BITS_BYTE = 8;
    static const unsigned int NUM_BITS_FLOAT_31 = 31;
    static const unsigned int NUM_BITS_FLOAT_30 = 30;
    static const unsigned int NUM_BITS_MIN_GAS = 248;

    static const char *EMPTY_TRADE_HISTORY = "65927491675782344981534105642433692294864120547424810690492392975145903570"
                                            "90";
    static const char *MAX_AMOUNT = "79228162514264337593543950335"; // 2^96 - 1
    static const char *FIXED_BASE = "1000000000000000000";           // 10^18
    static const unsigned int NUM_BITS_FIXED_BASE = 60;              // ceil(log2(10^18))
    static const unsigned int FEE_MULTIPLIER = 50;

    static const unsigned int BATCH_SPOT_TRADE_MAX_USER = 6;
    static const unsigned int BATCH_SPOT_TRADE_MAX_TOKENS = 3;
    static const unsigned int ORDER_SIZE_USER_MAX = 4;
    static const unsigned int ORDER_SIZE_USER_A = 4;
    static const unsigned int ORDER_SIZE_USER_B = 2;
    static const unsigned int ORDER_SIZE_USER_C = 1;
    static const unsigned int ORDER_SIZE_USER_D = 1;
    static const unsigned int ORDER_SIZE_USER_E = 1;
    static const unsigned int ORDER_SIZE_USER_F = 1;

    static const char *LogDebug = "Debug";
    static const char *LogInfo = "Info";
    static const char *LogError = "Error";

    struct FloatEncoding
    {
        unsigned int numBitsExponent;
        unsigned int numBitsMantissa;
        unsigned int exponentBase;
    };
    // Normal float32, 7 bits represent significant digits, 25 bits represent exponent, and decimal
    static const FloatEncoding Float32Encoding = {7, 25, 10};
    // Subtract the float31 of 1 bits, 7 bits represent the significant number, 24 bits represent the index, and decimal. 
    // Although the index is less than 1 bit, the significant number is also 7, and only the maximum value is less than float32
    // Float31 is mainly used for aggregation transaction publicdata, because it needs to represent positive and negative numbers, so 1 bit is left out
    static const FloatEncoding Float31Encoding = {7, 24, 10};
    static const FloatEncoding Float30Encoding = {5, 25, 10};
    static const FloatEncoding Float29Encoding = {5, 24, 10};
    static const FloatEncoding Float24Encoding = {5, 19, 10};
    static const FloatEncoding Float23Encoding = {5, 18, 10};
    static const FloatEncoding Float16Encoding = {5, 11, 10};

    struct Accuracy
    {
        unsigned int numerator;
        unsigned int denominator;
    };
    static const Accuracy Float32Accuracy = {10000000 - 2, 10000000};
    static const Accuracy Float24Accuracy = {100000 - 2, 100000};
    static const Accuracy AutoMarketCompleteAccuracy = {10000 - 1, 10000};
    static const Accuracy AutoMarketAmountAccuracy = {10000 - 1, 10000};
    static const Accuracy Float16Accuracy = {1000 - 5, 1000};

} // namespace Loopring

#endif