#ifndef GLOBAL_H_
#define GLOBAL_H_

//#define __INPUT_DEBUG__			// debug ausgabe für ausgewertete daten
//#define __APS_DEBUG__			// debug ausgabe für actions per second (nur mit __input_debug__ zusammen)

#ifdef __APS_DEBUG__
extern uint8_t __fps, __sps;
#endif

extern volatile uint8_t flags, adc_state;
extern volatile uint32_t systick;

#endif /* GLOBAL_H_ */