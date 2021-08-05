#include "fd_sdcard.h"

#include "fd_log.h"
#include "fd_timing.h"

#include <string.h>

typedef struct {
    fd_sdcard_spi_t spi;
    fd_sdcard_csd_t csd;
    uint32_t write_multiple_blocks_count;
    fd_timing_t write_block_timing;
    fd_timing_t write_block_transceive_timing;
    fd_timing_t write_block_wait_timing;
} fd_sdcard_t;

static fd_sdcard_t fd_sdcard;

static void fd_sdcard_spi_dummy_uint8(void) {
    uint8_t data = 0xff;
    fd_sdcard.spi.transceive(&data, 1, NULL, 0);
}

static uint8_t fd_sdcard_spi_rx_uint8(void) {
    uint8_t data;
    fd_sdcard.spi.transceive(NULL, 0, &data, 1);
    return data;
}

static void fd_sdcard_spi_tx_uint8(uint8_t byte) {
    fd_sdcard.spi.transceive(&byte, 1, NULL, 0);
}

static uint16_t fd_sdcard_spi_rx_uint16(void) {
    uint8_t data[2];
    fd_sdcard.spi.transceive(NULL, 0, data, sizeof(data));
    uint16_t result = (data[0] << 8) | data[1];
    return result;
}

static void fd_sdcard_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    uint8_t data[] = {
        0x40 | cmd,
        (uint8_t)(arg >> 24),
        (uint8_t)(arg >> 16),
        (uint8_t)(arg >> 8),
        (uint8_t)(arg),
        crc | 0x01,
    };
    fd_sdcard.spi.transceive(data, sizeof(data), NULL, 0);
}

#define fd_sdcard_r1_in_idle_state 0x01

static uint8_t fd_sdcard_read_r1() {
    uint8_t r1;
    for (int i = 0; i < 1024; ++i) {
        r1 = fd_sdcard_spi_rx_uint8();
        if ((r1 & 0x80) == 0) {
            return r1;
        }
    }
    fd_log_assert_fail("sd r1 not received");
    return 0xff;
}

static uint8_t fd_sdcard_read_r7(uint32_t *data) {
    uint8_t r1 = fd_sdcard_read_r1();
    fd_log_assert((r1 & ~fd_sdcard_r1_in_idle_state) == 0);
    uint8_t bytes[4];
    fd_sdcard.spi.transceive(NULL, 0, bytes, sizeof(bytes));
    *data = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    return r1;
}

// reset the SD card and go to idle state
static uint8_t fd_sdcard_send_cmd0(void) {
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_send_cmd(0, 0x00000000, 0x95);
    uint8_t r1 = fd_sdcard_read_r1();
    fd_log_assert(r1 == fd_sdcard_r1_in_idle_state);
    return r1;
}

static void fd_sdcard_send_cmd8(void) {
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_send_cmd(8, 0x000001AA, 0x87);
    uint32_t data;
    uint8_t r1 = fd_sdcard_read_r7(&data);
    fd_log_assert(r1 == fd_sdcard_r1_in_idle_state);
}

static uint8_t fd_sdcard_send_cmd25(uint32_t address) {
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_send_cmd(25, address, 0x00);
    uint8_t r1 = fd_sdcard_read_r1();
    return r1;
}

static void fd_sdcard_send_cmd55(void) {
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_send_cmd(55, 0x00000000, 0x65);
    uint8_t r1 = fd_sdcard_read_r1();
//    fd_log_assert(r1 == 0x01);
}

static void fd_sdcard_send_cmd58(void) {
    fd_sdcard_send_cmd(58, 0x00000000, 0x00);
    uint32_t data;
    uint8_t r1 = fd_sdcard_read_r7(&data);
    fd_log_assert(r1 == 0x00);
    fd_log_assert(data & 0x80000000);
}

static uint8_t fd_sdcard_send_acmd22(uint32_t *count) {
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_send_cmd(22, 0x00000000, 0x00);
    uint8_t r1 = fd_sdcard_read_r1();
    uint8_t response;
    for (int i = 0; i < 1024; ++i) {
        response = fd_sdcard_spi_rx_uint8();
        if (response == 0xfe) {
            break;
        }
    }
    fd_log_assert(response == 0xfe);
    uint8_t bytes[6]; // count + crc
    fd_sdcard.spi.transceive(NULL, 0, bytes, sizeof(bytes));
    *count = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    return r1;
}

