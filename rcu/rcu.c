#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wushengbang");
MODULE_DESCRIPTION("A simple Linux kernel module");
MODULE_VERSION("0.1");

struct config_data {
    int version;
    int max_connections;
    int timeout;
    char server_name[20];
    struct rcu_head rcu;    // 必须有
};

static struct config_data __rcu *g_config_data;
static DEFINE_SPINLOCK(config_lock);

static void reader_fn(int reader_id)
{
    struct config_data *config;
    rcu_read_lock();
    config = rcu_dereference(g_config_data);
    if (config) {
        printk(KERN_INFO "read_id: %d, version=%d, max_conn=%d, timeout=%d, server_name=%s\n",
            reader_id, config->version, config->max_connections, config->timeout, config->server_name
        );
        msleep(100);
    }
    rcu_read_unlock();
}

static void config_free_rcu(struct rcu_head *head)
{
    struct config_data *config = container_of(head, struct config_data, rcu);
    printk(KERN_INFO "Freeing old config version: %d\n", config->version);
    kfree(config);
}

static void writer_fn(int new_version)
{
    struct config_data *old_conf, *new_conf;
    new_conf = kmalloc(sizeof(*new_conf), GFP_KERNEL);
    if (!new_conf) {
        printk(KERN_ERR "Failed to allocate new config\n");
        return;
    }
    new_conf->version = new_version;
    new_conf->max_connections = 100 + new_version;
    new_conf->timeout = 30 + new_version;
    snprintf(new_conf->server_name, 20, "server-%d", new_version);

    spin_lock(&config_lock);
    old_conf = rcu_dereference_protected(g_config_data, lockdep_is_held(&config_lock));
    rcu_assign_pointer(g_config_data, new_conf);
    spin_unlock(&config_lock);

    if (old_conf) {
        call_rcu(&old_conf->rcu, config_free_rcu);
    }
}

static int reader_thread_fn(void *data)
{
    int read_id = (int)(long)data;
    while (!kthread_should_stop()) {
        reader_fn(read_id);
        msleep(500);
    }

    return 0;
}

static int writer_thread_fn(void *data) 
{
    int version = 1;
    while (!kthread_should_stop()) {
        writer_fn(version++);
        msleep(2000);
    }
    return 0;
}

static struct task_struct *readers[3], *writer;

static int __init my_rcu_init(void) {
    printk(KERN_INFO "Hello, kernel world!\n");

    writer_fn(0);

    int i;
    for (i = 0; i < 3; i++) {
        readers[i] = kthread_run(reader_thread_fn, (void *)i, "rcu_reader_%d", i);
        if (IS_ERR(readers[i])) {
            printk(KERN_ERR "kthread_run failed\n");
            return PTR_ERR(readers[i]);
        }
    }

    writer = kthread_run(writer_thread_fn, NULL, "rcu_writer");
    if (!writer) {
        printk(KERN_ERR "create writer kthread failed\n");
        return PTR_ERR(writer);
    }

    return 0;
}

static void __exit rcu_exit(void) {
    int i;
    for (i = 0; i < 3; i++) {
        if (readers[i]) {
            kthread_stop(readers[i]);
        }
    }

    if (writer)
        kthread_stop(writer);

    spin_lock(&config_lock);
    struct config_data *config;
    config = rcu_dereference_protected(g_config_data, lockdep_is_held(&config_lock));
    RCU_INIT_POINTER(g_config_data, NULL);
    spin_unlock(&config_lock);

    if (config) {
        synchronize_rcu();
        kfree(config);
    }

    printk(KERN_INFO "Goodbye, kernel world!\n");
}

module_init(my_rcu_init);
module_exit(rcu_exit);
