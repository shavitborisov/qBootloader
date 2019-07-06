#include <stdint.h>
asm(".code16gcc");

#define SECTOR_SIZE 512

char getchar(void) {
    int ax = 0;
    __asm__ __volatile__("int $0x16" : "+a"(ax) ::); // INT 0x16(0) AH=0 returns character in ax
    return ax;
}

void putchar(char c) {
    const uint16_t interrupt_arg = (0x0e << 8) + c; // AH=0xe, AL=character
    __asm__ __volatile__("int $0x10" :: "a"(interrupt_arg) :);
}

void puts(const char* str) {
    while (*str != '\0') {
        putchar(*(str++));
    }
}

void print_base_unsigned(uint32_t x, uint32_t base) {
    const char* base_digits = "0123456789ABCDEF";
    static char number[33] = { 0 };

    char* p = number + sizeof(number) - 1;
    *p = '\0';

    do {
        *(--p) = base_digits[x % base];
        x /= base;
    } while (x != 0);

    puts(p);    
}

void print_base_signed(int32_t x, uint32_t base) {
    if (x < 0) {
        putchar('-');
        x = -x;
    }

    print_base_unsigned((uint32_t)x, base);
}

void print_decimal(int32_t x) {
    print_base_signed(x, 10);
}

void print_hex(uint32_t x) {
    print_base_unsigned(x, 16);
}

void hexdump(const void* buffer, uint16_t size) {
    const uint8_t* data = buffer;

    for (uint16_t i = 0; i < size; ++i) {
        if (i % 16 == 0)
            puts("\r\n");

        print_hex(data[i]);
        putchar(' ');
    }

    puts("\r\n");
}

void get_string(char str[], uint16_t size) {
    if (size == 0)
        return;

    --size; // for null terminator
    for (uint16_t i = 0; i < size; ++i) {
        char c = getchar();
        if (c == '\r') {
            break;
        }

        putchar(c);
        *(str++) = c;
    }

    *str = '\0';
}

typedef struct __attribute__((packed)) {
    uint8_t   size;
    uint8_t   unused;
    uint16_t  blocks;
    uint16_t  buffer_offset;
    uint16_t  buffer_segment;
    uint32_t  lba_low;
    uint32_t  lba_high;
} disk_address_packet_t;

/**
 * Reads a single sector from a given disk to memory, by calling INT 0x13(0x42) - Extended
 * read sectors from drive (supporting 32-bit LBA reading).
 * 
 * @param bios_disk [disk to read sector from]
 * @param buffer [destination buffer - assumed to be a valid 16-bit address]
 * @param lba [sector in bios_disk to read from]
 *
 * @return [success status]
 */
int read_sector_extended(uint8_t bios_disk, void* buffer, uint32_t lba) {
    disk_address_packet_t packet = {
        .size = sizeof(disk_address_packet_t),
        .unused = 0,
        .blocks = 1,
        .buffer_offset = (uint16_t)(uint32_t)buffer,
        .buffer_segment = 0,
        .lba_low = lba,
        .lba_high = 0,
    };

    volatile uint16_t error = 0;
    __asm__ __volatile__ (
        "movw  $0, %0\n"
        "int   $0x13\n"
        "setcb %0\n"
        : "=m"(error)
        : "a"(0x4200), "d"(bios_disk), "S"(&packet)
        : "memory"
    );

    // puts("read_sector_extended bios_disk=0x");
    // print_hex((uint32_t)bios_disk);
    // puts(" buffer=0x");
    // print_hex((uint32_t)buffer);
    // puts(" lba=");
    // print_decimal(lba);
    // puts(error ? " failed\r\n" : " succeeded\r\n");

    return !error;
}

typedef struct __attribute__((packed)) {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  flags_limit;
    uint8_t  base_high;
} gdt_entry_t;

// typedef struct __attribute__((packed)) {
//     uint16_t size;
//     uint32_t first_entry_address;
// } gdt_selector_t;

// gdt_entry_t gdt_entries[] = {
//     { 0 }, // null entry,
//     { // Code segment (flat memory)
//         .limit_low = 0xffff,
//         .base_low = 0,
//         .base_middle = 0,
//         .access = 0x9a,
//         .flags_limit = 0xcf,
//         .base_high = 0,
//     },
//     { // Data segment (flat memory)
//         .limit_low = 0xffff,
//         .base_low = 0,
//         .base_middle = 0,
//         .access = 0x92,
//         .flags_limit = 0xcf,
//         .base_high = 0,
//     }
// };

// gdt_selector_t gdt_selector = {
//     .size = sizeof(gdt_entries) - 1,
//     .first_entry_address = (uint32_t)gdt_entries,
// };

