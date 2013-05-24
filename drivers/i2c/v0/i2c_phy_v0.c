/******************************************************************************
 ** File Name:      I2C_phy_v0.c                                                 *
 ** Author:         liuhao                                                   *
 ** DATE:           06/28/2010                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file define the physical layer of I2C device.      *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 06/28/2010     liuhao     Create.                                   *
 ******************************************************************************/

/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/
//#include "os_api.h"
//#include "chip_plf_export.h"
//#include "timer_drvapi.h"
#include "i2c_reg_v0.h"
#include "asm/arch/sys_timer_reg_v0.h"
//#include "sci_types.h"
#include <asm/arch/chip_drv_common_io.h>

#include "../sc8830_i2c_cfg.h"
#include "../i2c_phy.h"

/**---------------------------------------------------------------------------*
 **                         Debugging Flag                                    *
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/**---------------------------------------------------------------------------*
 **                            Macro Define
 **---------------------------------------------------------------------------*/
//extern uint32 TIMER_GetSystemCounterReg (void);
//extern void SyscntEnable (void);

#define I2C_TIMEOUT_FACTOR  500000  //the critical value is 10000

#define I2C_WAIT_INT                                                  \
    {                                                                     \
        timetick = SYSTEM_CURRENT_CLOCK;                                  \
        while(!(ptr->ctl & I2CCTL_INT))                                   \
        {                                                                 \
            uint32 wait_counter=0x00;   \
            for(wait_counter=0x00; wait_counter<0xff;wait_counter++)    \
            {                                                   \
                wait_counter++;                         \
            }                                                    \
            if((SYSTEM_CURRENT_CLOCK - timetick) >= g_i2c_timeout)        \
            {                                                             \
                if(ERR_I2C_NONE == ret_value)                             \
                {                                                         \
                    ret_value = ERR_I2C_INT_TIMEOUT;                      \
                }                                                         \
                break;                                                    \
            }                                                             \
            \
        }                                                                 \
        \
    }

#define I2C_WAIT_BUSY                                                 \
    {                                                                     \
        timetick = SYSTEM_CURRENT_CLOCK;                                  \
        while(ptr->cmd & I2CCMD_BUS)                                      \
        {                                                                 \
            if ((SYSTEM_CURRENT_CLOCK - timetick) >= g_i2c_timeout)       \
            {                                                             \
                if(ERR_I2C_NONE == ret_value)                             \
                {                                                         \
                    ret_value = ERR_I2C_BUSY_TIMEOUT;                     \
                }                                                         \
                break;                                                    \
            }                                                             \
        }                                                                 \
        \
    }

#define I2C_WAIT_ACK                                                  \
    {                                                                     \
        timetick = SYSTEM_CURRENT_CLOCK;                                  \
        while(ptr->cmd & I2CCMD_ACK)                                      \
        {                                                                 \
            if ((SYSTEM_CURRENT_CLOCK - timetick) >= g_i2c_timeout)       \
            {                                                             \
                if(ERR_I2C_NONE == ret_value)                             \
                {                                                         \
                    ret_value = ERR_I2C_ACK_TIMEOUT;                      \
                }                                                         \
                break;                                                    \
            }                                                             \
        }                                                                 \
    }


#define I2C_CLEAR_INT                                                 \
    {                                                                     \
        I2C_WAIT_BUSY                                                     \
        ptr->cmd |= I2CCMD_INT_ACK; \
    }

/**---------------------------------------------------------------------------*
 **                            Local Variables
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                            Global Variables
 **---------------------------------------------------------------------------*/
