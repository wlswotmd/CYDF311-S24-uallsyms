#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/utsname.h>

#include <uallsyms/types.h>

arch_t current_arch(void)
{
    struct utsname buf;
    int ret;

    memset(&buf, 0, sizeof(buf));
    
    ret = uname(&buf);
    if (ret < 0)
        return UNKNOWN_ARCH;

    if (strcmp("x86_64", buf.machine) == 0)
        return X86_64;
    else if (strcmp("i386", buf.machine) == 0)
        return X86;
    else if (strcmp("arm", buf.machine) == 0)
        return ARM;
    else if (strcmp("arm64", buf.machine) == 0)
        return ARM64;

    return UNKNOWN_ARCH;
}

kver_t current_kver(void)
{
    struct utsname buf;
    char *cur;
    int ret;

    DEFINE_INVALID_KVER(kver);

    memset(&buf, 0, sizeof(buf));
    
    ret = uname(&buf);
    if (ret < 0)
        return kver;

    /* 
     * release의 형식은 대략 이런 식으로 되어 있다.
     * KERNELVERSION = $(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRAVERSION) 
     */
    cur = buf.release;

    /* kver.version이 0일 수도 있는데, atoi는 error시 0을 반환하여 반환값으로 error를 판단할 수 없어 미리 확인해줘야한다. */
    if (!isdigit(cur[0]))
        return kver;

    kver.version = atoi(cur);
    cur = strchr(cur, '.');

    if (cur == NULL)
        return kver;

    cur += 1;

    if (!isdigit(cur[0]))
        return kver;

    kver.patch_level = atoi(cur);
    cur = strchr(cur, '.');

    if (cur == NULL)
        return kver;

    cur += 1;

    if (!isdigit(cur[0]))
        return kver;

    kver.sub_level = atoi(cur);

    return kver;
}