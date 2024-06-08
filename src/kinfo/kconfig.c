#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <sys/utsname.h>

char *kconfig = NULL;

static int try_read_config_proc(void)
{
    
}

static int try_read_config_boot_release(void)
{
    FILE *fp;
    char config_path[PATH_MAX];
    long config_len;
    size_t read_len;
    struct utsname tmp;
    int ret;

    if (kconfig)
        return 0;

    ret = uname(&tmp);
    if (ret < 0)
        goto err_uname;
    
    snprintf(config_path, sizeof(config_path) - 1, "/boot/config-%s", tmp.release);

    fp = fopen(config_path, "r");
    if (fp == NULL)
        goto err_fopen;
    
    config_len = ftell(fp);
    if (config_len < 0)
        goto err_ftell;

    kconfig = malloc(config_len + 1);
    if (kconfig == NULL)
        goto err_malloc;

    read_len = fread(kconfig, sizeof(*kconfig), config_len, fp);
    if (read_len != config_len)
        goto err_fread;

    fclose(fp);

    return 0;

err_fread:
    free(kconfig);
    kconfig = NULL;
err_malloc:
err_ftell:
    fclose(fp);
err_fopen:
err_uname:
    return -1;
}

int try_read_config_boot(void)
{
    FILE *fp;
    char config_path[] = "/boot/config";
    long config_len;
    size_t read_len;
    int ret;

    if (kconfig)
        return 0;

    fp = fopen(config_path, "r");
    if (fp == NULL)
        goto err_fopen;
    
    config_len = ftell(fp);
    if (config_len < 0)
        goto err_ftell;

    kconfig = malloc(config_len + 1);
    if (kconfig == NULL)
        goto err_malloc;

    read_len = fread(kconfig, sizeof(*kconfig), config_len, fp);
    if (read_len != config_len)
        goto err_fread;

    fclose(fp);

    return 0;

err_fread:
    free(kconfig);
    kconfig = NULL;
err_malloc:
err_ftell:
    fclose(fp);
err_fopen:
    return -1;
}

int try_read_config(void)
{
    if (!try_read_config_boot())
        return 0;

    if (!try_read_config_boot_release())
        return 0;

    if (!try_read_config_proc())
        return 0;

    return -1;
}

int has_config(const char *config_name)
{
    if (!kconfig || try_read_config()) {
        
    }
}