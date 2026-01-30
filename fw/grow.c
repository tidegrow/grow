/*
 * Tidegrow Grow Firmware for Zwölf LS10 module
 * Copyright (c) 2025 Lone Dynamics Corporation. All rights reserved.
 *
 */

#include "ch32v003fun.h"
#include "i2c_slave.h"
#include <stdio.h>

#define GROW_POWER	100	// pwm duty cycle (percentage)
#define GROW_MINS	(14*60)	// minutes on per 24-hour cycle (14 hours = 840 mins)

// --

#define GROW_I2C_ADDR	0x99

#define GROW_I2C_SCL_PORT GPIOC	// ZA
#define GROW_I2C_SCL_PIN 2

#define GROW_I2C_SDA_PORT GPIOC	// ZB
#define GROW_I2C_SDA_PIN 1

#define GROW_PWM_PORT GPIOD		// ZC (low = off, float = on full)
#define GROW_PWM_PIN 6

#define GROW_TEMP_PORT GPIOD		// ZD (temperature, 0V - 1.1V)
#define GROW_TEMP_PIN 5

volatile uint8_t i2c_regs[32] = {0x00};

// Time tracking for GROW_MINS functionality
// Counts minutes from power-on (0-1439), then wraps back to 0 for 24-hour cycle
// Light turns ON at power-on (minute 0), stays on for GROW_MINS,
// then turns OFF for (1440-GROW_MINS), creating a repeating 24-hour cycle
volatile uint32_t seconds_counter = 0;
volatile uint16_t current_minute = 0;
volatile uint8_t light_on = 0;

void i2c_onWrite(uint8_t reg, uint8_t length);
void i2c_onRead(uint8_t reg);
void update_light_state(void);
void configure_pwm_power(void);

void adc_init(void)
{
	// ADCCLK = 24 MHz => RCC_ADCPRE = 0: divide by 2
	RCC->CFGR0 &= ~(0x1F<<11);

	// Enable ADC
	RCC->APB2PCENR |= RCC_APB2Periph_ADC1;
	
	// PD5 is analog input chl 5
	GPIOD->CFGLR &= ~(0xf<<(4*5));	// CNF = 00: Analog, MODE = 00: Input
	
	// Reset the ADC to init all regs
	RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
	RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;
	
	// Set up single conversion for ch 5
	ADC1->RSQR1 = 0;
	ADC1->RSQR2 = 0;
	ADC1->RSQR3 = 5;	// 0-9 for 8 ext inputs and two internals
	
	// set sampling time for chl 5
	ADC1->SAMPTR2 &= ~(ADC_SMP0<<(3*5));
   // Possible times: 0->3,1->9,2->15,3->30,4->43,5->57,6->73,7->241 cycles
   ADC1->SAMPTR2 = 1/*9 cycles*/ << (3/*offset per channel*/ * 7/*channel*/);

	// turn on ADC and set rule group to sw trig
	ADC1->CTLR2 |= ADC_ADON | ADC_EXTSEL;

	// Reset calibration
	ADC1->CTLR2 |= ADC_RSTCAL;
	while(ADC1->CTLR2 & ADC_RSTCAL);
	
	// Calibrate
	ADC1->CTLR2 |= ADC_CAL;
	while(ADC1->CTLR2 & ADC_CAL);
	
	// should be ready for SW conversion now
}

uint16_t adc_get(void)
{
	// start sw conversion (auto clears)
	ADC1->CTLR2 |= ADC_SWSTART;
	
	// wait for conversion complete
	while(!(ADC1->STATR & ADC_EOC));
	
	// get result
	return ADC1->RDATAR;
}

// mask for the CCxP bits
// We use positive polarity (0x00) because we invert in software for open-drain PWM
#define TIM2_DEFAULT 0x00

