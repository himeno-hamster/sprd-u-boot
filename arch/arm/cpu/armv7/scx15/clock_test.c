#include <ubi_uboot.h>
#include <linux/compiler.h>
#include <asm/arch/clock.h>

extern const u32 __clkinit0 __clkinit_begin;
extern const u32 __clkinit2 __clkinit_end;

static unsigned int get_clock_rate(const char *clk_name)
{	
	unsigned int rate;
	struct clk *clk;

	clk =clk_get(0, clk_name);

	if (clk == NULL)
	{
		printf("%s can't find!\r\n");
		return 0;
	}

	if (clk->ops)
	{
		rate = clk->ops->get_rate(clk);
	}
	else
	{
		rate = clk->rate;
	}
	printf("%s : %d\r\n", clk_name, rate);
	return rate;
}

static unsigned int change_clock_rate(const *clk_name, unsigned int rate)
{
	struct clk *clk;

	clk =clk_get(0, clk_name);

	if (clk == NULL)
	{
		printf("%s can't find!\r\n");
		return -1;
	}

	if (clk->ops)
	{
		if (clk->ops->set_rate)
		{
			clk->ops->set_rate(clk, rate);
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
	printf("%s ==> %d\r\n", clk_name, rate);
	return 0;
}

void clock_test()
{
	struct clk *clk;
		
	sci_clock_init();

	printf("clock_test, __clkinit_begin:%08X (%08X), __clkinit_end:%08X (%08X)\r\n\r\n", 
		&__clkinit_begin, __clkinit_begin, &__clkinit_end, __clkinit_end);

	{
		struct clk_lookup *cl = (struct clk_lookup *)(&__clkinit_begin + 1);
		while (cl < (struct clk_lookup *)&__clkinit_end)
		{
			clk =clk_get(0, cl->con_id);
			if (clk != ERR_PTR(-2))
			{
				if (clk->ops)
				{
					printf("1.clk: %s, rate: %d\r\n", cl->con_id, clk->ops->get_rate(clk));
				}
				else
				{
					printf("2.clk: %s, rate: %d\r\n", cl->con_id, clk->rate);
				}
			}
			else
			{
				printf("get %s error!\r\n", cl->con_id);
			}
			cl++;
		}
	}
	printf("---------------------------------------\r\n");
	get_clock_rate("clk_arm");
	get_clock_rate("clk_axi");
	get_clock_rate("clk_ahb");
	get_clock_rate("clk_apb");
	
	get_clock_rate("clk_emmc");
	change_clock_rate("clk_emmc", 192000000);
	get_clock_rate("clk_emmc");
	
	get_clock_rate("clk_apb");
	change_clock_rate("clk_apb", 100000000);
	get_clock_rate("clk_apb");


	get_clock_rate("clk_emc");
	change_clock_rate("clk_emc", 100000000);
	get_clock_rate("clk_emc");

	get_clock_rate("clk_dbg");
	change_clock_rate("clk_dbg", 200000000);
	get_clock_rate("clk_dbg");

	printf("clock test end!\r\n");
}

