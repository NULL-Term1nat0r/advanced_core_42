#include "malloc.h"


int main(void)
{
    int number_char_pointers = 2;
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

    void *pointer1 = malloc(2000);
    void *pointer2 = malloc(2000);
    void *pointer3 = malloc(2000);
    void *pointer4 = malloc(2000);

    show_alloc_mem();

    void * new_pointer1 = realloc(pointer1, 4000);

    show_alloc_mem();

    return 0;
}