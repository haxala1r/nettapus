#include <err.h>
#include <tty.h>

int64_t print_stat(int64_t stat) {
	switch (stat) {
	case GENERIC_SUCCESS:
		kputs("OK");
		break;
	case ERR_INVALID_PARAM:
		kputs("INVALID PARAMETER WAS GIVEN");
		break;
	case ERR_INCOMPAT_PARAM:
		kputs("INCOMPATIBLE PARAMETER WAS GIVEN");
		break;
	case ERR_NOT_FOUND:
		kputs("REQUESTED OBJECT NOT FOUND");
		break;
	case ERR_NO_RESULT:
		kputs("NO VALID RESULT WAS FOUND");
		break;
	case ERR_DISK:
		kputs("A DISK ERROR OCCURED");
		break;
	default:
		kputs("UNKNOWN ERROR");
		break;
	}
	return stat;
};

