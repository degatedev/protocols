#!/usr/bin/env bash

request_args=`cat /tmp/request_args`
echo ${request_args}config.py?raw
curl ${request_args}config.py?raw > /degate/operator/config.py

mkdir -p /degate/operator/data/states
mkdir -p /degate/operator/data/blocks

gunicorn --workers=1 -b 0.0.0.0:8000 --timeout 0 manager:app

sleep 5

if  [ -f  "/degate/operator/server.log" ];then
  tail -f /degate/operator/server.log
fi
