#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "procfs.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wushengbang");
MODULE_DESCRIPTION("A simple Linux kernel module");
MODULE_VERSION("0.1");


//////////////// 测试 ////////////
struct proc_dir_entry *test, *test_arr, *test_arr2, *test3;
struct stu {
    char name[16];
    int age;
};
struct stu wsb[2];

int da[] = {100, 2, 200, 3, 5, 6, 99, 110};
const char *names[] = {
    "test_str1",
    "test_str2",
    "test_str3"
};


static int stu_show(struct seq_file *m, void *v)
{

    struct stu *s = (struct stu *)v;
    seq_printf(m, "name: %s,\tage: %d", s->name, s->age);
    return 0;
}

static int __init procfs_init(void) 
{    
    strncpy(wsb[0].name, "wushengbang", 16);
    wsb[0].age = 39;
    strncpy(wsb[1].name, "morgan", 16);
    wsb[1].age = 22;

    test_arr = proc_mkfile_array_i("test_arr", NULL, da, sizeof(da) / sizeof(int));
    if (!test_arr)
        return -2;
    test_arr2 = proc_mkfile_array_s("test_arr2", NULL, names, sizeof(names) / sizeof(char *));
    if (!test_arr2) {
        return -3;
    }
    test3 = proc_mkfile_array_o("test3", NULL, wsb, 2, sizeof(wsb[0]), stu_show);
    if (!test3)
        return -4;
    

    return 0;
}

static void __exit procfs_exit(void) 
{
    if (test)
        proc_remove(test);
    if (test_arr)
        proc_remove_array(test_arr);
    if (test_arr2)
        proc_remove_array(test_arr2);
    if (test3)
        proc_remove_array(test3);
}

module_init(procfs_init);
module_exit(procfs_exit);