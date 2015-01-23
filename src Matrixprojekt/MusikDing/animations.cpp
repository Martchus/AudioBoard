/*
Animationen für Matrix
*/

#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "animation.h"
#include "CLED.h"
#include "global.h"

extern CLED led;

/*
Mittelt Stereo zu Mono und zeigt Spektrum als Balken
*/
typedef struct
{
	uint16_t band_soll[ANIM_BAND_NUM], band_ist[ANIM_BAND_NUM];
	uint16_t farbe_soll, farbe_ist;
	uint8_t sat[ANIM_BAND_NUM];
} a_monospek_t;
	
void anim_monospektrum_init()
{
	uint8_t i;
	a_monospek_t *d = (a_monospek_t*)anim_buffer;
	
	memset(d, 0, sizeof(a_monospek_t));
	for(i = 0; i < ANIM_BAND_NUM; i++)
	{
		d->sat[i] = 255;
		d->band_ist[i] = 300;
	}
}

void anim_monospektrum_step()
{
	a_monospek_t *d = (a_monospek_t*)anim_buffer;
	uint16_t band;
	uint8_t x, y;

	d->farbe_soll = ((amplitude_l + amplitude_r) / 2) >> 4;
	
	if(d->farbe_soll > d->farbe_ist)
	{
		d->farbe_ist += 1;
	}
	else if(d->farbe_soll < d->farbe_ist)
	{
		d->farbe_ist -= 1;
	}
	
	for(x = 0; x < ANIM_BAND_NUM; x++)
	{
		d->band_soll[x] = (bands_l[x] + bands_r[x]);	// /2
		
		if(d->band_soll[x] > d->band_ist[x])
		{
			if(d->band_ist[x] < 9800)
			{
				d->band_ist[x] += 200;
			}
			else
			{
				d->band_ist[x] = 10000;
			}
		}
		else if(d->band_soll[x] < d->band_ist[x])
		{
			if(d->band_ist[x] > 400)
			{
				d->band_ist[x] -= 100;
			}
			else
			{
				d->band_ist[x] = 300;
			}
		}
		
		if(d->sat[x] < 255)
		{
			d->sat[x] += 5;
		}
	}
	
// 	if(beats & BEAT_LOW && d->sat[0] == 255)
// 	{
// 		d->sat[0] = 105;
// 		d->sat[1] = 105;
// 	}
// 	
// 	if(beats & BEAT_MID && d->sat[3] == 255)
// 	{
// 		d->sat[2] = 105;
// 		d->sat[3] = 105;
// 		d->sat[4] = 105;
// 	}
// 
// 	if(beats & BEAT_HIGH && d->sat[5] == 255)
// 	{
// 		d->sat[5] = 105;
// 		d->sat[6] = 105;
// 	}

	led.clear();
	for(x = 0; x < ANIM_BAND_NUM; x++)
	{
		band = d->band_ist[x];
		if(band > 9000)
		{
			band = 9000;
		}
		
		y = 0;
		while(band > 1500)
		{
			led.setLED_HSV(x, y, d->farbe_ist + (x * 25), d->sat[x], 255);
			y++;
			band -= 1500;
		}
		
		if(y < LED_NUM_Y)
		{
			led.setLED_HSV(x, y, d->farbe_ist + (x * 25), d->sat[x], band / 6);
		}
	}
	led.update();
}

/*
Stereo Spektrum
*/
typedef struct {
	uint16_t band_soll_l[6], band_soll_r[6], band_ist_l[6], band_ist_r[6];
} a_stereospek_t;

void anim_stereospektrum_init()
{
	a_stereospek_t *d = (a_stereospek_t*)anim_buffer;
	uint8_t i;
	
	memset(d, 0, sizeof(a_stereospek_t));
	for(i = 0; i < 6; i++)
	{
		d->band_ist_l[i] = 300;
		d->band_ist_r[i] = 300;
	}
}

