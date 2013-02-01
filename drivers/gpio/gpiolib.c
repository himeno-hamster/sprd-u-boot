#include <common.h>
//#include <linux/err.h>
#include <asm/arch/gpio.h>
#include <linux/gpio.h>
#include <ubi_uboot.h>
#include <asm/bitops.h>

/* Optional implementation infrastructure for GPIO interfaces.
 *
 * Platforms may want to use this if they tend to use very many GPIOs
 * that aren't part of a System-On-Chip core; or across I2C/SPI/etc.
 *
 * When kernel footprint or instruction count is an issue, simpler
 * implementations may be preferred.  The GPIO programming interface
 * allows for inlining speed-critical get/set operations for common
 * cases, so that access to SOC-integrated GPIOs can sometimes cost
 * only an instruction or two per bit.
 */


/* When debugging, extend minimal trust to callers and platform code.
 * Also emit diagnostic messages that may help initial bringup, when
 * board setup or driver bugs are most common.
 *
 * Otherwise, minimize overhead in what may be bitbanging codepaths.
 */
#ifdef	DEBUG
#define	extra_checks	1
#else
#define	extra_checks	0
#endif

/* gpio_lock prevents conflicts during gpio_desc[] table updates.
 * While any GPIO is requested, its gpio_chip is not removable;
 * each GPIO's "requested" flag serves as a lock and refcount.
 */
static DEFINE_SPINLOCK(gpio_lock);

struct gpio_desc {
	struct gpio_chip	*chip;
	unsigned long		flags;
/* flag symbols are bit numbers */
#define FLAG_REQUESTED	0
#define FLAG_IS_OUT	1
#define FLAG_RESERVED	2
#define FLAG_EXPORT	3	/* protected by sysfs_lock */
#define FLAG_SYSFS	4	/* exported via /sys/class/gpio/control */
#define FLAG_TRIG_FALL	5	/* trigger on falling edge */
#define FLAG_TRIG_RISE	6	/* trigger on rising edge */
#define FLAG_ACTIVE_LOW	7	/* sysfs value has active low */
#define FLAG_OPEN_DRAIN	8	/* Gpio is open drain type */
#define FLAG_OPEN_SOURCE 9	/* Gpio is open source type */

#define ID_SHIFT	16	/* add new flags before this one */

#define GPIO_FLAGS_MASK		((1 << ID_SHIFT) - 1)
#define GPIO_TRIGGER_MASK	(BIT(FLAG_TRIG_FALL) | BIT(FLAG_TRIG_RISE))

#ifdef CONFIG_DEBUG_FS
	const char		*label;
#endif
};
static struct gpio_desc gpio_desc[ARCH_NR_GPIOS];

#ifdef CONFIG_GPIO_SYSFS
static DEFINE_IDR(dirent_idr);
#endif

static inline void desc_set_label(struct gpio_desc *d, const char *label)
{
#ifdef CONFIG_DEBUG_FS
	d->label = label;
#endif
}

/* Warn when drivers omit gpio_request() calls -- legal but ill-advised
 * when setting direction, and otherwise illegal.  Until board setup code
 * and drivers use explicit requests everywhere (which won't happen when
 * those calls have no teeth) we can't avoid autorequesting.  This nag
 * message should motivate switching to explicit requests... so should
 * the weaker cleanup after faults, compared to gpio_request().
 *
 * NOTE: the autorequest mechanism is going away; at this point it's
 * only "legal" in the sense that (old) code using it won't break yet,
 * but instead only triggers a WARN() stack dump.
 */
static int gpio_ensure_requested(struct gpio_desc *desc, unsigned offset)
{
	const struct gpio_chip *chip = desc->chip;
	const int gpio = chip->base + offset;

	if (WARN(test_and_set_bit(FLAG_REQUESTED, &desc->flags) == 0,
			"autorequest GPIO-%d\n", gpio)) {
		if (!try_module_get(chip->owner)) {
			pr_err("GPIO-%d: module can't be gotten \n", gpio);
			clear_bit(FLAG_REQUESTED, &desc->flags);
			/* lose */
			return -EIO;
		}
		desc_set_label(desc, "[auto]");
		/* caller must chip->request() w/o spinlock */
		if (chip->request)
			return 1;
	}
	return 0;
}

