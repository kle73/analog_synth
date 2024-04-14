#ifndef FILTER_H
#define FILTER_H

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

void no_filter(void* buf, unsigned int frames, float alpha);
void high_pass_filter(void* buf, unsigned int frames, float alpha);

#endif
