#!/bin/sh

# NOTE: change this depending on how the USB are enumerated
ARDUINO=/dev/ttyUSB2
BOARD0=/dev/ttyUSB3
BOARD1=/dev/ttyUSB1
BOARD2=/dev/ttyUSB0

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
		*)
			return;;
	esac
	fwupgrade --arduino=$ARDUINO --device=$1 --enter
	flushbuffer $BOARD
	stm32flash -w $2 -v $BOARD
	fwupgrade --arduino=$ARDUINO --device=$1 --leave # Restart code execution
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
		*)
			return;;
	esac
	fwupgrade --arduino=$ARDUINO --device=$1 --enter
	flushbuffer $BOARD
	stm32flash -w $2 -v $BOARD
	fwupgrade --arduino=$ARDUINO --device=$1 --keep # Keep in reset state
}

releaseboards() {
	fwupgrade --arduino=$ARDUINO --device=0 --leave
	fwupgrade --arduino=$ARDUINO --device=1 --leave
	fwupgrade --arduino=$ARDUINO --device=2 --leave
}
