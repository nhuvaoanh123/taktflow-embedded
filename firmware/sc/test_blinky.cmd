/* Minimal linker command file — everything at 0x00 */

--entry_point=_c_int00

MEMORY
{
    FLASH (RX) : origin=0x00000000 length=0x00200000
}

SECTIONS
{
    .text : {} > FLASH
}
