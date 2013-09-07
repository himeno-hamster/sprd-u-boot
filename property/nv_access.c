#include "normal_mode.h"
#include "special_loading.h"

#define DEBUG_ON 1
//IMEI 0x5,0x179,0x186,0x1e4,
#define FIXNV_IMEI1_ID        0x5
#define FIXNV_IMEI2_ID        0x179
#define FIXNV_IMEI3_ID        0x186
#define FIXNV_IMEI4_ID        0x1e4
//blue tooth 0x191
#define FIXNV_BT_ID        0x191
//WIFI 0x199
#define FIXNV_WIFI_ID        0x199

#define AP_NV_ACCESS_IGNOR  0xA0
#define AP_NV_ACCESS_FAIL   0xA1
#define AP_NV_ACCESS_WRITE_SUCESS 0xA2
#define AP_NV_ACCESS_READ_SUCESS 0xA3
typedef struct _nv_access{
    unsigned char imei1[8];
    unsigned char imei2[8];
    unsigned char bt_addr[6];
    unsigned char engine_sn[24];
    unsigned char map_version[4];
    unsigned char activate_code[16];
    unsigned char wifi_addr[6];
    unsigned char res1[2];
    unsigned char imei3[8];
    unsigned char imei4[8];
    unsigned char res2[16];
}nv_access_t;

int nv_access_write(unsigned short sub_cmd,unsigned char * buf){
    int i = 0;
    int ret = 0,need_flush = 0;
    unsigned short cmd = sub_cmd&0x7f;

#ifdef DEBUG_ON
    printf("buf[]=");
    for(i=0;i<sizeof(nv_access_t);i++){
        if(i%16 == 0){
            printf("\n");
        }
        printf("0x%02x ",buf[i]);
    }
    printf("\n");
#endif
    nv_access_t* pNv = (nv_access_t*)buf;
    if(cmd){
#ifdef DEBUG_ON
        printf("loaded fixnv to memory ....\n");
#endif
        //maksure FIXNV_ADR keeped the latest fixnv.bin
        try_load_fixnv();
    }else{
#ifdef DEBUG_ON
        printf("cmd = 0x%02x\n",cmd);
#endif
        return AP_NV_ACCESS_IGNOR;
    }

    if(cmd&0x1){
        //imei1
#ifdef DEBUG_ON
        printf("trying write imei1 ...\n");
        printf("imei1 = \n");
        for(i=0;i<8;i++){
            printf("0x%02x ",pNv->imei1[i]);
        }
        printf("\n");
#endif
        ret = update_fixnv_by_id(FIXNV_ADR,FIXNV_IMEI1_ID,pNv->imei1,0,8,8);
        if(ret == -1)
            goto ERROR;
#ifdef DEBUG_ON
        printf("write imei1 success!!!\n");
#endif
        need_flush=1;
    }
    if(cmd&0x2){
        //imei2
#ifdef DEBUG_ON
        printf("trying write imei2 ...\n");
        printf("imei2 = \n");
        for(i=0;i<8;i++){
            printf("0x%02x ",pNv->imei2[i]);
        }
        printf("\n");
#endif
        ret = update_fixnv_by_id(FIXNV_ADR,FIXNV_IMEI2_ID,pNv->imei2,0,8,8);
        if(ret == -1)
            goto ERROR;
#ifdef DEBUG_ON
        printf("write imei2 success!!!\n");
#endif
        need_flush=1;
    }
    if(cmd&0x4){
        //bt addr
#ifdef DEBUG_ON
        printf("trying write bt addr ...\n");
        printf("bt addr = \n");
        for(i=0;i<6;i++){
            printf("0x%02x ",pNv->bt_addr[i]);
        }
        printf("\n");
#endif
        ret = update_fixnv_by_id(FIXNV_ADR,FIXNV_BT_ID,pNv->bt_addr,0,6,8);
        if(ret == -1)
            goto ERROR;
#ifdef DEBUG_ON
        printf("write bt addr success!!!\n");
#endif
        need_flush=1;
    }
    if(cmd&0x8){
        //GPS
        goto ERROR;
    }
    if(cmd&0x10){
        //imei3
#ifdef DEBUG_ON
        printf("trying write imei3 ...\n");
        printf("imei3 = \n");
        for(i=0;i<8;i++){
            printf("0x%02x ",pNv->imei3[i]);
        }
        printf("\n");
#endif
        ret = update_fixnv_by_id(FIXNV_ADR,FIXNV_IMEI3_ID,pNv->imei3,0,8,8);
        if(ret == -1)
            goto ERROR;
#ifdef DEBUG_ON
        printf("write imei3 success!!!\n");
#endif
        need_flush=1;
    }
    if(cmd&0x20){
        //imei4
#ifdef DEBUG_ON
        printf("trying write imei4 ...\n");
        printf("imei4 = \n");
        for(i=0;i<8;i++){
            printf("0x%02x ",pNv->imei4[i]);
        }
        printf("\n");
#endif
        ret = update_fixnv_by_id(FIXNV_ADR,FIXNV_IMEI4_ID,pNv->imei4,0,8,8);
        if(ret == -1)
            goto ERROR;
#ifdef DEBUG_ON
        printf("write imei4 success!!!\n");
#endif
        need_flush=1;
    }
    if(cmd&0x40){
        //wifi addr
#ifdef DEBUG_ON
        printf("trying write wifi addr ...\n");
        printf("wifi addr = \n");
        for(i=0;i<6;i++){
            printf("0x%02x ",pNv->wifi_addr[i]);
        }
        printf("\n");
#endif
        ret = update_fixnv_by_id(FIXNV_ADR,FIXNV_WIFI_ID,pNv->wifi_addr,0,6,20);
        if(ret == -1)
            goto ERROR;
#ifdef DEBUG_ON
        printf("write wifi addr success!!!\n");
#endif
        need_flush=1;
    }

    if(need_flush)
        ret = update_fixnv_by_id_flush(FIXNV_ADR);
    if(ret == 1)
        return AP_NV_ACCESS_WRITE_SUCESS;
ERROR:
#ifdef DEBUG_ON
    printf("fatal error,please check!!!\n");
#endif
    return AP_NV_ACCESS_FAIL;
}
