#ifndef TASK_DATA_H
#define TASK_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif
	typedef size_t task_id;

	typedef struct 
	{
		void* pointer;
		size_t image_size;
		size_t total_size;
		size_t alignment;
	} tls_image_data;
	
	typedef struct
	{
		task_id pid;
		tls_image_data tls;
	} process_info;

#define INVALID_TASK_ID (~(task_id)0x0)

#define WAIT_FOR_PROCESS 0x01
#ifdef __cplusplus
}
#endif

#endif