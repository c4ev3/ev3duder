#!/usr/bin/perl

use Curses;
use IPC::Open2; 
print open2(\*EV3OUT, \*EV3IN, "ev3 tunnel") or die "couldn't find: %!";

my $ch = 0; 
initscr, noecho, cbreak;
eval { keypad(1) }; 
addstr(0, 0, "press an arrow key on your keyboard: "); 
refresh(); 
while (1) 
{ 
    $ch = getch(); 
    addstr(2, 0, "         "); 
        if ($ch == KEY_UP) { addstr(2, 0, "CHARGE!");
			print EV3IN "0900xxxx8000 00 A3 00 09 00\n";
			print EV3IN "0C00xxxx8000 00 A4 00 09 50 A6 00 09\n";
	   	} 
        elsif ($ch == KEY_SPACE) { addstr(2, 0, "STOP!");
			print EV3IN "0900xxxx8000 00 A3 00 09 00\n";
		}
        elsif ($ch == KEY_LEFT) { addstr(2, 0, "LEFT!"); 
			print EV3IN "0900xxxx8000 00 A3 00 09 00\n";
			print EV3IN "0C00xxxx8000 00 A4 00 08 50 A6 00 08\n";
		}
        elsif ($ch == KEY_RIGHT) { addstr(2, 0, "RIGHT!"); 
			print EV3IN "0900xxxx8000 00 A3 00 09 00\n";
			print EV3IN "0C00xxxx8000 00 A4 00 01 50 A6 00 01\n";
		} 
        elsif ($ch == KEY_DOWN) { addstr(2, 0, "RETREAT!");
			print EV3IN "0900xxxx8000 00 A3 00 09 00\n";
			print EV3IN "0C00xxxx8000 00 A3 00 09 B0 A6 00 09\n";
	   	} 

    refresh;
}
END {endwin}
sub motor
{
}
