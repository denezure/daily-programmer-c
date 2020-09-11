#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

bool strcmp_modulus(const char *a, int a_index, const char *b, int b_index, int len)
{
    // This will short-circuit upon the first failure
    for (int i = 0; i < len; ++i)
    {
        if (a[(a_index + i) % len] != b[(b_index + i) % len])
        {
            return false;
        }
    }

    return true;
}

bool same_necklace(const char *a, const char *b)
{
    if (!a || !b)
    {
        return false;
    }

    // This can likely be avoided and the result calculated in a single pass
    int len = strlen(a);

    // The strings certainly have to be of the same length
    if (len != strlen(b))
    {
        return false;
    }

    if (len == 0)
    {
        return true;
    }

    for (int i = 0; i < len; ++i)
    {
        // Iterate through indices into a, checking equality with b using modular strcmp
        if (strcmp_modulus(a, i, b, 0, len))
        {
            return true;
        }
    }

    return false;
}

struct test_case
{
    const char *a, *b;
    const bool should_equal;
};

int main(int argc, char *argv[])
{
    const char *test_string = "nicole";

    struct test_case cases[] = {
        { "nicole", "icolen", true },
        { "nicole", "lenico", true },
        { "", "", true },
        { "nicole", "coneli", false },
        { "", " ", false },
        { NULL, NULL, false }
    };

    for (int i = 0; cases[i].a != NULL; ++i)
    {
        assert(same_necklace(cases[i].a, cases[i].b) == cases[i].should_equal);
    }

    return 0;
}
