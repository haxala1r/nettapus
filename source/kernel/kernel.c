#include <stdint.h>
#include <stddef.h>





#if defined(__linux__)
#error "You ain't using a cross-compiler, adventurer. Come back when you have one."
#endif 

#if !defined(__i686__)
#error "Oi, this is supposed to be compiled with a i686 cross-compiler! Well I guess you *could* just comment this out, but... still."
#endif

//stuff the compiler automatically has.
#include <stdint.h>



//loads headers from /source/libk/include if used correctly with the cross-compiler.
//note that these are different from the libc you get when compiling anything else
//in the hosted environment, if there even is one.
#include <string.h>
#include <tty.h>
#include <mem.h>
#include <io.h>
#include <pci.h>
#include <ide.h>

void kernel_main();	//defining it here so that kernel_prep2 can call it.


void kernel_prep2(struct multiboot_header *mbh) {
	//this function does the initialisation of pretty much everything.
	//then calls the kernel.
	
	//init_memory starts everything memory-management related.
	//namely; PMM, VMM and the heap. This is done first because a lot of
	//things depend on it.
	if (init_memory(mbh)){
		init_terminal();
		terminal_puts("Memory initialisation failed.\n");
		while (1) {
			asm("hlt;");
		}
	}
	
	
	
	//why not add this while preparing everything. I'll probably swap this
	//out for proper initalisation of the file systems and a proper tty.
	init_terminal();	
	
	
	//PCI, IDE and shit
	pci_scan_all_buses();
	init_ide();
	
	
	
	
	
	//now we hand over the control of execution to the kernel, and we're done!
	//we have successfully entered higher half and enabled paging. Heap is complete too!
	kernel_main();
	
	
	//if, for some reason, kernel returns, we should probably halt it rather than letting it return.
	//in the future I'd probably like to put the code for shutdown here,
	//so that we can safely shut down if something happens, but that's a far away dream for now.
	
	while (1) {
		asm("hlt;");
	}
}

void __attribute__((section(".text.kernelprep"))) kernel_prep1()  {
	//this function 
	uint32_t temp_pdir[1024] __attribute__((aligned(4096)));
	uint32_t temp_table[1024] __attribute__((aligned(4096)));
	
	
	//these are pointers I'm defining for temporary use, as right now
	//we haven't really entered higher half, and using the functions as they are
	//will cause an error (because they behave as if in higher half.)
	void (*lpd)(uint32_t*) = (void (*)(uint32_t*))((uint32_t)loadPageDirectory - 0xC0000000);
	void (*ep)() = (void (*)())((uint32_t)enablePaging - 0xC0000000);
	
	//filling up the temporary directory and table
	for (size_t i = 0; i < 1024; i++) {
		temp_pdir[i] = 0x2;
	}

	for (uint32_t i = 0; i < 1024; i++) {
		temp_table[i] = (i * 0x1000) | 3;
	}

	//we need to map it twice because we haven't jumped to higher half yet,
	//which will be taken care of by boot.S, and the un-mapping of the first
	//table will be done by kernel_prep2().
	//I genuinely apologize if my code is too god-damn messy.
	temp_pdir[0] = ((uint32_t)temp_table) | 3;
	temp_pdir[768] = ((uint32_t)temp_table) | 3;

	lpd(temp_pdir);
	ep();
	
	
	
	return;	//the assembly code takes care of the transition between kernel_prep1() and kernel_prep2().
}





void kernel_main() {
	terminal_puts("Hello from the kernel! \n");
	
	
	ide_print_devs();
	
	
};