void pwm_init(void) {

	// Enable GPIOD and TIM2
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;
	RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

	// Reset TIM2 to init all regs
	RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
	RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;
	
	// SMCFGR: default clk input is CK_INT
	// set TIM2 clock prescaler divider 
	// For 48MHz clock: 48MHz / (PSC+1) / (ATRLR+1) = PWM frequency
	// Target: ~390 Hz = 48MHz / 480 / 256
	TIM2->PSC = 479;  // 48MHz / 480 = 100kHz timer clock
	// set PWM total cycle width
	TIM2->ATRLR = 255;  // 100kHz / 256 = ~390 Hz PWM frequency
	
	// for channel 3, let CCxS stay 00 (output), set OCxM to 110 (PWM I)
	// enabling preload causes the new pulse width in compare capture register only to come into effect when UG bit in SWEVGR is set (= initiate update) (auto-clears)
	TIM2->CHCTLR2 |= TIM_OC3M_2 | TIM_OC3M_1 | TIM_OC3PE;

	// Set initial PWM value to 0 (off)
	TIM2->CH3CVR = 0;

	// CTLR1: default is up, events generated, edge align
	// enable auto-reload of preload
	TIM2->CTLR1 |= TIM_ARPE;

	// DON'T enable Channel 3 output yet - let configure_pwm_power do it
	// Just ensure polarity bit is cleared
	TIM2->CCER &= ~TIM_CC3P;

	// initialize counter
	TIM2->SWEVGR |= TIM_UG;

	// Enable TIM2
	TIM2->CTLR1 |= TIM_CEN;
}

void pwm_setpw(uint16_t width) {
	TIM2->CH3CVR = width;  // PD6 is TIM2_CH3 with full remap
	TIM2->SWEVGR |= TIM_UG; // load new value in compare capture register
}

// Configure PWM based on GROW_POWER setting
void configure_pwm_power(void) {
	if (GROW_POWER >= 100) {
		// 100% power - float the pin (high-Z state) per PAM2861 datasheet
		TIM2->CCER &= ~TIM_CC3E;  // Disable PWM output on CH3
		(GROW_PWM_PORT)->CFGLR &= ~(0xf<<(4*GROW_PWM_PIN));
		(GROW_PWM_PORT)->CFGLR |= (GPIO_CNF_IN_FLOATING)<<(4*GROW_PWM_PIN);
	} else if (GROW_POWER == 0) {
		// 0% power - drive LOW to turn off per PAM2861 datasheet
		TIM2->CCER &= ~TIM_CC3E;  // Disable PWM on CH3
		(GROW_PWM_PORT)->CFGLR &= ~(0xf<<(4*GROW_PWM_PIN));
		(GROW_PWM_PORT)->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*GROW_PWM_PIN);
		(GROW_PWM_PORT)->BCR = (1 << GROW_PWM_PIN);  // Set LOW
	} else {
		// 1-99% power - use open-drain PWM without inversion
		// Configure pin as PWM output, OPEN-DRAIN
		(GROW_PWM_PORT)->CFGLR &= ~(0xf<<(4*GROW_PWM_PIN));
		(GROW_PWM_PORT)->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD_AF)<<(4*GROW_PWM_PIN);
		
		// Enable PWM output on CH3 without polarity inversion
		TIM2->CCER &= ~TIM_CC3P;  // Clear polarity bit (non-inverted)
		TIM2->CCER |= TIM_CC3E;
		
		// Non-inverted: higher PWM value = more time "high" (floating) = brighter
		uint16_t pwm_value = (uint32_t)(GROW_POWER * 255) / 100;
		pwm_setpw(pwm_value);
	}
}

// Update light on/off state based on GROW_MINS
// Runs on GROW_MINS, then off for (1440-GROW_MINS), repeating every 24 hours (1440 mins)
void update_light_state(void) {
	if (GROW_MINS >= 1440) {
		// Always on (24 hours or more)
		light_on = 1;
	} else if (GROW_MINS == 0) {
		// Always off
		light_on = 0;
	} else {
		// On for GROW_MINS minutes, then off for (1440-GROW_MINS) minutes
		// Example: GROW_MINS=840 (14 hrs) means on for mins 0-839, off for mins 840-1439
		light_on = (current_minute < GROW_MINS) ? 1 : 0;
	}
	
	// Apply the state
	if (light_on) {
		configure_pwm_power();
	} else {
		// Turn off - set pin low
		TIM2->CCER &= ~TIM_CC3E;  // Disable PWM on CH3
		(GROW_PWM_PORT)->CFGLR &= ~(0xf<<(4*GROW_PWM_PIN));
		(GROW_PWM_PORT)->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*GROW_PWM_PIN);
		(GROW_PWM_PORT)->BCR = (1 << GROW_PWM_PIN);  // Set low
	}
}