static uint8_t fd_sdcard_send_acmd23(uint32_t count) {
    fd_sdcard_send_cmd(23, count, 0x00);
    uint8_t r1 = fd_sdcard_read_r1();
    return r1;
}

static uint8_t fd_sdcard_send_acmd41(void) {
    fd_sdcard_send_cmd(41, 0x40000000, 0x77);
    uint8_t r1 = fd_sdcard_read_r1();
    return r1;
}

static unsigned extract(uint8_t *data, int hi, int lo) {
    unsigned value = 0;
    for (int bit = hi; bit >= lo; --bit) {
        int index = 15 - bit / 8;
        int shift = bit % 8;
        value = (value << 1) | ((data[index] >> shift) & 0x01);
    }
    return value;
}

static void fd_sdcard_send_cmd9(void) {
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_send_cmd(9, 0x00000000, 0x00);
    uint8_t r1 = fd_sdcard_read_r1();
    fd_log_assert(r1 == 0x00);

    uint8_t response;
    for (int i = 0; i < 1024; ++i) {
        response = fd_sdcard_spi_rx_uint8();
        if (response == 0xfe) {
            break;
        }
    }
    fd_log_assert(response == 0xfe);

    uint8_t data[18]; // 127:1, 1, crc:16
    fd_sdcard.spi.transceive(NULL, 0, data, sizeof(data));
    fd_sdcard.csd.one = extract(data, 0, 0);
    fd_sdcard.csd.crc = extract(data, 7, 1);
    fd_sdcard.csd.reserved_9_8 = extract(data, 9, 8);
    fd_sdcard.csd.file_format = extract(data, 11, 10);
    fd_sdcard.csd.tmp_write_protect = extract(data, 12, 12);
    fd_sdcard.csd.perm_write_protect = extract(data, 13, 13);
    fd_sdcard.csd.copy = extract(data, 14, 14);
    fd_sdcard.csd.file_format_grp = extract(data, 15, 15);
    fd_sdcard.csd.reserved_20_16 = extract(data, 20, 16);
    fd_sdcard.csd.write_bl_partial = extract(data, 21, 21);
    fd_sdcard.csd.write_bl_len = extract(data, 25, 22);
    fd_sdcard.csd.r2w_factor = extract(data, 28, 26);
    fd_sdcard.csd.reserved_30_29 = extract(data, 30, 29);
    fd_sdcard.csd.wp_grp_enable = extract(data, 31, 31);
    fd_sdcard.csd.wp_grp_size = extract(data, 38, 32);
    fd_sdcard.csd.sector_size = extract(data, 45, 39);
    fd_sdcard.csd.erase_blk_en = extract(data, 46, 46);
    fd_sdcard.csd.reserved_47_47 = extract(data, 47, 47);
    fd_sdcard.csd.c_size = extract(data, 69, 48);
    fd_sdcard.csd.reserved_75_70 = extract(data, 75, 70);
    fd_sdcard.csd.dsr_imp = extract(data, 76, 76);
    fd_sdcard.csd.read_bl_misalign = extract(data, 77, 77);
    fd_sdcard.csd.write_bl_misalign = extract(data, 78, 78);
    fd_sdcard.csd.read_bl_partial = extract(data, 79, 79);
    fd_sdcard.csd.read_bl_len = extract(data, 83, 80);
    fd_sdcard.csd.ccc = extract(data, 95, 84);
    fd_sdcard.csd.tran_speed = extract(data, 103, 96);
    fd_sdcard.csd.nsac = extract(data, 111, 104);
    fd_sdcard.csd.taac = extract(data, 119, 112);
    fd_sdcard.csd.reserved_125_120 = extract(data, 125, 120);
    fd_sdcard.csd.csd_structure = extract(data, 127, 126);
}

fd_sdcard_csd_t fd_sdcard_get_csd(void) {
    return fd_sdcard.csd;
}

