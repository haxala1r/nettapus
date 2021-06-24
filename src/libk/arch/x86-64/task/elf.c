#include <task.h>
#include <string.h>
#include <mem.h>
#include <err.h>
#include <fs/fs.h>

char elf_magic[] = {'\x7f', 'E', 'L', 'F'};
/* This is the file where ELF files are loaded into memory. I decided the function
 * deserves its own file, because its so long.
 * This function loads an elf executable file and returns the address space.
 * It does not create a task or anything, just loads it.
 */
p_map_level4_table *load_elf(char *file_name, uintptr_t *entry_point) {
	if (file_name == NULL) { return NULL; }

	/* Might want to change this to use a fd-less approach. Or have an ELF loader
	 * kernel task.
	 */
	int32_t fd = kopen(file_name, 0);
	struct elf_hdr64 *hdr = kmalloc(sizeof(*hdr));
	if (fd < 0) { goto fail;}

	/* Read the entire elf header. On failure, return NULL. */
	if (kread(fd, hdr, 64) != 64) { goto fail; }

	/* It MUST be an ELF file. */
	if (memcmp(hdr->magic, elf_magic, 4)) { goto fail; }

	/* The file MUST be 64-bit. */
	if (hdr->word_size != 2) { goto fail; }

	/* Likewise, the file MUST use little-endian. */
	if (hdr->endianness != 1) { goto fail; }

	/* The file must be for the current version of ELF. */
	if (hdr->hdr_version != 1) { goto fail; }

	/* The file is simply not executable if the file doesnt have program headers*/
	if (hdr->phdr_entry_count == 0) { goto fail; }

	struct elf_phdr_entry entry;

	/* First, we need to check whether all entries are valid or not. Freeing a
	 * pml4t is very difficult, so we first check to ensure no errors happen
	 * rather than allocating then freeing the address space on error.
	 */
	for (size_t i = 0; i < hdr->phdr_entry_count; i++) {
		if (kread(fd, &entry, sizeof(entry)) != sizeof(entry)) {
			goto fail;
		}
		if (entry.segment_type == 0) {
			continue;
		}
		if (entry.vaddr >= 0xFFFFFF7000000000) {
			/* Invalid virtual addr. */
			goto fail;
		}
	}

	kseek(fd, 64);
	/* create_address_space() creates a blank address space with the kernel and
	 * the user/kernel stacks mapped.
	 */
	p_map_level4_table *pml4t = create_address_space();
	if (pml4t == NULL) {
		serial_puts("load_elf() could not create address space.\r\n");
		kpanic();
	}

	/* This next part has to happen atomically. */
	lock_scheduler();

	/* Now we can create the task. */
	for (size_t i = 0; i < hdr->phdr_entry_count; i++) {
		if (kread(fd, &entry, sizeof(entry)) != sizeof(entry)) {
			goto fail;
		}

		if (entry.segment_type == 1) {
			/* This indicates a LOAD segment. This is really the only one needed
			 * for a fully statically linked executable.
			 */

			/* Allocate the memory necessary. */
			size_t page_count = (entry.size_mem / 0x1000) + !!(entry.size_mem % 0x1000);
			size_t pp_base = allocpps(page_count);
			size_t vp_base = entry.vaddr / 0x1000;

			map_memory(pp_base * 0x1000, vp_base * 0x1000, page_count, pml4t, 1);
			/* temporarily map the same physical pages to the kernel PDPT at
			 * a predetermined address. see "doc/memory_map.txt" for info.
			 * TODO check page_count, and divide up if theres more than we can
			 * map. This is a serious bug, should be fixed.
			 * Since the address is in the kernel PDPT, we can access it without
			 * switching to pml4t.
			 */
			map_memory(pp_base * 0x1000, 0xFFFFFFFF98000000, page_count, pml4t, 0);
			loadPML4T(getCR3());

			char *mem_base = (char*)0xFFFFFFFF98000000;
			memset(mem_base, 0, entry.size_mem);

			/* We're going to read the data, don't lose our place. */
			size_t temp = ktell(fd);

			kseek(fd, entry.data_off);
			size_t red = kread(fd, mem_base, entry.size_file);
			if (red != entry.size_file) {
				serial_puts("[load_elf] Could not read bytes from file. Requested read: ");
				serial_putx(entry.size_file);
				serial_puts(" Amount actually read: ");
				serial_putx(red);
				serial_puts("\r\nFile name : ");
				serial_puts(file_name);
				serial_puts("\r\nPHDR index : ");
				serial_putx(i);
				serial_puts("\r\n");
				/* Don't return or anything, ignore the error. Attempting to
				 * recover from an error here would be very difficult.
				 */
			}

			kseek(fd, temp);

			unmap_memory(0xFFFFFFFF98000000, page_count, pml4t);
			loadPML4T(getCR3());
		} else {
			serial_puts("Encountered a non-recognized segment.\r\n");
			continue;
		}

	}

	unlock_scheduler();

	/* return */
	*entry_point = hdr->entry_addr;

	kfree(hdr);
	kclose(fd);

	return pml4t;
fail: /* Using a goto is cleaner than the alternative here. */
	kfree(hdr);
	kclose(fd);
	return NULL;
}
