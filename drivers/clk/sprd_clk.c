/*
 * drivers/clk/clkdev.c
 *
 *  Copyright (C) 2008 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Helper for the clk API to assist looking up a struct clk.
 */
#ifdef CONFIG_NKERNEL
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#else
#include <ubi_uboot.h>
#include <asm/arch/clock.h>
#endif
static LIST_HEAD(clocks);
static DEFINE_MUTEX(clocks_mutex);

/*
 * Find the correct struct clk for the device and connection ID.
 * We do slightly fuzzy matching here:
 *  An entry with a NULL ID is assumed to be a wildcard.
 *  If an entry has a device ID, it must match
 *  If an entry has a connection ID, it must match
 * Then we take the most specific entry - with the following
 * order of precedence: dev+con > dev only > con only.
 */
static struct clk_lookup *clk_find(const char *dev_id, const char *con_id)
{
	struct clk_lookup *p, *cl = NULL;
	int match, best = 0;

	list_for_each_entry(p, &clocks, node) {
		match = 0;
		if (p->dev_id) {
			if (!dev_id || strcmp(p->dev_id, dev_id))
				continue;
			match += 2;
		}
		if (p->con_id) {
			if (!con_id || strcmp(p->con_id, con_id))
				continue;
			match += 1;
		}

		if (match > best) {
			cl = p;
			if (match != 3)
				best = match;
			else
				break;
		}
	}
	return cl;
}

struct clk *clk_get_sys(const char *dev_id, const char *con_id)
{
	struct clk_lookup *cl;

	mutex_lock(&clocks_mutex);
	cl = clk_find(dev_id, con_id);
	//if (cl && !__clk_get(cl->clk))
	//	cl = NULL;
	mutex_unlock(&clocks_mutex);

	return cl ? cl->clk : ERR_PTR(-ENOENT);
}
EXPORT_SYMBOL(clk_get_sys);

struct clk *clk_get(struct device *dev, const char *con_id)
{
	//const char *dev_id = dev ? dev_name(dev) : NULL;

	return clk_get_sys(NULL, con_id);
}
EXPORT_SYMBOL(clk_get);

void clk_put(struct clk *clk)
{
//	__clk_put(clk);
}
EXPORT_SYMBOL(clk_put);

void clkdev_add(struct clk_lookup *cl)
{
	mutex_lock(&clocks_mutex);
	list_add_tail(&cl->node, &clocks);
	mutex_unlock(&clocks_mutex);
}
EXPORT_SYMBOL(clkdev_add);