/* caller holds gpio_lock *OR* gpio is marked as requested */
struct gpio_chip *gpio_to_chip(unsigned gpio)
{
	return gpio_desc[gpio].chip;
}

/* dynamic allocation of GPIOs, e.g. on a hotplugged device */
static int gpiochip_find_base(int ngpio)
{
	int i;
	int spare = 0;
	int base = -ENOSPC;

	for (i = ARCH_NR_GPIOS - 1; i >= 0 ; i--) {
		struct gpio_desc *desc = &gpio_desc[i];
		struct gpio_chip *chip = desc->chip;

		if (!chip && !test_bit(FLAG_RESERVED, &desc->flags)) {
			spare++;
			if (spare == ngpio) {
				base = i;
				break;
			}
		} else {
			spare = 0;
			if (chip)
				i -= chip->ngpio - 1;
		}
	}

	if (gpio_is_valid(base))
		pr_debug("%s: found new base at %d\n", __func__, base);
	return base;
}

/**
 * gpiochip_reserve() - reserve range of gpios to use with platform code only
 * @start: starting gpio number
 * @ngpio: number of gpios to reserve
 * Context: platform init, potentially before irqs or kmalloc will work
 *
 * Returns a negative errno if any gpio within the range is already reserved
 * or registered, else returns zero as a success code.  Use this function
 * to mark a range of gpios as unavailable for dynamic gpio number allocation,
 * for example because its driver support is not yet loaded.
 */
int __init gpiochip_reserve(int start, int ngpio)
{
	int ret = 0;
	unsigned long flags;
	int i;

	if (!gpio_is_valid(start) || !gpio_is_valid(start + ngpio - 1))
		return -EINVAL;

	spin_lock_irqsave(&gpio_lock, flags);

	for (i = start; i < start + ngpio; i++) {
		struct gpio_desc *desc = &gpio_desc[i];

		if (desc->chip || test_bit(FLAG_RESERVED, &desc->flags)) {
			ret = -EBUSY;
			goto err;
		}

		set_bit(FLAG_RESERVED, &desc->flags);
	}

	pr_debug("%s: reserved gpios from %d to %d\n",
		 __func__, start, start + ngpio - 1);
err:
	spin_unlock_irqrestore(&gpio_lock, flags);

	return ret;
}

/**
 * gpiochip_add() - register a gpio_chip
 * @chip: the chip to register, with chip->base initialized
 * Context: potentially before irqs or kmalloc will work
 *
 * Returns a negative errno if the chip can't be registered, such as
 * because the chip->base is invalid or already associated with a
 * different chip.  Otherwise it returns zero as a success code.
 *
 * When gpiochip_add() is called very early during boot, so that GPIOs
 * can be freely used, the chip->dev device must be registered before
 * the gpio framework's arch_initcall().  Otherwise sysfs initialization
 * for GPIOs will fail rudely.
 *
 * If chip->base is negative, this requests dynamic assignment of
 * a range of valid GPIOs.
 */
int gpiochip_add(struct gpio_chip *chip)
{
	unsigned long	flags;
	int		status = 0;
	unsigned	id;
	int		base = chip->base;

	if ((!gpio_is_valid(base) || !gpio_is_valid(base + chip->ngpio - 1))
			&& base >= 0) {
		status = -EINVAL;
		goto fail;
	}

	spin_lock_irqsave(&gpio_lock, flags);

	if (base < 0) {
		base = gpiochip_find_base(chip->ngpio);
		if (base < 0) {
			status = base;
			goto unlock;
		}
		chip->base = base;
	}

	/* these GPIO numbers must not be managed by another gpio_chip */
	for (id = base; id < base + chip->ngpio; id++) {
		if (gpio_desc[id].chip != NULL) {
			status = -EBUSY;
			break;
		}
	}
	if (status == 0) {
		for (id = base; id < base + chip->ngpio; id++) {
			gpio_desc[id].chip = chip;

			/* REVISIT:  most hardware initializes GPIOs as
			 * inputs (often with pullups enabled) so power
			 * usage is minimized.  Linux code should set the
			 * gpio direction first thing; but until it does,
			 * we may expose the wrong direction in sysfs.
			 */
			gpio_desc[id].flags = !chip->direction_input
				? (1 << FLAG_IS_OUT)
				: 0;
		}
	}

unlock:
	spin_unlock_irqrestore(&gpio_lock, flags);

	if (status)
		goto fail;

	pr_info("gpiochip_add: registered GPIOs %d to %d on device: %s\n",
		chip->base, chip->base + chip->ngpio - 1,
		chip->label ? : "generic");

	return 0;
fail:
	/* failures here can mean systems won't boot... */
	pr_err("gpiochip_add: gpios %d..%d (%s) failed to register\n",
		chip->base, chip->base + chip->ngpio - 1,
		chip->label ? : "generic");
	return status;
}
EXPORT_SYMBOL_GPL(gpiochip_add);

