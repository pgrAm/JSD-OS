#ifndef ISA_DMA_H
#define ISA_DMA_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum dma_mode {
	ISA_DMA_READ = 0x44,
	ISA_DMA_WRITE = 0x48
};

void isa_dma_begin_transfer(uint8_t channel, uint8_t mode, const uint8_t* buf, size_t length);
uint8_t* isa_dma_allocate_buffer(size_t size);
int isa_dma_free_buffer(uint8_t* buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif