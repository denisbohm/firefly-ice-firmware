/* Linker script to configure memory regions for EFM32LG330F256.
 *   FLASH.ORIGIN: starting address of flash
 *   FLASH.LENGTH: length of flash
 *   RAM.ORIGIN: starting address of RAM bank 0
 *   RAM.LENGTH: length of RAM bank 0
 *
 * Denis Bohm / Firefly Design
 */
MEMORY
{
  FLASH (rx) : ORIGIN = 0x0, LENGTH = 0x40000 /* 256K */
  RAM (rwx) : ORIGIN = 0x10000000, LENGTH = 0x8000 /* 32K */
}