void anim_stereospektrum_step()
{
	a_stereospek_t *d = (a_stereospek_t*)anim_buffer;
	uint8_t i, j;
	uint16_t temp;
	rgb_t col, *col_old;
	
	// vorbereiten

	for(i = 0; i < 6; i++)
	{
		d->band_soll_l[i] = bands_l[i];
		d->band_soll_r[i] = bands_r[i];
	}
	
	// berechnen
	for(i = 0; i < 6; i++)
	{
		if(d->band_ist_l[i] < d->band_soll_l[i])
		{
			d->band_ist_l[i] += 200;
			if(d->band_ist_l[i] > 9000)
			{
				d->band_ist_l[i] = 9000;
			}
		}
		else if(d->band_ist_l[i] > d->band_soll_l[i])
		{
			if(d->band_ist_l[i] > 300)
			{
				d->band_ist_l[i] -= 50;
			}
			else
			{
				d->band_ist_l[i] = 300;
			}
		}
		
		if(d->band_ist_r[i] < d->band_soll_r[i])
		{
			d->band_ist_r[i] += 200;
			if(d->band_ist_r[i] > 9000)
			{
				d->band_ist_r[i] = 9000;
			}
		}
		else if(d->band_ist_r[i] > d->band_soll_r[i])
		{
			if(d->band_ist_r[i] > 300)
			{
				d->band_ist_r[i] -= 50;
			}
			else
			{
				d->band_ist_r[i] = 300;
			}
		}
	}
	
	// ausgeben
	led.clear();
	for(i = 0; i < 6; i++)
	{
		// l
		col.r = i * 6;
		col.g = 255 - (i * 6);
		col.b = 0;
		temp = d->band_ist_l[i];
		j = 0;
		
		while(temp > 1200)
		{
			led.setLED_HSV(j, i, 83 - (i * 8), 255, 255);
			temp -= 1200;
			j++;
		}
		
		if(temp > 0 && j < LED_NUM_X)
		{
			led.setLED_HSV(j, i, 83 - (i * 8), 255, temp / 5);
		}
		
		// r
		temp = d->band_ist_r[i];
		j = 6;
		
		while(temp > 1200)
		{
			col_old = led.getLED(j, i);
			led.setLED_HSV(&col, 160 + (i * 8), 255, 255);
			col_old->r |= col.r;
			col_old->g |= col.g;
			col_old->b |= col.b;
			temp -= 1200;
			j--;
		}
		if(temp > 0 && j < LED_NUM_X)
		{
			col_old = led.getLED(j, i);
			led.setLED_HSV(&col, 160 + (i * 8), 255, temp / 5);
			col_old->r |= col.r;
			col_old->g |= col.g;
			col_old->b |= col.b;			
		}
	}
	led.update();
}

/*
fliegende partikel
*/
#define MAX_PARTIKEL		25

typedef struct 
{
	struct {
		uint8_t hue, val;	// farbe
		uint8_t x, y;		// position
		uint8_t vx, vy;		// geschwindigkeit
		uint8_t cx, cy;		// zähler für bewegung
		uint8_t dx, dy;		// richtung
	} partikel[MAX_PARTIKEL];
} a_partikel1_t;

void anim_partikel1_init()
{
	a_partikel1_t *d = (a_partikel1_t*)anim_buffer;
	
	memset(d, 0, sizeof(a_partikel1_t));
}

