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
				'vht','sense_temperature','send_timestamps','glossy','multi_hop',
				'sync_by_wire','comb','root_node_never_sleep',
				'node0_enable','node0_controller','node0_file','node0_hop',
				'node1_enable','node1_controller','node1_file','node1_hop',
				'node2_enable','node2_controller','node2_file','node2_hop',
				'node3_enable','node3_controller','node3_file','node3_hop',
				'node4_enable','node4_controller','node4_file','node4_hop',
				'node5_enable','node5_controller','node5_file','node5_hop',
				'node6_enable','node6_controller','node6_file','node6_hop',
				'node7_enable','node7_controller','node7_file','node7_hop',
				'node8_enable','node8_controller','node8_file','node8_hop',
				'node9_enable','node9_controller','node9_file','node9_hop');
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
	my ($srcfile, $binfile, $elffile, $hop, $controller)=@_;

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
	move('flopsync_v3/protocol_constants.h','flopsync_v3/protocol_constants.h.orig');
	open($infile, '<', 'flopsync_v3/protocol_constants.h.orig');
	open($outfile, '>', 'flopsync_v3/protocol_constants.h');
	while(<$infile>)
	{
		if(/#define RELATIVE_CLOCK/)
		{
			print $outfile '//' unless($config{'relative_clock'});
			print $outfile "#define RELATIVE_CLOCK\n";
		} elsif(/#define INTERACTIVE_ROOTNODE/) {
			print $outfile '//' unless($config{'interactive_rootnode'});
			print $outfile "#define INTERACTIVE_ROOTNODE\n";
		} elsif(/#define EVENT_TIMESTAMPING/) {
			print $outfile '//' unless($config{'event_timestamping'});
			print $outfile "#define EVENT_TIMESTAMPING\n";
		} elsif(/#define USE_VHT/) {
			print $outfile '//' unless($config{'vht'});
			print $outfile "#define USE_VHT\n";
		} elsif(/#define SENSE_TEMPERATURE/) {
			print $outfile '//' unless($config{'sense_temperature'});
			print $outfile "#define SENSE_TEMPERATURE\n";
		} elsif(/#define SEND_TIMESTAMPS/) {
			print $outfile '//' unless($config{'send_timestamps'});
			print $outfile "#define SEND_TIMESTAMPS\n";
		} elsif(/#define GLOSSY/) {
			print $outfile '//' unless($config{'glossy'});
			print $outfile "#define GLOSSY\n";
		}elsif(/#define MULTI_HOP/) {
			print $outfile '//' unless($config{'multi_hop'});
			print $outfile "#define MULTI_HOP\n";
		}elsif(/#define SYNC_BY_WIRE/) {
			print $outfile '//' unless($config{'sync_by_wire'});
			print $outfile "#define SYNC_BY_WIRE\n";
		}elsif(/#define COMB/) {
			print $outfile '//' unless($config{'comb'});
			print $outfile "#define COMB\n";
		}elsif(/#define ROOT_NODE_NEVER_SLEEP/){
			print $outfile '//' unless($config{'root_node_never_sleep'});
			print $outfile "#define ROOT_NODE_NEVER_SLEEP\n";
		}elsif(/^#define experimentName/) {
			my $n="#define experimentName \"$config{experiment_name}#$binfile";
			$n.='#hop.'.$hop if($config{'multi_hop'});
			$n.='#'.localtime();
			print $outfile "$n\"\n";
		} elsif(/^(const unsigned char node_hop=).*(;)/) {
			if($config{'multi_hop'} && !$config{'send_timestamps'})
			{
			    print $outfile "$1$hop$2\n";
			}
			else 
			{
			    print $outfile "$1'0'$2\n";
			}
		} elsif(/^(const unsigned char controller=).*(;)/) {
			if (!($controller<0 || $controller>4))
			{
			    print $outfile "$1$controller$2\n";
			}
		} elsif(/^(const unsigned long long nominalPeriod=static_cast<unsigned long long>\().*(\*hz\+0.5f\);)/) {
			print $outfile "$1$config{sync_period}$2\n";
		} else { print $outfile $_; }
	}
	close($infile);
	close($outfile);
	# Uncomment for debugging the substitution code
	my $e=$config{'experiment_name'};
	#copy('Makefile',"$e/$binfile.Makefile");
	#copy('flopsync_v3/protocol_constants.h',"$e/$binfile.protocol_constants");

	# Step 3: build the binary
	system('make -j4 1>/dev/null');
	copy('main.bin',"$config{'experiment_name'}/$binfile")
		or print "perl: cannot find main.bin\n";
	copy('main.elf',"$config{'experiment_name'}/$elffile")
		or print "perl: cannot find main.elf\n";
	system('make clean 1>/dev/null');

	# Step 4: restore original Makefile and protocol_constants.h
	unlink('Makefile');
	unlink('flopsync_v3/protocol_constants.h');
	move('Makefile.orig','Makefile');
	move('flopsync_v3/protocol_constants.h.orig','flopsync_v3/protocol_constants.h');
}

# Build files for all three nodes
`make clean 1>/dev/null 2>/dev/null`; # Make sure the build directory is clean
if($config{'node0_enable'}) {build($config{'node0_file'},'node0.bin','node0.elf',$config{'node0_hop'},$config{'node0_controller'});}
if($config{'node1_enable'}) {build($config{'node1_file'},'node1.bin','node1.elf',$config{'node1_hop'},$config{'node1_controller'});}
if($config{'node2_enable'}) {build($config{'node2_file'},'node2.bin','node2.elf',$config{'node2_hop'},$config{'node2_controller'});}
if($config{'node3_enable'}) {build($config{'node3_file'},'node3.bin','node3.elf',$config{'node3_hop'},$config{'node3_controller'});}
if($config{'node4_enable'}) {build($config{'node4_file'},'node4.bin','node4.elf',$config{'node4_hop'},$config{'node4_controller'});}
if($config{'node5_enable'}) {build($config{'node5_file'},'node5.bin','node5.elf',$config{'node5_hop'},$config{'node5_controller'});}
if($config{'node6_enable'}) {build($config{'node6_file'},'node6.bin','node6.elf',$config{'node6_hop'},$config{'node6_controller'});}
if($config{'node7_enable'}) {build($config{'node7_file'},'node7.bin','node1.elf',$config{'node7_hop'},$config{'node7_controller'});}
if($config{'node8_enable'}) {build($config{'node8_file'},'node8.bin','node8.elf',$config{'node8_hop'},$config{'node8_controller'});}
if($config{'node9_enable'}) {build($config{'node9_file'},'node9.bin','node9.elf',$config{'node9_hop'},$config{'node9_controller'});}

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
#`tar czfv \"$config{'experiment_name'}.tar.gz\" \"$config{'experiment_name'}\"`;
#`rm -rf \"$config{'experiment_name'}\"`;

