asm(".code16gcc");

// Dummy bootloader, infinite loop
void __attribute__((section(".main"))) second_stage_main() {
	while (1) { }
	// __asm__ __volatile__ ("int 0x13" ::: "a"(), "b"())
}