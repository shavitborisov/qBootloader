ENTRY("start")

SECTIONS {
    . = 0x7c00;             /* Sets the first address out of which all segments are mapped */

    .text : {               /* Text will be mapped to 0x7c00 as mentioned above */
        *(.text);
    }

    .data : {               /* Data will be mapped right after Text */
        *(.data);
    }

    .magic : AT(0x7dfe) {   /* MBR Magic at offset 0x1fe from start of 1st sector of disk (0x7dfe = 0x7c00 + 0x1fe) */
        SHORT(0xaa55);
    }
}