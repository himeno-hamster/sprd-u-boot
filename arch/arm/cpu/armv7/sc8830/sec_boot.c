#include <common.h>
#include <malloc.h>
#include <asm/arch/common.h>
#include <asm/arch/sprd_reg.h>
#include <asm/arch/chip_drv_common_io.h>

#ifdef CONFIG_NAND_SPL
#define panic(x...) do{}while(0)
#define printf(x...) do{}while(0)
#endif

#define SECURE_BOOT_ENABLE

typedef struct{
	struct{
		uint32_t e;
		uint8_t  m[128];
		uint8_t  r2[128];
	}key;
	uint8_t reserved[4];
}bsc_info_t;

#define VLR_MAGIC (0x524C56FF)
typedef struct {
	uint32_t magic;
	uint8_t  hash[128];
	uint32_t setting;
	uint32_t length;
	uint8_t  reserved[20];
}vlr_info_t;

typedef struct
{
	uint32_t intermediate_hash[5];
	uint32_t length_low;
	uint32_t length_high;
	uint32_t msg_block_idx;
	uint32_t W[80];
}sha1context_32;

typedef struct {
	uint32_t ver;
	uint8_t  cap;
	void (*efuse_init)(void);
	void (*efuse_close)(void);
	uint32_t (*efuse_read)(uint32_t block_id, uint32_t *data);
	uint32_t (*sha1reset_32)(sha1context_32*);
	uint32_t (*sha1input_32)(sha1context_32*, const uint32_t*, uint32_t);
	uint32_t (*sha1result_32)(sha1context_32*, uint8_t*);
	void (*rsa_modpower)(uint32_t *p, uint32_t *m, uint32_t *r2, uint32_t e);
}rom_callback_func_t;

rom_callback_func_t* get_rom_callback(void)
{
	rom_callback_func_t *rom_callback = NULL;
	rom_callback = (rom_callback_func_t *)(*((unsigned int*)0xFFFF0020));
	return rom_callback;
}

int secureboot_enabled(void)
{
#ifdef SECURE_BOOT_ENABLE
	uint32_t bonding = REG32(REG_AON_APB_BOND_OPT0);
	if (bonding & BIT_2)
		return 1;
#endif
	return 0;
}

#define MAKE_DWORD(a,b,c,d) (uint32_t)(((uint32_t)(a)<<24) | (uint32_t)(b)<<16 | ((uint32_t)(c)<<8) | ((uint32_t)(d)))

void RSA_Decrypt(unsigned char *p, unsigned char *m, unsigned char *r2, unsigned char *e)
{
	rom_callback_func_t *rom_callback = NULL;
	unsigned int _e = 0;
	unsigned int _m[32];
	unsigned int _p[32];
	unsigned int _r2[32];
	int i = 0;

	rom_callback = get_rom_callback();

	_e = MAKE_DWORD(e[0], e[1], e[2], e[3]);

	for(i=31; i>=0; i--)
	{
		_m[31-i] = MAKE_DWORD(m[4*i], m[4*i+1], m[4*i+2], m[4*i+3]);
		_p[31-i] = MAKE_DWORD(p[4*i], p[4*i+1], p[4*i+2], p[4*i+3]);
		_r2[31-i] = MAKE_DWORD(r2[4*i], r2[4*i+1], r2[4*i+2], r2[4*i+3]);
	}

	rom_callback->rsa_modpower(_p, _m, _r2, _e);

	for(i=31;i>=0;i--)
	{
		p[4*(31-i)] = (unsigned char)(_p[i]>>24);
		p[4*(31-i)+1] = (unsigned char)(_p[i]>>16);
		p[4*(31-i)+2] = (unsigned char)(_p[i]>>8);
		p[4*(31-i)+3] = (unsigned char)(_p[i]);
	}
}

int harshVerify(uint8_t *data, uint32_t data_len, uint8_t *data_hash, uint8_t *data_key)
{
	uint8_t              soft_hash_data[160];
	uint32_t             i, len;
	vlr_info_t*          vlr_info;
	bsc_info_t*          bsc_info;
	sha1context_32       sha;
	rom_callback_func_t* rom_callback;

	vlr_info     = (vlr_info_t*)data_hash;

	if (vlr_info->magic != VLR_MAGIC)
		return 0;

	bsc_info     = (bsc_info_t*)data_key;
	rom_callback = get_rom_callback();

	rom_callback->sha1reset_32(&sha);
	rom_callback->sha1input_32(&sha, (uint32_t*)data, data_len);
	rom_callback->sha1result_32(&sha, soft_hash_data);

	RSA_Decrypt(soft_hash_data, bsc_info->key.m, bsc_info->key.r2, (unsigned char*)(&bsc_info->key.e));
	len = sizeof(vlr_info->hash)>sizeof(soft_hash_data) ? sizeof(soft_hash_data) : sizeof(vlr_info->hash);
	for (i=0; i<len; i++)
	{
		printf("[%3d] : %02X, %02X . \r\n", i, soft_hash_data[i], vlr_info->hash[i]);
		if (soft_hash_data[i] != vlr_info->hash[i])
			return 0;
	}
	return 1;
}

void secure_check(uint8_t *data, uint32_t data_len, uint8_t *data_hash, uint8_t *data_key)
{
#ifdef SECURE_BOOT_ENABLE
	if (!secureboot_enabled())
		return;

	if(0 == harshVerify(data, data_len, data_hash, data_key))
	{
		while(1);
	}
#endif
}

#ifndef CONFIG_NAND_SPL
int cal_md5(void *data, uint32_t orig_len, void *harsh_data)
{
	return 0;
}
#endif

