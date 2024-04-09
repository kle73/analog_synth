#include <unistd.h>
#include <stdio.h>

#include "channel/channel.h"
#include "osc/osc.h"
#include "fft/fft.h"

#include "../include/raylib.h"


#define FFT_SIZE 4096
#define MIDI_CHANNEL_SIZE 100
#define DRAWABLE_SIZE 256

typedef enum{
	NOTE_ON,
	NOTE_OFF
}Event_Type;

typedef struct {
	float frequency;
	float amplitude;
	Event_Type type;
}MIDI_Event;

typedef enum{
	ON,
	OFF
}Switch_State;

typedef struct {
	float x_last;
	float y_last;
	float alpha;
	Switch_State s;
}filter_t;
// stores all currently playing frequencies
static float frequencies[MAX_SIM_NOTES] = {-1.0f};

//stores current phase for all playing frequencies
static float phases[MAX_SIM_NOTES] = {0.0f};

//stores the current waveform (sine, triangle, ...)
static float (*oscillate)(float freq, float* phase) = sine_wave;


static KeyboardKey keys[13] = {KEY_S, KEY_E, KEY_D, KEY_R, KEY_F, KEY_G, KEY_Y, 
												KEY_H, KEY_U, KEY_J, KEY_I, KEY_K, KEY_L};
static int is_pressed[13] = {0};

//stores fft visualization results, to be used by window 
static float drawables[DRAWABLE_SIZE] = {0.0f};

static filter_t lp_filter = {0, 0, 0.01, OFF};

channel_t midi_channel;


// MIDI CHANNEL
// ---------------------------------------------------------------------------

void send_midi_data(MIDI_Event* event, channel_t* chn){
	while (channel_send((void*)event, chn) < 0);
}

void receive_midi_data(MIDI_Event* event, channel_t* chn){
	while (channel_rcv((void*) event, chn) < 0);
}

// FILTER
// -------------------------------------------------------------------------------


void filter(void* buf, unsigned int frames){
	if (lp_filter.s == OFF) return;
	float* buffer = (float*)buf;	
	for (int i=0; i<frames; i++){
		float x_last = buffer[i];
		buffer[i] = (1-lp_filter.alpha)*lp_filter.y_last + 
								lp_filter.alpha*(buffer[i] + lp_filter.x_last)/2;
		lp_filter.x_last = x_last;
		lp_filter.y_last = buffer[i];
	}
}

// AMPLIFIER
// -------------------------------------------------------------------------------
typedef struct {

}amp_t;

void amplify(void* buf, unsigned int frames){
}


// OSCILLATOR
// ----------------------------------------------------------------------------
void AudioInputCallback(void* buffer, unsigned int frames){
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
	filter(buffer, frames);
	amplify(buffer, frames);
}


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
	}
	return NULL;
}

void reset(){
	for (int i=0; i<MAX_SIM_NOTES; i++){
		frequencies[i] = -1.0f;
		phases[i] = 0.0f;
	}
}

// KEYBOARD
// ---------------------------------------------------------------------------
void enumerate_keys(){
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

// VISUALIZATIONS
// ----------------------------------------------------------------------------

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

    minfft_realdft(in, out, a);
		int step = (FFT_SIZE/2)/DRAWABLE_SIZE;
		float acc;
		for (int i=0; i<FFT_SIZE/2; i+=step){
			acc = 0;
			for (int k=i; k<i+step; k++){
				acc += sqrt(out[k][0]*out[k][0] + out[k][1]*out[k][1]);
			}
			acc /= step;
			drawables[i/step] = acc;
		}
	}
	return NULL;
}

// INPUT LOOP
// ---------------------------------------------------------------------------
typedef struct {

}input_t;

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 450

void setup(){
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "analog_synth");
    SetTargetFPS(60); 
		channel_create(MIDI_CHANNEL_SIZE, sizeof(MIDI_Event), &midi_channel);
}

void cleanup(){
		channel_free(&midi_channel);
		CloseWindow();
}

void input_loop(){
	setup();
	while(!WindowShouldClose())
	{
			enumerate_keys();
			BeginDrawing();
			ClearBackground(RAYWHITE);
			
			// Draw freq visualization section:
			for (int i=0; i<DRAWABLE_SIZE; i++){
				DrawLine(100+i, 400-drawables[i],
									100+i, 401, MAROON);
			}

			EndDrawing();
	}
	cleanup();
}

// MAIN 
// -------------------------------------------------------------------------------

int main(){
    pthread_t tid[2];
		pthread_create(&(tid[0]), NULL, osc_run, NULL);
		pthread_create(&(tid[1]), NULL, visualize_frequencies, NULL);
	
		InitAudioDevice();
		SetAudioStreamBufferSizeDefault(MAX_SAMPLES_PER_UPDATE);
		AudioStream audio_stream = LoadAudioStream(SAMPLE_RATE, 32, 1);
		SetAudioStreamCallback(audio_stream, AudioInputCallback);
		PlayAudioStream(audio_stream);

		input_loop();

		UnloadAudioStream(audio_stream);
		CloseAudioDevice();

    return 0;
}
       
