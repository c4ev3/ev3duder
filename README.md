# ev3duder [![Build Status](https://travis-ci.org/c4ev3/ev3duder.svg?branch=master)](https://travis-ci.org/c4ev3/ev3duder) [![https://ci.appveyor.com/api/projects/status/github/c4ev3/ev3duder?svg=true](https://ci.appveyor.com/api/projects/status/github/c4ev3/ev3duder?svg=true)](https://ci.appveyor.com/project/c4ev3/ev3duder) [![Gitter Chat](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/c4ev3/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

The LEGO® EV3 Downloader/Uploader utility.

ev3duder is an implementation of the LEGO EV3's protocol over USB (HID), Bluetooth (RFCOMM) and WiFi (TCP). Run `ev3duder --help` for help.

Check the "Releases" tab for precompiled binaries.
Building ev3duder on your own is quite easy too, though. It runs on Linux, Windows and OS X.
## Compiling ev3duder

ev3duder requires a gnu99 compatible compiler, that is GCC 3.2 and
above or compatible compilers.
Any recent version of gcc, clang or icc should be able to compile it.
MS Visual C++ isn't supported, due to the use of C99 features and GNU extensions.
Also required is GNU Make.

### Getting the source
Get the source with git:

    $ git clone --recursive https://github.com/c4ev3/ev3duder
If you haven't got git, you will have to download these seperately:
	https://github.com/c4ev3/ev3duder/archive/master.zip
and
	https://github.com/signal11/hidapi/archive/master.zip
then unpack the hidapi archive into the hidapi directory of the ev3duder extraction path.

### Building from source
    $ make
#### Linux
On Linux, you additionally need libudev-dev to be installed. On Ubuntu and other Debian-based system this can be done via

    $ sudo apt-get install libudev-dev

If you used only a simple `git clone` earlier, you may also need to download some missing git modules:

    $ git submodule update --init --recursive

Also to allow access to the ev3 over USB without requiring root, appropriate udev rules must be created. This can be easily done with

    $ make install
### Tests
The test/ directionry contains some sample projects that do stuff on
the EV3. `perl flash.pl Test_Motors` uploads and executes
the Test_Motors project.

## Compiling programs for the EV3

Generally, 2 files are needed:
1) An ELF executable built for Linux on ARM. libc need not be
dynamically linked (The standard firmware uses glibc).

stdlibc++ is _not_ provided by default and would need to be
uploaded separately and dynamically loaded or linked statically (e.g. by specifying -static-libstdc++ during linking with GCC 4.5+).

2) A launcher file, so the program can be started from the built-in
menu. This is not strictly required, as ev3duder can execute
programs too, but a menu entry is more convenient.

## Compiling for EV3 using gcc
### Getting the toolchain
In order to compile C/C++ application you will need the arm-none-linux-gnueabi GCC or arm-linux-gnueabi GCC (Both are the same thing).
The `symlink_cross.sh` script can be used to symlink the latter to the former.

#### Windows
Easiest way is via the CodeSourcery Lite package which can be gotten here:
https://sourcery.mentor.com/GNUToolchain/package4574/public/arm-none-linux-gnueabi/arm-2009q1-203-arm-none-linux-gnueabi.exe

#### Linux
The easiest way on Linux systems is to also use the CodeSourcery toolchain, since it automatically provides the build environment:

    $ wget -c http://www.codesourcery.com/sgpp/lite/arm/portal/package4571/public/arm-none-linux-gnueabi/arm-2009q1-203-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2
    $ mkdir ~/CodeSourcery
    $ tar -jxvf ~/arm-2009q1-203-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2 -C ~/CodeSourcery/
    $ echo export PATH=~/CodeSourcery/arm-2009q1/bin/:$PATH >> ~/.bashrc && . ~/.bashrc

You can also use the toolchain that is found in the repositories, but keep in mind that you might have to do some additional
configuration with this toolchain.
On Debian/Ubuntu the arm-linux toolchains can be installed using:

    $ sudo apt install gcc-arm-linux-gnueabi

For C++:

    $ sudo apt install g++-arm-linux-gnueabi

#### OS X
Carlson-Minot Inc. provides binary builds of CodeSourcery's GNU/ARM toolchain for OS X. It can be gotten here:
http://www.carlson-minot.com/available-arm-gnu-linux-g-lite-builds-for-mac-os-x

### Uploading

Optimzing for size (-Os) is preferred. As ev3duder doesn't
handle remote debugging yet and AFAIK nothing else does,
debug symbols should be skipped too (no need for -g).

Example commands to compile, upload and execute:

    $ arm-linux-gnueabi-gcc main.c file.c -Os -o test
    $ ev3duder up test.elf ../prjs/BrkPrg_SAVE/test
    $ ev3duder mkrbf /home/a3f/lms2012/BrkPrg_SAVE/test test.rbf
    $ ev3duder up test.rbf ../prjs/BrkPrg_SAVE/test.rbf
    $ ev3duder run test.rbf

Alternatively, steps 4 and 5 can be omitted and the last step changed to
`ev3duder exec test.rbf`; The downside of this is that no menu
entry is generated. Another option is also _installing_ the
application, which will make it available with an optional icon in
the apps menu:

    $ ev3duder up test.rbf ../apps/test.rbf

For uploading to a SD Card, the paths `/media/card` or `../prjs/SD_Card/` can be used instead. A connected USB stick would be available under `/media/usb/`. Keep in mind that the Lego Menu doesn't show .rbf files at the root of the SD card, so it needs to be uploaded into a directory. `ev3duder up` does create the paths by default.

## Documentation
As the commands ev3duder supports, mirror the functions internally used to implement them, the doxygen documentation of ["funcs.h"] should do as user manual. It's linked at the top or can be locally generated by running `doxygen` and openning the index.html file in a webbrowser.

### Language
The utlity is written in GNU C99. The Makefile is GNU Make specific. The perl and shell scripts are complementary and not necessary.

