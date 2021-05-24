#include <err.h>

int64_t print_stat(int64_t stat) {
	switch (stat) {
	case GENERIC_SUCCESS:
		serial_puts("OK");
		break;
	case ERR_INVALID_PARAM:
		serial_puts("INVALID PARAMETER WAS GIVEN");
		break;
	case ERR_INCOMPAT_PARAM:
		serial_puts("INCOMPATIBLE PARAMETER WAS GIVEN");
		break;
	case ERR_NOT_FOUND:
		serial_puts("REQUESTED OBJECT NOT FOUND");
		break;
	case ERR_NO_RESULT:
		serial_puts("NO VALID RESULT WAS FOUND");
		break;
	case ERR_DISK:
		serial_puts("A DISK ERROR OCCURED");
		break;
	default:
		serial_puts("UNKNOWN ERROR");
		break;
	}
	return stat;
}

extern void kpanic();

struct source_location {
	char *file;
	uint32_t line;
	uint32_t column;
};

struct type_descriptor {
	uint16_t kind;
	uint16_t info;
	char name[];
};

struct type_mismatch_data {
	struct source_location loc;
	struct type_descriptor *typedes;
	uint8_t alignment;
	uint8_t type_check_kind;
};

char *type_check_kinds[] = {
    "load of", "store to", "reference binding to", "member access within",
    "member call on", "constructor call on", "downcast of", "downcast of",
    "upcast of", "cast to virtual base of", "_Nonnull binding to",
    "dynamic operation on"
};

void __ubsan_handle_pointer_overflow() {
	__asm__("cli;hlt;");
}

void __ubsan_handle_out_of_bounds() {
	__asm__("cli;hlt;");
}

void mismatch_nullptr() {
	__asm__("cli;hlt;");
}

void mismatch_misaligned() {
	__asm__("cli;hlt;");
}

void __ubsan_handle_type_mismatch_v1(struct type_mismatch_data *data, uintptr_t ptr) {
	if (ptr == 0) {
		/* NULL pointer access.*/
		serial_puts("UBSAN : NULL pointer access. \r\n");
	} else if (data->alignment && (ptr & (data->alignment - 1))) {
		serial_puts("UBSAN : Misaligned address access. \r\n");
	} else {
		serial_puts("UBSAN : Insufficient size\r\nTYPE: ");
		serial_puts(data->typedes->name);
		serial_puts(" CHECK KIND: '");
		serial_puts(type_check_kinds[data->type_check_kind]);
		serial_puts("' at addr ");
		serial_putx(ptr);
		serial_puts("\r\n");
	}
	serial_puts("SOURCE: in ");
	serial_puts(data->loc.file);
	serial_puts(" at line ");
	serial_putx(data->loc.line);
	serial_puts(" at column ");
	serial_putx(data->loc.column);
	serial_puts("\r\n");
	kpanic();
}

void __ubsan_handle_add_overflow() {
	__asm__("cli;hlt;");
}

void __ubsan_handle_sub_overflow() {
	__asm__("cli;hlt;");
}

void __ubsan_handle_mul_overflow() {
	__asm__("cli;hlt;");
}

void __ubsan_handle_divrem_overflow() {
	__asm__("cli;hlt;");
}

void __ubsan_handle_shift_out_of_bounds() {
	__asm__("cli;hlt;");
}

void __ubsan_handle_negate_overflow() {
	__asm__("cli;hlt;");
}
