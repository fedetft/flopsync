#!/usr/bin/perl

use 5.010;
use warnings;
use strict;
use File::Copy;



# This function builds a program to be flashed on the board
# srcfile=file containing main() to be added to the makefile
# binfile=name of the binary file that will be copied to the experiment folder
sub build
{
	my ($srcfile, $binfile,$elffile)=@_;

	# Step 1: create a Makefile with 'SRC := $srcfile \'
	# Note that all the lines but 'SRC :=' are not modified, so there
	# must be no source file containing a main() in the other lines
	move('../Makefile','../Makefile.orig') 
		or print "perl: can't move Makefile in Makefile.orig";
	
	open(my $infile, '<', '../Makefile.orig') 
		or print "perl: can't open Makefile.orig\n";
	open(my $outfile, '>', '../Makefile') 
		or print "perl: can't open Makefile\n";
	while(<$infile>)
	{
		if(/^SRC :=/)
		{
			print $outfile "SRC := $srcfile \\\n";
		} else {
			print $outfile $_;
		}
	} 
	close($infile);
	close($outfile);
	
	
	# Step 2: build the binary
	system('make -C ../ 1>/dev/null');
	copy('../main.bin',"$binfile")
		or print "perl: cannot find main.bin\n";
        copy('../main.elf',"$elffile")
		or print "perl: cannot find main.elf\n";
	system('make clean -C ../ 1>/dev/null');

	# Step 3: restore original Makefile
	unlink('../Makefile');
	move('../Makefile.orig','../Makefile') 
		or print "perl: can't move Makefile.orig in Makefile";
}
# Build files for all two nodes
#`..make clean 1>/dev/null 2>/dev/null`; # Make sure the build directory is clean
build('test_timer/test_timer.cpp','timer.bin','timer.elf')
	or die("perl: error while compiling test_timer\n");
print "-----Successifully compiled--------\n"
