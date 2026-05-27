#include "malloc_bonus.h"

static void write_char(char c) {
    write(STDOUT_FILENO, &c, 1);
}

static void write_str(const char *s) {
    while (*s)
        write_char(*s++);
}

static void write_hex(uintptr_t val) {
    char buf[18];
    int i = 17;
    buf[i] = '\0';
    if (val == 0) {
        write_str("0x0");
        return;
    }
    while (val > 0) {
        int digit = val & 0xF;
        buf[--i] = digit < 10 ? '0' + digit : 'A' + digit - 10;
        val >>= 4;
    }
    buf[--i] = 'x';
    buf[--i] = '0';
    write_str(buf + i);
}

static void write_dec(size_t val) {
    char buf[21];
    int i = 20;
    buf[i] = '\0';
    if (val == 0) {
        write_char('0');
        return;
    }
    while (val > 0) {
        buf[--i] = '0' + (val % 10);
        val /= 10;
    }
    write_str(buf + i);
}

static void write_hex_byte(unsigned char b) {
    char hi = (b >> 4) & 0xF;
    char lo = b & 0xF;
    write_char(hi < 10 ? '0' + hi : 'A' + hi - 10);
    write_char(lo < 10 ? '0' + lo : 'A' + lo - 10);
}

static void hex_dump(void *addr, size_t size) {
    unsigned char *p = (unsigned char *)addr;
    for (size_t i = 0; i < size; i += 16) {
        write_str("  0x");
        write_hex_byte((i >> 8) & 0xFF);
        write_hex_byte(i & 0xFF);
        write_str(": ");
        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) {
                write_hex_byte(p[i + j]);
                write_char(' ');
            } else {
                write_str("   ");
            }
            if (j == 7)
                write_char(' ');
        }
        
        write_str(" |");
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            unsigned char c = p[i + j];
            write_char(c >= 32 && c < 127 ? c : '.');
        }
        write_str("|\n");
    }
}

static size_t show_zone(zone_t *zone) {
    size_t total = 0;
    for (int i = 0; i < zone->number_of_blocks; i++) {
        if (!(zone->bitmap[i / 8] & (1 << (i % 8)))) {
            size_t req = zone->requested_sizes[i];
            void *start = calculate_zone_block_address(zone, i);
            void *end = (char *)start + req;
            write_hex((uintptr_t)start);
            write_str(" - ");
            write_hex((uintptr_t)end);
            write_str(" : ");
            write_dec(req);
            write_str(" bytes\n");
            total += req;
        }
    }
    return total;
}

void show_alloc_mem() {
    size_t total = 0;

    zone_t *first_tiny = global_heap.free_tiny_zones
        ? global_heap.free_tiny_zones : global_heap.full_tiny_zones;
    write_str("TINY : ");
    write_hex((uintptr_t)first_tiny);
    write_char('\n');
    zone_t *zone = global_heap.free_tiny_zones;
    while (zone) { total += show_zone(zone); zone = zone->next; }
    zone = global_heap.full_tiny_zones;
    while (zone) { total += show_zone(zone); zone = zone->next; }

    zone_t *first_small = global_heap.free_small_zones
        ? global_heap.free_small_zones : global_heap.full_small_zones;
    write_str("SMALL : ");
    write_hex((uintptr_t)first_small);
    write_char('\n');
    zone = global_heap.free_small_zones;
    while (zone) { total += show_zone(zone); zone = zone->next; }
    zone = global_heap.full_small_zones;
    while (zone) { total += show_zone(zone); zone = zone->next; }

    write_str("LARGE : ");
    write_hex((uintptr_t)global_heap.full_large_blocks);
    write_char('\n');
    block_t *block = global_heap.full_large_blocks;
    while (block) {
        void *start = (char *)block + sizeof(block_t) + sizeof(void *);
        void *end = (char *)start + block->size;
        write_hex((uintptr_t)start);
        write_str(" - ");
        write_hex((uintptr_t)end);
        write_str(" : ");
        write_dec(block->size);
        write_str(" bytes\n");
        total += block->size;
        block = block->next;
    }

    write_str("Total : ");
    write_dec(total);
    write_str(" bytes\n");
}

static size_t show_zone_ex(zone_t *zone) {
    size_t total = 0;
    for (int i = 0; i < zone->number_of_blocks; i++) {
        if (!(zone->bitmap[i / 8] & (1 << (i % 8)))) {
            size_t req = zone->requested_sizes[i];
            void *start = calculate_zone_block_address(zone, i);
            void *end = (char *)start + req;
            write_hex((uintptr_t)start);
            write_str(" - ");
            write_hex((uintptr_t)end);
            write_str(" : ");
            write_dec(req);
            write_str(" bytes\n");
            hex_dump(start, req);
            total += req;
        }
    }
    return total;
}

void show_alloc_mem_ex(void) {
    size_t total = 0;

    zone_t *first_tiny = global_heap.free_tiny_zones
        ? global_heap.free_tiny_zones : global_heap.full_tiny_zones;
    write_str("TINY : ");
    write_hex((uintptr_t)first_tiny);
    write_char('\n');
    zone_t *zone = global_heap.free_tiny_zones;
    while (zone) { total += show_zone_ex(zone); zone = zone->next; }
    zone = global_heap.full_tiny_zones;
    while (zone) { total += show_zone_ex(zone); zone = zone->next; }

    zone_t *first_small = global_heap.free_small_zones
        ? global_heap.free_small_zones : global_heap.full_small_zones;
    write_str("SMALL : ");
    write_hex((uintptr_t)first_small);
    write_char('\n');
    zone = global_heap.free_small_zones;
    while (zone) { total += show_zone_ex(zone); zone = zone->next; }
    zone = global_heap.full_small_zones;
    while (zone) { total += show_zone_ex(zone); zone = zone->next; }

    write_str("LARGE : ");
    write_hex((uintptr_t)global_heap.full_large_blocks);
    write_char('\n');
    block_t *block = global_heap.full_large_blocks;
    while (block) {
        void *start = (char *)block + sizeof(block_t) + sizeof(void *);
        void *end = (char *)start + block->size;
        write_hex((uintptr_t)start);
        write_str(" - ");
        write_hex((uintptr_t)end);
        write_str(" : ");
        write_dec(block->size);
        write_str(" bytes\n");
        hex_dump(start, block->size);
        total += block->size;
        block = block->next;
    }
    

    write_str("Total : ");
    write_dec(total);
    write_str(" bytes\n");
}