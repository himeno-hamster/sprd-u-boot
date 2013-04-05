#include <config.h>
#include <common.h>
#include <linux/types.h>
#include <asm/arch/bits.h>
#include <asm/arch/migrate.h>
#include <asm/arch/chip_drv_config_extern.h>
#include <asm/arch/sio_drv.h>
#include <asm/arch/fdl_crc.h>
#include <asm/arch/packet.h>
#include <asm/arch/common.h>
#include <asm/arch/fdl_channel.h>

#define __REG(x)     (*((volatile u32 *)(x)))

#ifdef CONFIG_SERIAL_MULTI
#warning "SC8800X driver does not support MULTI serials."
#endif

#if defined(PLATFORM_SC6800H)
#define GR_CTRL_REG        0x8b000004
/* GEN0_UART0_EN    (0x1 << 1) */
/* GEN0_UART1_EN    (0x1 << 2) */
#define GR_UART_CTRL_EN    (0x3<<1)
#elif defined(CONFIG_SC8830)
#define GR_CTRL_REG        CTL_BASE_APB
#define GR_UART_CTRL_EN    (0x3 << 13 )
#elif defined(CONFIG_SC8825)
#define GR_CTRL_REG        0x4b000008
#define GR_UART_CTRL_EN    ((0x7 << 20 ) | (1))  /*UART0 UART1 UART2 UART3 enable*/
#elif defined(PLATFORM_SC8800G) || defined(CONFIG_SC8810)
#define GR_CTRL_REG        0x8b000004
/* GEN0_UART0_EN    (0x1 << 20) */
/* GEN0_UART1_EN    (0x1 << 21) */
#define GR_UART_CTRL_EN    (0x3 << 20 )
#else
#define GR_CTRL_REG        0x8b000018
#define GR_UART_CTRL_EN    0x00400000
#endif

#ifdef FPGA_VERIFICATION 
#define ARM_APB_CLK    48000000UL
#else
#define ARM_APB_CLK    26000000UL
#endif

#if defined(CONFIG_SC7710G2)
#define GR_CTRL_REG1        0x8b0000b4
#define GR_UART3_CTRL_EN    1

#define UART_RX_FLOW_EN   BIT_7
#define UART_TX_FLOW_EN   BIT_8

#define UART_RX_THRESHOLD   120

#endif
typedef struct UartPort
{
    unsigned int regBase;
    unsigned int baudRate;
} UartPort_T;

UartPort_T gUart0PortInfo =
{
    ARM_UART0_BASE,
    115200
};

UartPort_T gUart1PortInfo =
{
    ARM_UART1_BASE,
    115200
};

#if defined(CONFIG_SC7710G2) || defined(CONFIG_SC8830)
UartPort_T gUart3PortInfo =
{
    ARM_UART3_BASE,
    115200
};
#endif

#if defined(CONFIG_SC8830)
UartPort_T gUart2PortInfo =
{
    ARM_UART2_BASE,
    115200
};
#endif

LOCAL unsigned int SIO_GetHwDivider (unsigned int baudrate)
{
    return (unsigned int) ( (ARM_APB_CLK + baudrate / 2) / baudrate);
}
LOCAL void SIO_HwOpen (struct FDL_ChannelHandler *channel, unsigned int divider)
{
    UartPort_T *port  = (UartPort_T *) channel->priv;

#if defined(CONFIG_SC7710G2)
    if(port->regBase == ARM_UART3_BASE){
	    /* Disable UART*/
	    * ( (volatile unsigned int *) (GR_CTRL_REG1)) &= ~ (GR_UART3_CTRL_EN);
	    /*Disable Interrupt */
	    * (volatile unsigned int *) (port->regBase + ARM_UART_IEN) = 0;
	    /* Enable UART*/
	    * (volatile unsigned int *) GR_CTRL_REG1 |= (GR_UART3_CTRL_EN);

    } else 
#endif
    {
	    /* Disable UART*/
	    * ( (volatile unsigned int *) (GR_CTRL_REG)) &= ~ (GR_UART_CTRL_EN);
	    /*Disable Interrupt */
	    * (volatile unsigned int *) (port->regBase + ARM_UART_IEN) = 0;
	    /* Enable UART*/
	    * (volatile unsigned int *) GR_CTRL_REG |= (GR_UART_CTRL_EN);
    }
    /* Set baud rate  */
    * (volatile unsigned int *) (port->regBase + ARM_UART_CLKD0) = LWORD (divider);
    * (volatile unsigned int *) (port->regBase + ARM_UART_CLKD1) = HWORD (divider);


    /* Set port for 8 bit, one stop, no parity  */
    * (volatile unsigned int *) (port->regBase + ARM_UART_CTL0) = UARTCTL_BL8BITS | UARTCTL_SL1BITS;
    * (volatile unsigned int *) (port->regBase+ ARM_UART_CTL1) = 0;
    * (volatile unsigned int *) (port->regBase + ARM_UART_CTL2) = 0;
}

