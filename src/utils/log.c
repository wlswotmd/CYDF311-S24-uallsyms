#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "log.h"

int __pr_log(char indicator, int log_level, const char *fmt, ...) 
{
    va_list arg;
    int ret;

    if (log_level > LOG_LEVEL)
        return 0;

    va_start(arg, fmt);
    printf("[%c] ", indicator);
    ret = vprintf(fmt, arg);
    va_end(arg);

    return ret;
}
 
void hexdump(void *mem, unsigned int len)
{
    unsigned int i, j;
    
    for (i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++) {
        /* print offset */
        if (i % HEXDUMP_COLS == 0)
            printf("0x%06x: ", i);

        /* print hex data */
        if (i < len)
            printf("%02x ", 0xFF & ((char*)mem)[i]);
        else /* end of block, just aligning for ASCII dump */
            printf("   ");
        
        /* print ASCII dump */
        if (i % HEXDUMP_COLS == (HEXDUMP_COLS - 1)) {
            for (j = i - (HEXDUMP_COLS - 1); j <= i; j++) {
                if (j >= len) /* end of block, not really printing */
                    putchar(' ');
                else if(isprint(((char*)mem)[j])) /* printable char */
                    putchar(0xFF & ((char*)mem)[j]);        
                else /* other char */
                    putchar('.');
            }
            putchar('\n');
        }
    }
}
 