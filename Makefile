SHELL := /bin/bash

first_stage_boot.elf: first_stage_boot.S first_stage_boot.ld
	i686-elf-gcc first_stage_boot.S -T first_stage_boot.ld -ffreestanding -nostdlib -o first_stage_boot.elf

second_stage_boot.elf: second_stage_boot.c second_stage_boot.ld
	i686-elf-gcc -DKERNEL_LBA=`sudo hdparm --fibmap $(KERNEL_FILE) | tail -n 1 | awk '{print $$2}'` \
	             -DINITRD_LBA=`sudo hdparm --fibmap $(INITRD_FILE) | tail -n 1 | awk '{print $$2}'` \
	             -DINITRD_SECTORS=`sudo hdparm --fibmap $(INITRD_FILE) | tail -n 1 | awk '{print $$4}'` \
	             -DKERNEL_CMD="\"`cat /proc/cmdline`\"" \
	             second_stage_boot.c -Os -T second_stage_boot.ld -ffreestanding -nostdlib -o second_stage_boot.elf	

disk.bin: first_stage_boot.elf second_stage_boot.elf
	i686-elf-objcopy first_stage_boot.elf -O binary disk.bin
	i686-elf-objcopy second_stage_boot.elf -O binary temp_second_stage_boot.bin
	dd if=temp_second_stage_boot.bin of=disk.bin bs=1 seek=512

run_qemu: disk.bin
	qemu-system-i386 -hda disk.bin

debug_qemu: disk.bin
	qemu-system-i386 -hda disk.bin -monitor stdio -s -S

install_to_disk: disk.bin
	sudo dd if=disk.bin of=$(KERNEL_DISK_FILE) bs=1 count=446
	sudo dd if=disk.bin of=$(KERNEL_DISK_FILE) skip=512 seek=512 bs=1

clean:
	rm -rf *.o
	rm -rf *.elf
	rm -rf *.bin