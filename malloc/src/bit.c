#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>


void set_bit(int *number, int index){
    *number |= (1 << index);
}

void set_char_bit(char *number, int index){
    *number |= (1 << index);
}

void unset_char_bit(char *number, int index){
    *number |= (0 << index);
}

void set_array_bit(char *number, int index){
    *number |= (1 << index);
}


void clear_bit(int *number, int index){
    *number &=  ~(1 << index);
}

void set_positive(int *number){
    *number &=  ~(1U << 31);
}

void flip_bits(int *number){
    for(int i = 0; i < 33; i++){
        *number ^= (1 << i);
    }
}

void flip_char_bits(char *number){
    for(int i = 0; i < 33; i++){
        *number ^= (1 << i);
    }
}




void set_bit_void(void **ptr, int index) {
    *ptr = (void *)((uint64_t)(*ptr) | (1ULL << index));
}

uint64_t create_integer_from_binary(const char *binary_str) {
    uint64_t result = 0;
    for (size_t i = 0; binary_str[i] != '\0'; i++) {
        if (binary_str[i] == '1') {
            result |= (1ULL << (63 - i));
        }
    }
    return result;
}

#define TINY_ZONE_SIZE  16384
#define TINY_ZONE_MASK  (~(TINY_ZONE_SIZE - 1))   // 0xFFFFFFFFFFFFC000

typedef struct zone {
    size_t block_size;
    char bitmap[100]; // Bitmap to track free/used blocks
    struct zone *next;
    struct zone *prev;
} zone_t;

// In create_zone: align mmap to TINY_ZONE_SIZE boundary
// zone_t *create_zone(size_t block_size, size_t blocks_per_zone) {
//     size_t total = sizeof(zone_t) + blocks_per_zone * block_size;
//     size_t alloc_size = total + ZONE_ALIGNMENT; // extra for alignment
    
//     void *raw = mmap(NULL, alloc_size, PROT_READ|PROT_WRITE, 
//                      MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
//     if (raw == MAP_FAILED) return NULL;

//     print_pointer_in_binary(raw);
    
//     // Align up to TINY_ZONE_SIZE boundary
//     void *aligned = (void *)(((uintptr_t)raw + ZONE_ALIGNMENT - 1) & ZONE_MASK);

//     print_pointer_in_binary(aligned);
    
//     zone_t *zone = (zone_t *)aligned;
//     zone->block_size = block_size;
//     print_pointer_in_binary((void *)zone);

//     return zone;
//     // ... rest of init
    
//     // Store raw pointer somewhere to munmap later, or use zone->memory for that
// }

void print_pointer_in_binary(void *ptr){
    uint64_t p = (uint64_t)ptr;
    for(int i = 63; i >= 0; i--){
        printf("%ld", (p >> i) & 1);
        if(i % 8 == 0) printf(" ");
    }
    printf(" %p\n", ptr);
}

void print_binary_from_integer(uint64_t value){
    for(int i = 63; i >= 0; i--){
        printf("%ld", (value >> i) & 1);
        if(i % 8 == 0) printf(" ");
    }
    // printf("\n");
}

void print_integer_from_binary(uint64_t value){
    printf("%zu", value);
}

#define TAG_SHIFT_BIT 56
#define TAG_MASK  (0x7F << TAG_SHIFT_BIT)   // bits 60-62


static inline void *ptr_pack(void *ptr, u_int64_t tag) {
    u_int64_t p = (u_int64_t)ptr;
    printf("pointer packing starts\n");
    // clear existing tag bits first
    p &= ~TAG_MASK;
    print_pointer_in_binary(p);

    // insert tag
    p |= ((tag) << TAG_SHIFT_BIT);
    print_pointer_in_binary(p);

    return (void *)p;
}

static inline void *ptr_unpack(void *packed) {
    u_int64_t p = (u_int64_t)packed;

    // clear tag bits
    p &= ~TAG_MASK;

    return (void *)p;
}

static inline u_int64_t ptr_get_tag(void *packed) {
    return (((u_int64_t)packed) >> TAG_SHIFT_BIT) & 0x7F;
}


int main(){
    
    
    size_t align = 16384;
    size_t size = 8793;
    size_t alloc_size = align + size;

    #define TINY_ZONE_SIZE  16384        // 16KB = 2^14
    #define TINY_ZONE_MASK  (~(TINY_ZONE_SIZE - 1))   // = ~0x3FFF = 0xFFFFFFFFFFFFC000

    void *raw = mmap(NULL, alloc_size, PROT_READ|PROT_WRITE, 
                     MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    if (raw == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    print_pointer_in_binary(raw);

    
    void *aligned = (void *)(((uint64_t)raw + align - 1) & ~(align - 1));
    size_t front_waste = aligned - raw;
    if (front_waste > 0)
        munmap(raw, front_waste);
    
    // Unmap the fragment AFTER our usable region
    size_t back_waste = alloc_size - front_waste - size;
    if (back_waste > 0)
        munmap(aligned + size, back_waste);
    print_pointer_in_binary(aligned);
    void *ptr = (void *)((char *)aligned + 8000);
    print_pointer_in_binary(ptr);
    void *new_zone_ptr = (void *)((uint64_t)ptr & (uint64_t)TINY_ZONE_MASK);
    print_pointer_in_binary(new_zone_ptr);
    void *packed_ptr = ptr_pack(new_zone_ptr, 100);
    u_int64_t tag = ptr_get_tag(packed_ptr);
    printf("bit_value = %li\n", tag);



    


    return 0;
}