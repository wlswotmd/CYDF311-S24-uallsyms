#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>

#include "dvm.h"

MODULE_AUTHOR("Junhyeon Song <umsun2357@korea.ac.kr>");
MODULE_DESCRIPTION("Damn Vulnerable Module");
MODULE_LICENSE("GPL v2");

static int dvm_ioctl_aar(struct aar_req *areq)
{
    if (copy_to_user(areq->to, areq->from, areq->n))
        return -EFAULT;
    
    return 0;
}

static int dvm_ioctl_bof(struct bof_req *breq)
{
    char victim[1];

    if (copy_from_user(victim, breq->from, breq->n))
        return -EFAULT;

    return 0;
}

static long dvm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    int ret = 0;

    switch (cmd) {
    case DVM_IOCTL_AAR:
        struct aar_req areq;

        if (copy_from_user(&areq, argp, sizeof(areq)))
            return -EFAULT;

        ret = dvm_ioctl_aar(&areq);
        break;

    case DVM_IOCTL_BOF:
        struct bof_req breq;

        if (copy_from_user(&breq, argp, sizeof(breq)))
            return -EFAULT;

        ret = dvm_ioctl_bof(&breq);
        break;

    default:
        break;
    }

    return ret;
}

static const struct file_operations dvm_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = dvm_ioctl
};

static struct miscdevice dvm_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "dvm",
    .fops = &dvm_fops,
};

static int __init dvm_init(void)
{
    return misc_register(&dvm_miscdev);
}

static void __exit dvm_exit(void)
{
    misc_deregister(&dvm_miscdev);
}

module_init(dvm_init);
module_exit(dvm_exit);