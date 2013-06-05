/*****************************************************************************
 **  File Name:    effuse_drv.h                                                 *
 **  Author:       Jenny Deng                                                *
 **  Date:         20/10/2009                                                *
 **  Copyright:    2009 Spreadtrum, Incorporated. All Rights Reserved.       *
 **  Description:  This file defines the basic operation interfaces of       *
 **                EFuse initilize and operation. It provides read and         *
 **                writer interfaces of 0~5 efuse. Efuse 0 for Sn block.     *
 **                Efuse 1 to 4 for Hash blocks. Efuse 5 for control block.  *
 *****************************************************************************
 *****************************************************************************
 **  Edit History                                                            *
 **--------------------------------------------------------------------------*
 **  DATE               Author              Operation                        *
 **  20/10/2009         Jenny.Deng          Create.                          *
 **  26/10/2009         Yong.Li             Update.                          *
 **  30/10/2009         Yong.Li             Update after review.             *
 *****************************************************************************/

#ifndef _EFuse_DRV_H
#define _EFuse_DRV_H

#include "sci_types.h"

/***********************structure define**************************************/

// the function return value
typedef enum
{
    EFuse_RESULT_SUCCESS        = 0x00,
    EFuse_READ_FAIL             = 0x01,
    EFuse_WRITE_FAIL            = 0x02,
    EFuse_WRITE_SOFT_COMPARE_FAIL   = 0x03, // compare write data with read data after write
    EFuse_WRITE_VERIFY_FAIL     	= 0x04,
    EFuse_PARAMETER_ERROR       	= 0x05,
    EFuse_ID_ERROR              	= 0x06,    //block id is not between 0 and 5
    EFuse_HAS_WRITEED_ERROR			= 0x07,
    EFuse_PTR_LENGTH_ERROR      	= 0x08,
    EFuse_WRITE_HARD_COMPARE_FAIL   = 0x09, // compare write data use hardware compare
    EFuse_WRITE_SOFT_HARD_COMPARE_FAIL   = 0x0A, // compare write data use hardware compare
    EFuse_BIST_FAIL					= 0xB,
    EFuse_BIST_TIMEOUT			    = 0xC,
    EFuse_NOT_LOCK				    = 0xD,
    EFuse_LOCKED				    = 0xE
} EFuse_RETURN_E;

// the EFuse read mode value, bit [6:5] of efuse_ctrl
typedef enum
{
    EFuse_USER_MODE     = 0,
    EFuse_MARGIN_MODE_2 = 2,
    EFuse_MARGIN_MODE_1 = 3
} EFuse_READ_MODE_E;

// the control bit of control block, bit 1 for secure_boot
typedef enum
{
    UNDEFINED        = 0,
    EFuse_SECURE_BOOT  = 1,
    EFuse_MAX_CTRL_BIT = 2
} EFuse_CONTROL_BIT_E;

typedef enum
{
    OTP_RESULT_SUCCESS        = 0x00,
    OTP_READ_FAIL             = 0x01,
    OTP_WRITE_FAIL            = 0x02,
    OTP_WRITE_COMPARE_FAIL    = 0x03, // compare write data with read data after write
    OTP_WRITE_VERIFY_FAIL     = 0x04,
    OTP_PARAMETER_ERROR       = 0x05,
    OTP_ID_ERROR              = 0x06,    //block id is not between 0 and 5
    OTP_PTR_LENGTH_ERROR      = 0x07
} OTP_RETURN_E;

typedef enum
{
    EFuse_MIN_ID        = 0,
    EFuse_SN_ID         = EFuse_MIN_ID,
    EFuse_HASH_STARTID  = 1,
    EFuse_HASH_ENDID    = 4,
    EFuse_CONTRL_ID     = 5,
    EFuse_MAX_ID        = 8
} EFuse_BLOCK_ID_E;

/***********************function declaration**********************************/


//*****************************************************************************
// Description : initilize EFuse
// Global resource dependence :
// Author: Jenny Deng
// Note:
//*****************************************************************************
typedef void (*EFuseInitilizePtr)(void);

//*****************************************************************************
// Description : close EFuse
// Global resource dependence :
// Author: Jenny Deng
// Note:
//*****************************************************************************
typedef void (*EFuseClosePtr)(void);

//*****************************************************************************
// Description : configure EFuse
// Global resource dependence :
// Author: Yong Li
// Note:
//*****************************************************************************
//*****************************************************************************
// Description : read data from EFuse block
// Global resource dependence :
// Author: Jenny Deng
// Note:
// OUTPUT: parameter r_data_ptr is the read data pointer
// RETURN VALUE: EFuse_ID_ERROR         when block_id is out of value 0~5
//               EFuse_READ_FAIL        when fail to read block data
//               EFuse_RESULT_SUCCESS   when read successfully
//*****************************************************************************
typedef EFuse_RETURN_E (*EFuseReadPtr)(
    unsigned int block_id,                 // the selected effuse block id
    unsigned int *r_data_ptr               // pointer of block data read from EFuse block
);