/**
 * gpiochip_remove() - unregister a gpio_chip
 * @chip: the chip to unregister
 *
 * A gpio_chip with any GPIOs still requested may not be removed.
 */
int gpiochip_remove(struct gpio_chip *chip)
{
	unsigned long	flags;
	int		status = 0;
	unsigned	id;

	spin_lock_irqsave(&gpio_lock, flags);

	for (id = chip->base; id < chip->base + chip->ngpio; id++) {
		if (test_bit(FLAG_REQUESTED, &gpio_desc[id].flags)) {
			status = -EBUSY;
			break;
		}
	}
	if (status == 0) {
		for (id = chip->base; id < chip->base + chip->ngpio; id++)
			gpio_desc[id].chip = NULL;
	}

	spin_unlock_irqrestore(&gpio_lock, flags);

	return status;
}
EXPORT_SYMBOL_GPL(gpiochip_remove);

/**
 * gpiochip_find() - iterator for locating a specific gpio_chip
 * @data: data to pass to match function
 * @callback: Callback function to check gpio_chip
 *
 * Similar to bus_find_device.  It returns a reference to a gpio_chip as
 * determined by a user supplied @match callback.  The callback should return
 * 0 if the device doesn't match and non-zero if it does.  If the callback is
 * non-zero, this function will return to the caller and not iterate over any
 * more gpio_chips.
 */
struct gpio_chip *gpiochip_find(const void *data,
				int (*match)(struct gpio_chip *chip,
					     const void *data))
{
	struct gpio_chip *chip = NULL;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&gpio_lock, flags);
	for (i = 0; i < ARCH_NR_GPIOS; i++) {
		if (!gpio_desc[i].chip)
			continue;

		if (match(gpio_desc[i].chip, data)) {
			chip = gpio_desc[i].chip;
			break;
		}
	}
	spin_unlock_irqrestore(&gpio_lock, flags);

	return chip;
}
EXPORT_SYMBOL_GPL(gpiochip_find);

/* These "optional" allocation calls help prevent drivers from stomping
 * on each other, and help provide better diagnostics in debugfs.
 * They're called even less than the "set direction" calls.
 */
int gpio_request(unsigned gpio, const char *label)
{
	struct gpio_desc	*desc;
	struct gpio_chip	*chip;
	int			status = -EINVAL;
	unsigned long		flags;

	spin_lock_irqsave(&gpio_lock, flags);

	if (!gpio_is_valid(gpio))
		goto done;
	desc = &gpio_desc[gpio];
	chip = desc->chip;
	if (chip == NULL)
		goto done;

	if (!try_module_get(chip->owner))
		goto done;

	/* NOTE:  gpio_request() can be called in early boot,
	 * before IRQs are enabled, for non-sleeping (SOC) GPIOs.
	 */

	if (test_and_set_bit(FLAG_REQUESTED, &desc->flags) == 0) {
		desc_set_label(desc, label ? : "?");
		status = 0;
	} else {
		status = -EBUSY;
		module_put(chip->owner);
		goto done;
	}

	if (chip->request) {
		/* chip->request may sleep */
		spin_unlock_irqrestore(&gpio_lock, flags);
		status = chip->request(chip, gpio - chip->base);
		spin_lock_irqsave(&gpio_lock, flags);

		if (status < 0) {
			desc_set_label(desc, NULL);
			module_put(chip->owner);
			clear_bit(FLAG_REQUESTED, &desc->flags);
		}
	}

done:
	if (status)
		pr_debug("gpio_request: gpio-%d (%s) status %d\n",
			gpio, label ? : "?", status);
	spin_unlock_irqrestore(&gpio_lock, flags);
	return status;
}
EXPORT_SYMBOL_GPL(gpio_request);

