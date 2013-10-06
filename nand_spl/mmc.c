#include <config.h>
#include "asm/arch/sci_types.h"
#if defined (CONFIG_SC8830) 
#include "asm/arch/sprd_reg_global.h" 
#include "asm/arch/sprd_reg_ahb.h"
#include "asm/arch/sprd_module_config.h" 
#include "asm/arch/regs_ahb.h"
#include "asm/arch/chip_drv_common_io.h"
#else
#include "asm/arch/sc8810_reg_global.h" 
#include "asm/arch/sc8810_reg_ahb.h"
#include "asm/arch/sc8810_module_config.h" 
#endif
#include <asm/arch/isr_drvapi.h>
#include <asm/arch/ldo.h>
#include <asm/arch/ldo_reg_v3.h>
#include <asm/arch/sdio_reg_v3.h>
#include <asm/arch/sys_timer_reg_v0.h>

#define SDIO_BASE_CLK_400K				400000
#define SDIO_BASE_CLK_26M				26000000			/* 26 MHz */
#define SDIO_BASE_CLK_48M				48000000			/* 48 MHz */
#define SDIO_BASE_CLK_153P6M				153600000			/* 153.6M */
#define SDIO_BASE_CLK_192M				192000000			/* 192 MHz */
#define SDIO_BASE_CLK_256M				256000000			/* 256 MHz */
#define SDIO_BASE_CLK_312M				312000000			/* 312 MHz */
#define SDIO_BASE_CLK_384M				384000000			/* 384 MHz */

#define NULL								0x0

#if defined(CONFIG_SC8830)
#define EMMC_CFG_ADDR					EMMC_BASE_ADDR		/* 0x2060_0000 */
/********** ahb clock enable **********/
#define EMMC_AHB_EN						AHB_EB
#define EMMC_AHB_RST					AHB_SOFT_RST

#define EMMC_AHB_EN_SHIFT				11					/* BIT_11 */
#define EMMC_AHB_RST_SHIFT				14					/* BIT_14 */

/********** base clock set ************/
#define EMMC_BASE_CLK					SDIO_BASE_CLK_26M
#define EMMC_BASE_AP_CKG				(CTL_BASE_AP_CKG + 0x54)

/********** master slot select *********/
#define EMMC_SLOT_SEL_REG				REG_AP_AHB_MISC_CFG
#define EMMC_SLOT_SEL_BIT				BIT_EMMC_SLOT_SEL

/******* emmc voltage controller *******/
#if defined (CONFIG_SPX15)

#define EMMC_VOLT_IO_CTRL				ANA_REG_GLB_LDO_V_CTRL2
#define EMMC_VOLT_CORE_CTRL				ANA_REG_GLB_LDO_V_CTRL7

#define EMMC_VOLT_IO_CTRL_SHIFT				0					/* [6:0] */
#define EMMC_VOLT_CORE_CTRL_SHIFT			8					/* [15:8] */

#define EMMC_VOLT_IO_MASK				0x7f
#define EMMC_VOLT_CORE_MASK                             0xff
#else
#define EMMC_VOLT_IO_CTRL				ANA_REG_GLB_LDO_V_CTRL0
#define EMMC_VOLT_CORE_CTRL			ANA_REG_GLB_LDO_V_CTRL0

#define EMMC_VOLT_IO_CTRL_SHIFT			12					/* [13:12] */
#define EMMC_VOLT_CORE_CTRL_SHIFT		14					/* [15:14] */
#define EMMC_VOLT_MASK					0x3
#endif

#elif defined(CONFIG_SC8825)
#define EMMC_CFG_ADDR					EMMC_BASE_ADDR		/* 0x20F0_0000 */
#define EMMC_AHB_EN						AHB_CTL0
#define EMMC_AHB_RST					AHB_SOFT_RST

#define EMMC_AHB_EN_SHIFT				23					/* BIT_23 */
#define EMMC_AHB_RST_SHIFT				21					/* BIT_21 */

/********** base clock set ************/
#define EMMC_BASE_CLK					SDIO_BASE_CLK_384M
#define EMMC_BASE_AP_CKG				GR_CLK_GEN5

/********** master slot select *********/
#define EMMC_SLOT_SEL_REG				AHB_SDIO_CTL
#define EMMC_SLOT_SEL_BIT				BIT_5

/******* emmc voltage controller *******/
#define EMMC_VOLT_IO_CTRL				ANA_LDO_VCTL4
#define EMMC_VOLT_CORE_CTRL			ANA_LDO_VCTL1

#define EMMC_VOLT_IO_CTRL_SHIFT			4					/* [7:6:5:4] */
#define EMMC_VOLT_CORE_CTRL_SHIFT		8					/* [11:10:9:8] */

#define EMMC_VOLT_MASK					0x0F

#elif defined(CONFIG_SC7710G2)
#define EMMC_CFG_ADDR					EMMC_BASE_ADDR		/* 0x20F0_0000 */
#define EMMC_AHB_EN						AHB_CTL6
#define EMMC_AHB_RST					AHB_SOFT2_RST

#define EMMC_AHB_EN_SHIFT				1					/* BIT_1 */
#define EMMC_AHB_RST_SHIFT				1					/* BIT_1 */

/********** base clock set ************/
#define EMMC_BASE_CLK					SDIO_BASE_CLK_384M
#define EMMC_BASE_AP_CKG				GR_CLK_GEN7

/********** master slot select *********/
#define EMMC_SLOT_SEL_REG				AHB_SDIO_CTL
#define EMMC_SLOT_SEL_BIT				BIT_5

/******* emmc voltage controller *******/
#define EMMC_VOLT_IO_CTRL				ANA_LDO_VCTL4
#define EMMC_VOLT_CORE_CTRL			ANA_LDO_VCTL4

#define EMMC_VOLT_IO_CTRL_SHIFT			12					/* [15:14:13:12] */
#define EMMC_VOLT_CORE_CTRL_SHIFT		8					/* [11:10:9:8] */

#define EMMC_VOLT_MASK					0x0F
#else
#define EMMC_CFG_ADDR					SDIO1_BASE_ADDR
#define EMMC_AHB_EN						AHB_CTL0
#define EMMC_AHB_RST					AHB_SOFT_RST

#define EMMC_AHB_EN_SHIFT				19					/* BIT_19 */
#define EMMC_AHB_RST_SHIFT				16					/* BIT_16 */

/********** base clock set ************/
#define EMMC_BASE_CLK					SDIO_BASE_CLK_48M
#define EMMC_BASE_AP_CKG				GR_CLK_GEN5

/********** master slot select *********/
#define EMMC_SLOT_SEL_REG				AHB_SDIO_CTL
#define EMMC_SLOT_SEL_BIT				BIT_5

#endif

/**************************   Card info   *********************************/
#define CARD_DATA_BLOCK_LEN				512

#define PARTITION_ENABLE_BOOT1_OFFSET	3
#define PARTITION_CFG_ENABLE_BOOT1		(1 << PARTITION_ENABLE_BOOT1_OFFSET)

#define PARTITION_CFG_ACCESS_USER		0
#define PARTITION_CFG_ACCESS_BOOT1		1
#define PARTITION_CFG_ACCESS_BOOT2		2
#define PARTITION_CFG_ACCESS_RPMB		3
#define PARTITION_CFG_ACCESS_GEN_P1		4
#define PARTITION_CFG_ACCESS_GEN_P2		5
#define PARTITION_CFG_ACCESS_GEN_P3		6
#define PARTITION_CFG_ACCESS_GEN_P4		7

#define EXT_CSD_CARD_WIDTH_1_BIT		0
#define EXT_CSD_CARD_WIDTH_4_BIT		1
#define EXT_CSD_CARD_WIDTH_8_BIT		2

#define CMD6_BIT_MODE_OFFSET_ACCESS	24
#define CMD6_BIT_MODE_MASK_ACCESS		0x03000000
#define CMD6_BIT_MODE_OFFSET_INDEX		16
#define CMD6_BIT_MODE_MASK_INDEX		0x00FF0000
#define CMD6_BIT_MODE_OFFSET_VALUE		8
#define CMD6_BIT_MODE_MASK_VALUE		0x0000FF00
#define CMD6_BIT_MODE_OFFSET_CMD_SET	0
#define CMD6_BIT_MODE_MASK_CMD_SET		0x00000003

#define CMD6_ACCESS_MODE_COMMAND_SET	(0 << CMD6_BIT_MODE_OFFSET_ACCESS)
#define CMD6_ACCESS_MODE_SET_BITS		(1 << CMD6_BIT_MODE_OFFSET_ACCESS)
#define CMD6_ACCESS_MODE_CLEAR_BITS	(2 << CMD6_BIT_MODE_OFFSET_ACCESS)
#define CMD6_ACCESS_MODE_WRITE_BYTE	(3 << CMD6_BIT_MODE_OFFSET_ACCESS)
#define CMD6_CMD_SET					(1 << CMD6_BIT_MODE_OFFSET_CMD_SET)

#define EXT_CSD_PARTITION_CFG_INDEX		179
#define EXT_CSD_BUS_WIDTH_INDEX			183			/* R/W */
#define EXT_CSD_HS_TIMING_INDEX			185			/* R/W */
#define EXT_CSD_CARD_TYPE_INDEX			196			/* RO */
#define EXT_CSD_SEC_CNT_INDEX			212			/* RO, 4 bytes */

#define SECTOR_MODE						0x40000000
#define BYTE_MODE						0x00000000 

/**************************   control info    *******************************/
typedef enum SDIO_VOL_E_TAG
{
	VOL_1_2								= 1200,
	VOL_1_3								= 1300,
	VOL_1_5								= 1500,
	VOL_1_8								= 1800,
	VOL_2_5								= 2500,
	VOL_2_8								= 2800,
	VOL_3_0								= 3000,
	VOL_RES
} SDIO_Vol_e;

typedef enum CARD_PARTITION_TAG
{
	PARTITION_USER,
	PARTITION_BOOT1,
	PARTITION_BOOT2,
	PARTITION_RPMB,
	PARTITION_GENERAL_P1,
	PARTITION_GENERAL_P2,
	PARTITION_GENERAL_P3,
	PARTITION_GENERAL_P4,
	PARTITION_MAX
} CARD_Partition_e;

typedef struct DATA_PARAM_TAG
{
	uint8								*data_buf;	/* the buffer address */
	uint32								blk_len;		/* block size */
	uint32								blk_num;		/* block number */
} DATA_Param_t;

