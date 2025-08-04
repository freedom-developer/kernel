#ifndef _WSB_PROCFS_H
#define _WSB_PROCFS_H

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define PROC_FT_INT 1
#define PROC_FT_STR 2
#define PROC_FT_OTH 3

struct proc_dir_entry *proc_mkfile_single(const char *name, umode_t mode, struct proc_dir_entry *parent, 
                                    int type, void *data, int (*_show)(struct seq_file *, void *));
#define proc_mkfile_single_i(name, parent, data) \
    proc_mkfile_single((name), 0, (parent), PROC_FT_INT, (data), NULL)
#define proc_mkfile_single_s(name, parent, data) \
    proc_mkfile_single((name), 0, (parent), PROC_FT_STR, (data), NULL)
#define proc_mkfile_single_o(name, parent, data, show) \
    proc_mkfile_single((name), 0, (parent), PROC_FT_OTH, (data), (show))


struct proc_dir_entry *proc_mkfile_array(const char *name, umode_t mode, struct proc_dir_entry *parent,
                                    int type, void *data, int nr, int ele_size,
                                    int (*show)(struct seq_file *, void *v));
#define proc_mkfile_array_i(name, parent, data, nr) \
    proc_mkfile_array((name), 0, (parent), PROC_FT_INT, (data), (nr), sizeof(int), NULL)
#define proc_mkfile_array_s(name, parent, data, nr) \
    proc_mkfile_array((name), 0, (parent), PROC_FT_STR, (data), (nr), sizeof(char *), NULL)
#define proc_mkfile_array_o(name, parent, data, nr, ele_size, show) \
    proc_mkfile_array((name), 0, (parent), PROC_FT_OTH, (data), (nr), ele_size, show)

void proc_remove_array(struct proc_dir_entry *pde);

#endif