void gpio_free(unsigned gpio)
{
	unsigned long		flags;
	struct gpio_desc	*desc;
	struct gpio_chip	*chip;

	if (!gpio_is_valid(gpio)) {
		WARN_ON(extra_checks);
		return;
	}


	spin_lock_irqsave(&gpio_lock, flags);

	desc = &gpio_desc[gpio];
	chip = desc->chip;
	if (chip && test_bit(FLAG_REQUESTED, &desc->flags)) {
		if (chip->free) {
			spin_unlock_irqrestore(&gpio_lock, flags);
			might_sleep_if(chip->can_sleep);
			chip->free(chip, gpio - chip->base);
			spin_lock_irqsave(&gpio_lock, flags);
		}
		desc_set_label(desc, NULL);
		module_put(desc->chip->owner);
		clear_bit(FLAG_ACTIVE_LOW, &desc->flags);
		clear_bit(FLAG_REQUESTED, &desc->flags);
		clear_bit(FLAG_OPEN_DRAIN, &desc->flags);
		clear_bit(FLAG_OPEN_SOURCE, &desc->flags);
	} else
		WARN_ON(extra_checks);

	spin_unlock_irqrestore(&gpio_lock, flags);
}
EXPORT_SYMBOL_GPL(gpio_free);

/**
 * gpio_request_one - request a single GPIO with initial configuration
 * @gpio:	the GPIO number
 * @flags:	GPIO configuration as specified by GPIOF_*
 * @label:	a literal description string of this GPIO
 */
int gpio_request_one(unsigned gpio, unsigned long flags, const char *label)
{
	int err;

	err = gpio_request(gpio, label);
	if (err)
		return err;

	if (flags & GPIOF_OPEN_DRAIN)
		set_bit(FLAG_OPEN_DRAIN, &gpio_desc[gpio].flags);

	if (flags & GPIOF_OPEN_SOURCE)
		set_bit(FLAG_OPEN_SOURCE, &gpio_desc[gpio].flags);

	if (flags & GPIOF_DIR_IN)
		err = gpio_direction_input(gpio);
	else
		err = gpio_direction_output(gpio,
				(flags & GPIOF_INIT_HIGH) ? 1 : 0);

	if (err)
		gpio_free(gpio);

	return err;
}
EXPORT_SYMBOL_GPL(gpio_request_one);

/**
 * gpio_request_array - request multiple GPIOs in a single call
 * @array:	array of the 'struct gpio'
 * @num:	how many GPIOs in the array
 */
int gpio_request_array(const struct gpio *array, size_t num)
{
	int i, err;

	for (i = 0; i < num; i++, array++) {
		err = gpio_request_one(array->gpio, array->flags, array->label);
		if (err)
			goto err_free;
	}
	return 0;

err_free:
	while (i--)
		gpio_free((--array)->gpio);
	return err;
}
EXPORT_SYMBOL_GPL(gpio_request_array);

/**
 * gpio_free_array - release multiple GPIOs in a single call
 * @array:	array of the 'struct gpio'
 * @num:	how many GPIOs in the array
 */
void gpio_free_array(const struct gpio *array, size_t num)
{
	while (num--)
		gpio_free((array++)->gpio);
}
EXPORT_SYMBOL_GPL(gpio_free_array);

/**
 * gpiochip_is_requested - return string iff signal was requested
 * @chip: controller managing the signal
 * @offset: of signal within controller's 0..(ngpio - 1) range
 *
 * Returns NULL if the GPIO is not currently requested, else a string.
 * If debugfs support is enabled, the string returned is the label passed
 * to gpio_request(); otherwise it is a meaningless constant.
 *
 * This function is for use by GPIO controller drivers.  The label can
 * help with diagnostics, and knowing that the signal is used as a GPIO
 * can help avoid accidentally multiplexing it to another controller.
 */
