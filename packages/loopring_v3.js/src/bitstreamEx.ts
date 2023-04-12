import { BigNumber } from "bignumber.js";
import BN from "bn.js";
const abi = require("ethereumjs-abi");

export class BitstreamEx {
    private data: string;
  
    constructor() {
        this.data = ""
    }

    public addNumberForBits(x: number, numBits = 1) {
        var xBig = new BigNumber(x)
        const formattedData = this.padString(xBig.toString(2), numBits);
        return this.insert(formattedData);
    }

    public toHex(numBytes = 80) {
        var dataNum = new BigNumber(this.data, 2)
        return this.padString(dataNum.toString(16), numBytes * 2)
    }

    public print() {
        console.log("data:" + this.data)
    }

    private insert(x: string) {
        const offset = this.length();
        this.data += x;
        return offset;
    }

    // Returns the number of bytes of data
    public length() {
        return this.data.length;
    }

    private padString(x: string, targetLength: number) {
        if (x.length > targetLength) {
            throw Error(
                "0x" +
                x +
                " is too big to fit in the requested length (" +
                targetLength +
                ")"
            );
        }
        while (x.length < targetLength) {
            x = "0" + x;
        }
        return x;
    }
}