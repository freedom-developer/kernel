#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wushengbang");
MODULE_DESCRIPTION("A simple Linux kernel module");
MODULE_VERSION("0.1");

#define _min(a, b) (a) < (b) ? (a) : (b)

struct proc_dir_entry *test_pde;
const char *test_name = "test";

int data;
const char *test_string = "this is a test string.";
int arr_data[] = {1, 2, 3, 10, 100, 1000, 3000, 20};
const char *arr_str[] = {
    "test1",
    "test2",
    "test3",
};

static void *ad_start(struct seq_file *m, loff_t *pos)
{
    if (*pos >= sizeof(arr_data) / sizeof(int)) return NULL;
    return (void *)&arr_data[*pos];
}

static void *ad_next(struct seq_file *m, void *v, loff_t *pos)
{
    (*pos)++;
    if (*pos >= sizeof(arr_data) / sizeof(int)) return NULL;
    return (void *)&arr_data[*pos];
}

static void ad_stop(struct seq_file *m, void *v)
{

}

static int ad_show(struct seq_file *m, void *_data)
{
    seq_printf(m, "%d\t", *(int *)_data);
    return 0;
}


struct seq_operations ad_ops = {
    .start = ad_start,
    .next = ad_next,
    .stop = ad_stop,
    .show = ad_show,
};


static void *so_start(struct seq_file *m, loff_t *pos)
{
    if (*pos >= sizeof(arr_str) / sizeof(char *)) return NULL;
    return arr_str[*pos];
}
static void *so_next(struct seq_file *m, void *v, loff_t *pos)
{
    (*pos)++;
    if (*pos >= sizeof(arr_str) / sizeof(char *)) return NULL;
    return arr_str[*pos];
}

static int so_show(struct seq_file *m, void *_data)
{
    const char *p = (const char *)_data;
    seq_printf(m, "%s\t", p);
    return 0;
}

struct seq_operations so = {
    .start = so_start,
    .next = so_next,
    .stop = ad_stop,
    .show = so_show,
};

// _data 必为NULL
static int show_single_data(struct seq_file *m, void *_data)
{
    seq_printf(m, "%d\n", data);
    return 0;
}

static int show_single_string(struct seq_file *m, void *_data)
{
    const struct file *filp = m->file;
    struct inode *inode = filp->f_inode;
    const char *p = (const char *)inode->i_private;
    seq_printf(m, "%s\n", p);
    return 0;
}

int rw_data;
struct proc_data_buf {
    void *buf;
    size_t size;
    size_t count;
};

static int rw_data_open(struct inode *inode, struct file *filp)
{
    // 准备好数据空间
    struct proc_data_buf *pdb = kmalloc(sizeof(*pdb), GFP_KERNEL);
    if (!pdb) return -ENOMEM;
    filp->private_data = pdb;
    pdb->size = 128;
    pdb->buf = kmalloc(pdb->size, GFP_KERNEL);
    if (!pdb->buf) {
        kfree(pdb);
        return -ENOMEM;
    }
    pdb->count = snprintf(pdb->buf, pdb->size, "%d\n", rw_data);
    
    return 0;
}
static ssize_t rw_data_read(struct file *filp, char __user *buf, size_t size, loff_t *pos)
{
    struct proc_data_buf *pdb = filp->private_data;
    ssize_t copy = _min(pdb->count - *pos, size);
    if (!buf) return -EINVAL;
    if (copy <= 0) return 0;
    if (copy_to_user(buf, (char *)pdb->buf + *pos, copy))
        return -EFAULT;
    *pos += copy;
    return  copy;
}
static ssize_t rw_data_write(struct file *filp, const char __user *buf, size_t size, loff_t *pos)
{
    struct proc_data_buf *pdb = filp->private_data;
    ssize_t copy = _min(pdb->size, size);
    if (!buf) return -EINVAL;
    if (copy <= 0) return 0;
    if (copy_from_user(pdb->buf + *pos, buf, copy))
        return -EFAULT;
    *pos += copy;
    pdb->count = *pos;
    if (sscanf(pdb->buf, "%d", &rw_data) <= 0)
        return -EINVAL;
    return copy;
}
static int rw_data_release(struct inode *inode, struct file *filp)
{
    struct proc_data_buf *pdb = filp->private_data;
    if (pdb->buf) {
        kfree(pdb->buf);
    }
    kfree(pdb);
    filp->private_data = NULL;
    return 0;
}

struct proc_ops rw_data_ops = {
    .proc_open = rw_data_open,
    .proc_read = rw_data_read,
    .proc_write = rw_data_write,
    .proc_release = rw_data_release,
};


#define RW_STR_LEN 4096
char rw_str[RW_STR_LEN];
size_t rw_str_length;
static int rw_str_open(struct inode *inode, struct file *filp)
{
    if (filp->f_flags & O_APPEND)
        filp->f_pos = rw_str_length;
    return 0;
}
static ssize_t rw_str_read(struct file *filp, char __user *buf, size_t size, loff_t *pos)
{
    ssize_t copy = _min(rw_str_length - *pos, size);
    if (copy <= 0) return 0;
    if (copy_to_user(buf, rw_str + *pos, copy)) return -EFAULT;
    *pos += copy;
    return copy;
}
static ssize_t rw_str_write(struct file *filp, const char __user *buf, size_t size, loff_t *pos)
{
    ssize_t copy = _min(RW_STR_LEN - *pos, size);
    if (copy <= 0) return 0;
    if (copy_from_user(rw_str + *pos, buf, copy)) return -EFAULT;
    *pos += copy;
    rw_str_length = *pos;
    return copy;
}
struct proc_ops rw_str_ops = {
    .proc_open = rw_str_open,
    .proc_read = rw_str_read,
    .proc_write = rw_str_write,
};

static int __init procfs_init(void) {
    printk(KERN_INFO "Hello, kernel world!\n");
    test_pde = proc_mkdir(test_name, NULL);
    if (!test_pde) return -1;

    data = 1009;
    struct proc_dir_entry *single_file = proc_create_single_data("data", 0444, test_pde, show_single_data, NULL);
    if (!single_file) return -1;

    struct proc_dir_entry *test_string_pde = proc_create_single_data("test_string", 0444, test_pde, show_single_string, (void *)test_string);
    if (!test_string_pde) return -1;

    struct proc_dir_entry *arr_data_pde = proc_create_seq("arr_data", 0444, test_pde, &ad_ops);
    if (!arr_data_pde) return -1;

    struct proc_dir_entry *arr_str_pde = proc_create_seq("arr_str", 0444, test_pde, &so);
    if (!arr_str_pde) return -1;

    struct proc_dir_entry *rw_data_pde = proc_create("rw_data", 0666, test_pde, &rw_data_ops);
    if (!rw_data_pde) return -1;

    struct proc_dir_entry *rw_str_pde = proc_create("rw_str", 0666, test_pde, &rw_str_ops);
    if (!rw_str_pde) return -1;

    return 0;
}

static void __exit procfs_exit(void) {
    printk(KERN_INFO "Goodbye, kernel world!\n");
    proc_remove(test_pde);
}

module_init(procfs_init);
module_exit(procfs_exit);
