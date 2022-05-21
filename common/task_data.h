#ifndef TASK_DATA
#define TASK_DATA

#ifdef __cplusplus
extern "C"
{
#endif

	typedef size_t task_id;

#define INVALID_TASK_ID (~(task_id)0x0)

#define WAIT_FOR_PROCESS 0x01
#ifdef __cplusplus
}
#endif

#endif