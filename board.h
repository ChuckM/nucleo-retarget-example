/*
 * board.h
 *
 * There are a number of differences in how LOC3 interacts with different
 * versions of the Cortex-M family. And the Nucleo boards differ in what
 * resources they use to implement the debug serial port, and how their
 * clocks are enabled. As a result, this is the 'bandaid' include, which
 * #defines some conceptual things that are identical from board to board
 * but differ in how they are implemented.
 *
 * Porting to a new board is a matter of changing these things to work
 * with that board.
 */
#ifndef __board_h__
#define __board_h__

#define BOARD_USART USART2

#define USART_ISR(v) usart2_isr()
#define USART_HAS_RXNE	(USART_SR(USART2) & USART_SR_RXNE)
#define USART_HAS_TXE	(USART_SR(USART2) & USART_SR_TXE)
#define USART_READ	USART_DR(USART2)
#define USART_WRITE(c)	USART_DR(USART2) = c
#define BOARD_CLK_FREQUENCY RCC_8MHZ_HSE_FREQUENCY

#define BOARD_CLOCK_SETUP		rcc_pll_clock_setup(RCC_8MHZ_HSE_96_MHZ, RCC_8MHZ_HSE_FREQUENCY)

#define RCC_ENABLE_USART \
	rcc_periph_clock_enable(RCC_GPIOA); \
	rcc_periph_clock_enable(RCC_USART2);

#define BOARD_GPIO_SETUP  \
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3); \
	gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);

#define BOARD_USART_SETUP(baud) \
	usart_set_baudrate(USART2, baud); \
	usart_set_databits(USART2, 8); \
	usart_set_stopbits(USART2, USART_STOPBITS_1); \
	usart_set_mode(USART2, USART_MODE_TX_RX); \
	usart_set_parity(USART2, USART_PARITY_NONE); \
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE); \
	usart_enable(USART2); \
	nvic_enable_irq(NVIC_USART2_IRQ); \
	usart_enable_rx_interrupt(USART2);

#define BOARD_LED_GPIO	GPIOA
#define BOARD_LED_PIN	GPIO5

/* Toggle PC3 in the systick handler, should have a 500Hz squarewave on it
 * if the clocks are set correctly.
 */
#define VALIDATE_SYSTICK_RATE
#endif
