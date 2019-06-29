ENTRY("second_stage_main")

SECTIONS {
    . = 0x8000; /* Must be aligned to 16 bytes */

    second_stage_start = .;

    .metadata : {
        SHORT((second_stage_end - second_stage_start) / 512 + 1);
        SHORT(second_stage_start / 16);
    }

    .text : {
    	*(.main);
        *(.text);
    }

    .data : {
        *(.data);
        *(.rodata);
        *(.bss);
        *(COMMON);
    }

    second_stage_end = .;
}
