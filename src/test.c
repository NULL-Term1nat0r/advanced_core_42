#include "malloc.h"


int main(void)
{
    printf("Starting test\n");
    int number_char_pointers = 200;
    char *arr[number_char_pointers];

    for (int i = 0; i < number_char_pointers; i++)
    {
        arr[i] = malloc(60); // Request a very large size to test large block allocation

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
        // printf("arr[%d]=%s\n", i, arr[i]);
        // free(arr[i]);
    }

    // char *pointer1 = malloc(2000);
    // pointer1[0] = 'h';
    // pointer1[1] = 'e';
    // pointer1[2] = 'l';
    // pointer1[3] = 'l';
    // pointer1[4] = 'o';
    // printf("pointer1=%s\n", pointer1);
    // // void *pointer2 = malloc(2000);
    // // void *pointer3 = malloc(2000);
    // // void *pointer4 = malloc(2000);


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

    // show_alloc_mem();

    return 0;
}