void fd_sdcard_erase(uint32_t address, size_t length) {
    fd_sdcard_send_cmd(32, address, 0x00);
    uint8_t r1 = fd_sdcard_read_r1();
    fd_log_assert(r1 == 0);
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_send_cmd(33, address + length, 0x00);
    r1 = fd_sdcard_read_r1();
    fd_log_assert(r1 == 0);
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_send_cmd(38, 0x00000000, 0x00);
    r1 = fd_sdcard_read_r1();
    fd_log_assert(r1 == 0);
    uint8_t response = 0;
    do {
        response = fd_sdcard_spi_rx_uint8();
    } while (response == 0);
}

void fd_sdcard_read_block(uint32_t address, uint8_t *data) {
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_send_cmd(17, address, 0x00);
    uint8_t r1 = fd_sdcard_read_r1();
    fd_log_assert(r1 == 0);
    for (int i = 0; i < 1024; ++i) {
        uint8_t response = fd_sdcard_spi_rx_uint8();
        if (response != 0xfe) {
            continue;
        }

        fd_sdcard.spi.transceive(NULL, 0, data, 512);
        uint16_t crc = fd_sdcard_spi_rx_uint16();
        return;
    }
}

void fd_sdcard_write_block(uint32_t address, const uint8_t *data) {
    fd_timing_start(&fd_sdcard.write_block_timing);

    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_send_cmd(24, address, 0x00);
    uint8_t r1 = fd_sdcard_read_r1();
    fd_log_assert(r1 == 0);

    fd_sdcard_spi_tx_uint8(0xfe);

    fd_timing_start(&fd_sdcard.write_block_transceive_timing);
    fd_sdcard.spi.transceive(data, 512, NULL, 0);
    fd_timing_end(&fd_sdcard.write_block_transceive_timing);

    uint8_t response = 0;
    for (int i = 0; i < 1024; ++i) {
        response = fd_sdcard_spi_rx_uint8();
        if ((response & 0x1f) == 0x05) {
            break;
        }
    }
    fd_log_assert((response & 0x1f) == 0x05);

    fd_timing_start(&fd_sdcard.write_block_wait_timing);
    for (int i = 0; i < 100000; ++i) {
        response = fd_sdcard_spi_rx_uint8();
        if (response != 0x00) {
            break;
        }
    }
    fd_log_assert(response != 0x00);
    fd_timing_end(&fd_sdcard.write_block_wait_timing);

    fd_timing_end(&fd_sdcard.write_block_timing);
}

void fd_sdcard_write_multiple_blocks_start(uint32_t address, uint32_t block_count) {
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_send_cmd55();
//    int sector_count = fd_sdcard.csd.sector_size + 1;
    uint8_t r1 = fd_sdcard_send_acmd23(block_count);
    fd_log_assert(r1 == 0x00);
    r1 = fd_sdcard_send_cmd25(address);
    fd_log_assert(r1 == 0x00);
    fd_sdcard.write_multiple_blocks_count = 0;
}

void fd_sdcard_write_multiple_blocks_data(const uint8_t *data) {
    fd_timing_start(&fd_sdcard.write_block_timing);

    fd_sdcard_spi_tx_uint8(0xfc); // start block
    fd_timing_start(&fd_sdcard.write_block_transceive_timing);
    fd_sdcard.spi.transceive(data, 512, NULL, 0);
    fd_timing_end(&fd_sdcard.write_block_transceive_timing);
    fd_sdcard_spi_tx_uint8(0x00); // crc[15:8]
    fd_sdcard_spi_tx_uint8(0x00); // crc[7:0]
    uint8_t response = fd_sdcard_spi_rx_uint8();
    fd_log_assert((response & 0x1f) == 0x05);
    ++fd_sdcard.write_multiple_blocks_count;

    fd_timing_start(&fd_sdcard.write_block_wait_timing);
    uint8_t after = 0;
    do {
        after = fd_sdcard_spi_rx_uint8();
    } while (after == 0x00);
    fd_log_assert(after == 0xff);
    fd_timing_end(&fd_sdcard.write_block_wait_timing);
    fd_timing_end(&fd_sdcard.write_block_timing);
}

void fd_sdcard_write_multiple_blocks_end(void) {
    fd_sdcard_spi_tx_uint8(0xfd); // stop block
    uint8_t response = fd_sdcard_spi_rx_uint8();
    fd_log_assert(0xff);

    fd_timing_start(&fd_sdcard.write_block_wait_timing);
    uint8_t after = 0;
    do {
        after = fd_sdcard_spi_rx_uint8();
    } while (after == 0x00);
    fd_log_assert(after == 0xff);
    fd_timing_end(&fd_sdcard.write_block_wait_timing);

    fd_sdcard_send_cmd55();
    uint32_t well_written_block_count = 0;
    uint8_t r1 = fd_sdcard_send_acmd22(&well_written_block_count);
    fd_log_assert(well_written_block_count == fd_sdcard.write_multiple_blocks_count);

    fd_timing_end(&fd_sdcard.write_block_timing);
}

void fd_sdcard_read(uint32_t address, uint8_t *data, uint32_t length) {
    uint8_t block[512];
    while (length > 0) {
        uint32_t block_address = address & (sizeof(block) - 1);
        uint32_t offset = address - block_address;
        uint32_t remainder = sizeof(block) - offset;
        uint32_t sublength = length <= remainder ? length : remainder;
        memset(block, 0, sizeof(block));
        fd_sdcard_read_block(block_address, block);
        memcpy(data, &block[offset], sublength);

        address += sizeof(block);
        data += sublength;
        length -= sublength;
    }
}

void fd_sdcard_write(uint32_t address, const uint8_t *data, uint32_t length) {
    uint8_t block[512];
    while (length > 0) {
        uint32_t block_address = address & (sizeof(block) - 1);
        uint32_t offset = address - block_address;
        uint32_t remainder = sizeof(block) - offset;
        uint32_t sublength = length <= remainder ? length : remainder;
        memset(block, 0, sizeof(block));
        fd_sdcard_read_block(block_address, block);
        memcpy(&block[offset], data, sublength);
        fd_sdcard_write_block(block_address, block);
        memset(block, 0, sizeof(block));
        fd_sdcard_read_block(block_address, block);

        address += sizeof(block);
        data += sublength;
        length -= sublength;
    }
}

void fd_sdcard_timing_clear(void) {
    fd_timing_clear(&fd_sdcard.write_block_timing);
    fd_timing_clear(&fd_sdcard.write_block_transceive_timing);
    fd_timing_clear(&fd_sdcard.write_block_wait_timing);
}

void fd_sdcard_timing_format(void) {
    char buffer[256];
    fd_timing_format(&fd_sdcard.write_block_timing, buffer, sizeof(buffer));
    fd_timing_format(&fd_sdcard.write_block_transceive_timing, buffer, sizeof(buffer));
    fd_timing_format(&fd_sdcard.write_block_wait_timing, buffer, sizeof(buffer));
}

void fd_sdcard_initialize(fd_sdcard_spi_t spi) {
    fd_sdcard.spi = spi;
    fd_sdcard_timing_clear();

    // send 74 or more clock cycles while not selected to synchronize
    fd_sdcard.spi.set_chip_select(true);
    fd_sdcard.spi.set_frequency_slow();
    uint8_t synchronize_data[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    fd_sdcard.spi.transceive(synchronize_data, sizeof(synchronize_data), NULL, 0);

    fd_sdcard.spi.set_chip_select(false);
    for (int i = 0; i < 8; ++i) {
        uint8_t r1 = fd_sdcard_send_cmd0();
        if (r1 == fd_sdcard_r1_in_idle_state) {
            break;
        }
    }
    fd_sdcard_send_cmd8();
    uint32_t r1;
    do {
        fd_sdcard_send_cmd55();
        r1 = fd_sdcard_send_acmd41();
    } while (r1 == fd_sdcard_r1_in_idle_state);
    fd_sdcard_send_cmd58();

    fd_sdcard.spi.set_frequency_fast();
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_spi_dummy_uint8();
    fd_sdcard_send_cmd9();

    // check support for individual block erase
    fd_log_assert(fd_sdcard.csd.erase_blk_en);
}
