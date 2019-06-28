# Loads second_stage_boot.c from the disk

# [0,512) - first_stage_boot
# [512,512 + size) - second_stage_boot
# |----> dword: size
# |----> dword: load address to memory
# |----> second_stage_boot content

.intel_syntax noprefix # set syntax to Intel-style
.code16 # assembler directive set to 16-bit mode

.text
.global start
start:
	
	# 1. Reset segments
	cli						# Clear interrupts
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, offset start 	# Stack grows downwards from 'start'
    sti						# Set (enable) interrupts

    # 2. Call interrupt 0x13(8) to get current drive parameters
    push dx					# Push parameters to stack
    push es

    mov ah, 8
    int 0x13				# dl specifies drive number which we get from the BIOS when loaded

    and cx, 0x3f			# Retrieving disk geometry
    mov track_sectors, cx
    mov al, cl
    movzx dx, dh
    add dx, 1
    mov num_sides, dx
    
    pop es					# Pop parameters from stack
    pop dx

.data
	track_sectors: 	.short 0
	num_sides: 		.short 0