#ifndef PCI_H
#define PCI_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <io.h>
#include <mem.h>


#define PCI_ADDRESS_PORT 0xcf8
#define PCI_VALUE_PORT	0xcfc



struct pci_device {
	uint32_t address;
	uint8_t header_type;
	uint32_t device_id, vendor_id;
	uint8_t class_code, subclass_code, prog_if, revision_id;
	uint32_t bar0, bar1, bar2, bar3, bar4, bar5;
	struct pci_device* next;	//it's a linked list.
};

typedef struct pci_device pci_device_t;		//it's a linked list.





uint32_t pci_get_address(uint8_t bus, uint8_t slot, uint8_t func);
uint32_t pci_read_field(uint32_t address, uint8_t offset);
void pci_write_field(uint32_t address, uint8_t offset, uint32_t value);
void pci_set_bar(uint32_t address, uint8_t bar, uint32_t value);

void pci_reg_device(uint8_t bus, uint8_t slot, uint8_t func);
void pci_check_function(uint8_t bus, uint8_t slot, uint8_t func);
void pci_scan_bus(uint8_t bus);
uint8_t pci_scan_all_buses();
pci_device_t* pci_get_first_dev();	//gets you the first device in the linked list.



#ifdef DEBUG

void pci_print_devs();

#endif

#ifdef __cplusplus
}
#endif

#endif
