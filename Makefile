first_stage_boot.elf: first_stage_boot.S first_stage_boot.ld
	i686-elf-gcc first_stage_boot.S -T first_stage_boot.ld -ffreestanding -nostdlib -o first_stage_boot.elf

second_stage_boot.elf: second_stage_boot.S second_stage_boot.ld
	i686-elf-gcc second_stage_boot.S -T second_stage_boot.ld -ffreestanding -nostdlib -o second_stage_boot.elf	

disk.bin: first_stage_boot.elf second_stage_boot.elf
	i686-elf-objcopy first_stage_boot.elf -O binary disk.bin
	i686-elf-objcopy second_stage_boot.elf -O binary temp_second_stage_boot.bin
	dd if=temp_second_stage_boot.bin of=disk.bin bs=1 seek=512
# 	rm temp_second_stage_boot.bin

run: disk.bin
	qemu-system-i386 -fda disk.bin

debug: disk.bin
	qemu-system-i386 -fda disk.bin -monitor stdio -s -S

clean:
	rm -rf *.o
	rm -rf *.elf
	rm -rf *.bin