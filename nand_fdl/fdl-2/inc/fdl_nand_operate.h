#ifndef FDL_NAND_OPERATE_H
#define FDL_NAND_OPERATE_H

#include <asm/arch/cmd_def.h>

#define NAND_SUCCESS                0
#define NAND_SYSTEM_ERROR           1
#define NAND_UNKNOWN_DEVICE         2
#define NAND_INVALID_DEVICE_SIZE    3
#define NAND_INCOMPATIBLE_PART      4
#define NAND_INVALID_ADDR           5
#define NAND_INVALID_SIZE           6

static __inline DLSTATUS convert_err (int err)
{
    switch (err)
    {
        case NAND_SUCCESS:
            return BSL_REP_ACK;
        case NAND_INVALID_ADDR:
            return BSL_REP_DOWN_DEST_ERROR;
        case NAND_INVALID_SIZE:
            return BSL_REP_DOWN_SIZE_ERROR;
        case NAND_UNKNOWN_DEVICE:
            return BSL_UNKNOWN_DEVICE;
        case NAND_INVALID_DEVICE_SIZE:
            return BSL_INVALID_DEVICE_SIZE;
        case NAND_INCOMPATIBLE_PART:
            return BSL_INCOMPATIBLE_PARTITION;
        default:
            return BSL_REP_OPERATION_FAILED;
    }
}
/**
 * fdl2_download_start
 *
 * Get download info from download start command which  
 * will used in next step
 *
 * @param part partition/volume name
 * @param size total download size
 * @param nv_checksum NA
 * @return 0 failed
 *             1 success
 */
int fdl2_download_start(char* name, unsigned long size, unsigned long nv_checksum);

/**
 * fdl2_download_midst
 *
 * Save data to fdl buf and finally write it to nand flash
 *
 * @param size total download size
 * @param buf data recvd
 * @return 0 failed
 *             1 success
 */
int fdl2_download_midst(unsigned short size, char *buf);

/**
 * fdl2_download_end
 *
 * Set download end
 *
 * @param void
 * @return 0 failed
 *             1 success
 */
int fdl2_download_end(void);

/**
 * fdl2_nand_read_start
 *
 * Get partition/volume info from read start command which  
 * will used in next step
 *
 * @param part partition/volume name
 * @param size total size
 * @return 0 failed
 *             1 success
 */
int fdl2_read_start(char* part, unsigned long size);

/**
 * fdl2_nand_read_midst
 *
 * Read partition/volume data
 *
 * @param size size to be read
 * @param off offset of begin of part/vol
 * @param buf data saved
 * @return 0 failed
 *             1 success
 */
int fdl2_read_midst(unsigned long size, unsigned long off, unsigned char *buf);

/**
 * fdl2_read_end
 *
 * Set read flash end
 *
 * @param void
 * @return 0 failed
 *             1 success
 */
int fdl2_read_end(void);

/**
 * fdl2_erase
 *
 * Erase partition/volume
 *
 * @param part partition/volume name
 * @param size size to be erased(no use now)
 * @return 0 failed
 *             1 success
 */
int fdl2_erase(char* part, unsigned long size);

/**
 * fdl2_repartition
 *
 * Resize/Add/Delete volumes
 *
 * @param vol_cfg volume cfg
 * @param total_vol_num
 * @return 0 failed
 *             1 success
 */
int fdl2_repartition(void* vol_cfg, unsigned short total_vol_num);

#endif  /*FDL_NAND_OPERATE_H*/

