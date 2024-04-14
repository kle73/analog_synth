#include <unistd.h>
#include <stdio.h>

#include "channel/channel.h"

#include "osc/osc.h"
#include "filter/filter.h"
#include "amp/amp.h"

#include "external/raylib.h"
#include "external/fft/fft.h"

#define SCREEN_WIDTH 580
#define SCREEN_HEIGHT 200
#define BORDER_SIZE 30

#define FFT_SIZE 4096
#define MIDI_CHANNEL_SIZE 100
#define VIZ_BARS_BOX_SIZE 512
#define MAX_VIZ_BAR_SIZE 70

#define VIZ_BARS_BOX_X (SCREEN_WIDTH - VIZ_BARS_BOX_SIZE) / 2
#define VIZ_BARS_BOX_Y (SCREEN_HEIGHT - MAX_VIZ_BAR_SIZE - BORDER_SIZE)
#define VIZ_BARS_BOX_BORDER 10
	
typedef enum{
	NOTE_ON,
	NOTE_OFF
}Event_Type;

typedef struct {
	float frequency;
	float amplitude;
	Event_Type type;
}MIDI_Event;


volatile int terminate = 0;

// stores all currently playing frequencies
static float frequencies[MAX_SIM_NOTES] = {-1.0f};
//stores current phase for all playing frequencies
static float phases[MAX_SIM_NOTES] = {0.0f};



//stores the current waveform (sine, triangle, ...)
static float (*oscillate)(float freq, float* phase) = sine_wave;
//stores the current filter (high_pass, low_pass, ...)
static void (*filter)(void* buf, unsigned int frames, float alpha) = no_filter;



static KeyboardKey keys[13] = {KEY_S, KEY_E, KEY_D, KEY_R, KEY_F, KEY_G, KEY_Y, 
												KEY_H, KEY_U, KEY_J, KEY_I, KEY_K, KEY_L};
static int is_pressed[13] = {0};


//stores fft visualization results, to be used for the GUI
static float frequency_viz_bars[VIZ_BARS_BOX_SIZE] = {0.0f};


channel_t midi_channel;



/* MIDI CHANNEL
* ---------------------------------------------------------------------------
* 1: wait until it gets a midi event.
* 2: wait until midi event is successfully send (channel not empty).
*/

// 1
static inline void send_midi_data(MIDI_Event* event, channel_t* chn){
	while (channel_send((void*)event, chn) < 0){
		if (terminate) break;
	}
}
// 2
static inline void receive_midi_data(MIDI_Event* event, channel_t* chn){
	while (channel_rcv((void*) event, chn) < 0){
		if (terminate) break;
	}
}



/* 
 * Called by raylib internal thread.
 * Reads values from all non-negative entries in the frequencies buffer
 * and produces signal for raudio.
 */
void audio_input_callback(void* buffer, unsigned int frames){
	float* samples = (float*)buffer;
	for (int i=0; i<frames; i++){
		samples[i] = 0.0f;
		for (int j=0; j<MAX_SIM_NOTES; j++){
			// keep samples synced:
			float sample = oscillate(frequencies[j], phases+j);
			if (frequencies[j] >= 0){
				samples[i] += sample;
			}
		}
	}
	filter(buffer, frames, 0);
}



/* OSCILLATOR thread: tid[0]
* --------------------------------------------------------------------
* Waits to get midi events through the midi_cannel.
* Sets the corresponding values in the freqiencies buffer.
*/
void* osc_run(void* arg){
	while(1){
		MIDI_Event ev;
		receive_midi_data(&ev, &midi_channel);
		int i = ev.frequency-69;
		if (ev.type == NOTE_OFF){
			frequencies[i] = -1.0f;
		}
		else {
			frequencies[i] = GET_FREQUENCY(ev.frequency);
		}
		if (terminate) break;
	}
	return NULL;
}


/* VISUALIZATIONS thread: tid[1]
* ---------------------------------------------------------------------
* Take a fixed number of samples over 1 second of the current 
* audio signal. (Using this time interval is crutial for the FFT
* to produce meaningful results. We cant just take the current sample
* buffer as these samples do not cover enough of the signal.)
* Run FFT on it and perform some cosmetic calculations to get the
* height of the corresponding frequency visualization bars.
*/

