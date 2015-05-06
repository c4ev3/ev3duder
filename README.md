# ev3duder

The LEGO® EV3 Downloader/Uploader utility.

ev3duder is an utitlity for ....

Click here for precompiled binaries for Linux, Windows and OS X.
Building ev3duder on your own is quite easy too, though.
# Compilation
## Compiling ev3duder

ev3duder requires a gnu99 compatible compiler, that is GCC 3.2 and
above or compatible compilers.
Any recent version of gcc, clang or icc should be able to compile it.
MS Visual C++ isn't supported, due to the use of C99 features and GNU extensions.
Also required is the Make utility and the HIDAPI library.

### Debian Linux
    $ apt-get install hidapi
    $ make
On other systems, use your packet manager to get HIDAPI installed.
Make and gcc are always there by default.
### Mac OS X
    $ brew install hidapi
    $ make
MacPorts (`ports install hidapi`) also works. If no package
manager is available, you could build it yourself following the
Windows instructions.
### Microsoft Windows
You will need to build HIDAPI yourself, which is quite straight
forward:
Get the HIDAPI sourced from here:

Get the Minimalist GNU for Windows toolchain from here:

HIDAPI is easily compiled by make:
     C:/> make
ev3duder is also only 1 make away:
     C:/> cd ev3duder
     C:/> make
### Tests
The test/ directionry contains some sample projects that do stuff on
the EV3. `perl flash.pl tests/test1.c` builds, uploads and executes
the test1 project and checks for the output to make sure it worked correctly. 

## Compiling programs for the EV3

Generally, 2 files are needed: 
1) An ELF executable built for Linux on ARM. libc need not be
dynamically linked (The standard firmware uses musl(?) libc).
stdlibc++ is _not_ provided by default and would need to be
uploaded separately and dynamically loaded or linked statically.

2) A launcher file, so the program can be started from the built-in
menu. This is not strictly required, as ev3duder can execute
programs too, but a menu entry is more convenient.

### Compiling for EV3 using gcc

Optimzing for size (-Os) is preferred. As ev3duder doesn't
handle remote debugging yet and AFAIK nothing else does,
debug symbols should be skipped too (no need for -g).
That's it, example commands to compile, upload and execute:
    $ arm-linux-gnueabi-gcc main.c file.c -Os -o test
    $ ev3duder cd ../prjs/BrkPrg_SAVE
    $ ev3duder up test .
    $ ev3duder mk-launcher test test.rbf
    $ ev3duder up test.rbf .
    $ ev3duder exec test.rbf

Alternatively, steps 4 and 5 can be omitted and the last step changed to
`ev3duder exev test.rbf`; The downside of thisis that no menu
entry is generated. Another option is also _installing_ the
application, which will make it available with an optional icon in
the apps menu:
    $ ev3duder install test
or
    $ ev3duder install test test.ico
