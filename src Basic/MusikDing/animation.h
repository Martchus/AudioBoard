#ifndef __CANIMATION_H__
#define __CANIMATION_H__

#include "CFFT.h"

#define BEAT_HIGH		0x01
#define BEAT_MID		0x02
#define BEAT_LOW		0x04

#define ANIM_BAND_NUM			7

extern uint16_t bands_l[ANIM_BAND_NUM], bands_r[ANIM_BAND_NUM];
extern uint16_t amplitude_l, amplitude_r;
extern uint8_t beats, bpm_h, bpm_m, bpm_l, bpm_all;
extern uint8_t fft_bucket_h_l, fft_bucket_h_r, fft_bucket_l_l, fft_bucket_l_r;

#define ANIM_CALIB_IDENT		0xAA
typedef struct 
{
	uint8_t ident;
	uint16_t bands_calib_l[ANIM_BAND_NUM], bands_calib_r[ANIM_BAND_NUM];
	uint16_t amplitude_l, amplitude_r;
} bands_calibration_t;

void anim_init();
void anim_frame();

void anim_inputData(fft_result_t *left, fft_result_t *right);

#endif //__CANIMATION_H__