LOCAL volatile uint32     g_i2c_timeout = 2; //unit is ms
extern const I2C_BASE_INFO __i2c_base_info[I2C_BUS_MAX];

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
**
**
**
** output parameters:
** Returned value:
*********************************************************************************************************/
LOCAL uint32  __I2C_PHY_GetBase (uint32 phy_id)
{
    int32 i;
    uint32 ret = 0;

    //SyscntEnable();

    for (i = 0; i < I2C_BUS_MAX; i++)
    {
        if (phy_id == (uint32) __i2c_base_info[i].phy_id)
        {
            ret = (uint32) __i2c_base_info[i].base_addr;
            
            switch(phy_id)
            {
		case 0:
			CHIP_REG_OR (GR_GEN0, (GEN0_I2C0_EN));
			break;	
		case 1:
			CHIP_REG_OR (GR_GEN0, (GEN0_I2C1_EN));
			break;	
		case 2:
			CHIP_REG_OR (GR_GEN0, (GEN0_I2C2_EN));
			break;
		case 3:
			CHIP_REG_OR (GR_GEN0, (GEN0_I2C3_EN));
			break;	
		case 4:
			CHIP_REG_OR (GR_GEN0, (GEN0_I2C4_EN));
			break;
                case 5:
                        CHIP_REG_OR (AON_APB_EB0, (AON_I2C_EN));
                        break;
	    }
            break;
        }
    }

    //SCI_PASSERT ( (0 != ret), ("get I2C controller base address fail!"));/*assert verified*/
    return ret;
}
/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
**
**
**
** output parameters:
** Returned value:
*********************************************************************************************************/
LOCAL ERR_I2C_E __I2C_PHY_SetSCL (uint32 phy_id, uint32 freq)
{
    uint32 APB_clk,i2c_dvd;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    APB_clk= 26*1000*1000;//CHIP_GetAPBClk();
    i2c_dvd=APB_clk/ (4*freq)-1;
    ptr->div0= (uint16) (i2c_dvd & 0xffff);
    ptr->div1= (uint16) (i2c_dvd>>16);
    g_i2c_timeout = I2C_TIMEOUT_FACTOR / (freq);

    if (g_i2c_timeout < 2)
    {
        g_i2c_timeout = 2;
    }

    return ERR_I2C_NONE;
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
**
**
**
** output parameters:
** Returned value:
*********************************************************************************************************/
LOCAL ERR_I2C_E __I2C_PHY_SetPort (uint32 port)
{
    return ERR_I2C_NONE;
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
**
**
**
** output parameters:
** Returned value:
*********************************************************************************************************/
LOCAL ERR_I2C_E I2C_PHY_ControlInit_V0 (uint32 phy_id, uint32 freq, uint32 port)
{
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);

    ptr->rst = BIT_0;//why reset
    ptr->ctl &= ~ (I2CCTL_EN); //you must first disable i2c module then change clock
    ptr->ctl &= ~ (I2CCTL_IE);
    //ptr->ctl &= ~ (I2CCTL_CMDBUF_EN);
    __I2C_PHY_SetSCL (phy_id, freq);

    if (I2C_PORT_NUM < port)
    {
        //SCI_ASSERT (0);/*assert to do*/
    }

    __I2C_PHY_SetPort (port);

    //CHIP_REG_OR (I2C_CTL, (I2CCTL_IE | I2CCTL_EN));
    ptr->ctl |=  (I2CCTL_IE | I2CCTL_EN);
    //Clear I2C int
    //CHIP_REG_OR (I2C_CMD, I2CCMD_INT_ACK);
    ptr->cmd &= ~ (I2CCMD_INT_ACK);
        
    IIC_PRINT ("[IIC DRV:]I2C_PHY_ControlInit_V0: freq=%d, port=%d", freq, port);
    return ERR_I2C_NONE;
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
**
**
**
** output parameters:
** Returned value:
*********************************************************************************************************/
LOCAL ERR_I2C_E I2C_PHY_StartBus_V0 (uint32 phy_id, uint8 addr, BOOLEAN rw, BOOLEAN ack_en)
{
    uint32 timetick = 0;
    uint32 cmd = 0;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    uint32   ret_value = ERR_I2C_NONE;

    if (rw)
    {
        /*read cmd*/
        cmd = ( (uint32) (addr |0x1)) <<8;
    }
    else
    {
        /*write cmd*/
        cmd = ( (uint32) addr) <<8;
    }

    cmd = cmd | I2CCMD_START | I2CCMD_WRITE;
    IIC_PRINT ("[IIC DRV:]I2C_PHY_StartBus_V0: cmd=%x", cmd);
    ptr->cmd = cmd;
    I2C_WAIT_INT
    I2C_CLEAR_INT

    //check ACK
    if (ack_en)
    {
        I2C_WAIT_ACK
    }

    return ERR_I2C_NONE;
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
**
**
**
** output parameters:
** Returned value:
*********************************************************************************************************/
LOCAL ERR_I2C_E I2C_PHY_WriteBytes_V0 (uint32 phy_id, uint8 *pCmd, uint32 len, BOOLEAN ack_en, BOOLEAN no_stop)
{
    uint32 timetick = 0;
    uint32 i = 0;
    uint32 cmd = 0;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    uint32   ret_value = ERR_I2C_NONE;

    for (i=0; i<len; i++)
    {
        cmd = ( (uint32) pCmd[i]) <<8;
        cmd = cmd | I2CCMD_WRITE ;

        if ( (i== (len-1)) && (!no_stop))
        {
            cmd = cmd | I2CCMD_STOP;
        }

        ptr->cmd = cmd;
        IIC_PRINT ("[IIC DRV:]I2C_PHY_WriteBytes_V0: cmd=%x", cmd);
        I2C_WAIT_INT
        I2C_CLEAR_INT

        //check ACK
        if (ack_en)
        {
            I2C_WAIT_ACK
        }
    }

    return ret_value;
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
**
**
**
** output parameters:
** Returned value:
*********************************************************************************************************/
LOCAL ERR_I2C_E I2C_PHY_ReadBytes_V0 (uint32 phy_id, uint8 *pCmd, uint32 len, BOOLEAN ack_en)
{
    uint32 timetick = 0;
    uint32 i = 0;
    uint32 cmd = 0;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    uint32   ret_value = ERR_I2C_NONE;

    for (i=0; i<len; i++)
    {
        cmd = I2CCMD_READ; /*FIXME |I2CCMD_TX_ACK;*/

        if (i== (len-1))
        {
            cmd = cmd |I2CCMD_STOP |I2CCMD_TX_ACK;
        }

        ptr->cmd = cmd;
        IIC_PRINT ("[IIC DRV:]I2C_PHY_ReadBytes_V0: cmd=%x", cmd);
        I2C_WAIT_INT
        I2C_CLEAR_INT
        pCmd[i] = (uint8) ( (ptr->cmd) >>8);
    }

    return ret_value;
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
**
**
**
** output parameters:
** Returned value:
*********************************************************************************************************/
LOCAL ERR_I2C_E I2C_PHY_StopBus_V0 (uint32 phy_id)
{
    uint32 timetick = 0;
    uint32 cmd = 0;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    uint32   ret_value = ERR_I2C_NONE;
    cmd = I2CCMD_STOP;
    ptr->cmd = cmd;
    I2C_WAIT_INT
    I2C_CLEAR_INT
    return ERR_I2C_NONE;
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
**
**
**
** output parameters:
** Returned value:
*********************************************************************************************************/
LOCAL ERR_I2C_E I2C_PHY_SendACK_V0 (uint32 phy_id)
{
    return ERR_I2C_NONE;
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:
**
**
**
** output parameters:
** Returned value:
*********************************************************************************************************/
LOCAL ERR_I2C_E I2C_PHY_GetACK_V0 (uint32 phy_id)
{
    return ERR_I2C_NONE;
}

/*this version armcc can not support this method :(*/
#if 0
PUBLIC I2C_PHY_FUN phy_fun_v0 = {
        .init = I2C_PHY_ControlInit_V0,
        .start = I2C_PHY_StartBus_V0,
        .stop = I2C_PHY_StopBus_V0,
        .read = I2C_PHY_ReadBytes_V0,
        .write = I2C_PHY_WriteBytes_V0,
        .sendack = I2C_PHY_SendACK_V0,
        .getack = I2C_PHY_GetACK_V0,
};

#else

PUBLIC I2C_PHY_FUN phy_fun_v0 =
{
    I2C_PHY_ControlInit_V0,
    I2C_PHY_StartBus_V0,
    I2C_PHY_WriteBytes_V0,
    I2C_PHY_ReadBytes_V0,
    I2C_PHY_StopBus_V0,
    I2C_PHY_SendACK_V0,
    I2C_PHY_GetACK_V0
};
#endif
/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/


#ifdef   __cplusplus
}
#endif

/*  End Of File */
