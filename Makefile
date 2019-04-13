boot.elf: boot.S boot.ld
	i686-elf-gcc boot.S -T boot.ld -ffreestanding -nostdlib -o boot.elf

dummy_kernel.elf: dummy_kernel.S dummy_kernel.ld
	i686-elf-gcc dummy_kernel.S -T dummy_kernel.ld -ffreestanding -nostdlib -o dummy_kernel.elf	

disk.bin: boot.elf dummy_kernel.elf
	i686-elf-objcopy boot.elf -O binary disk.bin
	i686-elf-objcopy dummy_kernel.elf -O binary temp_dummy_kernel.bin
	dd if=temp_dummy_kernel.bin of=disk.bin bs=1 seek=512
	rm temp_dummy_kernel.bin

run: disk.bin
	qemu-system-i386 -fda disk.bin

debug: disk.bin
	qemu-system-i386 -fda disk.bin -monitor stdio -s -S

clean:
	rm -rf *.o
	rm -rf *.elf
	rm -rf *.bin