typedef enum SDIO_ERROR_TAG
{
	SDIO_ERR_NONE						= 0,
	SDIO_ERR_RSP						= BIT_0,
	SDIO_ERR_CMD12						= BIT_1,
	SDIO_ERR_CUR_LIMIT					= BIT_2,
	SDIO_ERR_DATA_END					= BIT_3,
	SDIO_ERR_DATA_CRC					= BIT_4,
	SDIO_ERR_DATA_TIMEOUT				= BIT_5,
	SDIO_ERR_CMD_INDEX					= BIT_6,
	SDIO_ERR_CMD_END					= BIT_7,
	SDIO_ERR_CMD_CRC					= BIT_8,
	SDIO_ERR_CMD_TIMEOUT				= BIT_9
} SDIO_Error_e;

/**************************   card info    *********************************/
typedef enum SDIO_RST_TYPE_TAG
{
	RST_CMD_LINE,
	RST_DAT_LINE,
	RST_CMD_DAT_LINE,
	RST_ALL,
	RST_MODULE
} SDIO_Rst_e;

/**************************   emmc base handle    **************************/
typedef enum SDIO_BUSWIDTH_TAG
{
	SDIO_BUS_1_BIT,
	SDIO_BUS_4_BIT,
	SDIO_BUS_8_BIT
} SDIO_BusWidth_e;

typedef enum SDIO_ONOFF_TAG
{
	SDIO_OFF,
	SDIO_ON
} SDIO_OnOff_e;

typedef void (*SDIO_CALLBACK)(uint32 msg, uint32 errCode);

typedef struct SDIO_HANDLE_T_TAG
{
	volatile SDIO_REG_CFG					*host_cfg;	/* register configuration */
	BOOLEAN								open_flag;	/* open flag */
	uint32								base_clock;	/* base clock */
	uint32								sd_clock;
	uint32								err_filter;
	SDIO_CALLBACK						sig_callBack;
	volatile uint32							card_event;
	volatile uint32							card_errCode;

	uint16								rca;			/* card RCA */
	SDIO_BusWidth_e						bus_width;	/* card used bus width */
	uint32								block_len;	/* card block length */
	CARD_Partition_e						cur_partition;	/* card current partition */
} SDIO_Handle_t, *SDIO_Hd_Ptr;

/**************************   cmd info    *********************************/
#define TRANS_MODE_CMPLETE_SIG_EN		BIT_0
#define TRANS_MODE_MULTI_BLOCK			BIT_1
#define TRANS_MODE_READ					BIT_2
#define TRANS_MODE_CMD12_EN			BIT_3
#define TRANS_MODE_BLOCK_COUNT_EN		BIT_4
#define TRANS_MODE_DMA_EN				BIT_5
#define CMD_HAVE_DATA					BIT_6

typedef enum CMD_RSP_TYPE_E_TAG
{
	CMD_NO_RSP = 0,
	CMD_RSP_R1,
	CMD_RSP_R2,
	CMD_RSP_R3,
	CMD_RSP_R4,
	CMD_RSP_R5,
	CMD_RSP_R6,
	CMD_RSP_R7,
	CMD_RSP_R1B,
	CMD_RSP_R5B
} CARD_CmdRsp_e;

typedef enum SDIO_CMD_TYPE_E_TAG
{
	CMD_TYPE_NORMAL = 0,
	CMD_TYPE_SUSPEND,
	CMD_TYPE_RESUME,
	CMD_TYPE_ABORT
} SDIO_CmdType_e;

#define SIG_ERR							BIT_0
#define SIG_CARD_IN						BIT_1
#define SIG_CARD_INSERT					BIT_2
#define SIG_CARD_REMOVE					BIT_3
#define SIG_BUF_RD_RDY					BIT_4
#define SIG_BUF_WD_RDY					BIT_5
#define SIG_DMA_INT						BIT_6
#define SIG_BLK_CAP						BIT_7
#define SIG_TRANS_CMP					BIT_8
#define SIG_CMD_CMP						BIT_9
#define SIG_ALL							(SIG_ERR | SIG_CARD_IN | SIG_CARD_INSERT \
										| SIG_CARD_REMOVE | SIG_BUF_RD_RDY \
										| SIG_BUF_WD_RDY | SIG_DMA_INT \
										| SIG_BLK_CAP | SIG_TRANS_CMP \
										| SIG_CMD_CMP)

#define ERR_RSP							BIT_0
#define ERR_CMD12						BIT_1
#define ERR_CUR_LIMIT					BIT_2
#define ERR_DATA_END					BIT_3
#define ERR_DATA_CRC					BIT_4
#define ERR_DATA_TIMEOUT				BIT_5
#define ERR_CMD_INDEX					BIT_6
#define ERR_CMD_END						BIT_7
#define ERR_CMD_CRC						BIT_8
#define ERR_CMD_TIMEOUT					BIT_9
#define ERR_ALL							(ERR_RSP | ERR_CMD12 | ERR_CUR_LIMIT \
										| ERR_DATA_END | ERR_DATA_CRC \
										| ERR_DATA_TIMEOUT | ERR_CMD_INDEX \
										| ERR_CMD_END | ERR_CMD_CRC \
										| ERR_CMD_TIMEOUT)

//      response Name       cmd int filter                                                      ,rsp            ,cmd error filter
#define CARD_SDIO_NO_RSP				NULL/*|SIG_CARD_IN|SIG_CARD_INSERT|SIG_CARD_REMOVE*/|SIG_CMD_CMP    ,CMD_NO_RSP     ,NULL
#define CARD_SDIO_R1						NULL/*|SIG_CARD_IN|SIG_CARD_INSERT|SIG_CARD_REMOVE*/|SIG_CMD_CMP    ,CMD_RSP_R1     ,ERR_RSP|ERR_CMD_INDEX| ERR_CMD_END|    ERR_CMD_CRC|    ERR_CMD_TIMEOUT
#define CARD_SDIO_R2						NULL/*|SIG_CARD_IN|SIG_CARD_INSERT|SIG_CARD_REMOVE*/|SIG_CMD_CMP    ,CMD_RSP_R2     ,ERR_RSP|                           ERR_CMD_END|    ERR_CMD_CRC|    ERR_CMD_TIMEOUT
#define CARD_SDIO_R3						NULL/*|SIG_CARD_IN|SIG_CARD_INSERT|SIG_CARD_REMOVE*/|SIG_CMD_CMP    ,CMD_RSP_R3     ,ERR_RSP|                           ERR_CMD_END|                                ERR_CMD_TIMEOUT
#define CARD_SDIO_R4						NULL/*|SIG_CARD_IN|SIG_CARD_INSERT|SIG_CARD_REMOVE*/|SIG_CMD_CMP    ,CMD_RSP_R4     ,ERR_RSP|ERR_CMD_INDEX| ERR_CMD_END|    ERR_CMD_CRC|    ERR_CMD_TIMEOUT
#define CARD_SDIO_R5						NULL/*|SIG_CARD_IN|SIG_CARD_INSERT|SIG_CARD_REMOVE*/|SIG_CMD_CMP    ,CMD_RSP_R5     ,ERR_RSP|ERR_CMD_INDEX| ERR_CMD_END|    ERR_CMD_CRC|    ERR_CMD_TIMEOUT
#define CARD_SDIO_R6						NULL/*|SIG_CARD_IN|SIG_CARD_INSERT|SIG_CARD_REMOVE*/|SIG_CMD_CMP    ,CMD_RSP_R6     ,ERR_RSP|ERR_CMD_INDEX| ERR_CMD_END|    ERR_CMD_CRC|    ERR_CMD_TIMEOUT
#define CARD_SDIO_R7						NULL/*|SIG_CARD_IN|SIG_CARD_INSERT|SIG_CARD_REMOVE*/|SIG_CMD_CMP    ,CMD_RSP_R7     ,ERR_RSP|ERR_CMD_INDEX| ERR_CMD_END|    ERR_CMD_CRC|    ERR_CMD_TIMEOUT
#define CARD_SDIO_R1B					NULL/*|SIG_CARD_IN|SIG_CARD_INSERT|SIG_CARD_REMOVE*/|SIG_CMD_CMP    ,CMD_RSP_R1B   ,ERR_RSP|ERR_CMD_INDEX| ERR_CMD_END|    ERR_CMD_CRC|    ERR_CMD_TIMEOUT

typedef enum CARD_CMD_E_TAG
{
	// cmdindex,rsp,transmode
	CARD_CMD0_GO_IDLE_STATE,
	CARD_CMD1_SEND_OP_COND, 			/* MMC */
	CARD_CMD2_ALL_SEND_CID,
	CARD_CMD3_SET_RELATIVE_ADDR,		/* MMC */
	CARD_CMD7_SELECT_DESELECT_CARD,
	CARD_CMD12_STOP_TRANSMISSION,		/* It is auto performed by Host */
	CARD_CMD13_SEND_STATUS,
	CARD_CMD16_SET_BLOCKLEN,
	CARD_CMD18_READ_MULTIPLE_BLOCK,
	CARD_ACMD6_SET_EXT_CSD,
	CARD_CMDMAX
} CARD_Cmd_e;

typedef struct CARD_CMDCTLFLG_T_TAG
{
	CARD_Cmd_e							cmd;
	uint32								cmd_index;
	uint32								int_filter;
	CARD_CmdRsp_e						response;
	uint32								err_filter;
	uint32								transmode;
} CARD_CmdCtlFlg_t;

static const CARD_CmdCtlFlg_t s_cmdDetail[] =
{
	// cmdindex,rsp,transmode
	//#define CMDname                       cmdindex    ,data int filter    +   (cmd int filter+)rsp(+cmd error filter) +   ,data error filter                      ,transmode
	{   CARD_CMD0_GO_IDLE_STATE			,0	,NULL			|   CARD_SDIO_NO_RSP	|NULL                                                           ,NULL                                                                                                                               },
	{   CARD_CMD1_SEND_OP_COND			,1	,NULL			|   CARD_SDIO_R3			|NULL                                                           ,NULL                                                                                                                               },
	{   CARD_CMD2_ALL_SEND_CID			,2	,NULL			|   CARD_SDIO_R2			|NULL                                                           ,NULL                                                                                                                               },
	{   CARD_CMD3_SET_RELATIVE_ADDR		,3	,NULL			|   CARD_SDIO_R1			|NULL                                                           ,NULL                                                                                                                               },
	{   CARD_CMD7_SELECT_DESELECT_CARD	,7	,NULL			|   CARD_SDIO_R1			|NULL                                                           ,NULL                                                                                                                               },
	{   CARD_CMD12_STOP_TRANSMISSION	,12	,SIG_TRANS_CMP	|   CARD_SDIO_R1B		|NULL                                                           ,NULL                                                                                                                               },  //It is auto performed by Host
	{   CARD_CMD13_SEND_STATUS			,13	,NULL			|   CARD_SDIO_R1			|NULL                                                           ,NULL                                                                                                                               },
	{   CARD_CMD16_SET_BLOCKLEN			,16	,NULL			|   CARD_SDIO_R1			|NULL                                                           ,NULL                                                                                                                               },
	{   CARD_CMD18_READ_MULTIPLE_BLOCK	,18	,SIG_TRANS_CMP	|   CARD_SDIO_R1			|ERR_DATA_END|  ERR_DATA_CRC|   ERR_DATA_TIMEOUT                ,TRANS_MODE_MULTI_BLOCK|TRANS_MODE_READ                         |   TRANS_MODE_BLOCK_COUNT_EN|  TRANS_MODE_DMA_EN|  CMD_HAVE_DATA   },
	{   CARD_ACMD6_SET_EXT_CSD			,6	,NULL			|   CARD_SDIO_R1B		|NULL                                                           ,NULL                                                                                                                               },

	{   CARD_CMDMAX						,0	,NULL			|   CARD_SDIO_NO_RSP	|NULL                                                           ,NULL                                                                                                                               }
};

