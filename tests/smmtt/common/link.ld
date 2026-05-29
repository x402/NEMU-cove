OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    . = 0x80000000;

    .text : {
        *(.text.boot)
        *(.text)
    }

    .test_registry : {
        . = ALIGN(8);
        _test_registry_start = .;
        KEEP(*(.test_registry))
        _test_registry_end = .;
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        _bss_start = .;
        *(.bss)
        *(COMMON)
        _bss_end = .;
    }

    _end = .;
}
