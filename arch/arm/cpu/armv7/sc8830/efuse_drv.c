/******************************************************************************
 ** File Name:      module_test.c                                             *
 ** Author:         Yong.Li                                                   *
 ** DATE:           15/01/2010                                                *
 ** Copyright:      2003 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    define trace interface just for testing usage             *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 15/01/2010     Yong.Li          Create                                    *
 ******************************************************************************/
#include "asm/arch/sprd_reg.h"
#include "asm/arch/efuse_drv.h"
#include "asm/arch/chip_drv_common_io.h"

#define EFUSE_MAX_BLOCK          (8)
#define GR_SOFT_REST             (0x4b00004c)
#define EFUSE_REG_BASE           (SPRD_UIDEFUSE_PHYS)
#define EFUSE_EB	         BIT_7
#define EFUSE_SOFT_RST	         BIT_28

#define EFUSE_DATA_RD            (EFUSE_REG_BASE + 0x0000)
#define EFUSE_DATA_WR	         (EFUSE_REG_BASE + 0x0004)

#define EFUSE_BLOCK_INDEX        (EFUSE_REG_BASE + 0x0008)
#define EFUSE_MODE_CTRL	         (EFUSE_REG_BASE + 0x000c)
#define EFUSE_PGM_PARA	         (EFUSE_REG_BASE + 0x0010)
#define EFUSE_STATUS	         (EFUSE_REG_BASE + 0x0014)
#define EUSE_MEM_BLOCK_FLAGS     (EFUSE_REG_BASE + 0x0018)
#define EUSE_MEM_BLOCK_FLAGS_CLR (EFUSE_REG_BASE + 0x001c)
#define EFUSE_MAGIC_NUMBER	 (EFUSE_REG_BASE + 0x0020)

#define EFuse_LOCK_BIT           BIT_31
//efuse mode ctrl register bit define
#define EFUSE_PG_START		 BIT_0
#define EFUSE_RD_START		 BIT_1
#define EFUSE_BIST_START	 BIT_31

//efuse block index bit define
#define READ_INDEX_OFFSET	 (0)
#define PGM_INDEX_OFFSET	 (16)
#define BIST_START_INDEX_OFFSET	 (26)
#define BIST_END_INDEX_OFFSET	 (29)

//PGM_PARA register bit define
#define EFUSE_AUTO_TEST_EN	 BIT_16
#define CLK_EFS_EN		 BIT_28
#define EFS_VDD_ON		 BIT_29
#define PGM_EN			 BIT_31

//efuse status register bit define
#define EFUSE_PRG_BUSY     	 BIT_0
#define EFUSE_READ_BUSY    	 BIT_1
#define EFUSE_BIST_FAIL		 BIT_4
#define EFUSE_BIST_BUSY		 BIT_5

//Mem_block_flags register bit define
#define EFUSE_BLOCK_ERR_FLAG	 BIT_8
#define	WDG_EB_BIT	  	 (1  <<   2)
#define	RTC_WDG_EB_BIT	  	 (1  <<  10)

//timeout define
#define EFUSE_READ_TIMEOUT       5 //ms
#define EFUSE_WRITE_TIMEOUT      5 //ms

PUBLIC EFuse_RETURN_E IsEfuseLock(int block_id)
{
    uint32 read_data;
    EFuse_RETURN_E ret;
    ret = EFuseRead(block_id, (unsigned int *)&read_data);
    //if read fail return
    if(ret != EFuse_RESULT_SUCCESS)
    {
        return EFuse_READ_FAIL;
    }
    // if the lock bit is set return locked else return not locked
    if((read_data & (EFuse_LOCK_BIT)) == EFuse_LOCK_BIT)
    {
        return EFuse_LOCKED;
    }
    else
    {
        return EFuse_NOT_LOCK;
    }
}