LOCAL int SIO_Open (struct FDL_ChannelHandler  *channel, unsigned int baudrate)
{
    unsigned int divider;
    unsigned int i = 0;

	UartPort_T *port  = (UartPort_T *) channel->priv;
	
    divider = SIO_GetHwDivider (baudrate);
    SIO_HwOpen (channel, divider);

    while(!SIO_TRANS_OVER(port->regBase))  /* Wait until all characters are sent out */
    {
    	i++;
    	if(i >= UART_SET_BAUDRATE_TIMEOUT)
    	{
    	//	return -1;
		break;
    	}
    }

    return 0;

}
LOCAL int SIO_Read (struct FDL_ChannelHandler  *channel, const unsigned char *buf, unsigned int len)
{
#ifndef CONFIG_NAND_SPL
    unsigned char *pstart = (unsigned char *) buf;
    const unsigned char *pend = pstart + len;
    UartPort_T *port  = (UartPort_T *) channel->priv;

    while ( (pstart < pend)
            && SIO_RX_READY (SIO_GET_RX_STATUS (port->regBase)))
    {
        *pstart++ = SIO_GET_CHAR (port->regBase);
    }

    return pstart - (unsigned char *) buf;
#else
	return 0;
#endif
}
LOCAL char SIO_GetChar (struct FDL_ChannelHandler  *channel)
{
#ifndef CONFIG_NAND_SPL
    UartPort_T *port  = (UartPort_T *) channel->priv;

    while (!SIO_RX_READY (SIO_GET_RX_STATUS (port->regBase)))
    {

    }

    return SIO_GET_CHAR (port->regBase);
#else
	return 0;
#endif
}
LOCAL int SIO_GetSingleChar (struct FDL_ChannelHandler  *channel)
{
#ifndef CONFIG_NAND_SPL
    UartPort_T *port  = (UartPort_T *) channel->priv;
    char ch;

    if (!SIO_RX_READY (SIO_GET_RX_STATUS (port->regBase)))
    {
        return -1;

    }
    else
    {
        ch  = SIO_GET_CHAR (port->regBase);
    }

    return ch;
#else
	return 0;
#endif
}
LOCAL int SIO_Write (struct FDL_ChannelHandler  *channel, const unsigned char *buf, unsigned int len)
{
#ifndef CONFIG_NAND_SPL
    const unsigned char *pstart = (const unsigned char *) buf;
    const unsigned char *pend = pstart + len;
    UartPort_T *port  = (UartPort_T *) channel->priv;

    while (pstart < pend)
    {
        /* Check if tx port is ready.*/
        /*lint -save -e506 -e731*/
        while (!SIO_TX_READY (SIO_GET_TX_STATUS (port->regBase)))
        {

            /* Do nothing */
        }

        SIO_PUT_CHAR (port->regBase, *pstart);
        ++pstart;
    }

    /* Ensure the last byte is written successfully */
    while (!SIO_TX_READY (SIO_GET_TX_STATUS (port->regBase)))
    {
        /* Do nothing */
    }

    return pstart - (const unsigned char *) buf;
#else
	return 0;
#endif
}

LOCAL int SIO_PutChar (struct FDL_ChannelHandler  *channel, const unsigned char ch)
{
#ifndef CONFIG_NAND_SPL
    UartPort_T *port  = (UartPort_T *) channel->priv;

    while (!SIO_TX_READY (SIO_GET_TX_STATUS (port->regBase)))
    {
        /* Do nothing */
    }

    SIO_PUT_CHAR (port->regBase, ch);

    /* Ensure the last byte is written successfully */
    while (!SIO_TX_READY (SIO_GET_TX_STATUS (port->regBase)))
    {
        /* Do nothing */
    }

    return 0;
#else
	return 0;
#endif
}
LOCAL int SIO_SetBaudrate (struct FDL_ChannelHandler  *channel,  unsigned int baudrate)
{
    channel->Open (channel, baudrate);
    return 0;
}
LOCAL int SIO_Close (struct FDL_ChannelHandler  *channel)
{
    return 0;
}

