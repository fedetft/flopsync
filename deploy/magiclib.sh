#!/bin/sh

# Note: this code is just for testing locally, it not as complete as the full server code

# NOTE: change this depending on how the USB are enumerated
BOARD0=/dev/ttyUSB0
BOARD1=/dev/ttyUSB1
BOARD2=/dev/ttyUSB2
BOARD3=/dev/ttyUSB3

# $1=serial port to flush
flushbuffer() {
	# Use seriallogger to flush the buffer the OS keeps for the serial port
	seriallogger --port=$1 --baudrate=19200 --file=/dev/null &
	process=$!
	sleep 1
	kill $process
	sleep 1
}

# $1=device {0,1,2}
# $2=file to flash
flashboard() {
	case $1 in
		0)
			BOARD=$BOARD0;;
		1)
			BOARD=$BOARD1;;
		2)
			BOARD=$BOARD2;;
		3)
			BOARD=$BOARD3;;
		*)
			return;;
	esac
	flushbuffer $BOARD
	stm32flash -w $2 -v $BOARD
}

# $1=device {0,1,2}
# $2=file to flash
flashboard_keep() {
	case $1 in
		0)
			BOARD=$BOARD0;;
		1)
			BOARD=$BOARD1;;
		2)
			BOARD=$BOARD2;;
		3)
			BOARD=$BOARD3;;
		*)
			return;;
	esac
	flushbuffer $BOARD
	stm32flash -w $2 -v $BOARD
}

releaseboards() {
	echo
}
