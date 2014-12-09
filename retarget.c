/* retarget.c
 *
 * Stub routines to connect the Nucleo board's USART2
 * into the C library.
 */
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/systick.h>
#include "board.h"

#ifndef NULL
#define NULL	0	
#endif

#define BUFLEN 127

static void SystemInit(void);

void uart_putc(char c);
char uart_getc(int);
void retarget_init(void);
void _ttywrch(int c);
int _write (int fd, char *ptr, int len);
int _read (int fd, char *ptr, int len);
void get_buffered_line(void);
void msleep(uint32_t);
uint32_t mtime(void);

/* simple millisecond counter */
static volatile uint32_t system_millis;
static volatile uint32_t delay_timer;

/* systick handler */
void
sys_tick_handler(void) {
	system_millis++;
	gpio_toggle(GPIOC, GPIO3);
	if (delay_timer > 0) {
		delay_timer--;
	}
}

/* Simple spin loop waiting for time to pass
 * NB: Not multithreaded :-)
 */
void
msleep(uint32_t delay) {
	delay_timer = delay;
	while (delay_timer) ;
}

uint32_t
mtime(void) {
	return system_millis;
}

struct ringbuffer {
	volatile char *buf;
	volatile uint16_t nxt;
	volatile uint16_t cur;
};


/* Interrupt driven character input */
#define UART_BUF_SIZE	32

static volatile char recv_buf[UART_BUF_SIZE];
static volatile uint16_t nxt_recv_ndx;
static volatile uint16_t cur_recv_ndx;

static volatile char xmit_buf[UART_BUF_SIZE];
static volatile uint16_t nxt_xmit_ndx;
static volatile uint16_t cur_xmit_ndx;

void board_usart_isr(void);

/*
 * A bit hacky here, to avoid rewriting this same code for
 * multiple boards, there are some #define macros that come
 * from board.h, to wit:
 *		USART_ISR - This turns into the name of this boards
 *					USART isr function (usart2_isr for the
 *					Nucleo F411RE board for example)
 *		USART_HAS_RXNE - this evaluates to true when the board's
 *					USART recieve buffer is not empty.
 *		USART_HAS_TXE - this evaluates to true when the board's
 *					USART transmitter buffer is empty.
 *		USART_READ - this function returns a byte read from the
 *					usart.
 *		USART_WRITE(c) - this function writes a byte to the 
 *					boards USART.
 *
 * Called when we get a read interrupt from board's USART
 * note the macro USART_ISR transforms into the appropriate
 * function name (usart2_isr in the case of the F411RE)
 */
void
USART_ISR(void) {
	uint32_t	reg;		// Lets us capture the top of stack
	if (USART_HAS_RXNE) {
		recv_buf[nxt_recv_ndx] = USART_READ;
		/*
		 * This bit of code will jump to the ResetHandler if you
		 * hit ^C
		 */
		if (recv_buf[nxt_recv_ndx] == '\03') {
			volatile uint32_t *ret = (&reg) + 7;
			*ret = (uint32_t)(&reset_handler);
			return;
		}
		nxt_recv_ndx = (nxt_recv_ndx + 1) % UART_BUF_SIZE;
		/* note it doesn't watch for overflow */
	}
	if (USART_HAS_TXE) {
		if (nxt_xmit_ndx == cur_xmit_ndx) {
			usart_disable_tx_interrupt(USART2);
		} else {
			USART_WRITE(xmit_buf[cur_xmit_ndx]);
			cur_xmit_ndx = (cur_xmit_ndx + 1) % UART_BUF_SIZE;
		}
	}
}

/*
 * This is a small hack which takes function keys (arrow keys, etc)
 * and returns a 'character' for them. It waits briefly to (2mS) to see
 * if another character is going to follow an ESC and if it does then
 * it trys to collect the entire escape sequence which can be translated
 * into a function key (or special key).
 */
static uint8_t fkey_buf[5];
static int fkey_ndx;

static uint8_t map_function_key(void);

/*
 * Ok, if we got a function key sequence we look here to map it
 * into an 8 bit return value. It was simply $[<char> we take the
 * character returned and turn on bit 7.
 */
static uint8_t
map_function_key(void) {
    if (fkey_ndx == 1) {
        return 0x80 | fkey_buf[1];
    }
    return 0xff;
}

/*
 * This is a pretty classic ring buffer for characters
 */
static uint16_t start_ndx = 0;
static uint16_t end_ndx = 0;
static char buf[BUFLEN+1] = {0};
#define buf_len ((end_ndx - start_ndx) % BUFLEN)
#define inc_ndx(n) n = (n + 1) % BUFLEN
#define dec_ndx(n) n = (n - 1) % BUFLEN


/*
 * A buffered line editing function.
 */
void
get_buffered_line(void) {
	char	c;

	if (start_ndx != end_ndx) {
		return; // still has unread data
	}
	while (1) {
		c = uart_getc(1);
		if (c == '\r') {
			buf[end_ndx] = '\n';
			inc_ndx(end_ndx);
			buf[end_ndx] = '\0';
			uart_putc('\r'); uart_putc('\n');
			return;
		}
		if ((c == '\010') || (c == '\177')) { // ^H or DEL erase a char
			if (buf_len == 0) {
				uart_putc('\a');
			} else {
				dec_ndx(end_ndx);
				uart_putc('\010'); uart_putc(' '); uart_putc('\010');
			}
		} else if (c == 0x17) {	// ^W erase a word
			while ((buf_len > 0)&& (!(isspace((int) buf[end_ndx])))) {
				dec_ndx(end_ndx);
				uart_putc('\010'); uart_putc(' '); uart_putc('\010');
			}
		} else if (c == 0x15) { // ^U erase the line
			while (buf_len > 0) {
				dec_ndx(end_ndx);
				uart_putc('\010'); uart_putc(' '); uart_putc('\010');
			}
		} else { // non-editing character
			if (buf_len == (BUFLEN - 1)) {
				uart_putc('\a');
			} else {
				buf[end_ndx] = c;
				inc_ndx(end_ndx);
				uart_putc(c);
			}
		}
	}
}