// SysTick interrupt handler for time tracking
void SysTick_Handler(void) __attribute__((interrupt));
void SysTick_Handler(void) {
	seconds_counter++;
	
	// Update minute counter every 60 seconds
	if (seconds_counter >= 60) {
		seconds_counter = 0;
		current_minute++;
		if (current_minute >= 1440) {  // 24 hours = 1440 minutes
			current_minute = 0;
		}
		// Update light state when minute changes
		update_light_state();
	}
	
	SysTick->SR = 0;  // Clear interrupt flag
}

int main()
{

	SystemInit();
	Delay_Ms( 100 );
	funGpioInitAll();

	// TEMP
	(GROW_TEMP_PORT)->CFGLR &= ~(0xf<<(4*GROW_TEMP_PIN));
	(GROW_TEMP_PORT)->CFGLR |= (GPIO_CNF_IN_FLOATING)<<(4*GROW_TEMP_PIN);

	// configure I2C slave
   funPinMode(PC1, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SDA
   funPinMode(PC2, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SCL

//	SetupI2CSlave(GROW_I2C_ADDR, i2c_regs, sizeof(i2c_regs),
//		i2c_onWrite, i2c_onRead, false);

	// init adc
	adc_init();

	// Configure AFIO for TIM2 remap (PD6 = TIM2_CH1)
	RCC->APB2PCENR |= RCC_APB2Periph_AFIO;
	AFIO->PCFR1 |= AFIO_PCFR1_TIM2_REMAP;

	// pwm init
	pwm_init();

	// Setup SysTick for 1 second interrupts (48MHz / 48000000 = 1Hz)
	SysTick->CTLR = 0;  // Disable during setup
	SysTick->CMP = 48000000 - 1;  // Compare value for 1 second
	SysTick->CNT = 0;  // Clear counter
	SysTick->CTLR = 0x0F;  // Enable SysTick with interrupt, HCLK source
	
	NVIC_EnableIRQ(SysTicK_IRQn);

	// Initialize light state IMMEDIATELY before any printf delays
	update_light_state();

	// Now print banner and info
	printf("\r\n\r\n");
	printf("=== === ==  ===  == === === = = \r\n");
	printf(" =   =  = = ==  = = ==  = = === \r\n");
	printf(" =  === ==  === === = = === === \r\n");
	printf("\r\n\r\ngrow console\r\n\r\n");
	printf("GROW_POWER: %d%%\r\n", GROW_POWER);
	printf("GROW_MINS: %d minutes (%d hrs %d mins)\r\n", 
		GROW_MINS, GROW_MINS/60, GROW_MINS%60);
	
	printf("\r\nLight initialized: %s\r\n", light_on ? "ON" : "OFF");

	printf("\r\n24-hour cycle started at power-on\r\n");
	printf("Lights ON for %d mins, OFF for %d mins\r\n\r\n", 
		GROW_MINS, 1440-GROW_MINS);

	while(1)
	{
		printf("Cycle minute: %d/1440 (%02d:%02d), Light: %s", 
			current_minute, current_minute/60, current_minute%60, 
			light_on ? "ON " : "OFF");
		if (light_on && current_minute < GROW_MINS) {
			printf(" (%d mins remaining)", GROW_MINS - current_minute);
		} else if (!light_on && current_minute >= GROW_MINS) {
			printf(" (%d mins until next cycle)", 1440 - current_minute);
		}
		printf("\r\n");
		
		Delay_Ms( 5000 );

		uint16_t adcval = adc_get();
		printf("Temperature ADC: %d\r\n", adcval);
	}

}

void i2c_onWrite(uint8_t reg, uint8_t length) {
	printf("i2c_onWrite reg: %x len: %x val: %x\r\n", reg, length,
		i2c_regs[reg]);
	if (reg == 0x10) {
	}
}

void i2c_onRead(uint8_t reg) {
	printf("i2c_onRead reg: %x val: %x\r\n", reg, i2c_regs[reg]);
}

