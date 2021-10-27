#ifndef RS232_H
#define RS232_H
#ifdef __cplusplus
extern "C" {
#endif
#include <string.h>

#define COM1 0x3f8 //port arguments for rs232_init
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

char rs232_read_byte(unsigned int port);
void rs232_init(unsigned int port, unsigned int rate);
int rs232_isclear(unsigned int port);
int rs232_data_available(unsigned int port);
void rs232_send_byte(unsigned int port, char b);
void rs232_send_string(unsigned int port, const char* c);
void rs232_send_string_len(unsigned int port, const char* b, size_t len);
#ifdef __cplusplus
}
#endif
#endif