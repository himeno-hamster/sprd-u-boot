/****************************************************
*	Author: dayong.yang
*	Last modified: 2013-04-03 14:05
*	Filename: special_downloading.h
*	Description: 
****************************************************/
#ifndef __SPECIAL_DOWNLOADING__
#define __SPECIAL_DOWNLOADING__




/****************************************************
*	description: util function used to read fixnv
*			gBuff -- a global addr used to temp store the whole file
*			offset -- from where to read
*			size -- how long to read
*			buf -- where to put the readed data
****************************************************/
int fdl_read_fixnv(unsigned char * gBuf,unsigned int offset, unsigned int size, unsigned char *buf);

/****************************************************
*	description: util function used to read productinfo
*			gBuff -- a global addr used to temp store the whole file
*			offset -- from where to read
*			size -- how long to read
*			buf -- where to put the readed data
****************************************************/
int fdl_read_productinfo(unsigned char * gBuf,unsigned int offset, unsigned int size, unsigned char *buf);

/****************************************************
*	description: This function used to download fixnv
*				file in research download process.
****************************************************/
int fdl_download_fixnv(unsigned char *gBuf,int is_fixnv);

/****************************************************
*	description: This function used to download productinfo
*			file in research download process.
****************************************************/
void fdl_download_productinfo(unsigned char *gBuf);
#endif
