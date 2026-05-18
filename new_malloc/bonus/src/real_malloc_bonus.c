#include "stdlib.h"
#include "stdio.h"
// #define SIZE_MAX ((int)-1)

int main(){

    // size_t size ="";
    // printf("Testing malloc with size=%zu\n", size);
    // (void *)size; // To avoid unused variable warning
    // printf("Testing malloc with size=%p\n", size);
    // printf("first value 99 string: %s\n", (char *)size);

    char *ptr = malloc("999");
    if(ptr == NULL) {
        printf("malloc failed for size -1\n");
        return 1;
    }
    ptr[0] = 't';
    ptr[1] = 'e';
    ptr[2] = 's';
    ptr[3] = 't';
    printf("ptr=%s\n", ptr);
    free(ptr);
    return 0;
}