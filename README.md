![Logo](qB_logo.png)

# qBootloader
**qBootloader** is a simple, modular, multi-stage bootloader for Linux. 

## Requirements
* Linux on 32/64-bit x86 CPU. Supports both 32/64 bit Linux kernels.
* i686-elf-gcc cross compiler toolchain.

## Prerequisites
qBootloader only supports booting from non-fragmented kernel and and initrd files.
To make sure these are non fragmented (on ext4 FS), run the following:

```
sudo e4defrag /boot/<vmlinuz-kernel>
sudo e4defrag /boot/<initrd-image>
```

A non fragmented kernel file should have a similar output with hdparm.

```
$ sudo hdparm --fibmap /boot/vmlinuz-4.18.0-10-generic 
/boot/vmlinuz-4.18.0-10-generic:
 filesystem blocksize 4096, begins at LBA 2048; assuming 512 byte sectors.
 byte_offset  begin_LBA    end_LBA    sectors
           0    8226584    8243271      16688
```

The important thing is to have `byte_offset 0` and a single line of sectors.

## Installation
Build and install qBootloader to disk with:
```
make install_to_disk KERNEL_DISK_FILE=<kernel-disk-device> KERNEL_FILE=/boot/<vmlinuz-kernel> INITRD_FILE=</boot/<initrd-image>
```

## Screenshots
Building qBootloader:
![Building qBootloader](/screenshots/1.png "Building qBootloader")

Running qBootloader:
![Running qBootloader](/screenshots/2.png "Running qBootloader")

Ubuntu 18.10 (amd64) booted with qBootloader:
![Ubuntu 18.10 (amd64) booted with qBootloader](/screenshots/3.JPG "Ubuntu 18.10 (amd64) booted with qBootloader")

## License
[GNU GPLv3](https://choosealicense.com/licenses/gpl-3.0/)
