# Synopsis

This is a small C framework for UEFI development built on top of TianoCore EDK2.

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

You can either set up your own development environment ("Standalone" in the rest of this document), or use the supplied
Vagrantfile to create a development VM ("Vagrant").

## Vagrant

To use the included Vagrantfile you'll need:

* a 64-bit x86 processor (physical or virtual, see Recommendations below)
* a UEFI-capable mainboard (physical or virtual, see Recommendations below)
* Vagrant (tested with 2.0)
* VirtualBox (tested with 5.1)

## Standalone

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

## Vagrant

### General

The included Vagrantfile (in the `vagrant/` directory) automates the setup process by creating a new virtual machine for
you. This VM contains all the tools required to build UEFI applications and run the console-based ones.

The virtual machine will be based on Ubuntu 17.10 (Artful Aardvark).

By using the supplied Vagrantfile you agree to the licenses of all automatically installed pieces of software,
including, but not limited to:

* Ubuntu and its components, see [https://www.ubuntu.com/about/about-ubuntu/licensing](https://www.ubuntu.com/about/about-ubuntu/licensing)
* TianoCore and its components, see [https://github.com/tianocore/tianocore.github.io/wiki/Legalese](https://github.com/tianocore/tianocore.github.io/wiki/Legalese)
* UEFIStarter of course, see "License" near the end of this document.

### Configuration

In the `vagrant/config/` directory there are 3 files you can edit:

* `authorized_keys`: this file's contents will be added to `~vagrant/.ssh/authorized_keys`, allowing you to include as
  many ssh keys as you want to access the VM. You can leave this file empty if you want, in which case you can still
  access the VM with `vagrant ssh`.
* `gitconfig`: this will be user "vagrant"'s global git configuration. If you're planning on pushing commits somewhere
  you can add your user information here.
* `vagrant-config.yml`: this contains configuration settings for the virtual machine. Currently there's just one useful
  setting there: the VM's timezone, set it to e.g. "Antarctica/South\_Pole" if you happen to live there.

### Creating the Virtual Machine

Once you have working VirtualBox and Vagrant installations and have edited the VM's configuration files to suit your
needs you can tell Vagrant to build the VM:

    $ vagrant up

If all goes well this will create the virtual machine, install a basic system, download and build parts of TianoCore
edk2, then download and build UEFIStarter's "master" branch.

This will take a few minutes. Once it's done you should get output similar to this:

    ==> dev: 'UEFIStarter' development VM. Use 'vagrant ssh' or your installed ssh key
        (localhost:2222) to connect, then go to /usr/src/edk2 and execute 'make run'

### Using the Virtual Machine

You can always use `vagrant ssh` to access the VM. If you added keys to `config/authorized_keys` you can use those to
connect as well. By default Vagrant will make the VM listen for ssh connections on localhost, port 2222.

The sources root for TianoCore edk2 is `/usr/src/edk2`. The UEFIStarter image should already be built and ready to be
started:

    $ cd /usr/src/edk2
    make run

## Standalone

### Basic Structure

Before you start you should have a working EDK2 setup, e.g. in /usr/src/edk2. All following relative paths are relative
to this.

In your sources root you'll need a UEFIStarter/ directory - either clone the repository directly into this directory or
create a symlink pointing to the UEFIStarter directory.

The framework comes with a Makefile that needs to be linked into the sources root:

    ln -s UEFIStarter/Makefile.edk Makefile

After this you should have 2 new entries in your sources root (I have the sources on another volume so "UEFIStarter" is
a symlink in my setup):

    /usr/src/edk2$ ls -lnd Makefile UEFIStarter
    lrwxrwxrwx 1 1000 1000 24 Dec 28 20:39 Makefile -> UEFIStarter/Makefile.edk
    lrwxrwxrwx 1 1000 1000 26 Dec 28 20:39 UEFIStarter -> /mnt/ueficode/UEFIStarter


### Configuration

You'll need to edit Conf/target.txt and set these values:

    ACTIVE_PLATFORM = UEFIStarter/UEFIStarter.dsc
    TARGET_ARCH = X64
    TOOL_CHAIN_TAG = GCC5
    MAX_CONCURRENT_THREAD_NUMBER = 7

Set the maximum thread number to the number of available CPU cores +1. I'm working in a VM with 6 cores, so I'm using 7
threads. If you can spare the cores the build process will make good use of these.

The Makefile needs a few changes (a lot of these issues will be improved in [EFI-5](https://github.com/rinusser/UEFIStarter/issues/5)):

* set OVMF\_IMAGE to a copy of the OVMF image
* replace /mnt/ueficode/ with the path to your copy of this repository
* replace /mnt/uefi with a mount point for the generated file system
* replace /dev/loop0 with an available loop device

The system user needs sudo privileges for these commands:

* losetup
* mkdosfs
* mount
* umount

`UEFIStarter/Makefile.edk` contains a list of all commands executed as root: you can grant sudo access to individual
command lines, allow general sudo access or do anything in between.

### Building

Once you set everything up you should be able to just invoke `make` (output shortened):

    $ make
    [...]
    make[1]: Leaving directory '/usr/src/edk2/Build/UEFIStarter/DEBUG_GCC5/X64/UEFIStarter/tests/suites/lib/testlib'

    - Done -
    Build end time: 10:05:16, Dec.30 2017
    Build total time: 00:00:14

    test -f UEFIStarter/target/uefi.img || dd if=/dev/zero of=UEFIStarter/target/uefi.img bs=512 count=93750
    [...]
    mkisofs -quiet -input-charset utf8 -o UEFIStarter/target/uefi.iso /mnt/uefi/
    sudo umount /mnt/uefi/
    sudo losetup -d /dev/loop0

When this finishes successfully the UEFIStarter/target directory contains an .img file with a filesystem for QEMU, and
an .iso image with the same contents for e.g. a VirtualBox virtual CD/DVD drive.

If you encounter build issues while the loopback device is mounted the mount point or the loopback device may be left
active and following `make` attempts will fail with e.g. "Device is busy". To fix this there's a Makefile target:

    $ make free

This will unmount and free the loopback device. After that `make` should work again.


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

Copyright (C) 2017 Richard Nusser

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
