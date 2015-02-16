/*
basis system for calculations
*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <avr/eeprom.h>
#include <stdlib.h>

#include "animation.h"
#include "CFFT.h"
#include "ffft.h"
#include "global.h"

extern CFFT fft;

// user data l - left, r - right channel
uint16_t bands_l[ANIM_BAND_NUM], bands_r[ANIM_BAND_NUM];					// value of the combined bands
uint16_t amplitude_l, amplitude_r;											// amplitude
uint8_t beats, bpm_h, bpm_m, bpm_l, bpm_all;								// bpm in high, mid, low or all ranges
uint8_t fft_bucket_h_l, fft_bucket_h_r, fft_bucket_l_l, fft_bucket_l_r;		// highest/lowest FFT bucket

// working data
#define SPECTRUM_MA_NUM			16			// MA over spectrum
uint16_t ma_spectrum_low[SPECTRUM_MA_NUM], ma_spectrum_mid[SPECTRUM_MA_NUM], ma_spectrum_high[SPECTRUM_MA_NUM];
uint8_t ma_spectrum_wpos, ma_spectrum_rpos;
#define BPM_WMA_NUM				5			// WMA for BPM

#define ANIM_INPUT_PER_SECOND	20		// number of calls of anim_input per second

bands_calibration_t b_calib;
bands_calibration_t EEMEM b_calib_eeprom;

// init and load default calibration values
void anim_init()
{	
	memset(bands_l, 0, sizeof(uint16_t) * ANIM_BAND_NUM);
	memset(bands_r, 0, sizeof(uint16_t) * ANIM_BAND_NUM);

	memset(ma_spectrum_low, 0, sizeof(uint16_t) * SPECTRUM_MA_NUM);
	memset(ma_spectrum_mid, 0, sizeof(uint16_t) * SPECTRUM_MA_NUM);
	memset(ma_spectrum_high, 0, sizeof(uint16_t) * SPECTRUM_MA_NUM);
	ma_spectrum_rpos = 0;
	ma_spectrum_wpos = 0;
		
	eeprom_read_block((void*)&b_calib, (void*)&b_calib_eeprom, sizeof(bands_calibration_t));
	
	if(b_calib.ident != ANIM_CALIB_IDENT)
	{
		memset(&b_calib, 0, sizeof(bands_calibration_t));
		b_calib.bands_calib_l[0] = 6209;
		b_calib.bands_calib_l[1] = 3991;
		b_calib.bands_calib_l[2] = 37;
		b_calib.bands_calib_l[3] = 34;
		b_calib.bands_calib_l[4] = 56;
		b_calib.bands_calib_l[5] = 100;
		b_calib.bands_calib_l[6] = 146;
		
		b_calib.bands_calib_r[0] = 6099;
		b_calib.bands_calib_r[1] = 3886;
		b_calib.bands_calib_r[2] = 116;
		b_calib.bands_calib_r[3] = 141;
		b_calib.bands_calib_r[4] = 213;
		b_calib.bands_calib_r[5] = 424;
		b_calib.bands_calib_r[6] = 649;
		
		b_calib.amplitude_l = 46;
		b_calib.amplitude_r = 165;
	}
	
	beats = 0;
	bpm_h = 0;
	bpm_m = 0;
	bpm_l = 0;
}

void anim_frame()
{
	// TODO: add your code for your animation here
}

/*
64 buckets, 250Hz/bucket -> 7 bands
#	frequency range FFT bucket range
0	0		250		0
1	250		500		1
2	500		1000	2 ... 3
3	1000	2000	4 ... 7
4	2000	4000	8 ... 15
5	4000	8000	16 ... 31
6	8000	ende	32 ... 63

this function calculates combined bands, beats and some minor things from the FFT data
*/
void anim_inputData(fft_result_t *left, fft_result_t *right)
{
	uint32_t temp_r, temp_l, temp_h, temp_m;
	uint8_t i;
	static uint8_t cnt = 0, h_cnt = 0, m_cnt = 0, l_cnt = 0, all_cnt = 0;
	
	// bcombine bands
	bands_l[0] = left->spectrum[0];
	bands_r[0] = right->spectrum[0];
	
	bands_l[1] = left->spectrum[1];
	bands_r[1] = right->spectrum[1];
	
	temp_l = 0;
	temp_r = 0;
	for(i = 2; i <= 3; i++)
	{
		temp_l += left->spectrum[i];
		temp_r += right->spectrum[i];
	}
	bands_l[2] = temp_l / 2;
	bands_r[2] = temp_r / 2;
	
	temp_l = 0;
	temp_r = 0;
	for(i = 4; i <= 7; i++)
	{
		temp_l += left->spectrum[i];
		temp_r += right->spectrum[i];
	}
	bands_l[3] = temp_l / 4;
	bands_r[3] = temp_r / 4;
	
	temp_l = 0;
	temp_r = 0;
	for(i = 8; i <= 15; i++)
	{
		temp_l += left->spectrum[i];
		temp_r += right->spectrum[i];
	}
	bands_l[4] = temp_l / 8;
	bands_r[4] = temp_r / 8;
	
	temp_l = 0;
	temp_r = 0;
	for(i = 16; i <= 31; i++)
	{
		temp_l += left->spectrum[i];
		temp_r += right->spectrum[i];
	}
	bands_l[5] = temp_l / 16;
	bands_r[5] = temp_r / 16;
	
	temp_l = 0;
	temp_r = 0;
	for(i = 32; i <= 63; i++)
	{
		temp_l += left->spectrum[i];
		temp_r += right->spectrum[i];
	}
	bands_l[6] = temp_l / 28;
	bands_r[6] = temp_r / 28;
	
	// search highest/lowest buckest
	fft_bucket_h_l = 0;
	fft_bucket_h_r = 0;
	fft_bucket_l_r = 255;
	fft_bucket_l_l = 255;
	
	temp_h = left->spectrum[0];
	temp_l = right->spectrum[0];
	temp_m = 0xFFFFFFFF;
	temp_r = 0xFFFFFFFF;
	for(i = 0; i < (FFT_N / 2); i++)
	{
		if(left->spectrum[i] > temp_h)
		{
			temp_h = left->spectrum[i];
			fft_bucket_h_l = i;
		}
		if(right->spectrum[i] > temp_l)
		{
			temp_l = right->spectrum[i];
			fft_bucket_h_r = i;
		}
		
		if(left->spectrum[i] < temp_m)
		{
			temp_m = left->spectrum[i];
			fft_bucket_l_l = i;
		}
		if(right->spectrum[i] < temp_r)
		{
			temp_r = left->spectrum[i];
			fft_bucket_l_r = i;
		}
	}
	
	// save amplitude
	amplitude_l = left->adc_max - left->adc_min;
	amplitude_r = right->adc_max - right->adc_min;
	
	// MA over spectrum
	temp_l = 0;
	for(i = 0; i < 3; i++)
	{
		temp_l += (left->spectrum[i] + right->spectrum[i]) / 2;
	}
	ma_spectrum_low[ma_spectrum_wpos] = temp_l / 3;
	
	temp_l = 0;
	for(i = 3; i < 20; i++)
	{
		temp_l += (left->spectrum[i] + right->spectrum[i]) / 2;
	}
	ma_spectrum_mid[ma_spectrum_wpos] = temp_l / 17;

	temp_l = 0;
	for(i = 20; i < 63; i++)
	{
		temp_l += (left->spectrum[i] + right->spectrum[i]) / 2;
	}
	ma_spectrum_high[ma_spectrum_wpos] = temp_l / 43;
	
	ma_spectrum_rpos = ma_spectrum_wpos;
	ma_spectrum_wpos = (ma_spectrum_wpos + 1) % SPECTRUM_MA_NUM;
	
	// calibration
	for(i = 0; i < ANIM_BAND_NUM; i++)
	{
		if(bands_l[i] > b_calib.bands_calib_l[i])
		{
			bands_l[i] -= b_calib.bands_calib_l[i];
		}
		else
		{
			bands_l[i] = 0;
		}
		
		if(bands_r[i] > b_calib.bands_calib_r[i])
		{
			bands_r[i] -= b_calib.bands_calib_r[i];
		}
		else
		{
			bands_r[i] = 0;
		}
	}
	
	if(amplitude_l > b_calib.amplitude_l)
	{
		amplitude_l -= b_calib.amplitude_l;
	}
	else
	{
		amplitude_l = 0;
	}
	
	if(amplitude_r > b_calib.amplitude_r)
	{
		amplitude_r -= b_calib.amplitude_r;
	}
	else
	{
		amplitude_r = 0;
	}
	
	// simple beat detection
	temp_h = 0;
	temp_l = 0;
	temp_m = 0;
	
	for(i = 0; i < SPECTRUM_MA_NUM; i++)
	{
		temp_h += ma_spectrum_high[i];
		temp_m += ma_spectrum_mid[i];
		temp_l += ma_spectrum_low[i];
	}
	
	temp_h /= SPECTRUM_MA_NUM;
	temp_m /= SPECTRUM_MA_NUM;
	temp_l /= SPECTRUM_MA_NUM;
	
	
	beats = 0;
	if(ma_spectrum_high[ma_spectrum_rpos] > (uint16_t)((float)temp_h * 2.0f))
	{
		beats |= BEAT_HIGH;
		h_cnt++;
	}
	
	if(ma_spectrum_mid[ma_spectrum_rpos] > (uint16_t)((float)temp_m * 1.8f))
	{
		beats |= BEAT_MID;
		m_cnt++;
	}
	
	if(ma_spectrum_low[ma_spectrum_rpos] > (uint16_t)((float)temp_l * 1.5f))
	{
		beats |= BEAT_LOW;
		l_cnt++;
	}
	
	if(beats)
	{
		all_cnt++;
	}
	
	cnt++;
	if(cnt >= (ANIM_INPUT_PER_SECOND * 2))
	{
		bpm_l = ((bpm_l * (BPM_WMA_NUM - 1)) + (l_cnt * 30)) / BPM_WMA_NUM;
		l_cnt = 0;
		
		bpm_m = ((bpm_m * (BPM_WMA_NUM - 1)) + (m_cnt * 30)) / BPM_WMA_NUM;
		m_cnt = 0;
		
		bpm_h = ((bpm_h * (BPM_WMA_NUM - 1)) + (h_cnt * 30)) / BPM_WMA_NUM;
		h_cnt = 0;
		
		bpm_all = ((bpm_all * (BPM_WMA_NUM - 1)) + (all_cnt * 30)) / BPM_WMA_NUM;
		all_cnt = 0;
		
		cnt = 0;
	}
}
