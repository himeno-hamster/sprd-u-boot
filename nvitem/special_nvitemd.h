/****************************************************
*	Author: dayong.yang
*	Last modified: 2013-04-04 14:04
*	Filename: special_nvitemd.h
*	Description: 
****************************************************/
#ifndef __SPECIAL_NVITEMD_H__
#define __SPECIAL_NVITEMD_H__




/****************************************************
*	description: add crc16 to file ends 
****************************************************/
void nvitemd_add_crc16(unsigned char * buf,unsigned int size);

/****************************************************
*	description: add fixnv file length to the ends.
****************************************************/
void nvitemd_add_fixnv_len(unsigned char *buf,unsigned int size);

/****************************************************
*	description:check the file structure to identify 
*				whether it has been damaged or not.
****************************************************/
int file_check(unsigned char *array, unsigned long size);

/****************************************************
*	description: util function used to check the NV
*			length keept in file is wheather correct
*			or not.
****************************************************/
int check_fixnv_struct(unsigned int addr,unsigned int size);

#endif
