# Manual Installation

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

The Makefile needs a few changes:

* set OVMF\_IMAGE to your copy of the OVMF image
* set LOOP\_DEVICE to an available loop device

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
