#include "../segmentation.h"

OUTPUT_ARCH(POWERPC)

MEMORY
{
    m_text       :   org = TEXT_BASE,      len = TEXT_SIZE
    m_firmware   :   org = FIRMWARE_BASE,  len = FIRMWARE_SIZE
    m_data       :   org = DATA_BASE,      len = DATA_SIZE
    m_bss        :   org = BSS_BASE,       len = BSS_SIZE
}

SECTIONS
{
    .firmware :
    {
        __firmware_start = .;
        *(.firmware)
        __firmware_end = .; 
    } > m_firmware

    .text :
    {
        __text_start = .;
        *(.text)
        __text_end = .; 
    } > m_text

    .data :
    {
        __data_start = .;
        *(.data)
        __data_end = .; 
    } > m_data

    .bss :
    {
        __bss_start = .;
	/* CREATE_OBJECT_SYMBOLS */
        *(.bss)
        __bss_end = .; 
    } > m_bss
}

ENTRY(_start);
