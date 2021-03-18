# Texas Instrument MSP432 port

This port is intended to run MicroPython to the Texas Instrument MSP432 CPU
family. For now only the MSP-EXP432E401Y Development kit is supported.

## Building and running on the MSP-EXP432E401Y Development kit

Currently only Windows compilation is supported.

### Required tools

In order to compile this port you need to download and install the followings
SDK or tools:

* [Texas Instrument SimpleLink SKD](http://www.ti.com/wireless-connectivity/simplelink-solutions/overview/software.html)
* [Texas Instrument Code Composer Studio IDE](http://www.ti.com/tool/CCSTUDIO)
* [FreeRTOS sources](https://www.freertos.org/)

after installing these tools you need to update the file `mkconf.mk` with the corresponding
path. Alternatively you can use Environment variable  to setup  these path.

### Build from command line

You can build the port from the command line using GNU makefile.

    $ make

Note that if you are on Windows, you can use the gmake.exe binary that is part of
the Code Composer  Studio IDE.

## Building using Code Composer Studio IDE

To make development easier we also have include an Code Composer project.
You can import it in CCS with *file->import->CCS project*. *Durring import
do uncheck "Copy projects into workspace"*.