#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/list.h>

#include "procfs.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wushengbang");
MODULE_DESCRIPTION("A simple Linux kernel module");
MODULE_VERSION("0.1");

typedef int (*wsb_single_show_t)(struct seq_file *, void *);

struct proc_dir_entry *proc_mkfile(const char *name, struct proc_dir_entry *parent, int *data);

/////////// 一个元素的读
// 一个整数的读
static int wsb_single_show_i(struct seq_file *m, void *v)
{
    seq_printf(m, "%d\n", *(int *)m->private);
    return 0;
}
// 一个字符串的读
static int wsb_single_show_s(struct seq_file *m, void *v)
{
    seq_printf(m, "%s\n", (const char *)m->private);
    return 0;
}

struct proc_dir_entry *proc_mkfile_single(const char *name, umode_t mode, struct proc_dir_entry *parent, 
                                    int type, void *data, int (*_show)(struct seq_file *, void *))
{
    wsb_single_show_t show = _show;
    if (!show)
        show = type == PROC_FT_INT ? wsb_single_show_i : wsb_single_show_s;
    return proc_create_single_data(name, mode, parent, show, data);
}
///// end: 一个元素的读

////////// 数组元素的读
typedef struct pde_data_array_s {
    void *data;
    int nr;
    int type;
    int ele_size;
    int (*show)(struct seq_file *m, void *v);

    struct proc_dir_entry *pde;
    struct list_head list;
} pde_data_array_t;

#define SEQ2DATA(m) (m)->file->f_inode->i_private

static void *pda_start(struct seq_file *m, loff_t *ppos)
{
    pde_data_array_t *pda = (pde_data_array_t *)SEQ2DATA(m);
    if (*ppos >= pda->nr)
        return NULL;
    return pda->data + pda->ele_size * (*ppos);
}
static void *pda_next(struct seq_file *m, void *v, loff_t *ppos)
{
    *ppos++;
    pde_data_array_t *pda = (pde_data_array_t *)SEQ2DATA(m);
    if (*ppos >= pda->nr)
        return NULL;
    return pda->data + pda->ele_size * (*ppos);
}
static void pda_stop(struct seq_file *m, void *v) { }
static int pda_show(struct seq_file *m, void *v)
{
    pde_data_array_t *pda = (pde_data_array_t *)SEQ2DATA(m);
    void *last_data = pda->data + (pda->nr - 1) * pda->ele_size;
    
    if (pda->show) {
        int ret = pda->show(m, v);
        if (ret)
            return ret;
        if (last_data = v)
            seq_printf(m, "\n");
        return 0;
    }
    if (pda->type == PROC_FT_INT) {
        seq_printf(m, "%d", *(int *)v);
    } else {
        seq_printf(m, "%s", (const char *)v);
    }
    if (v != last_data) {
        seq_printf(m, ",\t");
    }
    return 0;
}

static const struct seq_operations pde_data_array_ops = {
    .start = pda_start,
    .next = pda_next,
    .stop = pda_stop,
    .show = pda_show,
};

LIST_HEAD(pda_head);
struct proc_dir_entry *proc_mkfile_array(const char *name, umode_t mode, struct proc_dir_entry *parent,
                                    int type, void *data, int nr, int ele_size, 
                                    int (*show)(struct seq_file *, void *))
{
    pde_data_array_t *pda = kmalloc(sizeof(pde_data_array_t), GFP_KERNEL);
    if (!pda)
        return ERR_PTR(ENOMEM);
    pda->data = data;
    pda->nr = nr;
    pda->type = type;
    pda->ele_size = ele_size;
    pda->show = show;
    INIT_LIST_HEAD(&pda->list);

    struct proc_dir_entry *pde = proc_create_seq_data(name, mode, parent, &pde_data_array_ops, pda);
    if (!pde) {
        kfree(pda);
        return ERR_PTR(ENOMEM);
    }
    pda->pde = pde;
    list_add_tail(&pda->list, &pda_head);

    return pde;
}

void proc_remove_array(struct proc_dir_entry *pde)
{
    pde_data_array_t *pda;
    int found = 0;
    list_for_each_entry(pda, &pda_head, list) {
        if (pda->pde == pde) {
            found = 1;
            break;
        }
    }
    if (found) {
        list_del(&pda->list);
        kfree(pda);
    }
    proc_remove(pde);
}
///// end: 数组元素的读




//////////////// 测试 ////////////
struct proc_dir_entry *test, *test_arr;
struct stu {
    char name[16];
    int age;
};
struct stu wsb;

int da[] = {100, 2, 200, 3, 5, 6, 99, 110};


int stu_show(struct seq_file *m, void *v)
{
    struct stu *s = (struct stu *)m->private;
    seq_printf(m, "name: %s,\tage: %d\n", s->name, s->age);
    return 0;
}

static int __init procfs_init(void) 
{    
    strncpy(wsb.name, "wushengbang", 16);
    wsb.age = 39;
    test = proc_mkfile_single_o("test", NULL, &wsb, stu_show);
    if (!test) 
        return -1;
    test_arr = proc_mkfile_array_i("test_arr", NULL, da, sizeof(da) / sizeof(int));
    if (!test_arr)
        return -2;
    
    return 0;
}

static void __exit procfs_exit(void) 
{
    if (test)
        proc_remove(test);
    if (test_arr)
        proc_remove_array(test_arr);
}

module_init(procfs_init);
module_exit(procfs_exit);
