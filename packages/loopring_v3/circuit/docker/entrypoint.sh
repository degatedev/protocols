#!/usr/bin/env bash

request_args=`cat /tmp/request_args`
echo ${request_args}protoBlock.json?raw
curl ${request_args}protoBlock.json?raw > /degate/zkpworker/protoBlock.json
curl ${request_args}worker_config.json?raw > /degate/zkpworker/worker_config.json
mkdir -p /degate/zkpworker/s3_data

./build/circuit/dex_circuit -server /degate/zkpworker/protoBlock.json 9090
