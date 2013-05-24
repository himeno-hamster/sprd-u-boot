/******************************************************************************
 ** File Name:      sc8810_i2c_cfg.c                                                *
 ** Author:         liuhao                                                   *
 ** DATE:           06/28/2010                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file define the hal layer of I2C device.      *
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
//#include "v0/i2c_reg_v0.h"
#include "sc8830_i2c_cfg.h"
#include "asm/arch/sci_types.h"

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

/**---------------------------------------------------------------------------*
 **                            Local Variables
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                            Global Variables
 **---------------------------------------------------------------------------*/
extern I2C_PHY_FUN phy_fun_v0;
//extern I2C_PHY_FUN phy_fun_v1;

const I2C_PHY_CFG __i2c_phy_cfg[I2C_ID_MAX] =
{
    /*Note: Only port 1 is pulled up internal, other port should be pulled up external*/
    /*logic id, controller id, port id, method*/
    {0, 0, 1, &phy_fun_v0}, /*hw i2c controller0*/
    {1, 1, 1, &phy_fun_v0}, /*hw i2c controller1*/
    {2, 2, 1, &phy_fun_v0}, /*hw i2c controller2*/
    {3, 3, 1, &phy_fun_v0}, /*hw i2c controller3*/
    {4, 4, 1, &phy_fun_v0}, /*hw i2c controller4*/
    {5, 5, 1, &phy_fun_v0}, /*hw i2c controller5*/
    //{4, 1, 1, &phy_fun_v1} /*sw simulation i2c controller1, port 1*/
};

const I2C_BASE_INFO __i2c_base_info[I2C_BUS_MAX] =
{
    /*hw controller id, base address*/
    {0, 0x70500000},/*hw i2c controller0, register base*/
    {1, 0x70600000},/*hw i2c controller1, register base*/
    {2, 0x70700000},/*hw i2c controller2, register base*/
    {3, 0x70800000},/*hw i2c controller3, register base*/
    {4, 0x70900000},/*hw i2c controller4, register base*/
    {5, 0x40080000},/*hw i2c controller5, register base*/

	//{1, 0} /*sw i2c controller1, no register base*/
};

//const I2C_GPIO_INFO __i2c_gpio_info[I2C_BUS_MAX] =
//{
    /*sw controller id, sda pin, scl pin*/
    //{0, 0, 0},/*hw i2c controller1, no gpio pin*/
    //{1, 1, 2} /*sw simulation i2c controller1, gpio pin, should be update*/
//};

/**---------------------------------------------------------------------------*
 **                      Function  Definitions
 **---------------------------------------------------------------------------*/


/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif

/*  End Of File */
