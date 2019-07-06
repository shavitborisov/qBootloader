# Loads second_stage_boot.c from the disk

.intel_syntax noprefix      # set syntax to Intel-style
.code16                     # assembler directive set to 16-bit mode

MAX_ATTEMPTS     = 5
SIZE_OF_SECTOR   = 512
METADATA_SEGMENT = 0x0800

.data
    cylinder_sectors:    .short  0
    num_sides:           .short  0
    original_boot_drive: .byte   0

.text
.global start
start:  
    jmp 0:start_boot # enforce 0:0x7c00 instead of (0x7c0:0)

start_boot:
    # 1. Reset segments
    cli                     # Clear interrupts
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, offset start    # Stack grows downwards from 'start'
    sti                     # Set (enable) interrupts

    mov byte ptr original_boot_drive, dl # Save the drive the bootloader was booted from

    # 2. Call interrupt 0x13(8) to get current drive parameters
    push dx                 # Push parameters to stack
    push es

    mov ah, 8
    int 0x13                # dl specifies drive number which we get from the BIOS when loaded

    and cx, 0x3f            # Retrieving disk geometry
    mov cylinder_sectors, cx
    mov al, cl              # TODO: ?
    movzx dx, dh
    add dx, 1
    mov num_sides, dx
    
    pop es                  # Pop parameters from stack
    pop dx

    # 3. Resets disk before reading
    call reset_drive

    # 4. Load second stage metadata into memory
    mov ax, 1               # LBA number
    mov cx, 1               # Number of sectors
    mov bx, METADATA_SEGMENT
    mov es, bx              # Buffer segment
    xor bx, bx              # Buffer offset
    call read_disk_to_memory

    # 5. Load entire second stage into specified load address
    mov ax, 1               # LBA number
    mov cx, METADATA_SEGMENT
    mov es, cx
    mov cx, [es:0]          # Second-stage size in sectors
    mov bx, [es:2]          # Second-stage load segment
    
    push bx                 # Save load segment for later
    
    mov es, bx              # Buffer segment
    xor bx, bx              # Buffer offset
    call read_disk_to_memory

    xor ax, ax              # Clear es segment
    mov es, ax

    pop bx                  # Restore load segment value

    shl bx, 4               # Multiply by 16
    add bx, 4               # Add code start offset
    mov ax, original_boot_drive # Pass AX=original_boot_drive as first parameter to second stage main
    jmp bx                  # Jump to second-stage-boot

    jmp boot_failure        # Error

# The funcion reads from disk into memeory and receives the following parameters:
# cx - Number of sectors, ax - LBA of sector 0, es:bx - Address to store read data
read_disk_to_memory:
    push cx
    call read_single_sector
    inc ax                  # ++LBA - going to next sector
    add bx, SIZE_OF_SECTOR  # Advancing to next sector address in memory and making sure it doesn't overflow
    jc read_disk_to_memory.segment_overflow
    jmp read_disk_to_memory.sector_next

read_disk_to_memory.segment_overflow:
    mov cx, es
    add ch, 0x10            # Handle overflow
    mov es, cx

read_disk_to_memory.sector_next:
    pop cx
    loopnz read_disk_to_memory
    ret

# The function reads a single sector into memory and receives the following parameters:
# ax - LBA of wanted sector, es:bx - Address to store read data
read_single_sector:
    xor cx, cx              # cx holds the number of tries so far (starting from 0)

read_single_sector.read:
    push ax
    push cx

    call translate_logical_address

    mov ah, 2               # Calls interrupt 0x13(2) to read disk sectors
    mov al, 1
    mov cl, sector
    mov ch, cylinder
    mov dh, head
    mov dl, original_boot_drive
    int 0x13

    jnc read_single_sector.success

    # Error - no carry. Trying again
    pop cx
    inc cx
    cmp cx, MAX_ATTEMPTS

    je boot_failure         # Too many tries - error

    call reset_drive
    pop ax
    jmp read_single_sector.read

read_single_sector.success:
    pop cx
    pop ax
    ret

.data
    cylinder: .byte   0
    head:     .byte   0
    sector:   .byte   0

.text
# Receives LBA and returns CHS according to documented formula
translate_logical_address:
    xor dx, dx
    div word ptr cylinder_sectors
    inc dx
    mov sector, dl
    xor dx, dx
    divw word ptr num_sides
    mov head, dl
    mov cylinder, al

    ret

# Reset drive specified by dl, calls interrupt 0x13(0) to reset disk system
reset_drive:
    mov ax, 0
    int 0x13                # Clears reset flag in controller, pulls heads to cylinder 0
    jc boot_failure         # If there's carry, jump to error
    ret

.data
    error_string:       .string     "We have reached an error...\0"

.text
boot_failure:
    mov bx, offset error_string
    call print_string
    hlt

# Print string at bx 
# Calls interrupt 0x10(0x0e) to write text in teletype mode
print_string:
    mov al, [bx]
    inc bx
    cmp al, 0
    je print_string.reached_null

    mov ah, 0x0e
    int 0x10

    jmp print_string

print_string.reached_null:
    ret