PUBLIC EFuse_RETURN_E EFuseWrite(
    unsigned int block_id,                  // the selected EFuse block id
    unsigned int data                       // the data to be writen into the EFuse block
)
{
    uint32 old_tick = 0;
    uint32 new_tick = 0;
    uint32 sts = 0; //states of efuse
    unsigned int read_data;
    EFuse_RETURN_E ret;

    EFuse_RETURN_E result = EFuse_RESULT_SUCCESS;  // return value

    if(((int32)block_id < EFuse_MIN_ID) || ((int32)block_id > EFuse_MAX_ID))
    {
        return EFuse_ID_ERROR;
    }

    /* if the block has locked return now */
    if(IsEfuseLock(block_id) == EFuse_LOCKED)
    {
        return EFuse_LOCKED;
    }

    REG32(EFUSE_MAGIC_NUMBER) = 0x8810;// Enable program
    REG32(EFUSE_BLOCK_INDEX)  = (block_id << READ_INDEX_OFFSET) | (block_id << PGM_INDEX_OFFSET);
    REG32(EFUSE_DATA_WR)      =  data & (~(EFuse_LOCK_BIT));
    REG32(EFUSE_PGM_PARA)    |=  (EFUSE_AUTO_TEST_EN << block_id);
    REG32(EFUSE_MODE_CTRL)   |=  EFUSE_PG_START;

    sts = (*(volatile uint32 *)EFUSE_STATUS) & EFUSE_PRG_BUSY;
    old_tick = SCI_GetTickCount();

    while((EFUSE_PRG_BUSY == sts))
    {
        sts = (*(volatile uint32 *)EFUSE_STATUS) & EFUSE_PRG_BUSY;
        new_tick = SCI_GetTickCount();
        if((new_tick - old_tick) > EFUSE_WRITE_TIMEOUT)
            break;
    }

    if(sts == EFUSE_PRG_BUSY)
    {
        *(volatile uint32 *)EFUSE_MAGIC_NUMBER = 0; // disable program
        return EFuse_WRITE_FAIL;
    }
    else
    {
        // get the hardware compare result
        sts = (*(volatile uint32 *)EUSE_MEM_BLOCK_FLAGS) & (EFUSE_BLOCK_ERR_FLAG << block_id);

        /* read data and compare */
        result = EFuseRead(block_id, &read_data);
        if(EFuse_RESULT_SUCCESS != result)
        {
            ret = EFuse_WRITE_VERIFY_FAIL;
        }
        else
        {
            //compare write and read data
            if((data & (((unsigned int)~EFuse_LOCK_BIT))) == (read_data & ((unsigned int)(~EFuse_LOCK_BIT))))
            {
                if(0 != sts)
                    ret = EFuse_WRITE_HARD_COMPARE_FAIL;
                else
                    ret = EFuse_RESULT_SUCCESS;
            }
            else
            {
                if(0 != sts)
                    ret = EFuse_WRITE_SOFT_HARD_COMPARE_FAIL;
                else
                    ret = EFuse_WRITE_SOFT_COMPARE_FAIL;
            }
        }
    }
    REG32(EFUSE_MAGIC_NUMBER) = 0;// disable program
    return ret;
}

PUBLIC EFuse_RETURN_E EFuseLock(
    unsigned int block_id                     // the selected EFuse block id
)
{
    unsigned int read_data;
    EFuse_RETURN_E ret;
    ret = EFuseRead(block_id, &read_data);
    if(ret != EFuse_RESULT_SUCCESS)
        return ret;

    if((read_data & ((unsigned int)EFuse_LOCK_BIT)) == ((unsigned int)EFuse_LOCK_BIT))
        return EFuse_RESULT_SUCCESS;

    read_data |= (unsigned int)EFuse_LOCK_BIT;
    return EFuseWrite(block_id, read_data);
}

PUBLIC void EFuseInitilize(void)
{
    uint32 i = 0;
    REG32(REG_AON_APB_APB_EB0)  |= BIT_13;        // enable efuse clock

    // efuse reset
    REG32(REG_AON_APB_APB_RST0) |=  BIT_14;        // enable efuse clock
    for(i = 0; i < 100; i++);
    REG32(REG_AON_APB_APB_RST0) &= ~BIT_14;        // enable efuse clock
    /* power on effuse */
    REG32(EFUSE_PGM_PARA)       |= EFS_VDD_ON;
    udelay(2000);

    REG32(REG_AON_APB_PWR_CTRL) |= BIT_3;
    udelay(2000);

    *(volatile uint32 *)EFUSE_PGM_PARA |= CLK_EFS_EN; // open efuse clk
    *(volatile uint32 *)EFUSE_PGM_PARA |= PGM_EN; // Enable program
    *(volatile uint32 *)EFUSE_MAGIC_NUMBER = 0x8810; // Enable program
    udelay(1000);
}