const char *gpiochip_is_requested(struct gpio_chip *chip, unsigned offset)
{
	unsigned gpio = chip->base + offset;

	if (!gpio_is_valid(gpio) || gpio_desc[gpio].chip != chip)
		return NULL;
	if (test_bit(FLAG_REQUESTED, &gpio_desc[gpio].flags) == 0)
		return NULL;
#ifdef CONFIG_DEBUG_FS
	return gpio_desc[gpio].label;
#else
	return "?";
#endif
}
EXPORT_SYMBOL_GPL(gpiochip_is_requested);


/* Drivers MUST set GPIO direction before making get/set calls.  In
 * some cases this is done in early boot, before IRQs are enabled.
 *
 * As a rule these aren't called more than once (except for drivers
 * using the open-drain emulation idiom) so these are natural places
 * to accumulate extra debugging checks.  Note that we can't (yet)
 * rely on gpio_request() having been called beforehand.
 */

int gpio_direction_input(unsigned gpio)
{
	unsigned long		flags;
	struct gpio_chip	*chip;
	struct gpio_desc	*desc = &gpio_desc[gpio];
	int			status = -EINVAL;

	spin_lock_irqsave(&gpio_lock, flags);

	if (!gpio_is_valid(gpio))
		goto fail;
	chip = desc->chip;
	if (!chip || !chip->get || !chip->direction_input)
		goto fail;
	gpio -= chip->base;
	if (gpio >= chip->ngpio)
		goto fail;
	status = gpio_ensure_requested(desc, gpio);
	if (status < 0)
		goto fail;

	/* now we know the gpio is valid and chip won't vanish */

	spin_unlock_irqrestore(&gpio_lock, flags);

	might_sleep_if(chip->can_sleep);

	if (status) {
		status = chip->request(chip, gpio);
		if (status < 0) {
			pr_debug("GPIO-%d: chip request fail, %d\n",
				chip->base + gpio, status);
			/* and it's not available to anyone else ...
			 * gpio_request() is the fully clean solution.
			 */
			goto lose;
		}
	}

	status = chip->direction_input(chip, gpio);
	if (status == 0)
		clear_bit(FLAG_IS_OUT, &desc->flags);

	trace_gpio_direction(chip->base + gpio, 1, status);
lose:
	return status;
fail:
	spin_unlock_irqrestore(&gpio_lock, flags);
	if (status)
		pr_debug("%s: gpio-%d status %d\n",
			__func__, gpio, status);
	return status;
}
EXPORT_SYMBOL_GPL(gpio_direction_input);

int gpio_direction_output(unsigned gpio, int value)
{
	unsigned long		flags;
	struct gpio_chip	*chip;
	struct gpio_desc	*desc = &gpio_desc[gpio];
	int			status = -EINVAL;

	/* Open drain pin should not be driven to 1 */
	if (value && test_bit(FLAG_OPEN_DRAIN,  &desc->flags))
		return gpio_direction_input(gpio);

	/* Open source pin should not be driven to 0 */
	if (!value && test_bit(FLAG_OPEN_SOURCE,  &desc->flags))
		return gpio_direction_input(gpio);

	spin_lock_irqsave(&gpio_lock, flags);

	if (!gpio_is_valid(gpio))
		goto fail;
	chip = desc->chip;
	if (!chip || !chip->set || !chip->direction_output)
		goto fail;
	gpio -= chip->base;
	if (gpio >= chip->ngpio)
		goto fail;
	status = gpio_ensure_requested(desc, gpio);
	if (status < 0)
		goto fail;

	/* now we know the gpio is valid and chip won't vanish */

	spin_unlock_irqrestore(&gpio_lock, flags);

	might_sleep_if(chip->can_sleep);

	if (status) {
		status = chip->request(chip, gpio);
		if (status < 0) {
			pr_debug("GPIO-%d: chip request fail, %d\n",
				chip->base + gpio, status);
			/* and it's not available to anyone else ...
			 * gpio_request() is the fully clean solution.
			 */
			goto lose;
		}
	}

	status = chip->direction_output(chip, gpio, value);
	if (status == 0)
		set_bit(FLAG_IS_OUT, &desc->flags);
lose:
	return status;
fail:
	spin_unlock_irqrestore(&gpio_lock, flags);
	if (status)
		pr_debug("%s: gpio-%d status %d\n",
			__func__, gpio, status);
	return status;
}
EXPORT_SYMBOL_GPL(gpio_direction_output);

