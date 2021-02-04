
#include <interrupts.h>
#include <tty.h>

void divide_by_zero_handler() {
	kputs("divide by zero biiitch.");
};


void double_fault_handler() {
	kputs("double fault biiitch.");
};




