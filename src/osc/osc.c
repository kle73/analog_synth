#include "osc.h"
#include <stdio.h>

float sine_wave(float freq, float* phase){
	if (phase == NULL) return 0.0f;
	float res = AMPLITUDE * sinf(2.0 * PI *  freq * (*phase));
	(*phase) += 1.0f / SAMPLE_RATE;
	return res;
}

 /*		 ___     ___
 	* __|	  |___|   |___...
  */
float square_wave(float freq, float* phase){
	if (phase == NULL) return 0.0f;
	float res;
	res = ((AMPLITUDE * sinf(2.0 * PI * freq * (*phase))) > 0) ? (AMPLITUDE) : (-AMPLITUDE); 
	(*phase) += 1.0f / SAMPLE_RATE;
	return res;
}

/*
 * |\ |\ |\
 * | \| \| \...
 */
float saw_wave(float freq, float* phase){
	if (phase == NULL) return 0.0f;
	float res = AMPLITUDE * 2 * (freq*(*phase) - floor(0.5 + freq*(*phase)));
	(*phase) += 1.0f / SAMPLE_RATE;
	return res;
}

float f_abs(float f){
	if (f > 0.0f) return f;
	else return (-1)*f;
}

float triangle_wave(float freq, float* phase){
	if (phase == NULL) return 0.0f;
	float res = AMPLITUDE * 2 * f_abs((freq*(*phase) - floor(0.5 + freq*(*phase))));
	(*phase) += 1.0f / SAMPLE_RATE;
	return res;
}

float silence(float freq, float* phase){
	if (phase == NULL) return 0.0f;
	(*phase) += freq / SAMPLE_RATE;
	return 0.0;
}

