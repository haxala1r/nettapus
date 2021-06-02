#include <syscall.h>
#include <tty.h>
uint64_t syscall_handler(uint64_t rax, uint64_t rbx) {
	kputs("Syscall ");
	kputx(rax);
	kputs("\n");
	return rbx;
}
