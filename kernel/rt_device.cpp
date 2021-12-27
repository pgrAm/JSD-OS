#include <vector>

#include "rt_device.h"

static std::vector<rt_device*> devices;

rt_device* find_realtime_device(int class_ID)
{
	for(auto* d : devices)
	{
		if(d->class_ID == class_ID)
			return d;
	}

	return nullptr;
}

void add_realtime_device(rt_device* d)
{
	devices.push_back(d);
}

void remove_realtime_device(rt_device* d)
{
}
