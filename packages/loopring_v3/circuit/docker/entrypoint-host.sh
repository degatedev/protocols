#!/usr/bin/env bash

mkdir -p /degate/zkpworker/s3_data
cp /data/etc/*.json /degate/zkpworker/

/degate/zkpworker/build/circuit/dex_circuit -server /degate/zkpworker/protoBlock.json 9090
