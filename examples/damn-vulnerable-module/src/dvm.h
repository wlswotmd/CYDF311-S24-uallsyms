#include <linux/compiler_types.h>

struct aar_req {
    void __user *to;
    void *from;
    unsigned int n;
};

struct bof_req {
    void __user *from;
    unsigned int n;
};

#define DVM_IOCTL_MAGIC 37
#define DVM_IOCTL_AAR _IOWR(DVM_IOCTL_MAGIC, 0, struct aar_req)
#define DVM_IOCTL_BOF _IOW(DVM_IOCTL_MAGIC, 1, struct bof_req)