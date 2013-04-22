/*
 * This file contains miscellaneous helper functions.
 */

#ifndef __MISC_API_H__
#define __MISC_API_H__

/*D-Die chip id define*/
#define CHIP_ID_AA 0x7710DA00
#define CHIP_ID_AB 0x7710DA01

/*A-Die chip id define*/
#define ANA_CHIP_ID_AA	0x7710A000
#define ANA_CHIP_ID_BA	0x7710A001


unsigned int CHIP_PHY_InitChipID(void);

unsigned int CHIP_PHY_GetChipID(void);

unsigned int CHIP_PHY_GetANAChipID(void);

#endif
