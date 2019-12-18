typedef struct
{
	volatile size_t front;
	size_t back;
	size_t size;
	uint8_t* buffer;
}
ringbuf;

void ringbuf_write(ringbuf* r, uint8_t val)
{
	r->buffer[r->front] = val;
	
	r->front = (r->front + 1) % r->size;
}

int ringbuf_can_read(ringbuf* r)
{
	return r->back != r->front;
}

uint8_t ringbuf_blocking_read(ringbuf* r)
{
	while(r->back == r->front); //wait till we get another character
	
	uint8_t c = r->buffer[r->back];
	r->back = (r->back + 1) % r->size;
	
	return c;
}

uint8_t ringbuf_read(ringbuf* r)
{
	uint8_t c = 0xFF;
	
	if(r->back != r->front) 
	{
		c = r->buffer[r->back];
	
		r->back = (r->back + 1) % r->size;
	}
	
	return c;
}

ringbuf_create(ringbuf* r, size_t size)
{
	r->size = size;
	r->front = r->back = 0;
	r->buffer = (uint8_t*)malloc(size);
}

ringbuf_free(ringbuf* r)
{
	free(r->buffer);
}