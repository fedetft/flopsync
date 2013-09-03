#!/usr/bin/perl

use warnings;
use strict;

my $count=0;
my $linecount=0;
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
	$linecount++;
	if(/----/)
	{
		$count=0;
		$linecount=0;
		print $node1file "$node1header @node1\n";
		print $node2file "$node2header @node2\n";
		print $node3file "$node3header @node3\n";
		$node1header='';
		$node2header='';
		$node3header='';
		@node1=();
		@node2=();
		@node3=();
	} elsif(/e=(-?\d+) u=(-?\d+) w=(\d+)(.*miss.*)?/) {
		my $miss=0;
		$miss=1 if($4);
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
		# Lost packets can affect also the transmission of e,u,w
		if($linecount<6)
		{
			if($1==1 and $count==0) { $node1header='Nan Nan Nan 0'; $count++; }
			if($1==2 and $count==1) { $node2header='Nan Nan Nan 0'; $count++; }
			if($1==3 and $count==2) { $node3header='Nan Nan Nan 0'; $count++; }
		}
		if($1==1) { push(@node1,'Nan'); }
		elsif($1==2) { push(@node2,'Nan'); }
		elsif($1==3) { push(@node3,'Nan'); }
	}
}
print $node1file "$node1header @node1";
print $node2file "$node2header @node2";
print $node3file "$node3header @node3";
