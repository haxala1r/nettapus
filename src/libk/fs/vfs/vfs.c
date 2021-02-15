
#include <fs/fs.h>
#include <mem.h>
#include <task.h>

#include <fs/ustar.h>
#include <fs/fat16.h>


int32_t seek(Task *task, int32_t file_des, uint64_t new_pos) {
	if (task == NULL) { return -1; };
	
	FILE* f = vfs_fd_lookup(task, file_des);
	
	if (!(new_pos < f->node->last)) { return -1; }
	
	f->position = new_pos;
	return 0;
};


int32_t kseek(int32_t file_des, uint64_t new_pos) {
	return seek(get_current_task(), file_des, new_pos);
};
