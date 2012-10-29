 
These scripts depend on stm32flash, a command line tool for programming stm32
microcontrollers via a serial port. Also, eventgen, fwupgrade and seriallogger
require the boost libraries to be compiled.
They are meant to leave the sensor nodes and arduino attached on a server
running Linux, and remotely log in via ssh and start an experiment that will
flash the boards and start logging data.
The experiment can be configured with the experiment.conf in the main folder,
and the experiment package to be transferred to the server can be made with
'perl deploy/mkpackage.pl'
