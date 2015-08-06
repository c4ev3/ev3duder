#!/usr/bin/perl
## Pair with ev3 and make VM available as FIFO
#sample usage:
# ./ev3pair.pl ~/ev3.sock &
# ./ev3duder -d ~/ev3.sock ls
use IO::Socket::INET;
use IO::Select;
use warnings;

my $data;
my ($udp_port, $ev3_addr, $tcp_port, $serial);

$FIFO_NAME = $ARGV[0] // "$ENV{HOME}/ev3.sock";

$sock=IO::Socket::INET->new(Proto => 'udp',
							LocalPort => 3015)
					   	or die "Can't bind: $@\n";

if($sock->recv($data, 128)) {
	($udp_port, $ipaddr) = sockaddr_in($sock->peername);
	$ev3_addr = inet_ntoa($ipaddr);
}
if ($data =~ /^
				Serial-Number:\ ([[:xdigit:]]+)\r\n
				Port:\ (\d+)\r\n
				Name:\ (.*?)\r\n
				Protocol:\ (.*?)\r\n
				$/x)
{
	($serial, $tcp_port, $ev3_name, $proto) = ($1,$2,$3,$4);
	print STDERR "$ev3_name ($serial) @ $proto://$ev3_addr:$tcp_port\n";
}
$ev3 = IO::Socket::INET->new(
    Proto    => 'udp',
    PeerPort => $udp_port,
    PeerAddr => $ev3_addr,
) or die "Could not create socket: $@\n";

$sock->send("\x00") or die "Send error: $!\n";
$ev3->close();

$ev3 = new IO::Socket::INET (
	Proto 	 => 'tcp',
	PeerHost => $ev3_addr,
	PeerPort => $tcp_port,
) or die "Can't bind : $@\n";
#$serial =~ s/./1/g; # strangely, this works.
$ev3->send("GET /target?sn=$serial VMTP1.0\nProtocol: $proto");
print STDERR <$ev3> . "\n";

require POSIX;
POSIX::mkfifo($FIFO_NAME."in", 0666) or die "can't mkfifo $FIFO_NAME: $!";
POSIX::mkfifo($FIFO_NAME."out", 0666) or die "can't mkfifo $FIFO_NAME: $!";
open($fifoin, "<", $FIFO_NAME . "in") or die "couldn't open fifo: $!";
open($fifoout, ">", $FIFO_NAME. "out") or die "couldn't open fifo: $!";
print $FIFO_NAME . "(in|out) online...\n";
%pair = ($ev3 => "fifoout", $fifoin => "ev3");
my $sel = IO::Select->new($ev3, $fifoin);
while (@ready = $sel->can_read)
{
	my $buffer;
	foreach $fh (@ready) {
		$read 	 = $fh->sysread($buffer, 1337) or die "couldn't read: $@";
		#$written = ${$pair{$fh}}->syswrite($buffer); # why does this duplicate?
		#print "$written/$read bytes sent to ", $pair{$fh}, "\n";
		#$buffer = <$fh>;
		no warnings; print ${$pair{$fh}} $buffer; use warnings;
		print length $buffer , " bytes sent to ", $pair{$fh}, "\n";
	}
}
$sel->remove($ev3, $fifoin);
close($fifoin), close($fifoout), shutdown($ev3, 2), close($ev3);
unlink $FIFO_NAME."in", $FIFO_NAME."out"; # Why can't I write $ARGV[0] here?

