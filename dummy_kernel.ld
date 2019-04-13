ENTRY("kernel_main")

SECTIONS {
    . = 0x20000;

    .text : {
        *(.text);
    }

    .data : {
        *(.data);
    }
}