// #define ENABLE_PROTECTED_MODE() \
//     do { \
//         __asm__ __volatile__("int $0x15" :: "a"(0x2401) :); \
//         __asm__ __volatile__( \
//             "cli\n" \
//             "lgdtl (%0)\n" \
//             "mov %%cr0, %%eax\n" \
//             "or $1, %%al\n" \
//             "mov %%eax, %%cr0\n" \
//             "\n" \
//             "ljmpl $0x08, $1f\n" \
//             "1:\n" \
//             "mov $0x10, %%ax\n" \
//             "mov %%ax, %%ds\n" \
//             "mov %%ax, %%es\n" \
//             "mov %%ax, %%gs\n" \
//             "mov %%ax, %%fs\n" \
//             "mov %%ax, %%ss\n" \
//             "sti\n" \
//             :: "m" (gdt_selector) \
//             : "eax" \
//         ); \
//     } while (0)

// #define DISABLE_PROTECTED_MODE() \
//     do { \
//         __asm__ __volatile__( \
//             "cli\n" \
//             "mov %%cr0, %%eax\n" \
//             "and $0xfe, %%al\n" \
//             "mov %%eax, %%cr0\n" \
//             "xor %%ax, %%ax\n" \
//             "mov %%ax, %%ds\n" \
//             "mov %%ax, %%gs\n" \
//             "mov %%ax, %%ss\n" \
//             "mov %%ax, %%fs\n" \
//             "ljmpl $0x00, $2f\n" \
//             "2:\n" \
//             "sti\n" \
//             :: \
//             : "eax" \
//         ); \
//     } while (0)

/**
 * Set the base address of a GDT entry.
 * 
 * @param entry [ptr to GDT entry]
 * @param addr [wanted base address]
 */
void set_gdt_base(gdt_entry_t* entry, const void* addr) {
    uint32_t base = (uint32_t)addr;

    entry->base_low    = (base & 0xFFFF);
    entry->base_middle = (base >> 16) & 0xFF;
    entry->base_high   = (base >> 24) & 0xFF;
}

/**
 * Copies memory from src to dest with 32-bit addressing by using INT 0x15(0x87) - Move
 * block to copy with 32-bit addressing, without explicitly entering protected mode.
 * 
 * @param dest [destination address]
 * @param src [source address]
 * @param size [number of bytes to copy]
 * 
 * @return [success status]
 */
int high_memory_copy(const void* dest, const void* src, uint16_t size) {
    static gdt_entry_t entries[] = {
        { 0 }, { 0 }, {             // source GDT entry
            .limit_low = 0xffff,
            .base_low = 0,
            .base_middle = 0,
            .access = 0x93,
            .flags_limit = 0xf,
            .base_high = 0,
        }, {                        // dest GDT entry
            .limit_low = 0xffff,
            .base_low = 0,
            .base_middle = 0,
            .access = 0x93,
            .flags_limit = 0xf,
            .base_high = 0,
        },
        { 0 }, { 0 }
    };
    
    set_gdt_base(&entries[2], src);
    set_gdt_base(&entries[3], dest);

    volatile uint16_t error = 0;
    __asm__ __volatile__ (
        "movw  $0, %0\n"
        "int   $0x15\n"
        "setcb %0\n"
        : "=m"(error)
        : "a"(0x8700), "c"(size/2), "S"(&entries)
        : "memory"
    );

    // puts("high_memory_copy dst=0x");
    // print_hex((uint32_t)dest);
    // puts(" src=0x");
    // print_hex((uint32_t)src);
    // puts(" size=");
    // print_decimal(size);
    // puts(error ? " failed\r\n" : " succeeded\r\n");

    return !error;
}

/**
 * Copies multiple sectors from disk to high memory.
 * 
 * @param bios_disk [disk to copy from]
 * @param dest [32-bit destination address in memory]
 * @param lba [bios_disk initial lba to copy from (supporting 32-bit address)]
 * @param sectors [number of sectors to copy from bios_disk (starting from lba)]
 * 
 * @return [success status]
 */
int copy_disk_to_high_memory(uint8_t bios_disk, void* dest, uint32_t lba, uint32_t sectors) {
    uint8_t sector[SECTOR_SIZE];

    for (uint32_t i = 0; i < sectors; ++i) {
        if (!read_sector_extended(bios_disk, sector, lba + i))
            return 0;

        if (!high_memory_copy((uint8_t*)dest + SECTOR_SIZE * i, sector, SECTOR_SIZE))
            return 0;
    }

    return 1;
}