void* visualize_frequencies(void* arg){
	minfft_real in[FFT_SIZE] = {0.0}; // Input signal
  minfft_cmpl out[FFT_SIZE] = {0.0}; // Output of the FFT
	minfft_aux *a;
	a = minfft_mkaux_realdft_1d(FFT_SIZE);
	while (1){
		float phase;
		minfft_real in[FFT_SIZE] = {0.0}; // Input signal
		for (int k=0; k<MAX_SIM_NOTES; k++){
			if (frequencies[k] >= 0){
				phase = 0.0f;
				for (int i = 0; i < FFT_SIZE; i++) {
						in[i] += oscillate(frequencies[k], &phase);
						phase = (float) i / FFT_SIZE;
				}	
			}
		}

		filter(in, FFT_SIZE, 0);

    minfft_realdft(in, out, a);
		int step = (FFT_SIZE/2)/VIZ_BARS_BOX_SIZE;
		float acc;
		for (int i=0; i<FFT_SIZE/2; i+=step){
			acc = 0;
			for (int k=i; k<i+step; k++){
				acc += sqrt(out[k][0]*out[k][0] + out[k][1]*out[k][1]);
			}
			acc /= step;
			if (acc > MAX_VIZ_BAR_SIZE) acc = MAX_VIZ_BAR_SIZE;
			frequency_viz_bars[i/step] = acc;
		}

		if (terminate) break;
	}
	return NULL;
}



/* KEYBOARD INPUT PROCESSING
* ------------------------------------------------------------------------
* Check the keys and trigger the corresponding action if one is pressed.
* Triggers MIDI events or changes the function pointers for:
* 1. the oscillator (oscillate)
* 2. the filter     (filter)
* 3. the amplifier  (amplify)
*/

static inline void reset(){
	for (int i=0; i<MAX_SIM_NOTES; i++){
		frequencies[i] = -1.0f;
		phases[i] = 0.0f;
	}
}

void check_keys(){
	for (int i=0; i<13; i++){
			if (IsKeyDown(keys[i])) {
				if (is_pressed[i] == 0){
					MIDI_Event ev = {69+i, 1.0, NOTE_ON};
					send_midi_data(&ev, &midi_channel);
					is_pressed[i] = 1;
				}
			}
			if (IsKeyUp(keys[i])) {
				if (is_pressed[i] == 1){
					MIDI_Event ev = {69+i, 1.0, NOTE_OFF};
					send_midi_data(&ev, &midi_channel);
					is_pressed[i] = 0;
				}
			}
	}
	if (IsKeyDown(KEY_ONE)){
		oscillate = sine_wave; reset();
	} else if (IsKeyDown(KEY_TWO)){
		oscillate = saw_wave; reset();
	} else if (IsKeyDown(KEY_THREE)){
		oscillate = triangle_wave; reset();
	} else if (IsKeyDown(KEY_FOUR)){
		oscillate = square_wave; reset();
	}
}




/* MAIN LOOP thread: main
*  ------------------------------------------------------------------------
*  Creates all necessary threads and runs the main input loop to get user
*  input and draw the GUI.
*/

void main_loop(){
	// setup windows and midi channel
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "analog_synth");
  SetTargetFPS(60); 
	
	// get user input and draw GUI
	while(!WindowShouldClose())
	{
			check_keys();
			BeginDrawing();
			ClearBackground(GRAY);
			
			// Draw freq visualization
			DrawRectangle(VIZ_BARS_BOX_X - VIZ_BARS_BOX_BORDER, // x-coord
										VIZ_BARS_BOX_Y - VIZ_BARS_BOX_BORDER, // y-coord
										VIZ_BARS_BOX_SIZE + 2 * VIZ_BARS_BOX_BORDER, // width
										MAX_VIZ_BAR_SIZE + 2 * VIZ_BARS_BOX_BORDER,  // height
										BLACK); 
			for (int i=0; i<VIZ_BARS_BOX_SIZE; i++){
				DrawLine(VIZ_BARS_BOX_X + i,  // starting x-coord
								 VIZ_BARS_BOX_Y + MAX_VIZ_BAR_SIZE - frequency_viz_bars[i], // starting y-coord
								 VIZ_BARS_BOX_X + i,  // ending x-coord
								 VIZ_BARS_BOX_Y + MAX_VIZ_BAR_SIZE + 1, // ending y-coord
								 MAROON); 
			}

			EndDrawing();
	}

	CloseWindow();
}


int main(){
    pthread_t tid[2];
		channel_create(MIDI_CHANNEL_SIZE, sizeof(MIDI_Event), &midi_channel);

		pthread_create(&(tid[0]), NULL, osc_run, NULL);
		pthread_create(&(tid[1]), NULL, visualize_frequencies, NULL);
	
		InitAudioDevice();
		SetAudioStreamBufferSizeDefault(MAX_SAMPLES_PER_UPDATE);
		AudioStream audio_stream = LoadAudioStream(SAMPLE_RATE, 32, 1);
		SetAudioStreamCallback(audio_stream, audio_input_callback);
		PlayAudioStream(audio_stream);

		main_loop();

		UnloadAudioStream(audio_stream);
		CloseAudioDevice();

		terminate = 1;

		pthread_join(tid[1], NULL);
		pthread_join(tid[0], NULL);

		channel_free(&midi_channel);

    return 0;
}
       