void anim_partikel1_step()
{
	a_partikel1_t *d = (a_partikel1_t*)anim_buffer;	
	uint8_t i, temp, x, y;
	rgb_t ncol, *ocol;
	
//	led.clear();
	
	// aktive bewegen
	for(i = 0; i < MAX_PARTIKEL; i++)
	{
		if(!d->partikel[i].val)
		{
			continue;
		}

		d->partikel[i].val--;
		
		if(d->partikel[i].cx)
		{
			d->partikel[i].cx--;
		}
		else
		{
			if(d->partikel[i].dx)
			{
				if(d->partikel[i].x < LED_NUM_X)
				{
					d->partikel[i].x++;
				}
				else
				{
					d->partikel[i].val = 0;
				}
			}
			else
			{
				if(d->partikel[i].x > 0)
				{
					d->partikel[i].x--;
				}
				else
				{
					d->partikel[i].val = 0;
				}
			}
			d->partikel[i].cx = d->partikel[i].vx;
		}	// /x richtung
		
		if(d->partikel[i].cy)
		{
			d->partikel[i].cy--;
		}
		else
		{
			if(d->partikel[i].dy)
			{
				if(d->partikel[i].y < LED_NUM_Y)
				{
					d->partikel[i].y++;
				}
				else
				{
					d->partikel[i].val = 0;
				}
			}
			else
			{
				if(d->partikel[i].y > 0)
				{
					d->partikel[i].y--;
				}
				else
				{
					d->partikel[i].val = 0;
				}
			}
			d->partikel[i].cy = d->partikel[i].vy;
		}	// /y richtung

	}	// /bewegen
	
	// leeren platz suchen
	temp = 255;
	for(i = 0; i < MAX_PARTIKEL; i++)
	{
		if(!d->partikel[i].val)
		{
			temp = i;
			break;
		}
	}
	
	// falls gefunden
	if(temp != 255)
	{
		// links nach rechts
		if(beats & BEAT_LOW)
		{
			d->partikel[temp].dx = 1;
			d->partikel[temp].dy = rand() & 1;
			
			d->partikel[temp].x = 0;
			d->partikel[temp].y = rand() % LED_NUM_Y;
			
			d->partikel[temp].vx = (255 - bpm_l) >> 4; //rand() % bpm_l + 10;
			d->partikel[temp].vy = rand() % 100 + 50;
			
			d->partikel[temp].cx = d->partikel[temp].vx;
			d->partikel[temp].cy = d->partikel[temp].vy;
			
			d->partikel[temp].val = rand () % 100 + 150;
			d->partikel[temp].hue = (fft_bucket_h_l - fft_bucket_l_l) << 2;
			
			beats &= ~BEAT_LOW;
		}
		
		// rechts nach links
		if(beats & BEAT_HIGH)
		{
			d->partikel[temp].dx = 0;
			d->partikel[temp].dy = rand() & 1;
			
			d->partikel[temp].x = LED_NUM_X - 1;
			d->partikel[temp].y = rand() % LED_NUM_Y;
			
			d->partikel[temp].vx = (255 - bpm_h) >> 2; // rand() % bpm_h + 10;
			d->partikel[temp].vy = rand() % 100 + 50;
			
			d->partikel[temp].cx = d->partikel[temp].vx;
			d->partikel[temp].cy = d->partikel[temp].vy;
			
			d->partikel[temp].val = rand () % 100 + 150;
			d->partikel[temp].hue = (fft_bucket_h_r - fft_bucket_l_r) << 2;
			
			beats &= ~BEAT_HIGH;
		}

		// oben<->unten
		if(beats & BEAT_MID)
		{
			d->partikel[temp].dx = rand() & 1;
			
			if(bpm_m & 1)
			{
				d->partikel[temp].dy = 0;
				d->partikel[temp].x = rand() % LED_NUM_X;
				d->partikel[temp].y = LED_NUM_Y - 1;				
			}
			else
			{
				d->partikel[temp].dy = 1;
				d->partikel[temp].x = rand() % LED_NUM_X;
				d->partikel[temp].y = 0;
			}
			
			d->partikel[temp].vx = rand() % 100 + 50;
			d->partikel[temp].vy = (255 - bpm_m) >> 2;	//rand() % bpm_m + 10;
			
			d->partikel[temp].cx = d->partikel[temp].vx;
			d->partikel[temp].cy = d->partikel[temp].vy;
			
			d->partikel[temp].val = rand () % 100 + 150;
			d->partikel[temp].hue = ((fft_bucket_h_l - fft_bucket_l_r) << 2) + (rand() & 0xFF);
			
			beats &= ~BEAT_MID;
		}
	}
	
	// ausgabe
	// altes faden
	for(x = 0; x < LED_NUM_X; x++)
	{
		for(y = 0; y < LED_NUM_Y; y++)
		{
			ocol = led.getLED(x, y);
			if(ocol->r > 3)
			{
				ocol->r -= 3;
			}
			else
			{
				ocol->r = 0;
			}
			
			if(ocol->g > 3)
			{
				ocol->g -= 3;
			}
			else
			{
				ocol->g = 0;
			}
			
			if(ocol->b > 3)
			{
				ocol->b -= 3;
			}
			else
			{
				ocol->b = 0;
			}
		}
	}
	
	// neues hinzufügen
	for(i = 0; i < MAX_PARTIKEL; i++)
	{
		if(!d->partikel[i].val)
		{
			continue;
		}
		
		ocol = led.getLED(d->partikel[i].x, d->partikel[i].y);
		led.setLED_HSV(&ncol, d->partikel[i].hue, 255, d->partikel[i].val);
		
		ocol->r |= ncol.r;
		ocol->g |= ncol.g;
		ocol->b |= ncol.b;
	}
	led.update();
}