PUBLIC  EFuse_RETURN_E EFuseRead(
    unsigned int block_id,                 // the selected efuse block id
    unsigned int *r_data_ptr               // pointer of block data read from EFuse block
)
{
    uint32 old_tick = 0;
    uint32 new_tick = 0;
    uint32 sts = 0; //states of efuse
    EFuse_RETURN_E result = EFuse_RESULT_SUCCESS;  // return value

    /* check the block_id */
    if(((int32)block_id < EFuse_MIN_ID) || ((int32)block_id > EFuse_MAX_ID))
        return EFuse_ID_ERROR;

    /* read index must be the same of the write index */
    *(volatile uint32 *)EFUSE_BLOCK_INDEX = (block_id << READ_INDEX_OFFSET) | (block_id << PGM_INDEX_OFFSET);
    *(volatile uint32 *)EFUSE_MODE_CTRL |= EFUSE_RD_START; // send read commmand
    sts = (*(volatile uint32 *)EFUSE_STATUS) & EFUSE_READ_BUSY;

    old_tick = SCI_GetTickCount();
    while((EFUSE_READ_BUSY == sts))
    {
        sts = (*(volatile uint32 *)EFUSE_STATUS) & EFUSE_READ_BUSY;
        new_tick = SCI_GetTickCount();
        if((new_tick - old_tick) > EFUSE_READ_TIMEOUT)
            break;
    }
    if(sts == EFUSE_READ_BUSY)
        result =  EFuse_READ_FAIL;
    else
        *r_data_ptr = *(volatile uint32 *)(EFUSE_DATA_RD);
    return result;
}

LOCAL void EFuseClose(void)
{
    REG32(EFUSE_PGM_PARA)       &= ~CLK_EFS_EN; // close efuse clk
    REG32(EFUSE_PGM_PARA)       &= ~PGM_EN; // disable program
    REG32(REG_AON_APB_PWR_CTRL) &= ~BIT_3;
    /* shut down the efuse power */
    REG32(EFUSE_PGM_PARA)       &= ~EFS_VDD_ON;
    REG32(REG_AON_APB_APB_EB0)  &= ~BIT_13;        // enable efuse clock
    *(volatile uint32 *)EFUSE_MAGIC_NUMBER = 0; // Enable program
}

typedef struct SHA1Context32
{
    unsigned int Intermediate_Hash[5]; /* Message Digest */
    unsigned int Length_Low; /* Message length in bits */
    unsigned int Length_High; /* Message length in bits */
    /* Index into message block array */
    unsigned int Message_Block_Index;
    unsigned int W[80]; /* 512-bit message blocks */
} SHA1Context_32;

struct ROM_CALLBACK_TABLE {
	unsigned int version; //version number
#define SHA1_SUPPORT		BIT_0
#define MD5_SUPPORT		BIT_1
#define RSA_256_SUPPORT		BIT_2
#define RSA_512_SUPPORT		BIT_3
#define RSA_1024_SUPPORT	BIT_4
#define RSA_2048_SUPPORT	BIT_5
	unsigned int cap;//capability
	void (*Efuse_Init)   (void);
	void (*Efuse_Close)  (void);
	int  (*Efuse_Read)   (unsigned int block_id, unsigned int* r_data_ptr);
	int  (*SHA1Reset_32) (SHA1Context_32 *);
	int  (*SHA1Input_32) (SHA1Context_32 *,const unsigned int *message, unsigned int len);
	int  (*SHA1Result_32)(SHA1Context_32 *context,unsigned char * Message_Digest);
	void (*RSA_ModPower) (unsigned int *p, unsigned int *m, unsigned int *r2, unsigned int e);
};