typedef struct __attribute__((packed)) {
    uint8_t  mbr_code[0x1f1];   // padding to setup_sects offset, assuming struct starts from sector start
    uint8_t  setup_sects;
    uint16_t root_flags;
    uint32_t syssize;
    uint16_t ram_size;
    uint16_t vid_mode;
    uint16_t root_dev;
    uint16_t boot_flag;
    uint16_t jump;
    uint32_t header;
    uint16_t version;
    uint32_t realmode_swtch;
    uint16_t start_sys_seg;
    uint16_t kernel_version;
    uint8_t  type_of_loader;
    uint8_t  loadflags;
    uint16_t setup_move_size;
    uint32_t code32_start;
    uint32_t ramdisk_image;
    uint32_t ramdisk_size;
    uint32_t bootsect_kludge;
    uint16_t heap_end_ptr;
    uint8_t  ext_loader_ver;
    uint8_t  ext_loader_type;
    uint32_t cmd_line_ptr;
    uint32_t initrd_addr_max;
    uint32_t kernel_alignment;
    uint8_t  relocatable_kernel;
    uint8_t  min_alignment;
    uint16_t xloadflags;
    uint32_t cmdline_size;
    uint32_t hardware_subarch;
    uint64_t hardware_subarch_data;
    uint32_t payload_offset;
    uint32_t payload_length;
    uint64_t setup_data;
    uint64_t pref_address;
    uint32_t init_size;
    uint32_t handover_offset;
} linux_boot_protocol_data_t;

#define KERNEL_SETUP          ((uint8_t*)0x010000)
#define KERNEL_STACK          ((uint8_t*)0x018000)
#define KERNEL_COMMAND        ((uint8_t*)0x01e000)
#define KERNEL_PROTECTED_MODE ((uint8_t*)0x100000)
#define KERNEL_RAMDISK        ((uint8_t*)0x7fab000)

#define CAN_USE_HEAP_END 0x80

#define BOOT_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            error_line = __LINE__; \
            goto boot_error; \
        } \
    } while (0)

/**
 * Second stage boot - Loads Linux kernel.
 * 
 * @param original_boot_drive [bios disk out of which we have booted]
 * 
 * This function never returns!
 */
