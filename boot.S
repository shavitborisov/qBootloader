.intel_syntax noprefix # set syntax to Intel-style
.code16 # assembler directive set to 16-bit mode

.text
.global start
start:
	
	mov al, 'A'
	mov ah, 0x0e # parameter for interrupt 0x10 denoting 'print'
	int 0x10
	cli
	hlt

.data
