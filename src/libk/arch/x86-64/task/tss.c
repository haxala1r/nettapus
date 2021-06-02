#include <task.h>
#include <err.h>
#include <string.h>
#include <mem.h>


/* These structs are pretty much only used here, so no need to put them in task.h */
struct gdt_entry {
	uint16_t limit0_15;
	uint16_t base0_15;
	uint8_t base16_23;
	uint8_t access;
	uint8_t limit16_19_flags; // lower four = limit 15-23, upper four = flags
	uint8_t base24_31;
	uint32_t base32_64;
	uint32_t reserved1;
} __attribute__((packed));

struct task_state_segment {
	uint32_t reserved1;
	uint32_t rsp0_low;
	uint32_t rsp0_high;
	uint32_t rsp1_low;
	uint32_t rsp1_high;
	uint32_t rsp2_low;
	uint32_t rsp2_high;
	uint32_t reserved2;
	uint32_t reserved3;
	uint32_t ist1_low;
	uint32_t ist1_high;
	uint32_t ist2_low;
	uint32_t ist2_high;
	uint32_t ist3_low;
	uint32_t ist3_high;
	uint32_t ist4_low;
	uint32_t ist4_high;
	uint32_t ist5_low;
	uint32_t ist5_high;
	uint32_t ist6_low;
	uint32_t ist6_high;
	uint32_t ist7_low;
	uint32_t ist7_high;
	uint32_t reserved4;
	uint32_t reserved5;
	uint16_t reserved6;
	uint16_t io_map_base;
} __attribute__((packed));



struct gdt_entry tss_entry;
struct task_state_segment *cur_tss = NULL;


extern void loadTSS(void *tss);

uint8_t load_tss(struct task_state_segment *t) {
	if (t == NULL) { return ERR_INVALID_PARAM; }

	memset(&tss_entry, 0, sizeof(tss_entry));

	/* Construct a proper entry here. */
	uintptr_t tss_addr = (uintptr_t)t;
	uint64_t tss_size = sizeof(*t) - 1;

	tss_entry.base0_15 = (uint16_t)(tss_addr & 0xFFFF);
	tss_entry.base16_23 = (uint8_t)((tss_addr >> 16) & 0xFF);
	tss_entry.base24_31 = (uint8_t)((tss_addr >> 24) & 0xFF);
	tss_entry.base32_64 = (uint32_t)(tss_addr >> 32);
	tss_entry.limit0_15 = tss_size & 0xFFFF;

	/* A lot of MAGIC!!!! */
	tss_entry.limit16_19_flags = (1 << 4) | ((tss_size >> 16) & 0x0F);
	tss_entry.access = 0x89;

	loadTSS(&tss_entry);
	return GENERIC_SUCCESS;
}

uint8_t init_tss() {
	lock_scheduler();
	if (cur_tss != NULL) {
		kfree(cur_tss);
	}

	cur_tss = kmalloc(sizeof(*cur_tss));
	if (cur_tss == NULL) {
		unlock_scheduler();
		return ERR_OUT_OF_MEM;
	}
	memset(cur_tss, 0, sizeof(*cur_tss));

	/* This is the "main" stack for syscalls. This stack is only used for
	 * a short time at the start of a syscall, right before the handler allocates
	 * another stack for itself. This is done in order to prevent various problems
	 * that would occur when two tasks try to use a syscall at the same time.
	 *
	 * TODO: Figure out how you're gonna actually "allocate" those stacks
	 * instead of being an all-talk dipshit. Perhaps another heap? or maybe
	 * an adjustment to the current one to allow the caller to request things
	 * like alignment etc. etc. (you want your stack to be aligned don't you?)
	 * an AVL tree is looking more and more like an excellent idea, the only
	 * downside being complexity.
	 */
	uintptr_t stack = ((uintptr_t)kmalloc(0x1000)) + 0x1000;
	cur_tss->rsp0_low = (uint32_t)(stack);
	cur_tss->rsp0_high = (uint32_t)(stack >> 32);

	load_tss(cur_tss);
	unlock_scheduler();
	return GENERIC_SUCCESS;
}
