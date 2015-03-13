$|++;

my $name = $ARGV[0];

my $localBinary 	= 'test\\' .$name;
my $remoteBinary 	= '../prjs/SD_Card/' . $name;
my $localLauncher 	= 'test\\myapps\\' . $name . 'Starter.rbf'; 
my $remoteLauncher 	= '../prjs/SD_Card/myapps/' . $name . 'Starter.rbf';

(
	printf("\n1) attempting getting <%s> to <%s>\n", $localBinary, $remoteBinary)
&&
	system('ev3_cp' , '.', $localBinary, $remoteBinary)
&&
	printf("\n2) attempting getting <%s> to <%s>\n", $localLauncher, $remoteLauncher)
&&
	system('ev3_cp', $remoteLauncher, $localLauncher, $remoteLauncher)
&&
	printf("\n3) Alll is well!\n")
) || printf("\nFailure with error code: %s", $?);
