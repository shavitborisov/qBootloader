.intel_syntax noprefix # set syntax to Intel-style
.code16 # assembler directive set to 16-bit mode

.text
.global kernel_main

kernel_main:

	mov al, 'Z'
	mov ah, 0x0e # parameter for interrupt 0x10 denoting 'print'
	int 0x10
	cli # clear interrupts - disabling hardware interrupts
	hlt

.data
