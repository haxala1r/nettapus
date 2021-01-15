
#include <pci.h>

pci_device_t* pci_first_device;

pci_device_t* pci_get_first_dev() {
	return pci_first_device;
};

uint32_t pci_get_address(uint8_t bus, uint8_t slot, uint8_t func) { 
	uint32_t lbus = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;
	
	return ((lbus << 16) | (lslot << 11) | (lfunc << 8) | 0x80000000);
}


uint32_t pci_read_field(uint32_t address, uint8_t offset) {
	outl(PCI_ADDRESS_PORT, address | (offset & 0xfc));
	return inl(0xcfc);
}


void pci_write_field(uint32_t address, uint8_t offset, uint32_t value) {
	outl(PCI_ADDRESS_PORT, address | (offset & 0xfc));
	outl(PCI_VALUE_PORT, value);
};

//stuff to set BARs

void pci_set_bar(uint32_t address, uint8_t bar, uint32_t value) {
	pci_write_field(address, (0x10 + 4 * bar), value);
}



//some stuff to retrieve regularly used information.

uint16_t pci_get_vendor_id(uint32_t address) {
	return (pci_read_field(address, 0)) & 0xFFFF;
};


uint16_t pci_get_device_id(uint32_t address) {
	return (pci_read_field(address, 0) >> 16) & 0xFFFF;
};




uint8_t pci_get_header_type(uint32_t address) {
	return (pci_read_field(address, 0xc) >> 16) & 0xFF;
};


uint8_t pci_is_multi_function(uint32_t address) {
	//returns 1 if device is multifunction. made solely to improve readability.
	return (pci_get_header_type(address) >> 7);
}


uint8_t pci_get_class_code(uint32_t address) {
	return (pci_read_field(address, 8) >> 24) & 0xFF;
};


uint8_t pci_get_subclass_code(uint32_t address) {
	return (pci_read_field(address, 8) >> 16) & 0xFF;
};


uint8_t pci_get_prog_if(uint32_t address) {
	return (pci_read_field(address, 8) >> 8) & 0xFF;
};




uint16_t pci_get_command(uint32_t address) {
	return (pci_read_field(address, 4)) & 0xFFFF;
};


uint16_t pci_get_status(uint32_t address) {
	return (pci_read_field(address, 4) >> 16) & 0xFFFF;
};




uint8_t pci_get_secondary_bus(uint32_t address) {
	return (pci_read_field(address, 0x18) >> 8) & 0xFF;
};



//some experimental stuff I suppose.
void pci_reg_device(uint8_t bus, uint8_t slot, uint8_t func) {
	//registers a device.
	uint32_t address = pci_get_address(bus, slot, func);
	
	uint16_t vendor_id = pci_get_vendor_id(address);
	if (vendor_id == 0xFFFF) {
		//invalid vendor id.
		//this also means the device doesn't exist.
		return;
	}
	pci_device_t* dev = kmalloc(sizeof(pci_device_t));
	dev->address = address;
	dev->header_type = pci_get_header_type(address);
	dev->vendor_id = vendor_id;
	dev->device_id = pci_get_device_id(address);
	dev->class_code = pci_get_class_code(address);
	dev->subclass_code = pci_get_subclass_code(address);
	dev->prog_if = pci_get_prog_if(address);
	dev->revision_id = (pci_read_field(address, 8) & 0xFF);
	dev->bar0 = pci_read_field(address, 10);
	dev->next = NULL;
	
	//let us do the 'bar's now.
	if ((dev->header_type & 0x7f) == 0) {
		
		dev->bar0 = pci_read_field(address, 0x10);
		dev->bar1 = pci_read_field(address, 0x14);
		dev->bar2 = pci_read_field(address, 0x18);
		dev->bar3 = pci_read_field(address, 0x1C);
		dev->bar4 = pci_read_field(address, 0x20);
		dev->bar5 = pci_read_field(address, 0x24);
	} else if ((dev->header_type & 0x7f) == 1) {
		//PCI-TO-PCI BRIDGE
		dev->bar0 = pci_read_field(address, 0x10);
		dev->bar1 = pci_read_field(address, 0x14);
	} 
	
	
	
	//if there's no device registered yet, then we gotta start somewhere,
	//right?
	if (pci_first_device == NULL) {
		pci_first_device = dev;
		return;
	}
	
	//then we gotta find the last element of the linked list, and add this
	//new device to the end.
	pci_device_t* i = pci_first_device;	
	while (1) {
		if (i->next == NULL) {
			break;
		}
		i = i->next;
		
	}
	
	//now we add it...
	i->next = dev;
	//and done.
};

void pci_check_function(uint8_t bus, uint8_t slot, uint8_t func) {
	uint32_t address = pci_get_address(bus, slot, func);
	
	pci_reg_device(bus, slot, func);
	
	uint8_t class = pci_get_class_code(address);
	uint8_t subclass = pci_get_subclass_code(address);
	
	if ((class == 0x6) && (subclass == 0x4)) {
		uint8_t second_bus = pci_get_secondary_bus(address);
		pci_scan_bus(second_bus);
	}
};



void pci_scan_bus(uint8_t bus) {
	uint8_t slot = 0;
	for (; slot < 32; slot++) {
		uint32_t address = pci_get_address(bus, slot, 0);
		uint16_t header_type = pci_get_header_type(address);
		
		if (header_type & 0x80) {
			//this basically means it's a multifunction device if we reach here.
			for (uint8_t func = 0; func < 8; func++){
				pci_check_function(bus, slot, func);
			}
		} else {
			//if it returns false, then it's a single-function device
			pci_check_function(bus, slot, 0);
		}
		
	};
};


uint8_t pci_scan_all_buses() {
	
	uint8_t header_type = pci_get_header_type(pci_get_address(0, 0, 0));
	if (header_type & 0x80) {
		//multi-host controller
		for (uint8_t func = 0; func < 8; func++) {
			if ((pci_get_vendor_id(pci_get_address(0, 0, func))) == 0xFFFF ) {
				continue;
			}
			pci_scan_bus(func);
		};
		return 1;
	} else {
		//single-host controller
		pci_scan_bus(0);
		return 0;
	}
	
};