/**
 * gpio_set_debounce - sets @debounce time for a @gpio
 * @gpio: the gpio to set debounce time
 * @debounce: debounce time is microseconds
 */
int gpio_set_debounce(unsigned gpio, unsigned debounce)
{
	unsigned long		flags;
	struct gpio_chip	*chip;
	struct gpio_desc	*desc = &gpio_desc[gpio];
	int			status = -EINVAL;

	spin_lock_irqsave(&gpio_lock, flags);

	if (!gpio_is_valid(gpio))
		goto fail;
	chip = desc->chip;
	if (!chip || !chip->set || !chip->set_debounce)
		goto fail;
	gpio -= chip->base;
	if (gpio >= chip->ngpio)
		goto fail;
	status = gpio_ensure_requested(desc, gpio);
	if (status < 0)
		goto fail;

	/* now we know the gpio is valid and chip won't vanish */

	spin_unlock_irqrestore(&gpio_lock, flags);

	might_sleep_if(chip->can_sleep);

	return chip->set_debounce(chip, gpio, debounce);

fail:
	spin_unlock_irqrestore(&gpio_lock, flags);
	if (status)
		pr_debug("%s: gpio-%d status %d\n",
			__func__, gpio, status);

	return status;
}
EXPORT_SYMBOL_GPL(gpio_set_debounce);

/* I/O calls are only valid after configuration completed; the relevant
 * "is this a valid GPIO" error checks should already have been done.
 *
 * "Get" operations are often inlinable as reading a pin value register,
 * and masking the relevant bit in that register.
 *
 * When "set" operations are inlinable, they involve writing that mask to
 * one register to set a low value, or a different register to set it high.
 * Otherwise locking is needed, so there may be little value to inlining.
 *
 *------------------------------------------------------------------------
 *
 * IMPORTANT!!!  The hot paths -- get/set value -- assume that callers
 * have requested the GPIO.  That can include implicit requesting by
 * a direction setting call.  Marking a gpio as requested locks its chip
 * in memory, guaranteeing that these table lookups need no more locking
 * and that gpiochip_remove() will fail.
 *
 * REVISIT when debugging, consider adding some instrumentation to ensure
 * that the GPIO was actually requested.
 */

/**
 * __gpio_get_value() - return a gpio's value
 * @gpio: gpio whose value will be returned
 * Context: any
 *
 * This is used directly or indirectly to implement gpio_get_value().
 * It returns the zero or nonzero value provided by the associated
 * gpio_chip.get() method; or zero if no such method is provided.
 */
int __gpio_get_value(unsigned gpio)
{
	struct gpio_chip	*chip;
	int value;

	chip = gpio_to_chip(gpio);
	/* Should be using gpio_get_value_cansleep() */
	WARN_ON(chip->can_sleep);
	value = chip->get ? chip->get(chip, gpio - chip->base) : 0;
	return value;
}
EXPORT_SYMBOL_GPL(__gpio_get_value);

/*
 *  _gpio_set_open_drain_value() - Set the open drain gpio's value.
 * @gpio: Gpio whose state need to be set.
 * @chip: Gpio chip.
 * @value: Non-zero for setting it HIGH otherise it will set to LOW.
 */
static void _gpio_set_open_drain_value(unsigned gpio,
			struct gpio_chip *chip, int value)
{
	int err = 0;
	if (value) {
		err = chip->direction_input(chip, gpio - chip->base);
		if (!err)
			clear_bit(FLAG_IS_OUT, &gpio_desc[gpio].flags);
	} else {
		err = chip->direction_output(chip, gpio - chip->base, 0);
		if (!err)
			set_bit(FLAG_IS_OUT, &gpio_desc[gpio].flags);
	}
	if (err < 0)
		pr_err("%s: Error in set_value for open drain gpio%d err %d\n",
					__func__, gpio, err);
}

/*
 *  _gpio_set_open_source() - Set the open source gpio's value.
 * @gpio: Gpio whose state need to be set.
 * @chip: Gpio chip.
 * @value: Non-zero for setting it HIGH otherise it will set to LOW.
 */
