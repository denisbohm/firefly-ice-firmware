#include "fd_hal_processor.h"
#include "fd_hal_system.h"
#include "fd_pins.h"
#include "fd_usb.h"

#include <em_cmu.h>
#include <em_emu.h>
#include <em_i2c.h>
#include <em_int.h>
#include <em_gpio.h>
#include <em_msc.h>
#include <em_system.h>
#include <em_usart.h>

#include <stddef.h>

/*
void *memset(void *s, int c, size_t n) {
    uint8_t* p = s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}
*/

void fd_hal_processor_interrupts_disable(void) {
    INT_Disable();
}

void fd_hal_processor_interrupts_enable(void) {
    INT_Enable();
}

/*
void CMU_IRQHandler(void) {
    uint32_t interrupts = CMU_IntGet();
    CMU_IntClear(CMU_IF_HFXORDY | CMU_IF_HFRCORDY);

    if (interrupts & CMU_IF_HFXORDY) {
        CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
//        CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
    } else
    if (interrupts & CMU_IF_HFRCORDY) {
        CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO);
//        CMU_OscillatorEnable(cmuOsc_HFXO, false, false);
    }
}
*/

void fd_hal_processor_sleep(void) {
    CMU_ClockEnable(cmuClock_HFPER, false);
    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_LFXO);
    CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
}

void fd_hal_processor_wake(void) {
    CMU_OscillatorEnable(cmuOsc_HFRCO, true, true);
    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO);
    CMU_ClockEnable(cmuClock_HFPER, true);
}

void fd_hal_processor_wait(void) {
    EMU_EnterEM2(true);
}

