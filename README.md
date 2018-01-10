# Synopsis

This is a small C framework for UEFI development built on top of TianoCore EDK2.

This repository includes a Vagrantfile for setting up a virtual machine to develop and run UEFI applications. If you're
familiar with Vagrant and have it set up already you can run your first UEFI applications in just a few minutes!

The sources are hosted on [GitHub](https://github.com/rinusser/UEFIStarter).


# General

### What this IS NOT

This project is not a comprehensive course in UEFI development. If you're just starting to write UEFI code you'll need
to use additional material like the [official TianoCore documentation](https://github.com/tianocore/tianocore.github.io/wiki/EDK-II-User-Documentation),
and the [UEFI Specification](http://www.uefi.org/specifications).

### What this IS

The library and UEFI applications included in this code are meant to simplify a few repetitive tasks when developing
UEFI code. For example there is a configurable command line argument parser that will validate input strings and convert
them into the target datatype, e.g. integers.

This project started out with another UEFI development kit (gnu-efi) but eventually outgrew the original SDK, so I
migrated it to TianoCore EDK2017. As a result of this there are still a few library functions included that are already
built-in into TianoCore.

It is my hope that this code helps anyone looking into, or starting with, UEFI development: I did that myself a few
months ago and found parts of the various documentations frustratingly lacking. If I can spare you some of the headache
I had I'm happy.


# Requirements

You can either set up your own development environment ("Manual Installation"), or use the supplied Vagrantfile to
create a development VM ("Vagrant").

## Vagrant

To use the included Vagrantfile you'll need:

* a 64-bit x86 processor (physical or virtual, see Recommendations below)
* a UEFI-capable mainboard (physical or virtual, see Recommendations below)
* Vagrant (tested with 2.0)
* VirtualBox (tested with 5.1)

## Manual Installation

### Bare Minimum

To set up your own development environment you'll need at least:

* a 64-bit x86 processor (physical or virtual, see Recommendations below)
* a UEFI-capable mainboard (physical or virtual, see Recommendations below)
* Linux (tested with Ubuntu 17.04) with sgdisk, mkisofs and mkdosfs installed (you'll need root access)
* a working TianoCore EDK2017 development setup (earlier versions might be OK too) with GCC

### Optional

* Doxygen (if you want to generate the documentation), plus whatever you may need for your desired output format

### Recommendations

You can reboot into the UEFI applications if you want (and have the spare machine or don't mind rebooting all the time),
but it's easier to use virtualization for development. There are multiple options:

* QEMU with OVMF - I use mostly this, except for running graphical applications. It's very fast to work with. The OVMF
  image adds EFI support to QEMU, you'll need to download and install it separately.
* VirtualBox - has built-in EFI support. I use this to run the graphical UEFI applications: it easily outperforms QEMU,
  at least on my machines.
* Other: you're probably fine with any hypervisor that supports EFI.


# Installation

The instructions on how to install the Vagrant virtual machine are in
[INSTALL.vagrant.md](https://github.com/rinusser/UEFIStarter/blob/master/INSTALL.vagrant%2Emd).

If you prefer to set up your own development environment see
[INSTALL.manual.md](https://github.com/rinusser/UEFIStarter/blob/master/INSTALL.manual%2Emd).


# Usage

### Running

If you're using QEMU you can start it with:

    $ make run

Alternatively you can mount the .iso image UEFIStarter/target/uefi.iso in e.g. VirtualBox and boot from it.

Either way, the default startup.nsh script will run all test suites and return to the UEFI shell.

The root directory contains all built applications.

### Development

There is a semi-automatically generated documentation hosted on [GitHub](https://rinusser.github.io/UEFIStarter/).
The functions, data types etc. are described on the [Modules](https://rinusser.github.io/UEFIStarter/modules.html) page.

The documentation is still a work in progress.

Each shell you want to run the edk2 build process in needs to import `edksetup.sh` in the edk2 sources root:

    $ . edksetup.sh

You only need to do this once per shell. The Makefile contains everything else you need to build the UEFIStarter
package:

    $ make

When this finishes successfully the UEFIStarter/target directory contains an .img file with a filesystem for QEMU, and
an .iso image with the same contents for e.g. a VirtualBox virtual CD/DVD drive.

# Tests

This project contains a test framework for writing tests. There are test suites for validating the framework functions
and the test framework itself. The tests are executed in the UEFI shell - there is a script to run all test suites:

    FS0:\> cd tests
    FS0:\tests\> run

Test suites are individual .efi executables that can be ran individually, e.g.:

    FS0:\tests\> testlib -verbosity 1
    .............
    Result: SUCCESS

    Successful tests: 13
    Failed tests:     0
    Incomplete tests: 0
    Skipped groups:   0

The `testself` suite contains a "runner" test group that fails on purpose, the `run.nsh` script skips this.

The test suites support command line parameters, for example you can skip test groups or change the output verbosity.
You can get a full list of parameters with "-help".


# Legal

### Copyright

Copyright (C) 2017-2018 Richard Nusser

### License

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

### Credits

Contains an excerpt from [The PCI ID Repository](http://pci-ids.ucw.cz/), licensed under GPLv3.

Contains an optional automated installer, a "Vagrantfile". By using this installer you additionally agree to the
licenses of all installed components, see the respective websites for details:
* Ubuntu: [https://www.ubuntu.com/about/about-ubuntu/licensing](https://www.ubuntu.com/about/about-ubuntu/licensing)
* TianoCore: [https://github.com/tianocore/tianocore.github.io/wiki/Legalese](https://github.com/tianocore/tianocore.github.io/wiki/Legalese)
