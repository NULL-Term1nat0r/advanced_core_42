#include "malloc_bonus.h"

#define NUM_THREADS		8
#define ALLOCS_PER_THREAD	50

typedef struct {
    int	thread_id;
    int	errors;
} thread_arg_t;

static void	*thread_alloc_free(void *arg)
{
    thread_arg_t	*targ = (thread_arg_t *)arg;
    int				id = targ->thread_id;
    void			*ptrs[ALLOCS_PER_THREAD];
    size_t			sizes[ALLOCS_PER_THREAD];
    size_t			size_patterns[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    int				n_patterns = 9;

    for (int i = 0; i < ALLOCS_PER_THREAD; i++)
    {
        sizes[i] = size_patterns[(id + i) % n_patterns];
        ptrs[i] = malloc(sizes[i]);
        if (!ptrs[i])
        {
            targ->errors++;
            printf("[Thread %d] malloc(%zu) failed at i=%d\n", id, sizes[i], i);
            continue;
        }
        unsigned char pattern = (unsigned char)((id * 37 + i) & 0xFF);
        memset(ptrs[i], pattern, sizes[i]);
    }

    for (int i = 0; i < ALLOCS_PER_THREAD; i++)
    {
        if (!ptrs[i])
            continue;
        unsigned char	pattern = (unsigned char)((id * 37 + i) & 0xFF);
        unsigned char	*p = (unsigned char *)ptrs[i];
        for (size_t j = 0; j < sizes[i]; j++)
        {
            if (p[j] != pattern)
            {
                targ->errors++;
                printf("[Thread %d] CORRUPTION at ptr[%d][%zu]: expected 0x%02X got 0x%02X\n",
                    id, i, j, pattern, p[j]);
                break;
            }
        }
    }

    for (int i = 0; i < ALLOCS_PER_THREAD; i++)
    {
        if (ptrs[i])
            free(ptrs[i]);
    }
    return NULL;
}

static void	*thread_realloc(void *arg)
{
    thread_arg_t	*targ = (thread_arg_t *)arg;
    int				id = targ->thread_id;
    size_t			small_size = 32;
    size_t			large_size = 512;

    char *ptr = malloc(small_size);
    if (!ptr)
    {
        targ->errors++;
        return NULL;
    }

    unsigned char fill = (unsigned char)(id & 0xFF);
    memset(ptr, fill, small_size);

    char *new_ptr = realloc(ptr, large_size);
    if (!new_ptr)
    {
        targ->errors++;
        free(ptr);
        return NULL;
    }

    unsigned char *p = (unsigned char *)new_ptr;
    for (size_t i = 0; i < small_size; i++)
    {
        if (p[i] != fill)
        {
            targ->errors++;
            printf("[Thread %d] REALLOC CORRUPTION at [%zu]: expected 0x%02X got 0x%02X\n",
                id, i, fill, p[i]);
            break;
        }
    }
    free(new_ptr);
    return NULL;
}

void	test_multithreading(void)
{
    pthread_t		threads[NUM_THREADS];
    thread_arg_t	args[NUM_THREADS];
    int				total_errors;

    printf("\n=== Multithreading Test ===\n");

    total_errors = 0;
    printf("[Test 1] %d threads concurrent malloc/write/verify/free...\n", NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; i++)
    {
        args[i].thread_id = i;
        args[i].errors = 0;
        pthread_create(&threads[i], NULL, thread_alloc_free, &args[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
        total_errors += args[i].errors;
    }
    printf("[Test 1] %s (errors: %d)\n", total_errors == 0 ? "PASS" : "FAIL", total_errors);

    total_errors = 0;
    printf("[Test 2] %d threads concurrent realloc with data integrity...\n", NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; i++)
    {
        args[i].thread_id = i;
        args[i].errors = 0;
        pthread_create(&threads[i], NULL, thread_realloc, &args[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
        total_errors += args[i].errors;
    }
    printf("[Test 2] %s (errors: %d)\n", total_errors == 0 ? "PASS" : "FAIL", total_errors);

    printf("=== Multithreading Test Complete ===\n\n");
}

int main(void)
{
    printf("Starting test\n");
    int number_char_pointers = 200;
    char *arr[number_char_pointers];

    for (int i = 0; i < number_char_pointers; i++)
    {
        arr[i] = malloc(60);

        if (!arr[i])
        {
            printf("allocation failed at i=%d", i);
            return 1;
        }

        arr[i][0] = 't';
        arr[i][1] = 'e';
        arr[i][2] = 's';
        arr[i][3] = 't';
    }

    for (int i = 0; i < number_char_pointers; i++)
    {
        printf("arr[%d]=%s\n", i, arr[i]);
        // free(arr[i]);
    }

    char *new_pointer1 = malloc(4000);
    new_pointer1[0] = 't';
    new_pointer1[1] = 'e';
    new_pointer1[2] = 's';
    new_pointer1[3] = 't';
    printf("new_pointer1=%s\n", new_pointer1);
    free(new_pointer1);
    char *new_pointer2 = malloc(4000);
    char *new_pointer3 = realloc(new_pointer2, 4001);
    free(new_pointer3);
    char *new_pointer4 = malloc(4001);

    test_multithreading();

    // show_alloc_mem_ex();

    (void)new_pointer4;
    return 0;
}