/*
kreisding
*/
#define KREISDING_MA			16
#define KREISDING_FAKTOR		128
#define KREISDING_MAX_X			(LED_NUM_X * KREISDING_FAKTOR)
#define KREISDING_MAX_Y			(LED_NUM_Y * KREISDING_FAKTOR)
typedef struct 
{
	uint16_t x, y;				// position
	uint16_t zx, zy;			// ziel
	uint16_t offset_ma;				// für farbe
	uint8_t offset;
} a_kreisding_t;

void anim_kreisding_init()
{
	a_kreisding_t *d = (a_kreisding_t*)anim_buffer;
	
	memset(d, 0, sizeof(a_kreisding_t));
	
	d->x = KREISDING_MAX_X / 2;
	d->y = KREISDING_MAX_Y / 2;
	
	d->zx = rand() % KREISDING_MAX_X;
	d->zy = rand() % KREISDING_MAX_Y;
}

void anim_kreisding_step()
{
	a_kreisding_t *d = (a_kreisding_t*)anim_buffer;
	int32_t tempx, tempy, x, y;
	uint32_t temp;
	
	temp = bpm_all / 16;
	if(d->x > d->zx)
	{
		if((d->x - d->zx) > temp)
		{
			d->x -= temp;
		}
		else
		{
			d->x--;
		}
	}
	else if(d->x < d->zx)
	{
		if((d->zx - d->x) > temp)
		{
			d->x += temp;
		}
		else
		{
			d->x++;
		}
	}
	else
	{
		d->zx = rand() % KREISDING_MAX_X;
	}
		
	temp = bpm_all / 16;
	if(d->y > d->zy)
	{
		if((d->y - d->zy) > temp)
		{
			d->y -= temp;
		}
		else
		{
			d->y--;
		}
	}
	else if(d->y < d->zy)
	{
		if((d->zy - d->y) > temp)
		{
			d->y += temp;
		}
		else
		{
			d->y++;
		}
	}
	else
	{
		d->zy= rand() % KREISDING_MAX_Y;
	}
	
	d->offset_ma = ((d->offset_ma * (KREISDING_MA - 1)) + ((fft_bucket_h_l + fft_bucket_h_r) / 2)) / KREISDING_MA;
	d->offset += d->offset_ma;
	
	for(x = 0; x < LED_NUM_X; x++)
	{
		for(y = 0; y < LED_NUM_Y; y++)
		{
			tempx = (x * KREISDING_FAKTOR) - d->x;
			tempy = (y * KREISDING_FAKTOR) - d->y;
			
			temp = sqrt(tempx * tempx + tempy * tempy);
			
			led.setLED_HSV(x, y, (temp / 3) + d->offset, 255, 200);
		}
	}
	led.update();
}