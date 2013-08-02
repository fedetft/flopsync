 #!/usr/bin/perl

use warnings;
use strict;

# Time from packet send to receive. Not only time to transfer packet
# on the air, but also the time to transfer it via SPI from the MCU
# and radio at both transmitter and receiver, and packet validation
# at the receiver. Measured with an oscilloscope, consistently 300us
my $packet_time=int(0.0003*16384+0.5);
my $t=60*16384;

my $dbg=0;
sub debug
{
	print shift if $dbg;
}

my $ist=0;
debug "Instantaneous mode\n" if($ist);

sub getsend
{
	my $file=shift;
	while(<$file>)
	{
		next unless /send=(\d+)/;
		return $1;
	}
}

sub ismultihop
{
	my $file=shift;
	for(my $i=0;$i<4;$i++)
	{
		$_=<$file>;
		return 1 if(/#secondhop/);
	}
	return 0;
}

open(my $node0, '<', 'node0.txt') or die;
open(my $node1, '<', 'node1.txt') or die;
open(my $node2, '<', 'node2.txt') or die;

getsend($node0); # Discard the first one
my $node1offset=getsend($node0);
my $node2offset=$node1offset; # Valid only if single hop, see below

if(ismultihop($node2))
{
	debug "Multi hop\n";
	# Read node2offset from the second packet sent by node1
	# However, a getsend($node1) will return the offset of node2 with
	# respect to node1, while we need the offset with respect to node0
	# so we apply the time translation formula to convert it
	seek($node0,0,0);

	my $tsrpre, my $tsrpost, my $sendcount=0;
	while(<$node1>)
	{
		if(/ctr=(\d+) e=(-?\d+) u=(-?\d+) w=(\d+) k=(\d+) tsr=(\d+)/)
		{
			$tsrpre=$tsrpost;
			$tsrpost=$6;
			last if($sendcount==2);
		}
		if(/send=(\d+)/)
		{
			$node2offset=$1;
			$sendcount++;
		}
	}

	my $pre=0, my $post=0;
	while($post<$node2offset)
	{
		$pre=$post;
		$post=getsend($node0);
	}

	debug "tsrpre=$tsrpre tsrpost=$tsrpost pre=$pre post=$post send=$node2offset\n";
	$node2offset=int($pre+$packet_time+($node2offset-$tsrpre)/($tsrpost-$tsrpre)*$t);
} else {
	debug "Single hop\n";
}

debug "node1offset=$node1offset node2offset=$node2offset\n";

seek($node0,0,0);
seek($node1,0,0);
seek($node2,0,0);

while(<$node0>)
{
	next unless(/event=(\d+)/);
	my $event0=$1;

	my $event, my $kpre, my $tsrpre, my $tsrpost, my $upre, my $found=0;
	while(<$node1>)
	{
		next unless(/event=(\d+) kpre=(\d+) trpre=(\d+) trpost=(\d+) upre=(-?\d+)/);
		$event=$1; $kpre=$2; $tsrpre=$3; $tsrpost=$4; $upre=$5;
		$found=1;
		last;
	}

	unless($found)
	{
		debug "Exit 1\n";
		last;
	}
	my $event1;
	unless($ist)
	{
		$event1=int($node1offset+$kpre*$t+$packet_time+($event-$tsrpre)/($tsrpost-$tsrpre)*$t);
	} else {
		$event1=int($node1offset+$kpre*$t+$packet_time+($event-$tsrpre)/($t+$upre)*$t);
	}

	$found=0;
	while(<$node2>)
	{
		next unless(/event=(\d+) kpre=(\d+) trpre=(\d+) trpost=(\d+) upre=(-?\d+)/);
		$event=$1; $kpre=$2; $tsrpre=$3; $tsrpost=$4; $upre=$5;
		$found=1;
		last;
	}

	unless($found)
	{
		debug "Exit 2\n";
		last;
	}
	my $event2;
	unless($ist)
	{
		$event2=int($node2offset+$kpre*$t+$packet_time+($event-$tsrpre)/($tsrpost-$tsrpre)*$t);
	} else {
		$event2=int($node2offset+$kpre*$t+$packet_time+($event-$tsrpre)/($t+$upre)*$t);
	}

	print "$event0 $event1 $event2\n";
}

