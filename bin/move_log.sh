#!/bin/bash

TIMESTAMP=$(date +"%Y%m%d%H%M%S")
LOG_DIR=/home/socket/mqttserver/log/
LOG_FILE=mqttserver.log
LOG_FILE_DEST=mqttserver.$TIMESTAMP.log

cd $LOG_DIR

/bin/cp $LOG_FILE $LOG_FILE_DEST
/bin/rm $LOG_FILE
/bin/tar -czvf $LOG_FILE_DEST.tar.gz $LOG_FILE_DEST
/bin/rm $LOG_FILE_DEST