void __attribute__((section(".main"))) __attribute__((regparm(3))) second_stage_main(uint16_t original_boot_drive) {
    int error_line = 0;

    puts(
        "         ____              __  __                __           \r\n"
        "  ____ _/ __ )____  ____  / /_/ /___  ____ _____/ /__  _____  \r\n"
        " / __ `/ __  / __ \\/ __ \\/ __/ / __ \\/ __ `/ __  / _ \\/ ___/  \r\n"
        "/ /_/ / /_/ / /_/ / /_/ / /_/ / /_/ / /_/ / /_/ /  __/ /      \r\n"
        "\\__, /_____/\\____/\\____/\\__/_/\\____/\\__,_/\\__,_/\\___/_/       \r\n"
        "  /_/                                                         \r\n");

    puts("\r\nWelcome to qBootloader! Press any key to continue...\r\n");
    getchar();

    // uint8_t sector[SECTOR_SIZE];

    // print_decimal(original_boot_drive);
    // puts("\r\n");
    // print_decimal(sizeof(disk_address_packet_t));
    // puts("\r\n");

    // // if (!read_sector_extended(sector, original_boot_drive, 8226584)) {
    // if (!read_sector_extended(sector, original_boot_drive, 0)) {
    //     puts("can't read sector 1\r\n");
    // }
    // hexdump(sector, 0x10);

    // if (!read_sector_extended(sector+SECTOR_SIZE, original_boot_drive, 1)) {
    //     puts("can't read sector 2\r\n");
    // }
    // hexdump(sector+SECTOR_SIZE, 0x10);

    // puts("sizeof(linux_boot_protocol_data_t): ");
    // print_decimal(sizeof(linux_boot_protocol_data_t));
    // puts("\r\n");

    uint8_t first_sector[SECTOR_SIZE];
    BOOT_ASSERT(read_sector_extended(original_boot_drive, first_sector, KERNEL_LBA));

    // hexdump(first_sector, 0x10);
    // puts("...");
    // hexdump(first_sector + 0x1f0, 0x10);

    uint8_t setup_sects = ((linux_boot_protocol_data_t*)first_sector)->setup_sects;
    if (setup_sects == 0)
        setup_sects = 4;

    // puts("setup_sects: 0x");
    // print_hex(setup_sects);
    // puts("\r\n");

    puts("\r\nLoading kernel setup...");
    BOOT_ASSERT(copy_disk_to_high_memory(original_boot_drive, KERNEL_SETUP, KERNEL_LBA, setup_sects + 1));

    // Initialize boot protocol data by copying it from KERNEL_SETUP, overriding mandatory values
    // and copying it back to KERNEL_SETUP. We can't access KERNEL_SETUP memory directly as it is
    // in high memory (above 16-bit), so we use high_memory_copy to copy in and out of high memory.

    linux_boot_protocol_data_t boot_protocol;
    BOOT_ASSERT(high_memory_copy(&boot_protocol, KERNEL_SETUP, sizeof(linux_boot_protocol_data_t)));

    boot_protocol.type_of_loader  = 0xe1;
    boot_protocol.ext_loader_type = 1;
    boot_protocol.loadflags      |= CAN_USE_HEAP_END;
    boot_protocol.heap_end_ptr    = KERNEL_COMMAND - KERNEL_SETUP - 0x200;
    boot_protocol.cmd_line_ptr    = (uint32_t)KERNEL_COMMAND;
    boot_protocol.ramdisk_image   = (uint32_t)KERNEL_RAMDISK;
    boot_protocol.ramdisk_size    = INITRD_SECTORS * SECTOR_SIZE;

    BOOT_ASSERT(high_memory_copy(KERNEL_SETUP, &boot_protocol, sizeof(linux_boot_protocol_data_t)));

    static const char cmd[] = KERNEL_CMD;
    BOOT_ASSERT(high_memory_copy(KERNEL_COMMAND, cmd, sizeof(cmd)));

    uint32_t kernel_pm_bytes   = boot_protocol.syssize * 16;
    uint32_t kernel_pm_sectors = kernel_pm_bytes / SECTOR_SIZE + 1;
    uint32_t kernel_pm_lba     = KERNEL_LBA + setup_sects + 1;

    // puts("\r\nkernel_pm_bytes: ");
    // print_decimal(kernel_pm_bytes);
    // puts("\r\nkernel_pm_sectors: ");
    // print_decimal(kernel_pm_sectors);
    // puts("\r\nkernel_pm_lba: ");
    // print_decimal(kernel_pm_lba);

    puts("\r\nLoading protected mode kernel...");
    BOOT_ASSERT(copy_disk_to_high_memory(original_boot_drive, KERNEL_PROTECTED_MODE, kernel_pm_lba, kernel_pm_sectors));

    // BOOT_ASSERT(high_memory_copy(first_sector, KERNEL_PROTECTED_MODE, SECTOR_SIZE));
    // hexdump(first_sector, 0x10);
    // BOOT_ASSERT(high_memory_copy(first_sector, KERNEL_PROTECTED_MODE + kernel_pm_bytes - 0x10, 0x10));
    // hexdump(first_sector, 0x10);

    puts("\r\nLoading ramdisk...");
    BOOT_ASSERT(copy_disk_to_high_memory(original_boot_drive, KERNEL_RAMDISK, INITRD_LBA, INITRD_SECTORS));

    puts("Running kernel with following boot paramers:\r\n");
    puts(cmd);
    puts("\r\n");

    // jump to kernel
    __asm__ __volatile__(
        "cli\r\n"
        "mov $0x1000, %ax\r\n"
        "mov %ax, %ds\r\n"
        "mov %ax, %es\r\n"
        "mov %ax, %fs\r\n"
        "mov %ax, %gs\r\n"
        "mov %ax, %ss\r\n"
        "mov $0xe000, %sp\r\n"
        "ljmp $0x1020,$0x0\r\n" // Kernel setup entry point
    );

    // puts("\r\nsetup_sects: ");
    // print_hex(boot_protocol.setup_sects);
    // puts("\r\nsyssize: ");
    // print_hex(boot_protocol.syssize);
    // puts("\r\nboot_flag: ");
    // print_hex(boot_protocol.boot_flag);
    // puts("\r\njump: ");
    // print_hex(boot_protocol.jump);
    // puts("\r\nheader: ");
    // print_hex(boot_protocol.header);
    // puts("\r\nversion: ");
    // print_hex(boot_protocol.version);
    // puts("\r\nstart_sys_seg: ");
    // print_hex(boot_protocol.start_sys_seg);
    // puts("\r\nkernel_version: ");
    // print_hex(boot_protocol.kernel_version);
    // puts("\r\ninitrd_addr_max: ");
    // print_hex(boot_protocol.initrd_addr_max);
    // puts("\r\nkernel_alignment: ");
    // print_hex(boot_protocol.kernel_alignment);
    // puts("\r\nrelocatable_kernel: ");
    // print_hex(boot_protocol.relocatable_kernel);
    // puts("\r\nmin_alignment: ");
    // print_hex(boot_protocol.min_alignment);
    // puts("\r\ncmdline_size: ");
    // print_hex(boot_protocol.cmdline_size);
    // puts("\r\npayload_offset: ");
    // print_hex(boot_protocol.payload_offset);
    // puts("\r\npayload_length: ");
    // print_hex(boot_protocol.payload_length);
    // puts("\r\npref_address: ");
    // print_hex(boot_protocol.pref_address);
    // puts("\r\ninit_size: ");
    // print_hex(boot_protocol.init_size);
    // puts("\r\nhandover_offset: ");
    // print_hex(boot_protocol.handover_offset);

boot_error:
    puts("Boot Failure :(\r\n");
    puts("Error Line: ");
    print_decimal(error_line);
    puts("\r\n");
    while (1) { }
}