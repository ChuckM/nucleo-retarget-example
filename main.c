#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "board.h"

extern uint32_t mtime(void);
extern void msleep(uint32_t );

int main(void);

int
main()
{
	double flash_rate = 3.14159;
	char buf[128];
	uint32_t done_time;

	printf("\nNUCLEO Retarget demo.\n");
	switch (rcc_system_clock_source()) {
		case 2:
			printf("Running on PLL (%s Source) at %f Mhz\n", 
					(RCC_PLLCFGR & RCC_PLLCFGR_PLLSRC) ? "HSE" : "HSI", 
					rcc_ahb_frequency / 1000000.0);
			break;
		case 0:
			printf("Running on HSI at %f Mhz\n", rcc_ahb_frequency / 1000000.0);
			break;
		case 1:
			printf("Running on HSE at %f Mhz\n", rcc_ahb_frequency / 1000000.0);
			break;
		default:
			printf("Unknown clock!\n");
	}
	printf("RCC Control Registers:\n");
	printf("      RCC_CFGR is : 0x%08x\n", (unsigned int) RCC_CFGR);
	printf("        RCC_CR is : 0x%08x\n", (unsigned int) RCC_CR);
	printf("   RCC_PLLCFGR is : 0x%08x\n", (unsigned int) RCC_PLLCFGR);

	/* This is the LED on the board */
	gpio_mode_setup(BOARD_LED_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BOARD_LED_PIN);

	/* You can print integers and floats */
    printf("Hello, world!\n");
	/* and create a running dialog with the console */
	printf("We're going to start blinking the LED, between 0 and 300 times per\n");
	printf("second.\n");
	while (1) {
		printf("Please enter a flash rate: ");
		fflush(stdout);
		fgets(buf, 128, stdin);
		buf[strlen(buf) - 1] = '\0';
		flash_rate = atof(buf);
		if (flash_rate == 0) {
			printf("Was expecting a number greater than 0 and less than 300.\n");
			printf("but got '%s' instead\n", buf);
		} else if (flash_rate > 300.0) {
			printf("The number should be less than 300.\n");
		} else {
			break;
		}
	}
	/* MS per flash */
	done_time = (int) (500 / flash_rate);
	printf("The closest we can come will be %f flashes per second\n", 500.0 / done_time);
	printf("With %d MS between states\n", (int) done_time);
	printf("Press ^C to restart this demo.\n");
	while (1) {
		msleep(done_time);
		gpio_toggle(BOARD_LED_GPIO, BOARD_LED_PIN);
	}
}
