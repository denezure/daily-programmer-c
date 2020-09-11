#include "../common.h"

#define FREE_CLEAR(s) do { free(s); s = NULL; } while(0)

struct bankers_state
{
    u32 time_step;
    u32 resource_count;
    u32 process_count; // Number of processes in process arrays
    u32 process_length; // Total length of the process arrays (allocation tracking)

    u32 *available; // Available resources for processes to acquire
    u32 *allocated; // Process-by-process allocated resources - row by row matrix.
    u32 *required; // ...
    u32 *need; // ...
};

static void bankers_state_free(struct bankers_state *state)
{
    if (!state)
    {
        return;
    }

    if (state->allocated)
    {
        FREE_CLEAR(state->allocated);
    }

    if (state->available)
    {
        FREE_CLEAR(state->available);
    }

    if (state->required)
    {
        FREE_CLEAR(state->required);
    }

    if (state->need)
    {
        FREE_CLEAR(state->need);
    }
}

static s32 bankers_state_read_available(struct bankers_state *state, char *s)
{
    state->resource_count = parse_u32_line(s, &state->available);

    return 0;
}

static s32 bankers_state_add_process(struct bankers_state *state, char *s)
{
    u32 *line_values = NULL;
    u32 line_values_count = parse_u32_line(s, &line_values);

    if (line_values_count <= 0 || line_values == NULL)
    {
        if (line_values)
        {
            FREE_CLEAR(line_values);
        }

        return -1;
    }

    // First half of array will be appended to the allocated array
    // Second half of array will be appended to the required array

    // There should always be 2x the resource count input
    if (line_values_count != (2 * state->resource_count))
    {
        if (line_values)
        {
            FREE_CLEAR(line_values);
        }
        return -2;
    }

    // Resize the allocated & required arrays
    if (state->process_count >= state->process_length)
    {
        u32 process_row_length = state->resource_count * sizeof(u32);

        u32 new_process_length = state->process_length == 0 ? 1 : 2 * state->process_length;
        u32 *new_allocated = reallocarray(state->allocated, new_process_length, process_row_length);

        if (!new_allocated)
        {
            return -3;
        }
        else
        {
            state->allocated = new_allocated;
        }

        u32 *new_required = reallocarray(state->required, new_process_length, process_row_length);

        if (!new_required)
        {
            return -3;
        }
        else
        {
            state->required = new_required;
        }

        state->process_length = new_process_length;
    }

    // Insert the row into each array
    u32 array_offset = state->process_count * state->resource_count;
    for (u32 i = 0; i < state->resource_count; ++i)
    {
        state->allocated[array_offset + i] = line_values[i];
        state->required[array_offset + i] = line_values[state->resource_count + i];
    }

    state->process_count += 1;

    if (line_values)
    {
        FREE_CLEAR(line_values);
    }

    return 0;
}

static s32 bankers_state_fill_need(struct bankers_state *state)
{
    // allocate and fill in the need array. It will be the same length as the other process arrays.
    
    return 0;
}

static s32 read_bankers_state_input(struct bankers_state *state)
{
    if (state == NULL)
    {
        return -1;
    }

    // Input format:
    // A B C D ... -- available amounts of each resource. M values. Known as 'Available'.
    // a0 b0 ... a0' b0' ... -- N rows of 2*M values where the first M values are 'Allocated' to each
    // a1 b1 a1' b1' ... -- process and the 2nd M values are 'Required' by each process.
    // ... <EOF>

    // The 'Need' of each process to complete is equal to required - allocated

    char *input_line = NULL;
    ssize_t input_read = 0;
    size_t input_length = 0;

    if ((input_read = getline(&input_line, &input_length, stdin)) == -1)
    {
        return -1;
    }

    bankers_state_read_available(state, input_line);

    // Read N lines - each is one process' parameters
    while ((input_read = getline(&input_line, &input_length, stdin)) != -1)
    {
        bankers_state_add_process(state, input_line);
    }

    if (!feof(stdin))
    {
        return -2;
    }

    // free() the memory that getline() has been using
    if (input_line)
    {
        FREE_CLEAR(input_line);
    }

    bankers_state_fill_need(state);

    return 0;
}

int main(void)
{
    challenge_print_header(344, "Banker's Algorithm");

    struct bankers_state state = {0};

    s32 res = 0;
    if ((res = read_bankers_state_input(&state)))
    {
        printf("Failed to input and parse state (%d). Exiting.\n", res);

        bankers_state_free(&state);
        return -1;
    }

    // Finally, run through the algorithm

    // Re-claim memory
    bankers_state_free(&state);

    printf("\nDone!\n");

    return 0;
}
