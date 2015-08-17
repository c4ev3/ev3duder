#!/usr/bin/perl
# stderr output can be filtered out on windows by appending 2> NUL to the line calling perl
use strict;
$|++;
eval "use autodie 'system';" if eval "use IPC::System::Simple;1;";


my $name = $ARGV[0];

my $localBinary 	= "test/$name";
my $remoteBinary 	= "../prjs/SD_Card/$name";
my $localLauncher 	= "test/myapps/${name}Starter.rbf"; 
my $remoteLauncher 	= "../prjs/SD_Card/myapps/${name}Starter.rbf";

system("./ev3duder" , 'up', $localBinary, $remoteBinary);

printf("\n2- Attempting getting starter <%s> to <%s>\n", $localLauncher, $remoteLauncher);
system("./ev3duder", 'up', $localLauncher, $remoteLauncher);

printf("\n3- Attempting start of starter <%s>\n", $remoteLauncher);
system("./ev3duder", 'run', $remoteLauncher);

printf("\n4- All is well!\n");

END {printf("\nLast error code: %s\n", $?>> 8);}

