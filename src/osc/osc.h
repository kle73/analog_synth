#ifndef OSC_H
#define OSC_H

#include <math.h>
#include <stdlib.h>

#define SAMPLE_RATE 						44100
#define AMPLITUDE 							0.25
#define MAX_SAMPLES_PER_UPDATE  4096
#define GET_FREQUENCY(note)     pow(2, (note - 69)/12.0f)*440.0f
#define MAX_SIM_NOTES           13
#define PI 											3.14159265358979323846f

float silence       (float freq, float* phase);
float sine_wave     (float freq, float* phase);
float square_wave   (float freq, float* phase);
float saw_wave      (float freq, float* phase);
float triangle_wave (float freq, float* phase);


#endif
