# Concord: coordinate conversion program

Program to convert coordinates between the New Zealand coordinate systems.  

This program represents an assemblage of code.  It is long overdue a
major rewrite.  

It is recommended that this code is used only for building and running
the concord program, and not as a basis for any other developments.

Warning: This program will not run correctly on big-endian systems.

## Disclaimer

Land Information New Zealand (LINZ) does not offer any support
for this software.  The software is provided "as is" and without
warranty of any kind. See the file LICENSE for the terms of use.

## Copyright

Copyright 2011 Crown copyright (c) Land Information New Zealand and the New
Zealand Government. All rights reserved.

## License

This program is released under the terms of the license contained in the file
LICENSE.

## Runing concord

See concord -? for more information

## How to Build on linux (GNU make)

Note: Build instructions are not available for MS Windows.  Prebuilt files for Windows are available on the [Toitū Te Whenua, Land Information New Zealand website](https://www.linz.govt.nz/products-services/geodetic/geodetic-software-downloads/snap-concord-downloads).

Building concord requires the gcc compiler and the GNU make utility is installed.  The test script also uses the perl utility.

Running concord requires installing the coordinate system definitions  in the [linz-coordsys](https://github.com/linz/linz-coordsys) github repository.

After downloading or  cloning this repository install concord using the following steps:

* Change directory to the linux subdirectory of the concord source directory

* Make concord

  ```shell
  make
  ```

* Test concord

  ```shell
  make test
  ```

  The output from this should end "All tests passed"

* Install concord (this may require root privileges)

  ```shell
  make install
  ```

  This installs concord into /usr/local/share/concord, and creates a symbolic
  link in /usr/local/bin/concord (see Makefile to change these locations)

You can also use "make clean" to remove the build files, and "make uninstall"
to remove the installed files
