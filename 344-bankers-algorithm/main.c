#include "../common.h"

#define FREE_CLEAR(s) do { free(s); s = NULL; } while(0)

struct bankers_state
{
    u32 resource_count;
    u32 process_count; // Number of processes in process arrays
    u32 process_length; // Number of processes which will fit in the current allocation

    u32 *available; // Available resources for processes to acquire
    u32 *allocated; // Process-by-process allocated resources - row by row matrix.
    u32 *required; // ...
    u32 *need; // ...

    u32 *process_id; // Original index of the processes in this run
    u32 process_index; // Used during algorithm to save process scanning index
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

    if (state->process_id)
    {
        FREE_CLEAR(state->process_id);
    }
}

static void bankers_state_copy(struct bankers_state *dst, struct bankers_state *src)
{
    if (!dst || !src)
    {
        return;
    }

    memcpy(dst, src, sizeof(struct bankers_state));

    // TODO: Verify allocations succeed

    u32 memsz = src->resource_count * sizeof(u32);

    dst->available = malloc(memsz);
    memcpy(dst->available, src->available, memsz);

    memsz = src->process_count * sizeof(u32);
    dst->process_id = malloc(memsz);
    memcpy(dst->process_id, src->process_id, memsz);

    memsz = src->resource_count * src->process_count * sizeof(u32);
    dst->allocated = malloc(memsz);
    memcpy(dst->allocated, src->allocated, memsz);

    dst->required = malloc(memsz);
    memcpy(dst->required, src->required, memsz);

    dst->need = malloc(memsz);
    memcpy(dst->need, src->need, memsz);

    dst->process_index = 0;
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
    // This function works as it should.

    u32 process_row_length = state->resource_count * sizeof(u32);
    
    state->need = calloc(state->process_count, process_row_length);

    if (!state->need)
    {
        return -1;
    }

    for (u32 process_index = 0; process_index < state->process_count; ++process_index)
    {
        u32 process_offset = process_index * state->resource_count;

        u32 *process_need_ptr = &state->need[process_offset];
        u32 *process_required_ptr = &state->required[process_offset];
        u32 *process_allocated_ptr = &state->allocated[process_offset];

        for (u32 resource_index = 0; resource_index < state->resource_count; ++resource_index)
        {
            process_need_ptr[resource_index] = 
                process_required_ptr[resource_index] - process_allocated_ptr[resource_index];
        }
    }
    
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

s32 bankers_state_run(struct bankers_state *state)
{
    // Initialize process indices
    state->process_id = malloc(state->process_count * sizeof(u32));
    for (u32 i = 0; i < state->process_count; ++i)
    {
        state->process_id[i] = i;
    }

    struct bankers_state *state_stack = calloc(state->process_count, sizeof(struct bankers_state));

    if (!state_stack)
    {
        return -1;
    }

    // Temporary buffer used during swap operations
    // Large enough to hold one process's required/need/allocation array
    u32 tmpsz = state->resource_count * sizeof(u32);
    u32 *process_tmp = malloc(tmpsz);

    if (!process_tmp)
    {
        FREE_CLEAR(state_stack);
        return -1;
    }

    // Clone the provided state into the first stack position
    bankers_state_copy(&state_stack[0], state);

    u32 stack_index = 0;
    do
    {
        struct bankers_state *cur_state = &state_stack[stack_index];

        // This indicates whether the stack level found a candidate
        // If false at the end, the stack frame will be freed and the search will
        // continue at the previous stack frame
        bool candidate_found = false;

        // Loop over processes until we find one whose need is less than the available
        for (u32 process_index = cur_state->process_index; 
                process_index < cur_state->process_count && !candidate_found; 
                ++process_index)
        {
            // Ensure that the need of every resource is met
            bool meets_need = true;
            u32 *cur_process_need = &cur_state->need[process_index * cur_state->resource_count];
            for (u32 resource_index = 0; resource_index < cur_state->resource_count; ++resource_index)
            {
                meets_need = meets_need && 
                    (cur_process_need[resource_index] <= cur_state->available[resource_index]);
            }

            if (meets_need)
            {
                // If this is the maximum stack depth, we have found a solution
                if (stack_index == state->process_count - 1)
                {
                    // The last stack frame will contain the solution
                    printf("Solution: ");
                    for (u32 i = 1; i < state->process_count; ++i)
                    {
                        printf("%u ", state_stack[i].process_id[state->process_count - i]);
                    }
                    printf("%u\n", state_stack[state->process_count - 1].process_id[0]);

                    return 0;
                }

                // Indicate that a candidate was found and the frame should not be freed
                candidate_found = true;

                // Save the process_index that should be used in the next round in case the
                // subsequent stack levels fail to find a solution
                cur_state->process_index = process_index + 1;

                struct bankers_state *next_state = &state_stack[stack_index + 1];

                // Copy the current state into the next stack index
                bankers_state_copy(next_state, cur_state);

                u32 *process_allocated = &cur_state->allocated[process_index * cur_state->resource_count];

                // Update available resources of the new frame
                // Incrementing by the resources released upon this process's completion
                for (u32 i = 0; i < state->resource_count; ++i)
                {
                    next_state->available[i] += process_allocated[i];
                }

                // Swap candidate to the end of the process arrays
                // The process_id field will ensure that we can identify what process they are

                u32 tmp_proc_id = next_state->process_id[process_index];
                next_state->process_id[process_index] = 
                    next_state->process_id[next_state->process_count - 1];
                next_state->process_id[next_state->process_count - 1] = tmp_proc_id;

                memcpy(process_tmp, 
                        &next_state->allocated[process_index * next_state->resource_count], 
                        tmpsz);
                memcpy(&next_state->allocated[next_state->resource_count * process_index],
                        &next_state->allocated[next_state->resource_count * (next_state->process_count - 1)],
                        tmpsz);
                memcpy(&next_state->allocated[next_state->resource_count * (next_state->process_count - 1)],
                        process_tmp,
                        tmpsz);

                memcpy(process_tmp, 
                        &next_state->required[process_index * next_state->resource_count], 
                        tmpsz);
                memcpy(&next_state->required[next_state->resource_count * process_index],
                        &next_state->required[next_state->resource_count * (next_state->process_count - 1)],
                        tmpsz);
                memcpy(&next_state->required[next_state->resource_count * (next_state->process_count - 1)],
                        process_tmp,
                        tmpsz);

                memcpy(process_tmp, 
                        &next_state->need[process_index * next_state->resource_count], 
                        tmpsz);
                memcpy(&next_state->need[next_state->resource_count * process_index],
                        &next_state->need[next_state->resource_count * (next_state->process_count - 1)],
                        tmpsz);
                memcpy(&next_state->need[next_state->resource_count * (next_state->process_count - 1)],
                        process_tmp,
                        tmpsz);

                // Decrement process count of the new frame
                next_state->process_count -= 1;

                // Increment the stack index to check subsequent frames
                stack_index++;
            }
            else
            {
                // Next process should be tested. No action needed.
            }
        }
        // If no candidate was found, this frame failed and we need to backtrack to the previous frame
        if (!candidate_found)
        {
            // Free the current frame and decrement the stack index
            bankers_state_free(&state_stack[stack_index]);
            if (stack_index == 0)
            {
                return -1;
            }
            stack_index -= 1;
        }
        else
        {
            // Decrement this to un-do what the last for-loop iteration did
            cur_state->process_index -= 1;
        }

        // If the process index increases beyond the provided processes, no solution exists
    } while (state_stack[0].process_index < state->process_count);

    return -1;
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
    bankers_state_run(&state);

    // Re-claim memory
    bankers_state_free(&state);

    printf("\nDone!\n");

    return 0;
}
