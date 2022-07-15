#include <drivers/portio.h>
#include <kernel/interrupt.h>
#include <kernel/kassert.h>
#include <kernel/rt_device.h>
#include <kernel/locks.h>
#include <stdio.h>

#define DATA_PORT 0x60
#define STATUS_PORT 0x64
#define CMD_PORT 0x64

enum i8042_cmd : uint8_t
{
	READ_BYTE = 0x20, // + N to 0x3F
	WRITE_BYTE = 0x60, // + N to 0x7F
	DISABLE_PORT1 = 0xA7,
	ENABLE_PORT1 = 0xA8,
	TEST_PORT1 = 0xA9,
	TEST_CTRL = 0xAA,
	TEST_PORT0 = 0xAB,
	DIAGNOSTIC_DUMP = 0xAC,
	DISABLE_PORT0 = 0xAD,
	ENABLE_PORT0 = 0xAE,
	READ_CTRL_INPUT_PORT = 0xC0,
	READ_CTRL_OUTPUT_PORT = 0xD0,
	NEXT_BYTE_OUT_CTRL = 0xD1,
	NEXT_BYTE_FROM_PORT0 = 0xD2,
	NEXT_BYTE_FROM_PORT1 = 0xD3,
	NEXT_BYTE_TO_PORT1 = 0xD4
};

#define PORT1_DISABLE (1 << 5)

static size_t num_channels = 0;
static size_t first_channel = 0;

rt_device* devices[2];

static void i8042_wait_ready()
{
	size_t timeout = 10000;
	while(timeout--)
	{
		if(!(inb(STATUS_PORT) & 0x02))
			return;
	}
}

static bool i8042_wait_response()
{
	size_t timeout = 10000;
	while(timeout--)
	{
		if(inb(STATUS_PORT) & 0x01)
			return true;
	}
	return false;
}

static bool i8042_wait_response(uint8_t value)
{
	size_t timeout = 100;
	while(i8042_wait_response() && timeout--)
	{
		if(inb(DATA_PORT) == value)
		{
			return true;
		}
	}

	return false;
}

static void i8042_send_cmd(uint8_t cmd)
{
	i8042_wait_ready();
	outb(CMD_PORT, cmd);
}

static uint8_t i8042_read_cmd(uint8_t cmd)
{
	i8042_send_cmd(cmd);
	i8042_wait_response();
	return inb(DATA_PORT);
}

static void i8042_write_cmd(uint8_t cmd, uint8_t data)
{
	i8042_send_cmd(cmd);
	i8042_wait_ready();
	outb(DATA_PORT, data);
}

static void i8042_device_send(size_t channel, uint8_t data)
{
	if(channel > 0)
	{
		i8042_send_cmd(NEXT_BYTE_TO_PORT1);
	}
	i8042_wait_ready();
	outb(DATA_PORT, data);
}

static INTERRUPT_HANDLER void port0_handler(interrupt_frame* r)
{
	setup_segs();

	const uint8_t data = inb(DATA_PORT);
	acknowledge_irq(1);

	if(devices[0] && devices[0]->on_new_data)
	{
		devices[0]->on_new_data(devices[0], &data, 1);
	}
}

static INTERRUPT_HANDLER void port1_handler(interrupt_frame* r)
{
	setup_segs();

	const uint8_t data = inb(DATA_PORT);
	acknowledge_irq(12);

	if(devices[1] && devices[1]->on_new_data)
	{
		devices[1]->on_new_data(devices[1], &data, 1);
	}
}

static bool i8042_wait_ack()
{
	return i8042_wait_response(0xFA);
}

static int i8042_detect_device(size_t channel)
{
	irq_enable(1, false);
	irq_enable(12, false);

	i8042_device_send(channel, 0xF5);
	if(!i8042_wait_ack())
		return -1;

	i8042_device_send(channel, 0xF2); //identify
	if(!i8042_wait_ack())
		return -1;

	int id = 0;

	if(i8042_wait_response())
	{
		id = inb(DATA_PORT);
	}
	else
	{
		return -1;
	}

	if(i8042_wait_response())
	{
		id <<= 8;
		id |= inb(DATA_PORT);
	}

	i8042_device_send(channel, 0xF4);
	i8042_wait_ack();

	irq_enable(1, true);
	irq_enable(12, true);

	return id;
}

static void i8042_send_data(rt_device* device, const uint8_t* data, size_t length)
{
	auto channel = (size_t)device->impl_data;

	for(size_t i = 0; i < length; i++)
	{
		i8042_device_send(channel, data[i]);
	}
}

static void i8042_read_data(rt_device* device, uint8_t* data, size_t length)
{
	for(size_t i = 0; i < length; i++)
	{
		if(i8042_wait_response())
		{
			data[i] = inb(DATA_PORT);
		}
	}
}

extern "C" void i8042_init()
{
	i8042_send_cmd(DISABLE_PORT0);
	i8042_send_cmd(DISABLE_PORT1);

	//flush
	while(inb(STATUS_PORT) & 0x01)
	{
		inb(DATA_PORT);
	}

	//READ CONFIG
	uint8_t config = i8042_read_cmd(READ_BYTE + 0);

	if(!(config & PORT1_DISABLE))
	{
		num_channels = 1;
	}

	config &= ~0x03; //disable interrupts

	i8042_write_cmd(WRITE_BYTE + 0, config);

	if(num_channels != 1)
	{
		//try enabling port 1
		i8042_send_cmd(ENABLE_PORT1);

		//see if that was ignored
		config = i8042_read_cmd(READ_BYTE + 0);
		if(config & PORT1_DISABLE)
		{
			num_channels = 1;
		}
		else
		{
			num_channels = 2;
		}
	}

	if(i8042_read_cmd(TEST_CTRL) != 0x55)
		return;

	//restore config
	i8042_write_cmd(WRITE_BYTE + 0, config);

	if(i8042_read_cmd(TEST_PORT0) != 0)
	{
		first_channel = 1;
	}

	if(num_channels >= 2)
	{
		if(i8042_read_cmd(TEST_PORT1) != 0)
			num_channels = 1;
	}

	i8042_send_cmd(ENABLE_PORT0);
	config |= 0x01;
	//irq_install_handler(1, port0_handler);

	if(num_channels >= 2)
	{
		i8042_send_cmd(ENABLE_PORT1);
		config |= 0x02;
	}

	//reenable interrupts for working ports
	i8042_write_cmd(WRITE_BYTE + 0, config);

	//reset devices
	for(size_t i = first_channel; i < num_channels; i++)
	{
		i8042_device_send(i, 0xFF);
		i8042_wait_ack();
	}

	for(size_t i = first_channel; i < num_channels; i++)
	{
		int device = i8042_detect_device(i);
		if(device != -1)
		{
			devices[i] = new rt_device{device, i8042_read_data, i8042_send_data, nullptr, nullptr, (void*)i};

			add_realtime_device(devices[i]);

			printf("PS/2 channel %d : %X\n", i, device);
		}
	}
	
	if(num_channels >= 2)
	{
		irq_install_handler(12, port1_handler);
	}
}