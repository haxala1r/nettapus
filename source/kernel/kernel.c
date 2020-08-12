#include <stdint.h>
#include <stddef.h>





#if defined(__linux__)
#error "You ain't using a cross-compiler, adventurer. Come back when you have one."
#endif 

#if !defined(__i686__)
#error "Oi, this is supposed to be compiled with a i686 cross-compiler! Well I guess you *could* just comment this out, but... still."
#endif



//loads headers from /source/libk/include if used correctly with the cross-compiler.
//note that these are different from the libc you get when compiling anything else
//in the hosted environment, if there even is one.
#include <string.h>
#include <tty.h>
#include <stdint.h>
#include <mem.h>
#include <io.h>
#include <disk/atapio.h>




void kernel_main(struct multiboot_header* mbd) {
	init_terminal();
	init_memory(mbd);
	terminal_puts("Hello from the kernel! \nI have successfully entered protected mode, and I have full control over the system.\n");
	
	memory_map_t* mm = getPhysicalMem();
	char str[16];
	memset(str, 0, 16);
	//just prints all memory it could detect.
	for (size_t i = 0; i < mm->num_blocks; i++) {
		xtoa(mm->blocks[i].base_page, str);
		terminal_puts(str);
		terminal_putc(' ');
		
		xtoa(mm->blocks[i].length, str);
		terminal_puts(str);
		terminal_putc('\n');
	}
	
	
	//let's do a simple test to read sectors.
	//uint8_t* ptr = kmalloc(512);	//to store the read bytes.
	
	
	
	
	
	
	while (1) {
		asm("hlt;");
	}
}


