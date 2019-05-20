#include "utilities.h"
#include <string.h>


/**
 * check if the string <i>str</i> is an unsigned int
 * @param str string to check
 * @return 1 if so; otherwise 0
 */
int is_uint(const char* str) {
    if (strlen(str) == 0)
        return 0;
    if (str[0] == '0' && strlen(str) > 1)
        return 0;

    int i;
    for (i = 0; i < strlen(str); i++) {
        if (str[i] < '0' || str[i] > '9')
            return 0;
    }
    return 1;
}