/*
 * uart_getc(int wait)
 *
 * If wait is non-zero the function blocks until a character
 * is available. If wait is 0 then the function returns immediately
 * with '0' as the value. (yes this means it can't read 'nul' bytes
 * interactively)
 */
char
uart_getc(int wait) {
    char res;

    while (cur_recv_ndx == nxt_recv_ndx) {
		/* return NUL if not asked to wait */
		if (wait == 0) {
			return '\000';
		}
	}
    res = recv_buf[cur_recv_ndx];
    cur_recv_ndx = (cur_recv_ndx + 1) % UART_BUF_SIZE;

    /* check for function key */
    /* as written this code depends on the function key
     * string coming in at full speed and there being a
     * slight delay with the next character, but that might
     * not be a good assumption. We'll see if it is good enough
     * for now.
     *
     * In testing 2 mS sleeps to be sure another character isn't
     * coming in seems to be ok, also you don't normally type the CSI
     * sequence (^[ [) on your own. you can "fool" the input driver
     * if you do into trying to interpret a function key sequence with
     * unpredictable results.
     */
    fkey_ndx = 0;
    if (res == 27) {
		/* pause 2 MS after seeing escape */
        msleep(2);
        if ((cur_recv_ndx != nxt_recv_ndx) &&
            (recv_buf[cur_recv_ndx] == 91)) {
			/* if you see '[' then you're looking at a function key */
            for (fkey_ndx = 0; fkey_ndx < 5; fkey_ndx++) {
                fkey_buf[fkey_ndx] = recv_buf[cur_recv_ndx];
                cur_recv_ndx = (cur_recv_ndx + 1) % UART_BUF_SIZE;
                msleep(2);
                if (cur_recv_ndx == nxt_recv_ndx) {
                    break; // no more characters
                }
            }
        }
    }
    return (fkey_ndx == 0) ? res : map_function_key();
}


/*
 * uart_putc
 *
 * This pushes a character into the transmit buffer for
 * the channel and turns on TX interrupts (which will fire
 * because initially the register will be empty.) If the
 * ISR sends out the last character it turns off the transmit
 * interrupt flag, so that it won't keep firing on an empty
 * transmit buffer.
 */
void
uart_putc(char c) {

    /* block if the transmit buffer is full */
    while (((nxt_xmit_ndx + 1) % UART_BUF_SIZE) == cur_xmit_ndx) ;
    xmit_buf[nxt_xmit_ndx] = c;
    nxt_xmit_ndx = (nxt_xmit_ndx + 1) % UART_BUF_SIZE;
    usart_enable_tx_interrupt(BOARD_USART);
}

/* 
 * Called by libc functions
 */
int 
_write (int fd, char *ptr, int len)
{
	int i = 0;

	/* 
	 * Write "len" of char from "ptr" to file id "fd"
	 * Return number of char written.
	 */
	if (fd > 2) {
		return -1;  // STDOUT, STDIN, STDERR
	}
	while (*ptr && (i < len)) {
		uart_putc(*ptr);
		if (*ptr == '\n') {
			uart_putc('\r');
		}
		i++;
		ptr++;
	}
  return i;
}

/*
 * This function is a bit broken, it is always called with
 * len == 1. And since the default standard libraries do no
 * line editing, it makes for a very distasteful input experience.
 * So the line_buffer() code creates a buffer layer in front of
 * this code for a nice user experience.
 */
int
_read (int fd, char *ptr, int len)
{
	int	my_len;

	if (fd > 2) {
		return -1;
	}

	get_buffered_line();
	my_len = 0;
	while ((buf_len > 0) && (len > 0)) {
		*ptr++ = buf[start_ndx];
		inc_ndx(start_ndx);
		my_len++;
		len--;
	}
	return my_len; // return the length we got
}

void _ttywrch(int ch) {
  /* Write one char "ch" to the default console
   * Need implementing with UART here. */
	uart_putc(ch);
}

/* SystemInit will be called before main */
__attribute__((constructor))
static void SystemInit() {

#ifdef SCB_CPACR
	SCB_CPACR |= SCB_CPACR_FULL * (SCB_CPACR_CP10 | SCB_CPACR_CP11);
#endif

	BOARD_CLOCK_SETUP;

	// Initialize UART
	/* MUST enable the GPIO clock in ADDITION to the USART clock */
	RCC_ENABLE_USART;
	BOARD_GPIO_SETUP;
	BOARD_USART_SETUP(115200);

	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	STK_CVR = 0;
	systick_set_reload(rcc_ahb_frequency / 1000);
	systick_counter_enable();
	systick_interrupt_enable();
	nxt_recv_ndx = cur_recv_ndx = 0;
	nxt_xmit_ndx = cur_xmit_ndx = 0;
	start_ndx = 0;
	end_ndx = 0;
	buf[0] = '\0';
}

