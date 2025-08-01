#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/container_of.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wushengbang");
MODULE_DESCRIPTION("A simple Linux kernel module");
MODULE_VERSION("0.1");

static ssize_t reg_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t reg_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static struct kobject *test_kobj;
struct kobj_file_s {
    struct kobj_attribute kattr;
    char buf[128];
    int data;
};

static struct kobj_file_s g_kfs = {
    .kattr = {
        .attr = {
            .name = "reg_file",
            .mode = 0644,
        },
        .show = reg_show,
        .store = reg_store
    },
    .data = 0,
};

static ssize_t reg_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf)
{
    ssize_t count = 0;
    struct kobj_file_s *kfs = container_of(kattr, struct kobj_file_s, kattr);
    struct attribute *attr = &kattr->attr;
    if (!kfs->data) {
        count = sprintf(buf, "This is a sysfs reg file, name %s, mode %04o\n", attr->name, attr->mode);
    } else {
        count = sprintf(buf, "%s", kfs->buf);
    }
    return count;
}

static ssize_t reg_store(struct kobject *kobj, struct kobj_attribute *kattr, const char *buf, size_t count)
{
    struct kobj_file_s *kfs = container_of(kattr, struct kobj_file_s, kattr);
    kfs->data = 1;
    return snprintf(kfs->buf, 128, "%s", buf);
}

static int __init mysysfs_init(void) {
    int ret;
    printk(KERN_INFO "Hello, kernel world!\n");
    
    // 创建 /sys/test目录
    test_kobj = kobject_create_and_add("test", NULL);
    if (!test_kobj) {
        printk(KERN_ERR "kobject_create_and_add failed\n");
        return -EFAULT;
    }
    
    // 创建 /sys/test/reg_file文件
    ret = sysfs_create_file(test_kobj, &g_kfs.kattr.attr);
    if (ret) {
        printk(KERN_ERR "sysfs_create_file failed\n");
        kobject_del(test_kobj);
        return -EFAULT;
    }

    return 0;
}

static void __exit mysysfs_exit(void) {
    if (test_kobj) {
        kobject_del(test_kobj);
        test_kobj = NULL;
    }
    printk(KERN_INFO "Goodbye, kernel world!\n");
}

module_init(mysysfs_init);
module_exit(mysysfs_exit);
