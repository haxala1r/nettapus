




#if defined(__linux__)
#error "You ain't using a cross-compiler, brave adventurer. Come back when you have one."
#endif 

#if !defined(__i686__)
#error "Oi, this is supposed to be compiled with a i686 cross-compiler! Well I guess you *could* just comment this out, but... still."
#endif

//stuff the compiler automatically has.
#include <stdint.h>
#include <stddef.h>




//loads headers from /source/libk/include if used correctly with the cross-compiler.
//note that these are different from the libc you get when compiling anything else
//in the hosted environment, if there even is one.
#include <string.h>
#include <tty.h>
#include <mem.h>
#include <io.h>
#include <pci.h>

#include <disk/ide.h>
#include <fs/fs.h>
#include <fs/ustar.h>
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

	
	/* now we initialise literally everything.
	 * This part simply calls the initialisation function of every part of the OS.
	 * I might want to replace this with a more... systematic approach.
	 * because currently replacing a driver with another one is kind of problematic.
	 * IDEA:
	 *		I can try to do modules. After the disk I/O and filesystem drivers are in place, we really
	 * 		have no reason to try and tuck every single driver into the kernel.
	 * 		This would let us get creative with some specific parts of the drivers
	 * 		and, more importantly, allow us to test the OS without having to re-compile
	 * 		the kernel everytime. It's going to be a bit hard to implement that though,
	 * 		so it's probably going to be done much later into development.
	 * 		
	 * 		Since I already have a way of knowing at which address exactly the kernel ends,
	 * 		this could be relatively simple, after I add support for at least a single
	 * 		file format.
	 * 
	 */
	init_terminal();
	
	uint8_t status = 0;	//this will be used to store the status for initialisation functions.
	
	
	//PCI.
	status = pci_scan_all_buses(); 
	if (status) {
		terminal_puts("[FATAL] PCI couldn't initalise properly. Halting process.\n");
		while (1) {
			asm("hlt;");
		}
	} else {
		terminal_puts("[OK] PCI initalised correctly.\n");
	}
	
	
	//IDE
	status = init_ide();
	if (status == 1) {
		terminal_puts("[FATAL] Could not find any ATA PIO compatible buses. Halting process.\n");
		while (1) {
			asm("hlt;");
		}
	} else if (status == 2) {	
		terminal_puts("[FATAL] Could not find any ATA PIO compatible drives. Halting process.\n");
		while (1) {
			asm("hlt;");
		}
	} else {
		terminal_puts("[OK] IDE drivers initalised correctly.\n");
	}
	
	
	//FS
	status = fs_init();
	if (status == 0xFF) {
		terminal_puts("[FATAL] FS drivers initialised correctly, but no supported FS was found. Halting process.\n");
		while (1) {
			asm("hlt;");
		}
	} else if (status) {
		terminal_puts("[FATAL] FS drivers could not be initialised.\n");
		while (1) {
			asm("hlt;");
		}
	} else {
		terminal_puts("[OK] FS drivers initialised correctly.\n");
	}
	
	
	
	
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
	terminal_puts("Welcome to Nettapus!\n");
	
	
	//This is some test code to read a file called "hello" and print its contents.
	file_system_t* root = fs_get_root();
	
	char* buf = kmalloc(512);	//a whole sector cuz why not.
	memset(buf, 0, 512);
	
	if (ustar_read_file(root, "hello", (void*)buf, 0, 0x60)) {
		terminal_puts("Error!\n");
		return;
	}
	terminal_puts(buf);
};


