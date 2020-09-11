#ifndef DAILY_PROGRAMMER_COMMON_H
#define DAILY_PROGRAMMER_COMMON_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

static void challenge_print_header(u32 num, const char *name)
{
    printf("%u - %s\n\n", num, name);
}

#define INIT_BUF_LEN 8

// Returns < 0 if error, otherwise return value is the output array length
// Output array must be free()'d by the caller.
static s32 parse_u32_line(char *input_line, u32 **output)
{
    if (!input_line)
        return -1;

    u32 *values = calloc(INIT_BUF_LEN, sizeof(u32));
    s32 values_len = INIT_BUF_LEN;
    s32 values_count = 0;

    if (!values)
    {
        return -3;
    }

    char *strtok_save;
    char *token = strtok_r(input_line, " ", &strtok_save);
    while (token)
    {
        // Attempt to parse the token as a u32
        u32 value = strtoul(token, NULL, 0);

        // If the result is UINT_MAX, out of bounds
        if (value == UINT_MAX)
        {
            free(values);
            return -4;
        }

        // Expand the array if needed
        if (values_count >= values_len)
        {
            values_len *= 2;
            u32 *new_values = reallocarray(values, values_len, sizeof(u32));
            
            if (!new_values)
            {
                free(values);
                return -5;
            }
            else
            {
                values = new_values;
            }
        }

        // Insert the new value into the array
        values[values_count++] = value;

        // Move on to next token
        token = strtok_r(NULL, " ", &strtok_save);
    }

    *output = values;

    return values_count;
}

#endif
