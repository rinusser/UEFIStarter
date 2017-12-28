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

### Bare Minimum

* a 64-bit x86 processor (physical or virtual, see Recommendations below)
* a UEFI-capable mainboard (physical or virtual, see Recommendations below)
* Linux (tested with Ubuntu 17.04) with sgdisk, mkisofs and mkdosfs installed
* a working TianoCore development setup

### Recommendations

You can reboot into the UEFI applications if you want (and have the spare machine or don't mind rebooting all the time),
but it's easier to use virtualization for development. There are multiple options:

* QEMU with OVMF - I use mostly this, except for running graphical applications. It's very fast to work with. The OVMF
  image adds EFI support to QEMU, you'll need to download and install it separately.
* VirtualBox - has built-in EFI support. I use this to run the graphical UEFI applications: it easily outperforms QEMU,
  at least on my machines.
* Other: you're probably fine with any hypervisor that supports EFI.


# Installation

TODO


# Usage

TODO


# Tests

TODO


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

