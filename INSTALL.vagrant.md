# Installation with Vagrant

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

In the `vagrant/config/` directory there are 3 files you'll probably want to edit:

* `authorized_keys`: this file's contents will be added to `~vagrant/.ssh/authorized_keys`, allowing you to include as
  many ssh keys as you want to access the VM. You can leave this file empty if you want, in which case you can still
  access the VM with `vagrant ssh`.
* `gitconfig`: this will be user "vagrant"'s global git configuration. If you're planning on pushing commits somewhere
  you can add your user information here.
* `vagrant-config.yml`: this contains configuration settings for the virtual machine. You can e.g. set your time zone
  and configure a shared directory with your host system.

### Creating the Virtual Machine

Once you have working VirtualBox and Vagrant installations and have edited the VM's configuration files to suit your
needs you can tell Vagrant to build the VM:

    $ vagrant up

If all goes well this will create the virtual machine, install a basic system, download and build parts of TianoCore
edk2, download UEFIStarter if not mounted already and then build it.

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