void fd_hal_processor_initialize(void) {
//    CMU_HFRCOBandSet(cmuHFRCOBand_14MHz);

//    CMU_ClockDivSet(cmuClock_HFPER, cmuClkDiv_1);
//    CMU_IntEnable(CMU_IF_HFXORDY | CMU_IF_HFRCORDY);
//    NVIC_EnableIRQ(CMU_IRQn);

//    CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
//    CMU_ClockEnable(cmuClock_HFPER, true);

    CMU_ClockEnable(cmuClock_GPIO, true);

    // unused port pins
    GPIO_PinModeSet(gpioPortA, 0, gpioModeDisabled, 0);
    GPIO_PinModeSet(gpioPortA, 5, gpioModeDisabled, 0);
    GPIO_PinModeSet(gpioPortA, 6, gpioModeDisabled, 0);
    //
    GPIO_PinModeSet(gpioPortC, 2, gpioModeDisabled, 0);
    GPIO_PinModeSet(gpioPortC, 3, gpioModeDisabled, 0);
    GPIO_PinModeSet(gpioPortC, 4, gpioModeDisabled, 0);
    GPIO_PinModeSet(gpioPortC, 5, gpioModeDisabled, 0);
    //
    GPIO_PinModeSet(gpioPortE, 8, gpioModeDisabled, 0);
    GPIO_PinModeSet(gpioPortE, 13, gpioModeDisabled, 0);
    GPIO_PinModeSet(gpioPortE, 14, gpioModeDisabled, 0);
    GPIO_PinModeSet(gpioPortE, 15, gpioModeDisabled, 0);
    //
    GPIO_PinModeSet(gpioPortF, 12, gpioModeDisabled, 0);

    GPIO_PinModeSet(BAT_VDIV2EN_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(BAT_VDIV2_PORT_PIN, gpioModeDisabled, 0); // analog input

    GPIO_PinModeSet(LED0_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED4_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED5_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED6_PORT_PIN, gpioModePushPull, 1);

    GPIO_PinModeSet(LED_EN_PORT_PIN, gpioModePushPull, 0);

    GPIO_PinModeSet(I2C0_SDA_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(I2C0_SCL_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(I2C0_INT_PORT_PIN, gpioModeInputPull, 0);

    GPIO_PinModeSet(MAG_INT_PORT_PIN, gpioModeInputPull, 0);
    GPIO_IntConfig(MAG_INT_PORT_PIN, true /* rising */, true /* falling */, true);

    GPIO_PinModeSet(ACC_CSN_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(ACC_INT_PORT_PIN, gpioModeInput, 0);
    GPIO_IntConfig(ACC_INT_PORT_PIN, true /* rising */, true /* falling */, true);

    GPIO_PinModeSet(MEM_CSN_PORT_PIN, gpioModePushPull, 0); // low because it is on a un-powered bus

    GPIO_PinModeSet(US0_PWR_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(US0_MISO_PORT_PIN, gpioModeInput, 0);
    GPIO_PinModeSet(US0_MOSI_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(US0_CLK_PORT_PIN, gpioModePushPull, 0);

    GPIO_PinModeSet(SWD_CLK_PORT_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(SWD_IO_PORT_PIN, gpioModeDisabled, 0);

    GPIO_PinModeSet(CHG_STAT_PORT_PIN, gpioModeInput, 0);
    GPIO_IntConfig(CHG_STAT_PORT_PIN, true /* rising */, true /* falling */, true);
    GPIO_PinModeSet(CHG_RATE_PORT_PIN, gpioModeDisabled, 0); // analog input

    GPIO_PinModeSet(PWR_SEL_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(PWR_MODE_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(PWR_HIGH_PORT_PIN, gpioModePushPull, 0);

    GPIO_PinModeSet(USB_DM_PORT_PIN, gpioModeInput, 0);
    GPIO_PinModeSet(USB_DP_PORT_PIN, gpioModeInput, 0);

    GPIO_PinModeSet(US1_MISO_PORT_PIN, gpioModeInputPull, 1);
    GPIO_PinModeSet(US1_MOSI_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(US1_CLK_PORT_PIN, gpioModePushPull, 1);

    GPIO_PinModeSet(CLK32_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(NRF_RESETN_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(NRF_REQN_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(NRF_RDYN_PORT_PIN, gpioModeInputPull, 1);
    GPIO_IntConfig(NRF_RDYN_PORT_PIN, true /* rising */, true /* falling */, true);

    GPIO_PinModeSet(I2C1_PWR_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(I2C1_SDA_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(I2C1_SCL_PORT_PIN, gpioModePushPull, 0);

//    GPIO_PinModeSet(LFXTAL_P_PORT_PIN, gpioModePushPull, 0);
//    GPIO_PinModeSet(LFXTAL_N_PORT_PIN, gpioModePushPull, 0);

//    GPIO_PinModeSet(HFXTAL_P_PORT_PIN, gpioModePushPull, 0);
//    GPIO_PinModeSet(HFXTAL_P_PORT_PIN, gpioModePushPull, 0);
}

void fd_delay_3x_cycles(uint32_t cycles) __attribute__((naked, used));

void fd_delay_3x_cycles(uint32_t cycles __attribute__((unused))) {
    __asm(
        "    subs r0, #1\n"
        "    bne fd_delay_3x_cycles\n"
        "    bx lr"
    );
}

#define CYCLES_PER_SECOND (48000000/2)

void fd_hal_processor_delay_us(uint32_t us) {
    fd_delay_3x_cycles((us * CYCLES_PER_SECOND) / 3000000);
}

void fd_hal_processor_delay_ms(uint32_t ms) {
    while (ms--) {
        fd_delay_3x_cycles(CYCLES_PER_SECOND / 3000);
    }
}

#define LOCK_BITS_ADDRESS 0x0FE04000
#define DEBUG_LOCK_WORD_ADDRESS (LOCK_BITS_ADDRESS + 127)

bool fd_hal_processor_get_debug_lock(void) {
    uint32_t *address = (uint32_t*)DEBUG_LOCK_WORD_ADDRESS;
    uint32_t debug_lock = *address;
    return (debug_lock & 0xf) != 0xf;
}

void fd_hal_processor_set_debug_lock(void) {
    if (!fd_hal_processor_get_debug_lock()) {
        uint32_t word = 0xfffffff0;
        fd_hal_processor_interrupts_disable();
        MSC_Init();
        MSC_WriteWord((uint32_t*)DEBUG_LOCK_WORD_ADDRESS, &word, 4);
        MSC_Deinit();
        fd_hal_processor_interrupts_enable();
    }
}

// 16 byte hardware id: vendor, product, version (major, minor), unique id
void fd_hal_processor_get_hardware_id(fd_binary_t *binary) {
    fd_binary_put_uint16(binary, fd_usb_get_vendor_id());
    fd_binary_put_uint16(binary, fd_usb_get_product_id());
    fd_binary_put_uint16(binary, fd_hal_system_get_hardware_major());
    fd_binary_put_uint16(binary, fd_hal_system_get_hardware_minor());
    fd_binary_put_uint64(binary, SYSTEM_GetUnique());
}

fd_range_t fd_hal_processor_get_boot_range(void) {
    return fd_range_make(0x00000000, 0x00007000);
}

fd_range_t fd_hal_processor_get_crypto_range(void) {
    return fd_range_make(0x00007000, 0x00000800);
}

fd_range_t fd_hal_processor_get_firmware_update_metadata_range(void) {
    return fd_range_make(0x00007800, 0x00000800);
}

fd_range_t fd_hal_processor_get_firmware_range(void) {
    return fd_range_make(0x00008000, 0x00040000 - 0x00008000);
}

fd_boot_data_t *fd_hal_processor_get_boot_data_address(void) {
     return (fd_boot_data_t *)0x6f00;
}

#define USER_DATA_ADDRESS 0x0fe00000 // user data is 2kB

uint8_t *fd_hal_processor_get_provision_map_address(void) {
    return (uint8_t *)(USER_DATA_ADDRESS + 20);
}

void fd_hal_processor_write_user_data(uint8_t *data, uint32_t length) {
    uint32_t *address = (uint32_t*)USER_DATA_ADDRESS;
    uint32_t n = (length + 3) & ~0x3; // round up to multiple of 4 bytes
    fd_hal_processor_interrupts_disable();
    MSC_Init();
    MSC_ErasePage(address);
    MSC_WriteWord(address, data, n);
    MSC_Deinit();
    fd_hal_processor_interrupts_enable();
}

void fd_hal_processor_write_flash_data(uint32_t address, uint8_t *data, uint32_t length) {
    fd_hal_processor_interrupts_disable();
    MSC_Init();
    MSC_ErasePage((void *)address);
    MSC_WriteWord((void *)address, data, length);
    MSC_Deinit();
    fd_hal_processor_interrupts_enable();
}