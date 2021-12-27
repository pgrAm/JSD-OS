#ifndef INPUT_EVENT
#define INPUT_EVENT

#include <time.h>

enum event_type
{
	BUTTON_DOWN = 0,
	BUTTON_UP,
	AXIS_MOTION
};

typedef enum event_type event_type;

struct input_event
{
	size_t device_index;
	size_t control_index;
	int data;
	event_type type;
	clock_t time_stamp;
};

typedef struct input_event input_event;

#endif