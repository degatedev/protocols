#!/usr/bin/env bash

cd ..

##ethsnarks
git submodule update --init --recursive --remote

make ZKP_WORKER_MODE=1

rm -rf circuit/build && mv build circuit/