struct FDL_ChannelHandler gUart0Channel =
{
    SIO_Open,
    SIO_Read,
    SIO_GetChar,
    SIO_GetSingleChar,
    SIO_Write,
    SIO_PutChar,
    SIO_SetBaudrate,
    SIO_Close,
    &gUart0PortInfo
};

struct FDL_ChannelHandler gUart1Channel =
{
    SIO_Open,
    SIO_Read,
    SIO_GetChar,
    SIO_GetSingleChar,
    SIO_Write,
    SIO_PutChar,
    SIO_SetBaudrate,
    SIO_Close,
    &gUart1PortInfo
};

#if defined(CONFIG_SC7710G2) || defined(CONFIG_SC8830)
struct FDL_ChannelHandler gUart3Channel =
{
    SIO_Open,
    SIO_Read,
    SIO_GetChar,
    SIO_GetSingleChar,
    SIO_Write,
    SIO_PutChar,
    SIO_SetBaudrate,
    SIO_Close,
    &gUart3PortInfo
};
#endif

#if defined(CONFIG_SC8830)
struct FDL_ChannelHandler gUart2Channel =
{
    SIO_Open,
    SIO_Read,
    SIO_GetChar,
    SIO_GetSingleChar,
    SIO_Write,
    SIO_PutChar,
    SIO_SetBaudrate,
    SIO_Close,
    &gUart2PortInfo
};
#endif

DECLARE_GLOBAL_DATA_PTR;
int __dl_log_share__ = 0;
void serial_setbrg(void)
{
	SIO_SetBaudrate(&gUart1Channel, 115200);
}
int serial_getc(void)
{
	return SIO_GetChar (&gUart1Channel);
}

void serial_putc(const char c)
{
	SIO_PutChar(&gUart1Channel, c);
 
	/* If \n, also do \r */
	if(__dl_log_share__ == 0){
		if (c == '\n')
			serial_putc ('\r');
	}
}

/*
 *  * Test whether a character is in the RX buffer
 *   */
int serial_tstc (void)
{
	UartPort_T *port  = (&gUart1Channel)->priv;
	/* If receive fifo is empty, return false */
	return SIO_RX_READY( SIO_GET_RX_STATUS( port->regBase) ) ;
}

void serial_puts (const char *s)
{
	if(__dl_log_share__ == 0){
		while(*s!=0){
			serial_putc(*s++);
		}
	}
}

/*
 *  * Initialise the serial port with the given baudrate. The settings
 *   * are always 8 data bits, no parity, 1 stop bit, no start bits.
 *    *
 *     */
int serial_init (void)
{
	SIO_Open(&gUart1Channel, 115200);
	/* clear input buffer */
	if(serial_tstc())
	  serial_getc();
	return 0;
}

/*
*   add UART0 driver for modem boot
*/
#if defined(CONFIG_SC7710G2)
void serial3_setbrg(void)
{
	SIO_SetBaudrate(&gUart3Channel, 115200);
}
int serial3_getc(void)
{
	return SIO_GetChar (&gUart3Channel);
}

void serial3_putc(const char c)
{
	SIO_PutChar(&gUart3Channel, c);
}

/*
 *  * Test whether a character is in the RX buffer
 *   */
int serial3_tstc (void)
{
	UartPort_T *port  = (&gUart3Channel)->priv;
	/* If receive fifo is empty, return false */
	return SIO_RX_READY( SIO_GET_RX_STATUS( port->regBase) ) ;
}

void serial3_puts (const char *s)
{
	while (*s) {
		serial3_putc (*s++);
	}
}

int serial3_init (void)
{
	SIO_Open(&gUart3Channel, 115200);
	/* clear input buffer */
	if(serial3_tstc())
	  serial3_getc();
	return 0;
}

int  serial3_flowctl_enable(void)
{
	/*enable tx/rx flow control*/	   
	* (volatile unsigned int *) (ARM_UART3_BASE + ARM_UART_CTL1)   |= UART_RX_FLOW_EN  | (UART_RX_THRESHOLD & 0x7F);    
	return 0;
}

#endif


