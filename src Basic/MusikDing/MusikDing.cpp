/*
this is the main file. you should only modify this at the marked locations

used peripherals for all basic modules:
TCC1 - systemt imer
TCC0 - sampling timer

TWIE - LCD

USARTC1 - LED

DMA0 - LED
DMA1 - ADC
DMA2 - ADC

ADCA
ADCB
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include "global.h"
#include "adc.h"
#include "CFFT.h"
#include "animation.h"

CFFT fft;

volatile uint32_t systick;			// system time [ms]

#define FLAG_FRAME		0x01
#define FLAG_100HZ		0x02
#define FLAG_DOSAMPLE	0x04
#define FLAG_FFTDONE	0x08
volatile uint8_t flags, adc_state;

// generate seed for RNG
unsigned short get_seed() {
	unsigned short seed = 0;
	unsigned short *p = (unsigned short*)(RAMEND + 1);
	extern unsigned short __heap_start;
	
	while(p >= &__heap_start + 1)
		seed ^= *(--p);
	
	return seed;
}

int main(void)
{
	// use external clock and activate PLL
	OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_16KCLK_gc;
	OSC.CTRL |= OSC_XOSCEN_bm;
	while(!(OSC.STATUS & OSC_XOSCRDY_bm))
		;
	OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | 2;
	OSC.CTRL |= OSC_PLLEN_bm;
	while(!(OSC.STATUS & OSC_PLLRDY_bm))
		;
	CCP = CCP_IOREG_gc;
	CLK.CTRL = CLK_SCLKSEL_PLL_gc;
	OSC.CTRL &= ~OSC_RC2MEN_bm;
	
	// system timer every ms
	systick = 0;
	TCC1.CTRLA = TC_CLKSEL_DIV1_gc;
	TCC1.INTCTRLA = TC_OVFINTLVL_MED_gc;
	TCC1.PER = 31999;
	
	DMA.CTRL = DMA_ENABLE_bm;
	
	_delay_ms(100);
	
	anim_init();
	adc_init();
	srand(get_seed());
	
	flags = 0;
	adc_state = ADC_STATE_IDLE;
	
	PMIC.CTRL |= (PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm);
	sei();
	
    while(1)
    {	
		// check sampling
		if(adc_state == ADC_STATE_SAMPLING)
		{
			adc_check();
		}
		
		// every 10 ms
		if(flags & FLAG_100HZ)
		{		
			// TODO: important tasks, which needs to be checked every 10ms e.g. poll inputs
			
			flags &= ~FLAG_100HZ;
		}
		
		// calculate new frame
		else if(flags & FLAG_FRAME)
		{
			// TODO: change this function in animtion.cpp so it calculates the next frame and sends it to an output
			anim_frame();

			flags &= ~FLAG_FRAME;
		}
		
		// sample new data
		else if((adc_state == ADC_STATE_IDLE) && (flags & FLAG_DOSAMPLE))
		{
			adc_startSampling();
			
			flags &= ~FLAG_DOSAMPLE;
		}
		
		// start calculations
		else if(adc_state == ADC_STATE_SAMPLING_DONE)
		{	
			fft.doFFT();
			adc_state = ADC_STATE_WAIT;
		}
		
		 // processing data
		 else if(adc_state == ADC_STATE_WAIT)
		 {
			if(!fft.doStep())
			{
				flags |= FLAG_FFTDONE;
				adc_state = ADC_STATE_IDLE;
			}
		 }
		 
		 // fft done. put data into animation system
		 else if(flags & FLAG_FFTDONE)
		 {
			anim_inputData(fft.getLeft(), fft.getRight()); 
			flags &= ~FLAG_FFTDONE;
		 }
		
		// if nothing else to do
		else
		{
			// TODO: here you can add unimportant tasks like sending data to a display
		}
    }
}

#define TIME_FRAME		16 //33
#define TIME_100HZ		10
#define TIME_SAMPLE		50

ISR(TCC1_OVF_vect)
{
	static uint8_t cnt_frame = TIME_FRAME, cnt_100hz = TIME_100HZ;
	static uint16_t cnt_capture = TIME_SAMPLE;
	
	cnt_frame--;
	if(!cnt_frame)
	{
		PORTF.OUTTGL = PIN0_bm;
		flags |= FLAG_FRAME;
		cnt_frame = TIME_FRAME;
	}
	
	cnt_100hz--;
	if(!cnt_100hz)
	{
		flags |= FLAG_100HZ;
		cnt_100hz = TIME_100HZ;
	}
	
	cnt_capture--;
	if(!cnt_capture)
	{
		flags |= FLAG_DOSAMPLE;
		cnt_capture = TIME_SAMPLE;
	}
	
	systick++;
}