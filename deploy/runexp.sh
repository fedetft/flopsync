#!/bin/sh

MAGICPATH=../deploy # Path to the script to reprogram the boards
. $MAGICPATH/magiclib.sh

EXPERIMENTTIME=<filled in by mkpackage.pl>
SLEEPTIME=`echo $EXPERIMENTTIME | bc`

echo 'Flashing boards...'
flashboard_keep 0 node0.bin
flashboard_keep 1 node1.bin
flashboard_keep 2 node2.bin

echo 'Starting loggers...'
seriallogger --port=$BOARD0 --baudrate=19200 --file=node0.txt &
seriallogger --port=$BOARD1 --baudrate=19200 --file=node1.txt &
seriallogger --port=$BOARD2 --baudrate=19200 --file=node2.txt &

echo 'Booting boards...'
releaseboards

if [ "$EVENT" = "1" ]; then
	sleep 120
	eventgen --port=$ARDUINO --baudrate=19200 --mininterval=125 --maxinterval=180 --file=e.txt &
fi

START=`date`
echo $START > expinfo.txt

echo "Experiment started @ $START, sleeping for $SLEEPTIME..."
sleep $SLEEPTIME

killall seriallogger
if [ "$EVENT" = "1" ]; then
	killall eventgen
fi

STOP=`date`
echo $STOP >> expinfo.txt