typedef struct
{
	uint32 mVersion; // 1
	uint32 mMagicNum; // 0xaa55a5a5
	uint32 mCheckSum;//check sum value for bootloader header
	uint32 mHashLen;//word length
	uint32 mSectorSize; // sector size 1-1024
	uint32 mAcyCle; // 0, 1, 2
	uint32 mBusWidth; // 0--8 bit, 1--16bit
	uint32 mSpareSize; // spare part size for one sector
	uint32 mEccMode; // 0--1bit, 1-- 2bit, 2--4bit, 3--8bit, 4--12bit, 5--16bit, 6--24bit
	uint32 mEccPostion; // ECC postion at spare part
	uint32 mSectPerPage; // sector per page
	uint32 mSinfoPos;
	uint32 mSinfoSize;
	uint32 mECCValue[27];
} NBLHeader;

typedef struct
{
	uint32 mVersion; // 1
	uint32 mMagicNum; // 0xaa55a5a5
	uint32 mCheckSum;//check sum value for bootloader header
	uint32 mHashLen;//word length
}EBLHeader;

PUBLIC void EfuseHashWrite(uint8 *bsc_code, uint32 hash_len)
{
	uint32         block_index, ret, softHashValue[8], readHashValue[8];
	NBLHeader*     eblheader = (NBLHeader *)(bsc_code+ 0x20);
	SHA1Context_32 sha;
	struct ROM_CALLBACK_TABLE  *rom_callback;

	printf("EfuseHashWrite\r\n");

	rom_callback = (struct ROM_CALLBACK_TABLE*)(*((unsigned int *)0xFFFF0020));

	memset(eblheader,     0, sizeof(NBLHeader));
	memset(softHashValue, 0, sizeof(softHashValue));
	memset(readHashValue, 0, sizeof(readHashValue));

	eblheader->mHashLen = hash_len>>2;
	rom_callback->SHA1Reset_32 (&sha);
	rom_callback->SHA1Input_32 (&sha, (unsigned int*)bsc_code, hash_len>>2);
	rom_callback->SHA1Result_32(&sha, (unsigned char*)(&softHashValue[2]));

	EFuseClose();
	udelay(100000);
	EFuseInitilize();
	udelay(100000);

	do
	{
		//softHashValue[1] = BIT_18;
		for(block_index = 2; block_index < 7; block_index++)
		{
			ret = EFuseWrite(block_index, softHashValue[block_index]);
			printf("block : %d", block_index);
			switch (ret)
			{
				case EFuse_ID_ERROR:
					printf("EFuse_ID_ERROR\r\n");
					break;
				case EFuse_LOCKED:
					printf("EFuse_LOCKED\r\n");
					break;
				case EFuse_WRITE_FAIL:
					printf("EFuse_WRITE_FAIL\r\n");
					break;
				case EFuse_WRITE_VERIFY_FAIL:
					printf("EFuse_WRITE_VERIFY_FAIL\r\n");
					break;
				case EFuse_WRITE_HARD_COMPARE_FAIL:
					printf("EFuse_WRITE_SOFT_HARD_COMPARE_FAIL\r\n");
					break;
				case EFuse_RESULT_SUCCESS:
					printf("EFuse_RESULT_SUCCESS\r\n");
					break;
				case EFuse_WRITE_SOFT_COMPARE_FAIL:
					printf("EFuse_WRITE_SOFT_COMPARE_FAIL\r\n");
					break;
				default:
					printf("UNKOWN Error.\r\n");
					break;
			}
			if (ret != EFuse_RESULT_SUCCESS)
			{
				//block_index--;
				udelay(1000000);
			}
			udelay(10000);
		}
		udelay(100000);
		for(block_index = 0; block_index < 8; block_index++)
		{
			ret = EFuseRead(block_index, &readHashValue[block_index]);
			{
				printf("[%d] write: %08x, read: %08x\r\n", block_index, softHashValue[block_index], readHashValue[block_index]);
			}
		}
		udelay(1000000);
		for(block_index = 0; block_index < 8; block_index++)
		{
			ret = EFuseRead(block_index, &readHashValue[block_index]);
			{
				printf("[%d] write: %08x, read: %08x\r\n", block_index, softHashValue[block_index], readHashValue[block_index]);
			}
		}
		for (block_index=0; block_index<8; block_index++)
		{
			ret = EFuseWrite(block_index, 0);
		}
		udelay(1000000);
	}while (0);
	EFuseClose();
}