//*****************************************************************************
// Description : write data to EFuse block
// Global resource dependence :
// Author: Jenny Deng
// Note:
// OUTPUT:
// RETURN VALUE: EFuse_ID_ERROR           when block_id is out of value 0~5
//               EFuse_READ_FAIL          when fail to read block data
//               EFuse_WRITE_FAIL         when fail to write data to block
//               EFuse_RESULT_SUCCESS     when write data successfully
//               EFuse_WRITE_COMPARE_FAIL when the data read after write is not
//                                     equal to the write data add the raw data
//               EFuse_WRITE_VERIFY_FAIL  when fail to read after write
//*****************************************************************************
typedef EFuse_RETURN_E (*EFuseWritePtr)(
    unsigned int block_id,                  // the selected EFuse block id
    unsigned int data                       // the data to be writen into the EFuse block
);

//*****************************************************************************
// Description : lock EFuse block
// Global resource dependence :
// Author: Jenny Deng
// Note:
// OUTPUT:
// RETURN VALUE: EFuse_RESULT_SUCCESS      when lock block successfully
//               EFuse_READ_FAIL           when fail to read block data
//               EFuse_WRITE_FAIL          when fail to write data to block
//               EFuse_WRITE_COMPARE_FAIL  when the data read after write is not
//                                     equal to the write data add the raw data
//               EFuse_WRITE_VERIFY_FAIL   when fail to read after write
//*****************************************************************************
typedef EFuse_RETURN_E (*EFuseLockPtr)(
    unsigned int block_id                     //the selected EFuse block id
);

//*****************************************************************************
// Description : lock EFuse Hash blocks (block 1~4)
// Global resource dependence :
// Author: Jenny Deng
// Note:
// OUTPUT:
// RETURN VALUE: EFuse_RESULT_SUCCESS      when lock hash block successfully
//               EFuse_READ_FAIL           when fail to read block data
//               EFuse_PARAMETER_ERROR     when con_bit is not "1"
//               EFuse_WRITE_FAIL          when fail to write data to block
//               EFuse_WRITE_COMPARE_FAIL  when the data read after write is not
//                                     equal to the write data add the raw data
//               EFuse_WRITE_VERIFY_FAIL   when fail to read after write
//*****************************************************************************
typedef  EFuse_RETURN_E (*EFuseLockHashPtr)(void);

//*****************************************************************************
// Description : lock EFuse Sn block (block 0)
// Global resource dependence :
// Author: Jenny Deng
// Note:
// OUTPUT:
// RETURN VALUE: EFuse_RESULT_SUCCESS      when lock sn block successfully
//               EFuse_READ_FAIL           when fail to read block data
//               EFuse_WRITE_FAIL          when fail to write data to block
//               EFuse_WRITE_COMPARE_FAIL  when the data read after write is not
//                                     equal to the write data add the raw data
//               EFuse_WRITE_VERIFY_FAIL   when fail to read after write
//*****************************************************************************
// Description : read EFuse Sn block data (block 0)
// Global resource dependence :
// Author: Jenny Deng
// Note:
// OUTPUT: parameter des_data is the read data pointer
// RETURN VALUE: EFuse_ID_ERROR        when block_id is out of value 0~5
//               EFuse_READ_FAIL       when fail to read block data
//               EFuse_RESULT_SUCCESS  when read successfully
//*****************************************************************************
// Description : read EFuse Hash blocks data (block 1~4)
// Global resource dependence :
// Author: Jenny Deng
// Note:
// OUTPUT: parameter des_data_ptr is the read data pointer
// RETURN VALUE: EFuse_ID_ERROR        when block_id is out of value 0~5
//               EFuse_READ_FAIL       when fail to read block data
//               EFuse_RESULT_SUCCESS  when read successfully
//*****************************************************************************
typedef EFuse_RETURN_E (*EFuseReadHashPtr)(
    unsigned int *des_data_ptr        //the pointer of destination data
);

//*****************************************************************************
// Description : write data to EFuse Hash blocks (block 1~4)
// Global resource dependence :
// Author: Jenny Deng
// Note:
// OUTPUT:
// RETURN VALUE: EFuse_ID_ERROR            when block_id is out of value 0~5
//               EFuse_READ_FAIL           when fail to read block data
//               EFuse_WRITE_FAIL          when fail to write data to block
//               EFuse_RESULT_SUCCESS      when write successfully
//               EFuse_WRITE_COMPARE_FAIL  when the data read after write is not
//                                     equal to the write data add the raw data
//               EFuse_WRITE_VERIFY_FAIL   when fail to read after write
//*****************************************************************************
typedef EFuse_RETURN_E (*EFuseWriteHashPtr)(
    unsigned int *src_dat_ptr        //the pointer of source data
);

//*****************************************************************************
// Description : If the EFuse block is locked the *b_is_lock_ptr is TRUE,
//               else is FALSE
// Global resource dependence :
// Author: Jenny Deng
// Note:
// OUTPUT: If the EFuse block is locked the *b_is_lock_ptr is TRUE, else is FALSE
// RETURN VALUE: EFuse_ID_ERROR             when block_id is out of value 0~5
//               EFuse_READ_FAIL            when fail to read block data
//               EFuse_RESULT_SUCCESS       when read data successfully
//*****************************************************************************
typedef  EFuse_RETURN_E (*EFuseIsLockPtr)(
    unsigned int block_id,                  //the selected EFuse block id
    BOOLEAN *b_is_lock_ptr            //the pointer to store the compare result
);
PUBLIC  EFuse_RETURN_E EFuseRead(
    unsigned int block_id,                 // the selected efuse block id
    unsigned int *r_data_ptr               // pointer of block data read from EFuse block
);

#endif

