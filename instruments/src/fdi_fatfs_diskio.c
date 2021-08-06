#include "ff.h"
#include "diskio.h"

#include "fd_sdcard.h"

#include "fd_unused.h"

DWORD get_fattime(void) {
    const int year = 2021;
    const int month = 1;
    const int day = 1;
    const int hour = 0;
    const int minute = 0;
    const int second = 0;
    return (DWORD)(
        ((year - 1980) << 25) | // bit31:25 - Year origin from the 1980 (0..127, e.g. 37 for 2017)
        (month << 21)         | // bit24:21 - Month (1..12)
        (day << 16)           | // bit20:16- Day of the month (1..31)
        (hour << 11)          | // bit15:11- Hour (0..23)
        (minute << 5)         | // bit10:5- Minute (0..59)
        second);                // bit4:0- Second / 2 (0..29, e.g. 25 for 50)
}

DSTATUS disk_status(BYTE drive fd_unused) {
    return 0;
}

DSTATUS disk_initialize(BYTE drive fd_unused) {
    return 0;
}

DRESULT disk_read(BYTE drive fd_unused, BYTE *data, LBA_t sector, UINT sector_count) {
    for (UINT i = 0; i < sector_count; ++i) {
        uint32_t address = (uint32_t)(sector + i);
        fd_sdcard_read_block(address, &data[i * 512]);
    }
    return RES_OK;
}

DRESULT disk_write(BYTE drive fd_unused, const BYTE *data, LBA_t sector, UINT sector_count) {
    for (UINT i = 0; i < sector_count; ++i) {
        uint32_t address = (uint32_t)(sector + i);
        fd_sdcard_write_block(address, &data[i * 512]);
    }
    return RES_OK;
}

DRESULT disk_ioctl(BYTE drive fd_unused, BYTE cmd, void *data) {
    switch (cmd) {
        case CTRL_SYNC: {
        } break;
        case GET_SECTOR_COUNT: {
            fd_sdcard_csd_t csd = fd_sdcard_get_csd();
            uint32_t sector_count = (csd.c_size + 1u) * 1024u;
            *(LBA_t *)data = sector_count;
        } break;
        case GET_SECTOR_SIZE: {
            *(WORD *)data = 512;
        } break;
        case GET_BLOCK_SIZE: {
            fd_sdcard_csd_t csd = fd_sdcard_get_csd();
            if (csd.erase_blk_en) {
                *(DWORD *)data = 512;
            } else {
                uint32_t block_size = (1u << csd.write_bl_len) * (csd.sector_size + 1u);
                if (block_size <= 32768) {
                    *(DWORD *)data = block_size;
                } else {
                    *(DWORD *)data = 1; // unknown
                }
            }
        } break;
        case CTRL_TRIM: {
        } break;
        default: {
            return RES_PARERR;
        } break;
    }
    return RES_OK;
}
