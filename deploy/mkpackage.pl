#!/usr/bin/perl

use 5.010;
use warnings;
use strict;
use File::Copy;

# Parse the experiment.conf configuration file
open(my $file, '<', 'experiment.conf') or die;
my %config;
while(<$file>)
{
	next if(/^#/);
	next unless(/(\w*)="(.*)"/);
	$config{$1}=$2;
}
close($file);
my @configkeys=('experiment_name','experiment_time','sync_period',
				'relative_clock','interactive_rootnode','event_timestamping',
				'node0_file','node0_second_hop','node1_file',
				'node1_second_hop','node2_file','node2_second_hop');
@configkeys=sort(@configkeys);
my @sortedconfig=sort(keys(%config));
#  If arrays not equal, some coniguration parameters are missing
die 'Config file error' unless(@configkeys ~~ @sortedconfig);

# Create the experiment directory
mkdir $config{'experiment_name'} or die 'experiment_name: directory exists';

# This function builds a program to be flashed on the board
# srcfile=file containing main() to be added to the makefile
# binfile=name of the binary file that will be copied to the experiment folder
# secondhop=wheter or not the program needs #define SECOND_HOP
sub build
{
	my ($srcfile, $binfile, $secondhop)=@_;

	# Step 1: create a Makefile with 'SRC := $srcfile \'
	# Note that all the lines but 'SRC :=' are not modified, so there
	# must be no source file containing a main() in the other lines
	move('Makefile','Makefile.orig');
	open(my $infile, '<', 'Makefile.orig');
	open(my $outfile, '>', 'Makefile');
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

	# Step 2: create a protocol_constants.h correctly configured
	move('protocol_constants.h','protocol_constants.h.orig');
	open($infile, '<', 'protocol_constants.h.orig');
	open($outfile, '>', 'protocol_constants.h');
	while(<$infile>)
	{
		if(/^const unsigned int nominalPeriod=static_cast<int>/)
		{
			print $outfile "const unsigned int nominalPeriod=static_cast<int>".
						   "($config{sync_period}*16384+0.5f);\n";
			next; # Do not print the original line
		}
		if(/#endif \/\/PROTOCOL_CONSTANTS_H/)
		{
			print $outfile "#undef RELATIVE_CLOCK\n".
						   "#undef SECOND_HOP\n".
						   "#undef INTERACTIVE_ROOTNODE\n".
						   "#undef EVENT_TIMESTAMPING\n".
						   "#undef experimentName\n";
			print $outfile "#define RELATIVE_CLOCK\n"
				if($config{'relative_clock'});
			print $outfile "#define SECOND_HOP\n"
				if($secondhop);
			print $outfile "#define INTERACTIVE_ROOTNODE\n"
				if($config{'interactive_rootnode'});
			print $outfile "#define EVENT_TIMESTAMPING\n"
				if($config{'event_timestamping'});
			my $n="#define experimentName \"$config{experiment_name}#$binfile";
			$n=$n.'#secondhop' if($secondhop);
			print $outfile "$n\"\n";
		}
		print $outfile $_;
	}
	close($infile);
	close($outfile);
	# Uncomment for debugging the substitution code
	#my $e=$config{'experiment_name'};
	#copy('Makefile',"$e/$binfile.Makefile");
	#copy('protocol_constants.h',"$e/$binfile.protocol_constants.h");

	# Step 3: build the binary
	system('make 1>/dev/null');
	copy('main.bin',"$config{'experiment_name'}/$binfile")
		or print "perl: cannot find main.bin\n";
	system('make clean 1>/dev/null');

	# Step 4: restore original Makefile and protocol_constants.h
	unlink('Makefile');
	unlink('protocol_constants.h');
	move('Makefile.orig','Makefile');
	move('protocol_constants.h.orig','protocol_constants.h');
}

# Build files for all three nodes
`make clean 1>/dev/null 2>/dev/null`; # Make sure the build directory is clean
build($config{'node0_file'},'node0.bin',$config{'node0_second_hop'});
build($config{'node1_file'},'node1.bin',$config{'node1_second_hop'});
build($config{'node2_file'},'node2.bin',$config{'node2_second_hop'});

# Fixup runexp.sh to set the experimen duration
open(my $infile, '<', 'deploy/runexp.sh');
open(my $outfile, '>', "$config{'experiment_name'}/runexp.sh");
# This copies the #!/bin/sh
$_=<$infile>;
print $outfile $_;
print $outfile "EVENT=1" if($config{'event_timestamping'});
while(<$infile>)
{
	if(/EXPERIMENTTIME=/)
	{
		print $outfile "EXPERIMENTTIME=$config{'experiment_time'}\n";
	} else {
		print $outfile $_;
	}
}
close($infile);
close($outfile);

# Also copy the experiment.conf file for reference
copy('experiment.conf',"$config{'experiment_name'}/experiment.conf");

# Lastly, pack everything in a tarball ready to be copied to the server
`tar czfv \"$config{'experiment_name'}.tar.gz\" \"$config{'experiment_name'}\"`;
`rm -rf \"$config{'experiment_name'}\"`;
