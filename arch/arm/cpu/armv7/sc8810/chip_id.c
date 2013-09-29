#include <asm/arch/sci_types.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch/regs_adi.h>
#include <asm/arch/regs_ana.h>
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/sc8810_reg_ahb.h>
#include <asm/arch/misc_api.h>

unsigned int CHIP_PHY_GetANAChipID(void)
{
#ifdef CONFIG_SC7710G2
	unsigned int reg_val;
        static unsigned int glb_ana_chipid = 0;

        if(glb_ana_chipid)
            return glb_ana_chipid;

	glb_ana_chipid = (ADI_Analogdie_reg_read(ANA_CHIP_ID_HIGH) << 16) |
					ADI_Analogdie_reg_read(ANA_CHIP_ID_LOW);

	if (glb_ana_chipid == ANA_CHIP_ID_BA) {
		/* enable audif register in A-die APB */
		reg_val = ANA_REG_GET(ANA_AUDIO_CTL);

		ANA_REG_OR(ANA_AUDIO_CTL, BIT_15 | BIT_0);

		if (!(ANA_REG_GET(VOICE_BAND_CODEC_BEGIN + 0xC0) & BIT_6)) {
			glb_ana_chipid = (unsigned int)ANA_CHIP_ID_BB;
		}

		ANA_REG_SET(ANA_AUDIO_CTL, reg_val);
	}

        return glb_ana_chipid;
#else
        return 0;
#endif
}
