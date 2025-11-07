// mydevice.c - Minimal, modern character device example
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "charDevTempl.h"

static dev_t        mydev_devnum;
static struct cdev  mydev_cdev;
static struct class *mydev_class;
static struct device *mydev_device;

/* Interner Puffer und Locking */
static char  *kbuf;
static size_t data_size; /* gültige Datenmenge im Puffer */
static DEFINE_MUTEX(mydev_lock);

/* Protos */
static int      mydev_open(struct inode *inode, struct file *file);
static int      mydev_release(struct inode *inode, struct file *file);
static ssize_t  mydev_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t  mydev_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static loff_t   mydev_llseek(struct file *file, loff_t offset, int whence);

static const struct file_operations mydev_fops = {
    .owner   = THIS_MODULE,
    .open    = mydev_open,
    .release = mydev_release,
    .read    = mydev_read,
    .write   = mydev_write,
    .llseek  = mydev_llseek,
};

static int mydev_open(struct inode *inode, struct file *file)
{
    /* keine per-file private Daten notwendig derzeit */
    if (!mutex_trylock(&mydev_lock))
        return -EBUSY; /* verhindert mehrfaches gleichzeitiges open */
    /* keep lock until release to serialize accesses via open/close semantics */
    return 0;
}

static int mydev_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&mydev_lock);
    return 0;
}

/* llseek: einfache Implementierung (allow standard SEEK_SET, SEEK_CUR, SEEK_END) */
static loff_t mydev_llseek(struct file *file, loff_t offset, int whence)
{
    loff_t newpos;

    switch (whence) {
    case SEEK_SET:
        newpos = offset;
        break;
    case SEEK_CUR:
        newpos = file->f_pos + offset;
        break;
    case SEEK_END:
        newpos = data_size + offset;
        break;
    default:
        return -EINVAL;
    }

    if (newpos < 0 || newpos > MYDEVICE_BUF_SIZE)
        return -EINVAL;

    file->f_pos = newpos;
    return newpos;
}

static ssize_t mydev_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    ssize_t available;
    ssize_t to_copy;
    ssize_t not_copied;

    if (*ppos >= data_size)
        return 0; /* EOF */

    available = data_size - *ppos;
    to_copy = min((size_t)available, count);

    if (to_copy == 0)
        return 0;

    not_copied = copy_to_user(buf, kbuf + *ppos, to_copy);
    if (not_copied)
        return -EFAULT;

    *ppos += to_copy;
    return to_copy;
}

static ssize_t mydev_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    ssize_t to_copy;
    ssize_t not_copied;

    /* Schreibe immer ab Anfang (overwrite) -- einfaches Verhalten. Kann angepasst werden. */
    to_copy = min(count, (size_t)MYDEVICE_BUF_SIZE);

    not_copied = copy_from_user(kbuf, buf, to_copy);
    if (not_copied)
        return -EFAULT;

    data_size = to_copy;
    *ppos = to_copy;
    return to_copy;
}

/* Modul init/exit */
static int __init mydev_init(void)
{
    int ret;

    /* Reserve device number (dynamic major) */
    ret = alloc_chrdev_region(&mydev_devnum, 0, 1, MYDEVICE_NAME);
    if (ret) {
        pr_err("mydevice: alloc_chrdev_region failed: %d\n", ret);
        return ret;
    }

    /* cdev init & add */
    cdev_init(&mydev_cdev, &mydev_fops);
    mydev_cdev.owner = THIS_MODULE;

    ret = cdev_add(&mydev_cdev, mydev_devnum, 1);
    if (ret) {
        pr_err("mydevice: cdev_add failed: %d\n", ret);
        goto err_unregister_chrdev;
    }

    /* class & device (automatische /dev/ Erzeugung) */
    mydev_class = class_create(MYDEVICE_NAME);
    if (IS_ERR(mydev_class)) {
        ret = PTR_ERR(mydev_class);
        pr_err("mydevice: class_create failed: %d\n", ret);
        goto err_cdev_del;
    }

    mydev_device = device_create(mydev_class, NULL, mydev_devnum, NULL, MYDEVICE_NAME);
    if (IS_ERR(mydev_device)) {
        ret = PTR_ERR(mydev_device);
        pr_err("mydevice: device_create failed: %d\n", ret);
        goto err_class_destroy;
    }

    /* interner Puffer */
    kbuf = kzalloc(MYDEVICE_BUF_SIZE, GFP_KERNEL);
    if (!kbuf) {
        ret = -ENOMEM;
        pr_err("mydevice: kzalloc failed\n");
        goto err_device_destroy;
    }
    data_size = 0;
    mutex_init(&mydev_lock);

    pr_info("mydevice: module loaded. Major: %d Minor: %d\n",
            MAJOR(mydev_devnum), MINOR(mydev_devnum));
    return 0;

/* cleanup bei Fehlern in umgekehrter Reihenfolge */
err_device_destroy:
    device_destroy(mydev_class, mydev_devnum);
err_class_destroy:
    class_destroy(mydev_class);
err_cdev_del:
    cdev_del(&mydev_cdev);
err_unregister_chrdev:
    unregister_chrdev_region(mydev_devnum, 1);
    return ret;
}

static void __exit mydev_exit(void)
{
    kfree(kbuf);
    device_destroy(mydev_class, mydev_devnum);
    class_destroy(mydev_class);
    cdev_del(&mydev_cdev);
    unregister_chrdev_region(mydev_devnum, 1);

    pr_info("mydevice: module unloaded\n");
}

module_init(mydev_init);
module_exit(mydev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Minimal character device example");
MODULE_VERSION("0.1");
