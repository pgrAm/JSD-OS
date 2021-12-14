#include <stdint.h>
#include <stdio.h>
#include <drivers/portio.h>

#include "pci.h" 

#define PCI_CACHE_LINE_SIZE      0x0c // 1
#define PCI_LATENCY_TIMER        0x0d // 1
#define PCI_HEADER_TYPE          0x0e // 1
#define PCI_BIST                 0x0f // 1

#define PCI_SECONDARY_BUS        0x19 // 1

#define PCI_HEADER_TYPE_DEVICE  0
#define PCI_HEADER_TYPE_BRIDGE  1
#define PCI_HEADER_TYPE_CARDBUS 2

#define PCI_ADDRESS_PORT 0xCF8
#define PCI_VALUE_PORT   0xCFC

static void pci_scan_bus(pci_func f, size_t d_class, size_t d_subclass, size_t bus, void* udata);

static inline constexpr uint32_t pci_get_addr(pci_device device, size_t field)
{
	return 0x80000000 
		| (device.bus << 16) 
		| (device.slot << 11) 
		| (device.func << 8) 
		| ((field) & 0xFC);
}

template<> void pci_write<uint32_t>(pci_device device, size_t field, uint32_t value)
{
	outd(0xCF8, pci_get_addr(device, field));
	outd(0xCFC, value);
}

template<> void pci_write<uint16_t>(pci_device device, size_t field, uint16_t value)
{
	outd(0xCF8, pci_get_addr(device, field));
	outw(0xCFC + (field & 3), value);
}

template<> void pci_write<uint8_t>(pci_device device, size_t field, uint8_t value)
{
	outd(0xCF8, pci_get_addr(device, field));
	outb(0xCFC + (field & 3), value);
}

template<> uint32_t pci_read<uint32_t>(pci_device device, size_t field)
{
	outd(0xCF8, pci_get_addr(device, field));
	return ind(0xCFC);
}

template<> uint16_t pci_read<uint16_t>(pci_device device, size_t field)
{
	outd(0xCF8, pci_get_addr(device, field));
	return (uint16_t)((ind(0xCFC) >> ((field & 2) * 8)) & 0xFFFF);
}

template<> uint8_t pci_read<uint8_t>(pci_device device, size_t field)
{
	outd(0xCF8, pci_get_addr(device, field));
	return (uint8_t)((ind(0xCFC) >> ((field & 3) * 8)) & 0xFF);
}

static bool pci_check_type(pci_device device, size_t d_class, size_t d_subclass)
{
	return	(pci_read<uint8_t>(device, PCI_CLASS) == d_class) &&
			(pci_read<uint8_t>(device, PCI_SUBCLASS) == d_subclass);
}

static void pci_scan_func(pci_func f, size_t d_class, size_t d_subclass, size_t bus, size_t slot, size_t func, void* udata)
{
	auto device = pci_device{(uint8_t)bus, (uint8_t)slot, (uint8_t)func};
	if((d_class == 0xFF && d_subclass == 0xFF) ||
	   pci_check_type(device, d_class, d_subclass))
	{
		f(device,
		  pci_read<uint16_t>(device, PCI_VENDOR_ID),
		  pci_read<uint16_t>(device, PCI_DEVICE_ID),
		  udata);
	}

	//pci bridge
	if(pci_check_type(device, 0x06, 0x04))
	{
		pci_scan_bus(f, d_class, d_subclass, 
					 pci_read<uint8_t>(device, PCI_SECONDARY_BUS), udata);
	}
}

static void pci_scan_slot(pci_func f, size_t d_class, size_t d_subclass, size_t bus, size_t slot, void* udata)
{
	auto device = pci_device{(uint8_t)bus, (uint8_t)slot, 0};
	if(pci_read<uint16_t>(device, PCI_VENDOR_ID) == PCI_NONE)
	{
		return;
	}
	pci_scan_func(f, d_class, d_subclass, bus, slot, 0, udata);
	if(!pci_read<uint8_t>(device, PCI_HEADER_TYPE))
	{
		return;
	}
	for(size_t func = 1; func < 8; func++)
	{
		auto device = pci_device{(uint8_t)bus, (uint8_t)slot, (uint8_t)func};
		if(pci_read<uint16_t>(device, PCI_VENDOR_ID) != PCI_NONE)
		{
			pci_scan_func(f, d_class, d_subclass, bus, slot, func, udata);
		}
	}
}

static void pci_scan_bus(pci_func f, size_t d_class, size_t d_subclass, size_t bus, void* udata)
{
	for(size_t slot = 0; slot < 32; ++slot)
	{
		pci_scan_slot(f, d_class, d_subclass, bus, slot, udata);
	}
}

static bool pci_scan_funcs(pci_func f, size_t d_class, size_t d_subclass, void* udata)
{
	for(size_t func = 0; func < 8; func++)
	{
		auto d = pci_device{0, 0, (uint8_t)func};

		if(pci_read<uint16_t>(d, PCI_VENDOR_ID) == PCI_NONE)
			return false;

		pci_scan_bus(f, d_class, d_subclass, func, udata);
	}
	return true;
}

void pci_device_by_class(pci_func f, size_t d_class, size_t d_subclass, void* udata)
{
	auto device0 = pci_device{0, 0, 0};

	//if this device is multifunction scan those functions
	if(pci_read<uint8_t>(device0, PCI_HEADER_TYPE) & 0x80)
	{
		if(!pci_scan_funcs(f, d_class, d_subclass, udata))
		{
			for(size_t bus = 0; bus < 256; bus++)
			{
				for(size_t slot = 0; slot < 32; slot++)
				{
					pci_scan_slot(f, d_class, d_subclass, bus, slot, udata);
				}
			}
		}
	}
	else
	{
		pci_scan_bus(f, d_class, d_subclass, 0, udata);
	}
}