static void _gpio_set_open_source_value(unsigned gpio,
			struct gpio_chip *chip, int value)
{
	int err = 0;
	if (value) {
		err = chip->direction_output(chip, gpio - chip->base, 1);
		if (!err)
			set_bit(FLAG_IS_OUT, &gpio_desc[gpio].flags);
	} else {
		err = chip->direction_input(chip, gpio - chip->base);
		if (!err)
			clear_bit(FLAG_IS_OUT, &gpio_desc[gpio].flags);
	}
	if (err < 0)
		pr_err("%s: Error in set_value for open source gpio%d err %d\n",
					__func__, gpio, err);
}


/**
 * __gpio_set_value() - assign a gpio's value
 * @gpio: gpio whose value will be assigned
 * @value: value to assign
 * Context: any
 *
 * This is used directly or indirectly to implement gpio_set_value().
 * It invokes the associated gpio_chip.set() method.
 */
void __gpio_set_value(unsigned gpio, int value)
{
	struct gpio_chip	*chip;

	chip = gpio_to_chip(gpio);
	/* Should be using gpio_set_value_cansleep() */
	WARN_ON(chip->can_sleep);
	if (test_bit(FLAG_OPEN_DRAIN,  &gpio_desc[gpio].flags))
		_gpio_set_open_drain_value(gpio, chip, value);
	else if (test_bit(FLAG_OPEN_SOURCE,  &gpio_desc[gpio].flags))
		_gpio_set_open_source_value(gpio, chip, value);
	else
		chip->set(chip, gpio - chip->base, value);
}
EXPORT_SYMBOL_GPL(__gpio_set_value);

/**
 * __gpio_cansleep() - report whether gpio value access will sleep
 * @gpio: gpio in question
 * Context: any
 *
 * This is used directly or indirectly to implement gpio_cansleep().  It
 * returns nonzero if access reading or writing the GPIO value can sleep.
 */
int __gpio_cansleep(unsigned gpio)
{
	struct gpio_chip	*chip;

	/* only call this on GPIOs that are valid! */
	chip = gpio_to_chip(gpio);

	return chip->can_sleep;
}
EXPORT_SYMBOL_GPL(__gpio_cansleep);

/**
 * __gpio_to_irq() - return the IRQ corresponding to a GPIO
 * @gpio: gpio whose IRQ will be returned (already requested)
 * Context: any
 *
 * This is used directly or indirectly to implement gpio_to_irq().
 * It returns the number of the IRQ signaled by this (input) GPIO,
 * or a negative errno.
 */
int __gpio_to_irq(unsigned gpio)
{
	struct gpio_chip	*chip;

	chip = gpio_to_chip(gpio);
	return chip->to_irq ? chip->to_irq(chip, gpio - chip->base) : -ENXIO;
}
EXPORT_SYMBOL_GPL(__gpio_to_irq);



/* There's no value in making it easy to inline GPIO calls that may sleep.
 * Common examples include ones connected to I2C or SPI chips.
 */

int gpio_get_value_cansleep(unsigned gpio)
{
	struct gpio_chip	*chip;
	int value;

	might_sleep_if(extra_checks);
	chip = gpio_to_chip(gpio);
	value = chip->get ? chip->get(chip, gpio - chip->base) : 0;
	trace_gpio_value(gpio, 1, value);
	return value;
}
EXPORT_SYMBOL_GPL(gpio_get_value_cansleep);

void gpio_set_value_cansleep(unsigned gpio, int value)
{
	struct gpio_chip	*chip;

	might_sleep_if(extra_checks);
	chip = gpio_to_chip(gpio);
	trace_gpio_value(gpio, 0, value);
	if (test_bit(FLAG_OPEN_DRAIN,  &gpio_desc[gpio].flags))
		_gpio_set_open_drain_value(gpio, chip, value);
	else if (test_bit(FLAG_OPEN_SOURCE,  &gpio_desc[gpio].flags))
		_gpio_set_open_source_value(gpio, chip, value);
	else
		chip->set(chip, gpio - chip->base, value);
}
EXPORT_SYMBOL_GPL(gpio_set_value_cansleep);

