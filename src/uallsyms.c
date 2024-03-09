/*
 * uallsyms: uallsyms helpers
 *
 * uallsyms library 관련 helper들이 구현되어 있다.
 */

#include <stdlib.h>

#include <uallsyms/uallsyms.h>

#include "uallsyms.h"

uas_t *uas_init(uas_aar_t aar_function)
{
    uas_t *uas;

    uas = calloc(1, sizeof(*uas));
    if (!uas)
        return NULL;

    return uas;
}