//LOCAL volatile uint32						s_CardErrCode = 0;
//LOCAL volatile uint32						s_CardEvent = 0;
LOCAL SDIO_Handle_t						s_EmmcCtl = {0};
LOCAL SDIO_Hd_Ptr						p_EmmcHd;

uint32 SCI_GetTickCount(void)
{
	volatile uint32 tmp_tick1;
	volatile uint32 tmp_tick2;

	tmp_tick1 = SYSTEM_CURRENT_CLOCK;
	tmp_tick2 = SYSTEM_CURRENT_CLOCK;

	while (tmp_tick1 != tmp_tick2)
	{
		tmp_tick1 = tmp_tick2;
		tmp_tick2 = SYSTEM_CURRENT_CLOCK;
	}

	return tmp_tick1;
}

void SDIO_Mdelay(uint32 val)
{
	uint32 pre_tick, cur_tick;
	pre_tick = SCI_GetTickCount(); /*set start tick value*/
	cur_tick = pre_tick;
	while(cur_tick  - pre_tick < val)
	{
		cur_tick = SCI_GetTickCount();
	}
}

PUBLIC uint32 SDHOST_GetDmaAddr (SDIO_Hd_Ptr pHd)
{
	uint32 dmaAddr;
	dmaAddr = pHd->host_cfg->DMA_SYS_ADD;
	return dmaAddr;
}

PUBLIC void SDHOST_SetDmaAddr (SDIO_Hd_Ptr pHd, uint32 dmaAddr)
{
	pHd->host_cfg->DMA_SYS_ADD = dmaAddr;
}

LOCAL void SetCardEvent (SDIO_Hd_Ptr pHd, uint32 EventId)
{
	pHd->card_event |= EventId;
}

