#!/bin/bash

export PROC=./mqttserver

export LOG_FILE=/home/socket/mqttserver/log/mqttserver.log

export DB_HOST=localhost
export DB_NAME=gs
export DB_USER=root
export DB_PASSWD=QV5QVE07JE

#export MQ_HOST=45.167.121.163
export MQ_HOST=192.168.8.220
export MQ_PORT=1883
export MQ_USER=placa1
export MQ_PASSWD=12345678

nohup $PROC &

#$PROC
