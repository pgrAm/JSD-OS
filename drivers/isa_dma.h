#ifndef ISA_DMA_H
#define ISA_DMA_H

#include <string.h>

void isa_dma_begin_transfer(uint8_t channel, uint8_t mode, uint8_t* buf, size_t length);

#endif