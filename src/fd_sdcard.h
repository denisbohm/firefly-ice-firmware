#ifndef fd_sdcard_h
#define fd_sdcard_h

#include "fd_sdcard_spi.h"

typedef struct __attribute__((packed)) {
    unsigned one:1;
    unsigned crc:7;
    unsigned reserved_9_8:2;
    unsigned file_format:2;
    unsigned tmp_write_protect:1;
    unsigned perm_write_protect:1;
    unsigned copy:1;
    unsigned file_format_grp:1;
    unsigned reserved_20_16:5;
    unsigned write_bl_partial:1;
    unsigned write_bl_len:4;
    unsigned r2w_factor:3;
    unsigned reserved_30_29:2;
    unsigned wp_grp_enable:1;
    unsigned wp_grp_size:7;
    unsigned sector_size:7;
    unsigned erase_blk_en:1;
    unsigned reserved_47_47:1;
    unsigned c_size:22;
    unsigned reserved_75_70:6;
    unsigned dsr_imp:1;
    unsigned read_bl_misalign:1;
    unsigned write_bl_misalign:1;
    unsigned read_bl_partial:1;
    unsigned read_bl_len:4;
    unsigned ccc:12;
    unsigned tran_speed:8;
    unsigned nsac:8;
    unsigned taac:8;
    unsigned reserved_125_120:6;
    unsigned csd_structure:2;
} fd_sdcard_csd_t;

void fd_sdcard_initialize(fd_sdcard_spi_t spi);

fd_sdcard_csd_t fd_sdcard_get_csd(void);

void fd_sdcard_erase(uint32_t address, size_t length);
void fd_sdcard_read_block(uint32_t address, uint8_t *data);
void fd_sdcard_write_block(uint32_t address, const uint8_t *data);
void fd_sdcard_write_multiple_blocks_start(uint32_t address, uint32_t block_count);
void fd_sdcard_write_multiple_blocks_data(const uint8_t *data);
void fd_sdcard_write_multiple_blocks_end(void);

void fd_sdcard_read(uint32_t address, uint8_t *data, uint32_t length);
void fd_sdcard_write(uint32_t address, const uint8_t *data, uint32_t length);

void fd_sdcard_timing_clear(void);
void fd_sdcard_timing_format(void);

#endif
