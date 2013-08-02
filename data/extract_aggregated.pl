#!/usr/bin/perl

use warnings;
use strict;

my $count=0;
my $node1header='';
my $node2header='';
my $node3header='';
my @node1=();
my @node2=();
my @node3=();
open(my $node1file, '>', 'node1.dat') or die;
open(my $node2file, '>', 'node2.dat') or die;
open(my $node3file, '>', 'node3.dat') or die;
$"=' '; # Separator when interpolating array to string
while(<STDIN>)
{
	if(/----/)
	{
		$count=0;
		print $node1file "$node1header @node1\n";
		print $node2file "$node2header @node2\n";
		print $node3file "$node3header @node3\n";
		$node1header='';
		$node2header='';
		$node3header='';
		@node1=();
		@node2=();
		@node3=();
	} elsif(/e=(-?\d+) u=(-?\d+) w=(\d+)/) {
		my $miss=0;
		$miss=1 if(/miss/);
		if($count==0) { $node1header="$1 $2 $3 $miss"; }
		elsif($count==1) { $node2header="$1 $2 $3 $miss"; }
		elsif($count==2) { $node3header="$1 $2 $3 $miss"; }
		$count++;
	} elsif(/node(\d).e=(-?\w+)/) {
		my $value=$2;
		if($1==1) { push(@node1,$value); }
		elsif($1==2) { push(@node2,$value); }
		elsif($1==3) { push(@node3,$value); }
	} elsif(/node(\d) timeout/) {
		my $timeout_marker=20;
		if($1==1) { push(@node1,$timeout_marker); }
		elsif($1==2) { push(@node2,$timeout_marker); }
		elsif($1==3) { push(@node3,$timeout_marker); }
	}
}
print $node1file "$node1header @node1";
print $node2file "$node2header @node2";
print $node3file "$node3header @node3";
