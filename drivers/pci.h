#ifndef PCI_H
#define PCI_H

#define PCI_NONE 0xFFFF


#define PCI_VENDOR_ID            0x00 // 2
#define PCI_DEVICE_ID            0x02 // 2
#define PCI_COMMAND              0x04 // 2
#define PCI_STATUS               0x06 // 2
#define PCI_REVISION_ID          0x08 // 1

#define PCI_PROG_IF              0x09 // 1

#define PCI_BAR0                 0x10 // 4
#define PCI_BAR1                 0x14 // 4
#define PCI_BAR2                 0x18 // 4
#define PCI_BAR3                 0x1C // 4
#define PCI_BAR4                 0x20 // 4
#define PCI_BAR5                 0x24 // 4

#define PCI_SUBCLASS             0x0a // 1
#define PCI_CLASS                0x0b // 1

#define PCI_INTERRUPT_LINE       0x3C // 1
#define PCI_INTERRUPT_PIN        0x3D

struct pci_device
{
	uint8_t bus;
	uint8_t slot;
	uint8_t func;
	uint8_t padding;
};

typedef void (*pci_func)(pci_device device, uint16_t vendor_id, uint16_t device_id,
						 void* udata);

void pci_device_by_class(pci_func f, 
						 size_t device_class, 
						 size_t device_subclass, 
						 void* extra);

template<typename T> T pci_read(pci_device device, size_t field);

template<typename T> void pci_write(pci_device device, size_t field, T value);

#endif