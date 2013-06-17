/****************************************************
*	Author: dayong.yang
*	Last modified: 2013-04-02 23:10
*	Filename: special_loading.h
*	Description: 
****************************************************/
#ifndef __SPECIAL_LOADING_H__
#define __SPECIAL_LOADING_H__

/****************************************************
*	description: In the second part of OTA process,
*				we need to try updating 'spl.bin' in
*				case there is a need to update it.
****************************************************/
int try_update_spl(void);

/****************************************************
*	description: This function designed to deal with
*				NV partion's update and loading logic.
****************************************************/
int try_load_fixnv(void);

/****************************************************
*	description: loading runtime nv partition 
****************************************************/
int try_load_runtimenv(void);

/****************************************************
*	description: loading productinfo partition 
****************************************************/
int try_load_productinfo(void);

/****************************************************
*	description: get uart log flag 
****************************************************/
int is_uart_log_off(void);
#endif