LOCAL uint32 _WaitCardEvent (SDIO_Hd_Ptr pHd, uint32 EventId)
{
	if ( EventId == (pHd->card_event & EventId))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

/*********************************************************************
**	Description :
**		This function used to get normal interrupt status.
**	Param :
**		pHd : emmc control handle.
**	Return :
**		msg : interrupt message.
**	Date					Author				Operation
**	2013/05/09			ypxie				optimization
*********************************************************************/
PUBLIC uint32 SDHOST_GetNMLIntStatus (SDIO_Hd_Ptr pHd)
{
	volatile uint32 tmpReg;
	uint32 msg;

	tmpReg = pHd->host_cfg->INT_STA;
	msg = 0;

	if (0 != (tmpReg & BIT_15))
	{
		msg |= SIG_ERR;
	}

	if (0 != (tmpReg & BIT_8))
	{
		msg |= SIG_CARD_IN;
	}

	if (0 != (tmpReg & BIT_6))
	{
		msg |= SIG_CARD_INSERT;
	}

	if (0 != (tmpReg & BIT_7))
	{
		msg |= SIG_CARD_REMOVE;
	}

	if (0 != (tmpReg & BIT_5))
	{
		msg |= SIG_BUF_RD_RDY;
	}

	if (0 != (tmpReg & BIT_4))
	{
		msg |= SIG_BUF_WD_RDY;
	}

	if (0 != (tmpReg & BIT_3))
	{
		msg |= SIG_DMA_INT;
	}

	if (0 != (tmpReg & BIT_2))
	{
		msg |= SIG_BLK_CAP;
	}

	if (0 != (tmpReg & BIT_1))
	{
		msg |= SIG_TRANS_CMP;
	}

	if (0 != (tmpReg & BIT_0))
	{
		msg |= SIG_CMD_CMP;
	}

	return msg;
} /* end of SDHOST_GetNMLIntStatus*/

/*********************************************************************
**	Description :
**		This function used to get error interrupt status.
**	Param :
**		pHd : emmc control handle.
**	Return :
**		msg : interrupt message.
**	Date					Author				Operation
**	2013/05/09			ypxie				optimization
*********************************************************************/
PUBLIC uint32 SDHOST_GetErrCode (SDIO_Hd_Ptr pHd)
{
	volatile uint32 tmpReg;
	uint32 err_msg;

	tmpReg = pHd->host_cfg->INT_STA;
	err_msg = 0;

	if (0 != (tmpReg & BIT_28))
	{
		err_msg |= ERR_RSP;
	}

	if (0 != (tmpReg & BIT_24))
	{
		err_msg |= ERR_CMD12;
	}

	if (0 != (tmpReg & BIT_23))
	{
		err_msg |= ERR_CUR_LIMIT;
	}

	if (0 != (tmpReg & BIT_22))
	{
		err_msg |= ERR_DATA_END;
	}

	if (0 != (tmpReg & BIT_21))
	{
		err_msg |= ERR_DATA_CRC;
	}

	if (0 != (tmpReg & BIT_20))
	{
		err_msg |= ERR_DATA_TIMEOUT;
	}

	if (0 != (tmpReg & BIT_19))
	{
		err_msg |= ERR_CMD_INDEX;
	}

	if (0 != (tmpReg & BIT_18))
	{
		err_msg |= ERR_CMD_END;
	}

	if (0 != (tmpReg & BIT_17))
	{
		err_msg |= ERR_CMD_CRC;
	}

	if (0 != (tmpReg & BIT_16))
	{
		err_msg |= ERR_CMD_TIMEOUT;
	}

	return err_msg;
} /* end of SDHOST_GetErrCode */

/*********************************************************************
**	Description :
**		This function used to disable signal interrupt, status int and clear int status.
**	Param :
**		pHd : emmc control handle.
**		msg : irq information.
**	Return :
**		msg : interrupt message.
**	Date					Author				Operation
**	2013/05/10			ypxie				optimization
*********************************************************************/
PUBLIC void SDHOST_NML_INT_SigSt_Dis (SDIO_Hd_Ptr pHd, uint32 msg)
{
	volatile uint32 tmpReg = 0;

	if (0 != (msg & SIG_ERR))
	{
		tmpReg |= (BIT_28 | BIT_24 | BIT_23 | BIT_22 | BIT_21 | BIT_20
					| BIT_19 | BIT_18 | BIT_17 | BIT_16);
	}

	if (0 != (msg & SIG_CARD_IN))
	{
		tmpReg |= BIT_8;
	}

	if (0 != (msg & SIG_CARD_INSERT))
	{
		tmpReg |= BIT_6;
	}

	if (0 != (msg & SIG_CARD_REMOVE))
	{
		tmpReg |= BIT_7;
	}

	if (0 != (msg & SIG_BUF_RD_RDY))
	{
		tmpReg |= BIT_5;
	}

	if (0 != (msg & SIG_BUF_WD_RDY))
	{
		tmpReg |= BIT_4;
	}

	if (0 != (msg & SIG_DMA_INT))
	{
		tmpReg |= BIT_3;
	}

	if (0 != (msg & SIG_BLK_CAP))
	{
		tmpReg |= BIT_2;
	}

	if (0 != (msg & SIG_TRANS_CMP))
	{
		tmpReg |= BIT_1;
	}

	if (0 != (msg & SIG_CMD_CMP))
	{
		tmpReg |= BIT_0;
	}

	pHd->host_cfg->INT_SIG_EN &= ~tmpReg;
	pHd->host_cfg->INT_STA_EN &= ~tmpReg;
	pHd->host_cfg->INT_STA = tmpReg;
} /* end of SDHOST_NML_INT_SigSt_Dis */

/*********************************************************************
**	Description :
**		This function used to enable interrupt status & signal.
**	Param :
**		pHd : emmc control handle.
**		msg : irq information.
**	Return :
**		msg : interrupt message.
**	Date					Author				Operation
**	2013/05/10			ypxie				optimization
*********************************************************************/
PUBLIC void SDHOST_NML_INT_SigSt_En (SDIO_Hd_Ptr pHd,uint32 msg)
{
	volatile uint32 tmpReg = 0;
	uint32 err_msg;

	if (0 != (msg&SIG_ERR))
	{
		//_ERR_IntStatus_En (sdhost_handler,sdhost_handler->err_filter);
		err_msg = pHd->err_filter;
		if (0 != (err_msg & ERR_RSP))
		{
			tmpReg |= BIT_28;
		}

		if (0 != (err_msg & ERR_CMD12))
		{
			tmpReg |= BIT_24;
		}

		if (0 != (err_msg & ERR_CUR_LIMIT))
		{
			tmpReg |= BIT_23;
		}

		if (0 != (err_msg & ERR_DATA_END))
		{
			tmpReg |= BIT_22;
		}

		if (0 != (err_msg & ERR_DATA_CRC))
		{
			tmpReg |= BIT_21;
		}

		if (0 != (err_msg & ERR_DATA_TIMEOUT))
		{
			tmpReg |= BIT_20;
		}

		if (0 != (err_msg & ERR_CMD_INDEX))
		{
			tmpReg |= BIT_19;
		}

		if (0 != (err_msg & ERR_CMD_END))
		{
			tmpReg |= BIT_18;
		}

		if (0 != (err_msg & ERR_CMD_CRC))
		{
			tmpReg |= BIT_17;
		}

		if (0 != (err_msg & ERR_CMD_TIMEOUT))
		{
			tmpReg |= BIT_16;
		}

	}

	if (0 != (msg & SIG_CARD_IN))
	{
		tmpReg |= BIT_8;
	}

	if (0 != (msg & SIG_CARD_INSERT))
	{
		tmpReg |= BIT_6;
	}

	if (0 != (msg & SIG_CARD_REMOVE))
	{
		tmpReg |= BIT_7;
	}

	if (0 != (msg & SIG_BUF_RD_RDY))
	{
		tmpReg |= BIT_5;
	}

	if (0 != (msg & SIG_BUF_WD_RDY))
	{
		tmpReg |= BIT_4;
	}

	if (0 != (msg & SIG_DMA_INT))
	{
		tmpReg |= BIT_3;
	}

	if (0 != (msg & SIG_BLK_CAP))
	{
		tmpReg |= BIT_2;
	}

	if (0 != (msg & SIG_TRANS_CMP))
	{
		tmpReg |= BIT_1;
	}

	if (0 != (msg & SIG_CMD_CMP))
	{
		tmpReg |= BIT_0;
	}
	pHd->host_cfg->INT_STA_EN |= tmpReg;
	pHd->host_cfg->INT_SIG_EN |= tmpReg;
} /* end of SDHOST_NML_INT_SigSt_En */

/*********************************************************************
**	Description :
**		This function is irq process.
**	Param :
**		isrNum :
**	Return :
**		CALL_HISR
**	Date					Author				Operation
**	2013/05/09			ypxie				optimization
*********************************************************************/
PUBLIC ISR_EXE_T _SDHOST_IrqHandle (uint32 isrNum)
{
	uint32 msg;
	uint32 errCode;
	SDIO_Hd_Ptr pHd;

	pHd = &s_EmmcCtl;

	msg = SDHOST_GetNMLIntStatus (pHd);
	errCode = SDHOST_GetErrCode (pHd);

	SDHOST_NML_INT_SigSt_Dis(pHd, msg);
	pHd->sig_callBack(msg,  errCode);
	return CALL_HISR;
} /* end of _SDHOST_IrqHandle*/

/*********************************************************************
**	Description :
**		This function used to process irq.
**	Param :
**		msg		: interrupt message
**		errCode	: error information
**		slotNum	: slot number
**	Return :
**		none
**	Date					Author				Operation
**	2013/05/09			ypxie				optimization
*********************************************************************/
LOCAL void  _irqCardProc (uint32 msg, uint32 errCode)
{
	SDIO_Hd_Ptr pHd;
	pHd = &s_EmmcCtl;

	/* card interrupt happened , may be some operation error happened */
	if (0 != (SIG_CARD_IN & msg))
	{
		SetCardEvent (pHd, SIG_CARD_IN);
	}

	if (0 != (SIG_CARD_INSERT & msg)) 			/* card is inserted in slot */
	{
		SetCardEvent (pHd, SIG_CARD_INSERT);
	}
	else if (0 != (SIG_CARD_REMOVE & msg))		/* card is plug out from slot */
	{
		SetCardEvent (pHd, SIG_CARD_REMOVE);
	}
	else if (0 != (SIG_ERR&msg))		/* some error happened during the transmission */
	{
		pHd->card_errCode |= errCode;
		SetCardEvent (pHd, SIG_ALL);
		return;
	}

	if (0 != (SIG_TRANS_CMP & msg))			/* transmission complete */
	{
		SetCardEvent (pHd, SIG_TRANS_CMP);
	}
	else if (0 != (SIG_DMA_INT & msg))
	{
		volatile uint32 nextAddr;
		nextAddr = SDHOST_GetDmaAddr (pHd);
		SDHOST_SetDmaAddr (pHd, nextAddr);
		SDHOST_NML_INT_SigSt_En(pHd, SIG_DMA_INT);
	}

	if (0 != (SIG_CMD_CMP&msg))
	{
		SetCardEvent (pHd, SIG_CMD_CMP);
	}
} /* end of _irqCardProc*/

PUBLIC void SDHOST_SdClk_Enable (SDIO_Hd_Ptr pHd, SDIO_OnOff_e enFlg)
{
	if (enFlg == SDIO_ON)
	{
		pHd->host_cfg->HOST_CTL1 |= BIT_2;
	}
	else /* if (enFlg == SDIO_OFF) */
	{
		if ((pHd->host_cfg->HOST_CTL1 & BIT_2) != 0) {
			pHd->host_cfg->HOST_CTL1 &= ~BIT_2;
			SDIO_Mdelay(1);
		}
	}
}

PUBLIC void SDHOST_InternalClk_Enable (SDIO_Hd_Ptr pHd, SDIO_OnOff_e enFlg)
{
	if (enFlg == SDIO_ON)
	{
		/* Enable internal clock */
		pHd->host_cfg->HOST_CTL1 |= BIT_0;
		/* Wait internal clock stable */
		while (0 == (pHd->host_cfg->HOST_CTL1 & BIT_1)) ;
	}
	else /* if (enFlg == SDIO_OFF) */
	{
		pHd->host_cfg->HOST_CTL1 &= (~BIT_0);
	}

}

/*********************************************************************
**	Description :
**		This function used to set control reset.
**	Param :
**		pHd		: emmc control handle.
**		rstType	: reset type.
**	Return :
**		none
**	Date					Author				Operation
**	2013/05/09			ypxie				optimization
*********************************************************************/
PUBLIC void SDHOST_RST (SDIO_Hd_Ptr pHd, SDIO_Rst_e rstType)
{
	switch (rstType)
	{
		case RST_DAT_LINE:
			pHd->host_cfg->HOST_CTL1 |= BIT_25;
			while (0 != (pHd->host_cfg->HOST_CTL1 & BIT_25)) ;
			break;

		case RST_CMD_LINE:
			pHd->host_cfg->HOST_CTL1 |= BIT_26;
			while (0!= (pHd->host_cfg->HOST_CTL1 & BIT_26)) ;
			break;

		case RST_CMD_DAT_LINE:
			pHd->host_cfg->HOST_CTL1 |= (BIT_25 | BIT_26);
			while (0 != (pHd->host_cfg->HOST_CTL1 & (BIT_25 | BIT_26)));
			break;

		case RST_ALL:
			SDHOST_SdClk_Enable(pHd, SDIO_OFF);
			pHd->host_cfg->HOST_CTL1 |= BIT_24;
			while (0 != (pHd->host_cfg->HOST_CTL1 & BIT_24));
			break;

		default:
			break;
	}
} /* end of SDHOST_RST */

/*********************************************************************
**	Description :
**		This function used to set emmc control base clock.
**	Param :
**		base_clk	: emmc base clock.
**	Return :
**		none
**	Date					Author				Operation
**	2013/05/17			ypxie				optimization
*********************************************************************/
PUBLIC uint32 SDHOST_BaseClk_Set (uint32 baseClk)
{
	uint32 clk = 0;

#if defined (CONFIG_SC8830)
#if defined (CONFIG_SPX15)
	REG32(EMMC_BASE_AP_CKG) &= ~(BIT_0 | BIT_1);

	if (baseClk >= SDIO_BASE_CLK_384M)
	{
		REG32(EMMC_BASE_AP_CKG) |= 0x3;
		clk = SDIO_BASE_CLK_384M;
	}
	else if (baseClk >= SDIO_BASE_CLK_312M)
	{
		REG32(EMMC_BASE_AP_CKG) |= 0x2;
		clk = SDIO_BASE_CLK_312M;
	}
	else if (baseClk >= SDIO_BASE_CLK_256M)
	{
		REG32(EMMC_BASE_AP_CKG) |= 0x1;
		clk = SDIO_BASE_CLK_256M;
	}
	else
	{
		clk = SDIO_BASE_CLK_26M;						/* default */
	}
#else
	REG32(EMMC_BASE_AP_CKG) &= ~(BIT_0 | BIT_1);

	if (baseClk >= SDIO_BASE_CLK_312M)
	{
		REG32(EMMC_BASE_AP_CKG) |= 0x3;
		clk = SDIO_BASE_CLK_312M;
	}
	else if (baseClk >= SDIO_BASE_CLK_256M)
	{
		REG32(EMMC_BASE_AP_CKG) |= 0x2;
		clk = SDIO_BASE_CLK_256M;
	}
	else if (baseClk >= SDIO_BASE_CLK_192M)
	{
		REG32(EMMC_BASE_AP_CKG) |= 0x1;
		clk = SDIO_BASE_CLK_192M;
	}
	else
	{
		clk = SDIO_BASE_CLK_26M;						/* default */
	}
#endif
#elif defined (CONFIG_SC8825)
	REG32 (EMMC_BASE_AP_CKG) &= ~ (BIT_23 | BIT_24);

	if (baseClk >= SDIO_BASE_CLK_384M)
	{
		clk = SDIO_BASE_CLK_384M;					/* default */
	}
	else if (baseClk >= SDIO_BASE_CLK_256M)
	{
		REG32(EMMC_BASE_AP_CKG) |= BIT_23;
		clk = SDIO_BASE_CLK_256M;
	}
	else if (baseClk >= SDIO_BASE_CLK_153P6M)
	{
		REG32(EMMC_BASE_AP_CKG) |= BIT_24;
		clk = SDIO_BASE_CLK_153P6M;
	}
	else
	{
		REG32(EMMC_BASE_AP_CKG) |= (BIT_23 | BIT_24);
		clk = SDIO_BASE_CLK_26M;
	}

#elif defined (CONFIG_SC7710G2)
	REG32 (EMMC_BASE_AP_CKG) &= ~(BIT_23 | BIT_24);

	if (baseClk >= SDIO_BASE_CLK_384M)
	{
		REG32(EMMC_BASE_AP_CKG) |= BIT_23;
		clk = SDIO_BASE_CLK_384M;					/* default */
	}
	else if (baseClk >= SDIO_BASE_CLK_256M)
	{
		REG32(EMMC_BASE_AP_CKG) |= BIT_24;
		clk = SDIO_BASE_CLK_256M;
	}
	else if (baseClk >= SDIO_BASE_CLK_153P6M)
	{
		REG32(EMMC_BASE_AP_CKG) |= (BIT_23 | BIT_24);
		clk = SDIO_BASE_CLK_153P6M;
	}
	else
	{
		clk = SDIO_BASE_CLK_26M;
	}

#else
	REG32 (EMMC_BASE_AP_CKG) &= ~ (BIT_19 | BIT_20);
	if (baseClk >= SDIO_BASE_CLK_96M)
	{
		clk = SDIO_BASE_CLK_96M;
	}
	else if (baseClk >= SDIO_BASE_CLK_64M)
	{
		REG32(EMMC_BASE_AP_CKG) |= BIT_19;
		clk = SDIO_BASE_CLK_64M;
	}
	else if (baseClk >= SDIO_BASE_CLK_48M)
	{
		REG32(EMMC_BASE_AP_CKG) |= BIT_20;
		clk = SDIO_BASE_CLK_48M;						/* default */
	}
	else
	{
		REG32(EMMC_BASE_AP_CKG) |= (BIT_19 | BIT_20);
		clk = SDIO_BASE_CLK_26M;
	}
#endif
	return clk;
} /* end of SDHOST_BaseClk_Set */

/*********************************************************************
**	Description :
**		This function used to set emmc voltage.
**	Param :
**		emmcCore	:  default used 3.0V.
**		emmcIo		: default used 1.8V.
**	Return :
**		none
**	Date					Author				Operation
**	2013/08/13			ypxie				creat
**	2013/08/14			ypxie				support tiger vlotage.
*********************************************************************/
PUBLIC void SDHOST_Volt_Set (SDIO_Vol_e emmcIo, SDIO_Vol_e emmcCore)
{
	uint16 tmpReg;

#if defined (CONFIG_SC8830)

#if defined(CONFIG_SPX15)	/* set emmc_io */

	tmpReg = ANA_REG_GET(EMMC_VOLT_IO_CTRL);
	tmpReg &= ~(EMMC_VOLT_IO_MASK<<EMMC_VOLT_IO_CTRL_SHIFT);

	switch (emmcIo)
	{
		case VOL_1_5:
			tmpReg |= (7<<3) << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		case VOL_1_8:
			tmpReg |= (13<<3) << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		case VOL_1_3:
			tmpReg |= (3<<3)<< EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		case VOL_1_2:
			tmpReg |= (1<<3)<< EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		default :	/* default used 1.8V */
			tmpReg |= (13<<3) << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
	}

	ANA_REG_SET(EMMC_VOLT_IO_CTRL, tmpReg);

	/* set emmc_core */
	tmpReg = ANA_REG_GET(EMMC_VOLT_CORE_CTRL);
	tmpReg &= ~(EMMC_VOLT_CORE_MASK << EMMC_VOLT_CORE_CTRL_SHIFT);
	switch (emmcCore)
	{
		case VOL_2_8:
			tmpReg |= 160 << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		case VOL_3_0:
			tmpReg |= 180 << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		case VOL_2_5:
			tmpReg |= 120 << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		case VOL_1_8:
			tmpReg |= 60 << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		default :	/* default used 3.0V */
			tmpReg |= 180 << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
	}

	ANA_REG_SET(EMMC_VOLT_CORE_CTRL, tmpReg);
#else
	tmpReg = ANA_REG_GET(EMMC_VOLT_IO_CTRL);
	tmpReg &= ~(EMMC_VOLT_MASK << EMMC_VOLT_IO_CTRL_SHIFT);

	switch (emmcIo)
	{
		case VOL_1_5:
			break;
		case VOL_1_8:
			tmpReg |= 1 << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		case VOL_1_3:
			tmpReg |= 2 << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		case VOL_1_2:
			tmpReg |= 3 << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		default :	/* default used 1.8V */
			tmpReg |= 1 << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
	}

	ANA_REG_SET(EMMC_VOLT_IO_CTRL, tmpReg);

	/* set emmc_core */
	tmpReg = ANA_REG_GET(EMMC_VOLT_CORE_CTRL);
	tmpReg &= ~(EMMC_VOLT_MASK << EMMC_VOLT_CORE_CTRL_SHIFT);
	switch (emmcCore)
	{
		case VOL_2_8:
			break;
		case VOL_3_0:
			tmpReg |= 1 << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		case VOL_2_5:
			tmpReg |= 2 << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		case VOL_1_8:
			tmpReg |= 3 << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		default :	/* default used 3.0V */
			tmpReg |= 1 << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
	}

	ANA_REG_SET(EMMC_VOLT_CORE_CTRL, tmpReg);
#endif
#elif defined (CONFIG_SC8825)
	/* set emmc_io */
	tmpReg = ANA_REG_GET(EMMC_VOLT_IO_CTRL);
	tmpReg &= ~(EMMC_VOLT_MASK << EMMC_VOLT_IO_CTRL_SHIFT);
	switch (emmcIo)
	{
		case VOL_1_5:
			tmpReg |= (BIT_3 | BIT_1) << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		case VOL_1_8:
			tmpReg |= (BIT_3 | BIT_0) << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		case VOL_1_3:
			tmpReg |= (BIT_2 | BIT_1) << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		case VOL_1_2:
			tmpReg |= (BIT_2 | BIT_0) << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		default :	/* default used 1.8V */
			tmpReg |= (BIT_3 | BIT_0) << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
	}

	ANA_REG_SET(EMMC_VOLT_IO_CTRL, tmpReg);

	/* set emmc_core */
	tmpReg = ANA_REG_GET(EMMC_VOLT_CORE_CTRL);
	tmpReg &= ~(EMMC_VOLT_MASK << EMMC_VOLT_CORE_CTRL_SHIFT);
	switch (emmcCore)
	{
		case VOL_2_8:
			tmpReg |= (BIT_3 | BIT_1) << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		case VOL_3_0:
			tmpReg |= (BIT_3 | BIT_0) << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		case VOL_2_5:
			tmpReg |= (BIT_2 | BIT_1) << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		case VOL_1_8:
			tmpReg |= (BIT_2 | BIT_0) << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		default :	/* default used 3.0V */
			tmpReg |= (BIT_3 | BIT_0) << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
	}

	ANA_REG_SET(EMMC_VOLT_CORE_CTRL, tmpReg);
#elif defined (CONFIG_SC7710G2)
	/* set emmc_io */
	tmpReg = ANA_REG_GET(EMMC_VOLT_IO_CTRL);
	tmpReg &= ~(EMMC_VOLT_MASK << EMMC_VOLT_IO_CTRL_SHIFT);
	switch (emmcIo)
	{
		case VOL_1_8:
			tmpReg |= (BIT_3 | BIT_1) << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		case VOL_1_5:
			tmpReg |= (BIT_3 | BIT_0) << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		case VOL_1_3:
			tmpReg |= (BIT_2 | BIT_1) << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		case VOL_1_2:
			tmpReg |= (BIT_2 | BIT_0) << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
		default :	/* default used 1.8V */
			tmpReg |= (BIT_3 | BIT_1) << EMMC_VOLT_IO_CTRL_SHIFT;
			break;
	}

	ANA_REG_SET(EMMC_VOLT_IO_CTRL, tmpReg);

	/* set emmc_core */
	tmpReg = ANA_REG_GET(EMMC_VOLT_CORE_CTRL);
	tmpReg &= ~(EMMC_VOLT_MASK << EMMC_VOLT_CORE_CTRL_SHIFT);
	switch (emmcCore)
	{
		case VOL_3_0:
			tmpReg |= (BIT_3 | BIT_1) << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		case VOL_2_8:
			tmpReg |= (BIT_3 | BIT_0) << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		case VOL_2_5:
			tmpReg |= (BIT_2 | BIT_1) << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		case VOL_1_8:
			tmpReg |= (BIT_2 | BIT_0) << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
		default :	/* default used 3.0V */
			tmpReg |= (BIT_3 | BIT_1) << EMMC_VOLT_CORE_CTRL_SHIFT;
			break;
	}

	ANA_REG_SET(EMMC_VOLT_CORE_CTRL, tmpReg);
#else

#endif

} /* end of SDHOST_Volt_Set */

/*********************************************************************
**	Description :
**		This function used to enable voltage for emmc.
**	Param :
**		emmcCore	:  default used 3.0V.
**		emmcIo		: default used 1.8V.
**	Return :
**		none
**	Date					Author				Operation
**	2013/08/13			ypxie				creat
**	2013/08/15			ypxie				added tiger power on or off
*********************************************************************/
PUBLIC void SDHOST_Volt_Enable (SDIO_OnOff_e voltFlg)
{
	if (SDIO_ON == voltFlg)
	{
	#if defined (CONFIG_SC8830)
	#if defined(CONFIG_SPX15)
		ANA_REG_AND(ANA_REG_GLB_LDO_DCDC_PD, ~BIT_7);
		SDIO_Mdelay(2);
		ANA_REG_AND(ANA_REG_GLB_LDO_DCDC_PD, ~BIT_6);
		SDIO_Mdelay(10);
	#else
		/* set emmc core first. */
		ANA_REG_OR(ANA_REG_GLB_LDO_DCDC_PD_RTCCLR, BIT_8);
		ANA_REG_AND(ANA_REG_GLB_LDO_DCDC_PD_RTCSET, ~ BIT_8);
		SDIO_Mdelay(2);
		ANA_REG_OR(ANA_REG_GLB_LDO_DCDC_PD_RTCCLR, BIT_7);
		ANA_REG_AND(ANA_REG_GLB_LDO_DCDC_PD_RTCSET, ~BIT_7);
		SDIO_Mdelay(10);
	#endif
	#elif defined (CONFIG_SC8825)
		/* set emmc core first. */
		ANA_REG_OR(ANA_LDO_PD_CTL1,  BIT_7);
		ANA_REG_AND(ANA_LDO_PD_CTL1, ~BIT_6);
		SDIO_Mdelay(2);
		ANA_REG_OR(ANA_LDO_PD_CTL1, BIT_5);
		ANA_REG_AND(ANA_LDO_PD_CTL1, ~BIT_4);
		SDIO_Mdelay(10);
	#elif defined (CONFIG_SC7710G2)
		ANA_REG_OR(ANA_LDO_PD_RST,  BIT_9);
		ANA_REG_AND(ANA_LDO_PD_SET, ~BIT_9);
		SDIO_Mdelay(2);
		ANA_REG_OR(ANA_LDO_PD_RST,  BIT_10);
		ANA_REG_AND(ANA_LDO_PD_SET, ~BIT_10);
		SDIO_Mdelay(10);
	#else
	#endif
	}
	else if (SDIO_OFF == voltFlg)
	{
	#if defined (CONFIG_SC8830)
	#if defined(CONFIG_SPX15)
		ANA_REG_OR(ANA_REG_GLB_LDO_DCDC_PD, BIT_7);
		ANA_REG_OR(ANA_REG_GLB_LDO_DCDC_PD, BIT_6);
		SDIO_Mdelay(10);
	#else
		ANA_REG_AND(ANA_REG_GLB_LDO_DCDC_PD_RTCCLR, ~(BIT_7 | BIT_8));
		ANA_REG_OR(ANA_REG_GLB_LDO_DCDC_PD_RTCSET, (BIT_7 | BIT_8));
		/* must make sure power off. maybe need delay 200~300 ms. */
		SDIO_Mdelay(10);
	#endif
	#elif defined (CONFIG_SC8825)
		ANA_REG_AND(ANA_LDO_PD_CTL1,  ~(BIT_7 | BIT_5));
		ANA_REG_OR(ANA_LDO_PD_CTL1, (BIT_6 | BIT_4));
		SDIO_Mdelay(10);
	#elif defined (CONFIG_SC7710G2)
		ANA_REG_AND(ANA_LDO_PD_RST, ~(BIT_9 | BIT_10));
		ANA_REG_OR(ANA_LDO_PD_SET, BIT_9 | BIT_10);
		SDIO_Mdelay(10);
	#else
	#endif
	}
} /* end of SDHOST_Volt_Enable */

/*********************************************************************
**	Description :
**		This function used to register emmc control.
**	Param :
**		fun	: irq call back fun.
**	Return :
**		none
**	Date					Author				Operation
**	2013/05/17			ypxie				optimization
**	2013/08/12			ypxie				add disable sdclk fot clear glitch
*********************************************************************/
PUBLIC SDIO_Hd_Ptr SDHOST_Register (SDIO_CALLBACK fun)
{
	/* Set emmc controller base address first */
	s_EmmcCtl.host_cfg = (SDIO_REG_CFG *) ( (volatile uint32 *) EMMC_CFG_ADDR);
	/* enable emmc ahb_clock, and reset it. */
	REG32 (EMMC_AHB_EN) |= 1 << EMMC_AHB_EN_SHIFT;

	SDHOST_SdClk_Enable(&s_EmmcCtl, SDIO_OFF);

	REG32 (EMMC_AHB_RST) |= 1 << EMMC_AHB_RST_SHIFT;
	SDIO_Mdelay(1);
	REG32 (EMMC_AHB_RST) &= ~(1 << EMMC_AHB_RST_SHIFT);

	s_EmmcCtl.open_flag = TRUE;
	s_EmmcCtl.base_clock = SDHOST_BaseClk_Set(EMMC_BASE_CLK);
	s_EmmcCtl.sig_callBack = fun;
	s_EmmcCtl.err_filter = 0;

	return &s_EmmcCtl;
} /* end of SDHOST_Register */

PUBLIC void SDHOST_SetErrCodeFilter (SDIO_Hd_Ptr pHd, uint32 err_msg)
{
	pHd->err_filter = err_msg;
}

/*********************************************************************
**	Description :
**		This function used to set hareware timeout tick clock. (2 ^ (clkcnt+13))*T_BSCLK
**	Param :
**		pHd		: emmc handle
**		clk_cnt	: clock count.
**	Return :
**		none
**	Date					Author				Operation
**	2013/05/17			ypxie				optimization
*********************************************************************/
PUBLIC void SDHOST_SetDataTimeOutValue (SDIO_Hd_Ptr pHd, uint8 clk_cnt)
{
	volatile uint32 tmpReg,tmpIntReg;

	tmpIntReg = pHd->host_cfg->INT_STA_EN;

	pHd->host_cfg->INT_STA_EN &= ~BIT_20;

	tmpReg = pHd->host_cfg->HOST_CTL1;
	tmpReg &= ~ (0xF << 16);
	tmpReg |= (clk_cnt << 16);
	pHd->host_cfg->HOST_CTL1 = tmpReg;

	pHd->host_cfg->INT_STA_EN = tmpIntReg;
}

/*********************************************************************
**	Description :
**		This function used to send command of sub.
**	Param :
**		pHd		: emmc handle
**		clk_cnt	: clock count.
**	Return :
**		none
**	Date					Author				Operation
**	2013/05/17			ypxie				optimization
*********************************************************************/
PUBLIC void SDHOST_SetCmd (SDIO_Hd_Ptr pHd,
							uint32 cmdIndex,
							uint32 transmode,
							SDIO_CmdType_e cmd_type,
							CARD_CmdRsp_e Response)
{
	volatile uint32 tmpReg;

	tmpReg = pHd->host_cfg->CMD_TRANSMODE;
	tmpReg &= (~ (0x7F | 0x30000 | 0x3FF80000));

	if (0 != (TRANS_MODE_CMPLETE_SIG_EN & transmode))
	{
		tmpReg |= SDIO_TRANS_COMP_ATA;
	}

	if (0 != (TRANS_MODE_MULTI_BLOCK & transmode))
	{
		tmpReg |= SDIO_TRANS_MULTIBLK;
	}

	if (0 != (TRANS_MODE_READ & transmode))
	{
		tmpReg |= SDIO_TRANS_DIR_READ;
	}

	if (0 != (TRANS_MODE_CMD12_EN & transmode))
	{
		tmpReg |= SDIO_TRANS_AUTO_CMD12_EN;
	}

	if (0 != (TRANS_MODE_BLOCK_COUNT_EN & transmode))
	{
		tmpReg |= SDIO_TRANS_BLK_CNT_EN;
	}

	if (0 != (TRANS_MODE_DMA_EN & transmode))
	{
		tmpReg |= SDIO_TRANS_DMA_EN;
	}

	if (0 != (CMD_HAVE_DATA & transmode))
	{
		tmpReg |= SDIO_CMD_DATA_PRESENT;
	}

	switch (cmd_type)
	{
		case CMD_TYPE_NORMAL:
			tmpReg |= SDIO_CMD_TYPE_NML;
			break;

		case CMD_TYPE_SUSPEND:
			tmpReg |= SDIO_CMD_TYPE_SUSPEND;
			break;

		case CMD_TYPE_RESUME:
			tmpReg |= SDIO_CMD_TYPE_RESUME;
			break;

		case CMD_TYPE_ABORT:
			tmpReg |= SDIO_CMD_TYPE_ABORT;
			break;

		default:
			SCI_ASSERT (0);	/*assert verified*/
			break;
	}

	switch (Response)
	{
		case CMD_NO_RSP:
			tmpReg |= SDIO_NO_RSP;
			break;

		case CMD_RSP_R1:
			tmpReg |= SDIO_R1;
			break;

		case CMD_RSP_R2:
			tmpReg |= SDIO_R2;
			break;

		case CMD_RSP_R3:
			tmpReg |= SDIO_R3;
			break;

		case CMD_RSP_R4:
			tmpReg |= SDIO_R4;
			break;

		case CMD_RSP_R5:
			tmpReg |= SDIO_R5;
			break;

		case CMD_RSP_R6:
			tmpReg |= SDIO_R6;
			break;

		case CMD_RSP_R7:
			tmpReg |= SDIO_R7;
			break;

		case CMD_RSP_R1B:
			tmpReg |= SDIO_R1B;
			break;

		case CMD_RSP_R5B:
			tmpReg |= SDIO_R5B;
			break;

		default:
			SCI_ASSERT (0);/*assert verified*/
			break;

	}

	tmpReg |= (cmdIndex<<24);

	pHd->host_cfg->CMD_TRANSMODE = tmpReg;

} /* end of SDHOST_SetCmd */

PUBLIC void SDHOST_SetDataParam (SDIO_Hd_Ptr pHd, uint32 block_size, uint32 block_cnt)
{
	volatile uint32 tmpReg;

	tmpReg = pHd->host_cfg->BLK_SIZE_COUNT;

	/* Set Block Size */
	tmpReg &= (~BIT_15);
	tmpReg &= (~ (0xFFF));
	if (0x1000 == block_size)
	{
		tmpReg |= BIT_15;
	}
	else
	{
		tmpReg |= block_size;
	}

	/* Set Block Cnt */
	tmpReg &= (~0xFFFF0000);
	tmpReg |= (block_cnt << 16);

	/* Set DMA Buf Size */
	tmpReg &= (~ (0x07<<12));

	pHd->host_cfg->BLK_SIZE_COUNT = tmpReg;
}

/*********************************************************************
**	Description :
**		This function used to get rsp from buffer.
**	Param :
**		pHd			: emmc handle
**		Response		: .
**		rspBuf		: .
**	Return :
**		none
**	Date					Author				Operation
**	2013/05/17			ypxie				optimization
*********************************************************************/
PUBLIC void SDHOST_GetRspFromBuf (SDIO_Hd_Ptr pHd,
								CARD_CmdRsp_e Response,
								uint8 *rspBuf)
{
	uint32 tmpRspBuf[4];
	uint32 i;

	tmpRspBuf[0] = pHd->host_cfg->RSP0;
	tmpRspBuf[1] = pHd->host_cfg->RSP1;
	tmpRspBuf[2] = pHd->host_cfg->RSP2;
	tmpRspBuf[3] = pHd->host_cfg->RSP3;

	for (i = 0; i < 4; i++)
	{
		rspBuf[0 + (i << 2) ] = (uint8) ( (tmpRspBuf[i] >> 24) & 0xFF);
		rspBuf[1 + (i << 2) ] = (uint8) ( (tmpRspBuf[i] >> 16) & 0xFF);
		rspBuf[2 + (i << 2) ] = (uint8) ( (tmpRspBuf[i] >> 8) & 0xFF);
		rspBuf[3 + (i << 2) ] = (uint8) (tmpRspBuf[i] & 0xFF);
	}

	switch (Response)
	{
		case CMD_NO_RSP:
			break;

		case CMD_RSP_R1:
		case CMD_RSP_R1B:
		case CMD_RSP_R3:
		case CMD_RSP_R4:
		case CMD_RSP_R5:
		case CMD_RSP_R6:
		case CMD_RSP_R7:
		case CMD_RSP_R5B:
			rspBuf[0] = (uint8) ( (tmpRspBuf[0]>>24) &0xFF);
			rspBuf[1] = (uint8) ( (tmpRspBuf[0]>>16) &0xFF);
			rspBuf[2] = (uint8) ( (tmpRspBuf[0]>>8) &0xFF);
			rspBuf[3] = (uint8) (tmpRspBuf[0]&0xFF);
			break;

		case CMD_RSP_R2:
			rspBuf[0] = (uint8) ( (tmpRspBuf[3]>>16) &0xFF);
			rspBuf[1] = (uint8) ( (tmpRspBuf[3]>>8) &0xFF);
			rspBuf[2] = (uint8) (tmpRspBuf[3]&0xFF);

			rspBuf[3] = (uint8) ( (tmpRspBuf[2]>>24) &0xFF);
			rspBuf[4] = (uint8) ( (tmpRspBuf[2]>>16) &0xFF);
			rspBuf[5] = (uint8) ( (tmpRspBuf[2]>>8) &0xFF);
			rspBuf[6] = (uint8) (tmpRspBuf[2]&0xFF);

			rspBuf[7] = (uint8) ( (tmpRspBuf[1]>>24) &0xFF);
			rspBuf[8] = (uint8) ( (tmpRspBuf[1]>>16) &0xFF);
			rspBuf[9] = (uint8) ( (tmpRspBuf[1]>>8) &0xFF);
			rspBuf[10] = (uint8) (tmpRspBuf[1]&0xFF);

			rspBuf[11] = (uint8) ( (tmpRspBuf[0]>>24) &0xFF);
			rspBuf[12] = (uint8) ( (tmpRspBuf[0]>>16) &0xFF);
			rspBuf[13] = (uint8) ( (tmpRspBuf[0]>>8) &0xFF);
			rspBuf[14] = (uint8) (tmpRspBuf[0]&0xFF);
			break;

		default:
			SCI_ASSERT (0);/*assert verified*/
			break;
	}
	return;
} /* end of SDHOST_GetRspFromBuf */

/*********************************************************************
**	Description :
**		This function used to send cmd.
**	Param :
**		pHd		: emmc handle
**		cmd		: command.
**		argument	: 
**		dataParam	: blk buf,size, num.
**	Return :
**		none
**	Date					Author				Operation
**	2013/05/17			ypxie				optimization
*********************************************************************/
PUBLIC SDIO_Error_e SDIO_SendCmd ( SDIO_Hd_Ptr pHd,
								CARD_Cmd_e cmd,
								uint32 argument,
								DATA_Param_t *dataParam,
								uint8 *rspBuf)
{
	uint32 tmpIntFilter;

	SDHOST_SdClk_Enable(pHd, SDIO_ON);
	SDHOST_NML_INT_SigSt_Dis(pHd, SIG_ALL);

	SDHOST_SetDataTimeOutValue (pHd, 0xE);
	SDHOST_SetErrCodeFilter (pHd, s_cmdDetail[cmd].err_filter);
	tmpIntFilter = s_cmdDetail[cmd].int_filter;

	if (NULL != s_cmdDetail[cmd].err_filter)
	{
		tmpIntFilter |= SIG_ERR;
	}

	if (NULL != dataParam)
	{
		tmpIntFilter |= SIG_DMA_INT;
	}

	SDHOST_NML_INT_SigSt_En(pHd, tmpIntFilter);

	pHd->card_errCode = 0;
	pHd->card_event = 0;

	if (NULL != dataParam)
	{
		uint32 bufferSize = 0;
		bufferSize = dataParam->blk_len  *  (dataParam->blk_num);
		SDHOST_SetDmaAddr (pHd, (uint32) (dataParam->data_buf));
		SDHOST_SetDataParam (pHd, dataParam->blk_len, dataParam->blk_num);
	}

	pHd->host_cfg->CMD_ARGUMENT = argument;
	SDHOST_SetCmd (pHd, 
					s_cmdDetail[cmd].cmd_index,
					s_cmdDetail[cmd].transmode,
					CMD_TYPE_NORMAL, 
					s_cmdDetail[cmd].response);

	while (0 != _WaitCardEvent(pHd, s_cmdDetail[cmd].int_filter))
	{
		_SDHOST_IrqHandle((uint32)pHd);
	}

	SDHOST_RST (pHd, RST_CMD_DAT_LINE);
	SDHOST_GetRspFromBuf (pHd, s_cmdDetail[cmd].response, rspBuf);

	return SDIO_ERR_NONE;
}/* end of SDIO_SendCmd */

/*********************************************************************
**	Description :
**		This function used to set sd clock.
**	Param :
**		pHd		: emmc handle
**		sdio_clk	: want to set clock.
**	Return :
**		none
**	Date					Author				Operation
**	2013/05/17			ypxie				optimization
**	2013/08/12			ypxie				delete old code.
*********************************************************************/
PUBLIC uint32 SDHOST_ClkFreq_Set (SDIO_Hd_Ptr pHd, uint32 sdioClk)
{
	volatile uint32 tmpReg;
	uint32 clkDiv;

	/* SDCLK Frequency Select ,Configure SDCLK select */
	clkDiv = pHd->base_clock / sdioClk;
#if defined (CONFIG_SC8825) || defined(CONFIG_SC7710G2) || defined (CONFIG_SC8830)
	clkDiv /= 2;
#endif
	if (0 != pHd->base_clock % sdioClk)
	{
		clkDiv++;
	}

	tmpReg = pHd->host_cfg->HOST_CTL1;
#if defined (CONFIG_SC8825) || defined(CONFIG_SC7710G2) || defined (CONFIG_SC8830)
	if (clkDiv > 0)
	{
		clkDiv--;	
	}

	tmpReg &= (~(0x3ff << 6));
	tmpReg |= ((clkDiv >> 8) & 0x3) << 6;
	tmpReg |= (clkDiv & 0xff) << 8;

	pHd->sd_clock = pHd->base_clock / (2 * (clkDiv + 1));
#else    
	tmpReg &= (~ (0xff<<8));

	for (uint32 i = 0; i < 8; i++)
	{
		if (((clkDiv > (1 << i)) && (clkDiv <= (1 << (i + 1)))) || (i ==  7))
		{
			clkDiv = (1 << (i + 1));
			tmpReg |= (i << 8);
			break;
		}
	}

	pHd->sd_clock = pHd->base_clock/clkDiv;
#endif

	pHd->host_cfg->HOST_CTL1 = tmpReg;

	return pHd->sd_clock;
} /* end of SDHOST_ClkFreq_Set */

void SDHOST_MasterSel(void )
{
	REG32(EMMC_SLOT_SEL_REG)  &= ~EMMC_SLOT_SEL_BIT;
}

PUBLIC BOOLEAN SDIO_SetClk (SDIO_Hd_Ptr pHd, uint32 sdioClk)
{
	SDHOST_SdClk_Enable (pHd, SDIO_OFF);
	SDHOST_ClkFreq_Set (pHd, sdioClk);
	SDHOST_SdClk_Enable (pHd, SDIO_ON);

	return TRUE;
}

/*********************************************************************
**	Description :
**		This function used to set card to high speed mode.
**	Param :
**		pHd			: emmc handle
**		widthType	: want to set clock.
**	Return :
**		none
**	Date					Author				Operation
**	2013/07/19			ypxie				optimization
*********************************************************************/
PUBLIC BOOLEAN SDIO_SetBusWidth (SDIO_Hd_Ptr pHd, SDIO_BusWidth_e busWidth)
{
	uint32 regVal;

	regVal = pHd->host_cfg->HOST_CTL0;

	/* clear bus width set bits */
	regVal &= ~((1 << 5) | (1 << 1));
	switch (busWidth)
	{
		case SDIO_BUS_1_BIT:
			break;

		case SDIO_BUS_4_BIT:
			regVal |= 1 << 1;
			break;

		case SDIO_BUS_8_BIT:
			regVal |= 1 << 5;
			break;

		default:
			break;
	}

	pHd->host_cfg->HOST_CTL0 = regVal;

	return TRUE;
}


PUBLIC BOOLEAN CARD_SDIO_SetBlockLength(SDIO_Hd_Ptr pHd,uint32 length)
{
	uint8 rspBuf[16];

	if(0 != SDIO_SendCmd(pHd, CARD_CMD16_SET_BLOCKLEN,
						length, (DATA_Param_t *)NULL, rspBuf))
	{
		return FALSE;
	}
	pHd->block_len = length;
	return TRUE;
}

/*********************************************************************
**	Description :
**		This function used to select current partition.boot1, boot2, user.
**	Param :
**		pHd			: emmc handle
**		cardPartition	: want to set clock.
**	Return :
**		none
**	Date					Author				Operation
**	2013/07/18			ypxie				optimization
*********************************************************************/
BOOLEAN CARD_SDIO_SelCurPartition( SDIO_Hd_Ptr pHd, CARD_Partition_e cardPartiton)
{
	uint8 rspBuf[16];
	uint32 argument = 0;

	if (pHd->cur_partition < PARTITION_MAX)
	{
		if (pHd->cur_partition == cardPartiton)
		{
			return TRUE;
		}
	}

	argument = CMD6_ACCESS_MODE_WRITE_BYTE |
				( EXT_CSD_PARTITION_CFG_INDEX << CMD6_BIT_MODE_OFFSET_INDEX &
				CMD6_BIT_MODE_MASK_INDEX) | CMD6_CMD_SET;

	switch(cardPartiton)
	{
		case PARTITION_USER:
			argument |= (PARTITION_CFG_ENABLE_BOOT1 |PARTITION_CFG_ACCESS_USER)
						<< CMD6_BIT_MODE_OFFSET_VALUE;
			break;
			
		case PARTITION_BOOT1:
			argument |= (PARTITION_CFG_ENABLE_BOOT1 |PARTITION_CFG_ACCESS_BOOT1)
						<< CMD6_BIT_MODE_OFFSET_VALUE;
			break;
				
		case PARTITION_BOOT2:
			argument |= (PARTITION_CFG_ENABLE_BOOT1 |PARTITION_CFG_ACCESS_BOOT2)
						<< CMD6_BIT_MODE_OFFSET_VALUE;
			break;
			
		case PARTITION_RPMB:
		case PARTITION_GENERAL_P1:
		case PARTITION_GENERAL_P2:
		case PARTITION_GENERAL_P3:
		case PARTITION_GENERAL_P4:
		default:
			return FALSE;
			break;

	}	

	if(0 != SDIO_SendCmd(pHd, CARD_ACMD6_SET_EXT_CSD, argument, NULL, rspBuf))
	{
		return FALSE;
	}

	pHd->cur_partition = cardPartiton;
	
	return TRUE;
}

/*********************************************************************
**	Description :
**		This function used to set card to high speed mode.
**	Param :
**		pHd			: emmc handle
**		cardPartition	: want to set clock.
**	Return :
**		none
**	Date					Author				Operation
**	2013/07/19			ypxie				optimization
*********************************************************************/
LOCAL BOOLEAN MMC_SWITCH(SDIO_Hd_Ptr pHd, uint8 index, uint8 value)
{
	uint8 rspBuf[16];
	uint32 argument = 0;

	argument = CMD6_ACCESS_MODE_WRITE_BYTE | CMD6_CMD_SET;
	argument |= index << CMD6_BIT_MODE_OFFSET_INDEX;
	argument |= value << CMD6_BIT_MODE_OFFSET_VALUE;


	if(0 != SDIO_SendCmd(pHd, CARD_ACMD6_SET_EXT_CSD, argument, NULL, rspBuf))
	{
		return FALSE;
	}

	return TRUE;
}

/*********************************************************************
**	Description :
**		This function used to set card and host bus width.
**	Param :
**		pHd			: emmc handle
**		width		: bus width.
**	Return :
**		none
**	Date					Author				Operation
**	2013/07/19			ypxie				optimization
**	2013/08/12			ypxie				support 1,8 bits buswidth
*********************************************************************/
LOCAL BOOLEAN CARD_SDIO_SetBusWidth(SDIO_Hd_Ptr pHd, SDIO_BusWidth_e busWidth)
{
	uint8 rspBuf[16];
	uint32 argu = 0;

	argu = CMD6_ACCESS_MODE_WRITE_BYTE |
			( EXT_CSD_BUS_WIDTH_INDEX << CMD6_BIT_MODE_OFFSET_INDEX &
			CMD6_BIT_MODE_MASK_INDEX) ;

	switch (busWidth)
	{
		case SDIO_BUS_1_BIT:
			argu |= EXT_CSD_CARD_WIDTH_1_BIT << CMD6_BIT_MODE_OFFSET_VALUE;
			break;

		case SDIO_BUS_4_BIT:
			argu |= EXT_CSD_CARD_WIDTH_4_BIT << CMD6_BIT_MODE_OFFSET_VALUE;
			break;

		case SDIO_BUS_8_BIT:
			argu |= EXT_CSD_CARD_WIDTH_8_BIT << CMD6_BIT_MODE_OFFSET_VALUE;
			break;

		default:
			argu |= EXT_CSD_CARD_WIDTH_1_BIT << CMD6_BIT_MODE_OFFSET_VALUE;
			busWidth = SDIO_BUS_1_BIT;
			break;
	}

	if(0 != SDIO_SendCmd(pHd, CARD_ACMD6_SET_EXT_CSD, argu, NULL, rspBuf))
	{
		return FALSE;
	}

	SDIO_SetBusWidth(pHd, busWidth);

	pHd->bus_width = busWidth;

	return TRUE;
}

/*********************************************************************
**	Description :
**		This function used to initialize card.
**	Param :
**		pHd			: emmc handle
**		width		: bus width.
**	Return :
**		none
**	Date					Author				Operation
**	2013/07/19			ypxie				optimization
**	2013/08/12			ypxie				added params for card
*********************************************************************/
PUBLIC BOOLEAN CARD_SDIO_InitCard(SDIO_Hd_Ptr pHd,
								uint32 sdioClk,
								SDIO_BusWidth_e busWidth)
{
	uint8 rspBuf[16];
	uint32 pre_tick, cur_tick;

	if (0 != SDIO_SendCmd(pHd, CARD_CMD0_GO_IDLE_STATE, 0, NULL, rspBuf))
	{
		return FALSE;
	}

	pre_tick = SCI_GetTickCount(); /*set start tick value*/       
	do
	{
		if (0 != SDIO_SendCmd(pHd, CARD_CMD1_SEND_OP_COND,
							0x00FF8000 | SECTOR_MODE, NULL, rspBuf))
		{
			continue;
		}

		if(0 != (rspBuf[0] & BIT_7))
		{
			break;
		}

		cur_tick = SCI_GetTickCount();
		if( 5000 <  (cur_tick - pre_tick))
		{
			return FALSE;
		} 
	} while(1);

	/* Get CID */
	if (0 != SDIO_SendCmd(pHd, CARD_CMD2_ALL_SEND_CID, 0, NULL, rspBuf))
	{	
		return FALSE;
	}

	if (0 != SDIO_SendCmd(pHd, CARD_CMD3_SET_RELATIVE_ADDR, 1 << 16, NULL, rspBuf))
	{
		return FALSE;
	}

	if(0 != SDIO_SendCmd(pHd, CARD_CMD13_SEND_STATUS, 1<<16, NULL, rspBuf))
	{
		return FALSE;
	}

	if(0 != SDIO_SendCmd(pHd, CARD_CMD7_SELECT_DESELECT_CARD, 1<<16, NULL, rspBuf))
	{
		return FALSE;
	}
	if(0 != SDIO_SendCmd(pHd, CARD_CMD13_SEND_STATUS, 1<<16, NULL, rspBuf))
	{
		return FALSE;
	}

	/* get Boot2 Capacity */
	if ( FALSE == CARD_SDIO_SelCurPartition(pHd, PARTITION_BOOT2) )
	{
		return FALSE;
	}

	if(0 != SDIO_SendCmd(pHd, CARD_CMD13_SEND_STATUS, 1<<16, NULL, rspBuf))
	{
		return FALSE;
	}

	if (FALSE == MMC_SWITCH(pHd, EXT_CSD_HS_TIMING_INDEX, 1))
	{
		return FALSE;
	}

	if(0 != SDIO_SendCmd(pHd,CARD_CMD13_SEND_STATUS, 1<<16,NULL,rspBuf))
	{
		return FALSE;
	}

	SDIO_SetClk(pHd, sdioClk);

	if(FALSE == CARD_SDIO_SetBusWidth(pHd, busWidth))
	{
		return FALSE;
	}

	if(FALSE == CARD_SDIO_SetBlockLength(pHd, CARD_DATA_BLOCK_LEN))
	{
		return FALSE;
	}
	return TRUE;
} /* end of CARD_SDIO_InitCard */

/*********************************************************************
**	Description :
**		This function used to set sdio poower on or off
**	Param :
**		pHd			: emmc handle
**		width		: bus width.
**	Return :
**		none
**	Date					Author				Operation
**	2013/07/19			ypxie				optimization
*********************************************************************/
PUBLIC BOOLEAN SDIO_PowerCtl (SDIO_Hd_Ptr pHd, SDIO_OnOff_e pwrFlg)
{
	switch (pwrFlg)
	{
		case SDIO_ON:
			/* disable sd clock, and reset controller.*/
			SDHOST_RST(pHd, RST_ALL);

			SDHOST_Volt_Set(VOL_1_8, VOL_3_0);

			SDHOST_Volt_Enable (SDIO_ON);

			/* set frequece for sdio. */
			SDHOST_ClkFreq_Set (pHd, SDIO_BASE_CLK_400K);
			SDHOST_InternalClk_Enable(pHd, SDIO_ON);
			SDHOST_SdClk_Enable(pHd, SDIO_ON);
			break;

		case SDIO_OFF:
			/* disable sd clock. */
			SDHOST_SdClk_Enable(pHd, SDIO_OFF);
			SDHOST_RST (pHd, RST_ALL);
			SDHOST_Volt_Enable (SDIO_OFF);
			break;

		default:
			break;
	}
	return TRUE;
}

/*********************************************************************
**	Description :
**		This function used to check card is busy or ready.
**	Param :
**		pHd			: emmc handle
**		width		: bus width.
**	Return :
**		none
**	Date					Author				Operation
**	2013/08/09			ypxie				optimization
**	2013/08/12			ypxie				clear redundant variable
*********************************************************************/
LOCAL BOOLEAN IsCardReady(SDIO_Hd_Ptr pHd)
{
	uint32 pre_tick = 0, cur_tick = 0;
	uint8 rspBuf[16] ;

	pre_tick = SCI_GetTickCount();
	do
	{
		if(0 != SDIO_SendCmd(pHd, CARD_CMD13_SEND_STATUS, 1 << 16, NULL, rspBuf))
		{
			return FALSE;
		}

		if(0 != (rspBuf[2] & BIT_0))
		{
			return TRUE;
		}

		cur_tick = SCI_GetTickCount();
		if(5000 <  (cur_tick - pre_tick))
		{
			return FALSE;
		}
	}
	while(1);
}

/*********************************************************************
**	Description :
**		This function used to initialize emmc.
**	Param :
**		none
**	Return :
**		none
**	Date					Author				Operation
**	2013/08/09			ypxie				optimization
*********************************************************************/
PUBLIC BOOLEAN Emmc_Init( void )
{
	uint32 ret = 0;

	p_EmmcHd = SDHOST_Register ( _irqCardProc);

	SDHOST_MasterSel();

	//SDIO_PowerCtl(p_EmmcHd, SDIO_OFF);
	SDIO_PowerCtl(p_EmmcHd, SDIO_ON);

	p_EmmcHd->block_len = 0;
	p_EmmcHd->rca = 1;
	p_EmmcHd->bus_width = SDIO_BUS_1_BIT;
	p_EmmcHd->cur_partition = PARTITION_MAX;
	ret = CARD_SDIO_InitCard(p_EmmcHd, SDIO_BASE_CLK_26M, SDIO_BUS_4_BIT);

	return ret;
}

/*********************************************************************
**	Description :
**		This function used to read from emmc card.
**	Param :
**		none
**	Return :
**		none
**	Date					Author				Operation
**	2013/08/09			ypxie				optimization
*********************************************************************/
PUBLIC BOOLEAN CARD_SDIO_ReadMultiBlock(CARD_Partition_e  cardPartiton,
									SDIO_Hd_Ptr pHd,
									uint32 startBlk,
									uint32 num,
									uint8* buf)
{
	uint8 rspBuf[16];
	uint32 addr = 0xFFFFFFFF;
	DATA_Param_t data;

	addr = startBlk*pHd->block_len;

	data.blk_len = pHd->block_len;
	data.blk_num = num;
	data.data_buf = buf;

	if (FALSE == IsCardReady(pHd))
	{
		return FALSE;
	}

	if(0 != SDIO_SendCmd(pHd, CARD_CMD18_READ_MULTIPLE_BLOCK, addr, &data, rspBuf))
	{
		SDIO_SendCmd(pHd, CARD_CMD12_STOP_TRANSMISSION, NULL, NULL, rspBuf);
		return FALSE;
	}
	if(0 != SDIO_SendCmd(pHd,CARD_CMD12_STOP_TRANSMISSION,NULL,NULL,rspBuf))
	{
		return FALSE;
	}
	return TRUE;

}

PUBLIC BOOLEAN Emmc_Read(CARD_Partition_e  cardPartiton, uint32 startBlk,uint32 num,uint8* buf)
{
	uint32 ret = 0;
	uint8 rspBuf[16];

	if (p_EmmcHd->cur_partition != cardPartiton)
	{
		if(0 != SDIO_SendCmd(p_EmmcHd,CARD_CMD13_SEND_STATUS, 1<<16,NULL,rspBuf))
		{
			return FALSE;
		}

		if ( FALSE == CARD_SDIO_SelCurPartition(p_EmmcHd, cardPartiton))
		{
			return FALSE;
		}
	}

	ret = CARD_SDIO_ReadMultiBlock(cardPartiton, p_EmmcHd, startBlk, num, buf);    
	return ret;
}

PUBLIC void Emmc_DisSdClk(void )
{
	SDHOST_SdClk_Enable(p_EmmcHd, SDIO_OFF);
}
