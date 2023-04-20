#include "stdlib.h"

#ifndef CMALLOC
#define CMALLOC

#include "math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef char cmi8_t;
typedef short cmi16_t;
typedef int cmi32_t;
typedef long cmi64_t;
typedef unsigned char cmu8_t;
typedef unsigned short cmu16_t;
typedef unsigned int cmu32_t;
typedef unsigned long cmu64_t;
typedef cmu64_t cmsize_t;

struct block
{
    void* data; // Pointer to the start of the block
    void* bitmap_ptr; // Pointer to the bitmap 
    void* last_found; // The last location that was found to be free (in the bitmap)
    cmsize_t num_locs; // Number of locations in the bitmap
    cmsize_t size; // The number of bits that 1 location is worth
};

void* cmalloc(cmsize_t size);
void cfree(void* loc);
inline void* cmalloc_array(cmsize_t size, cmsize_t array_size) { return cmalloc(size * array_size); }
void cfree_array(void* loc) { free(loc); }

#define CMALLOC_COMPILE

#ifdef CMALLOC_COMPILE
block* blocks;

void* sys_alloc(cmsize_t size)
{
    // For now, just use malloc
    // In the future, use mmap/sbrk or VirtualAlloc
    return malloc(size * 4096);
}

void sys_free(void* loc)
{
    // For now, just use free
    // In the future, use munmap/sbrk or VirtualFree
    free(loc);
}

inline cmi32_t bit_scan_fw(cmu64_t x) 
{
   asm ("bsfq %0, %0" : "=r" (x) : "0" (x));
   return (cmi32_t) x;
}

cmsize_t find_free(block bk)
{
    // Find the first free location in the bitmap
    // Return the location

    for (cmsize_t i = 0; i < bk.num_locs; i++)
    {
        if (*(cmu64_t*)((cmu8_t*)bk.last_found + i) != 0) // If the location block is not free
        {
            // Update the last found location block
            bk.last_found = (cmu8_t*)bk.last_found + i;

            // Find the first free bit in the location block
            cmi32_t loc_in_block = bit_scan_fw(*(cmu64_t*)(bk.last_found));

            // Update the bitmap
            *(cmu64_t*)(bk.last_found) &= ~(1 << loc_in_block);

            // Return the location
            return (cmu64_t) ((cmu8_t*)bk.last_found - (cmu8_t*)bk.bitmap_ptr) * bk.size + loc_in_block;
        }
    }
}

void* cmalloc(cmsize_t size)
{
    // Find the block that has the closest to size of the allocation 
    // Find the first free location in the bitmap
    // Expand to actual pointer in block

    if (size > 4095) return sys_alloc((long long) size / 4096 + 1); // If the size is too big for the allocator, use the system allocator (page allocation)

    // Find the block or the closest block
    double log2_size = log2(size);
    if (log2_size - (long long) log2_size != 0) // If the size is not a power of 2
    {
        log2_size = (long long) log2_size + 1;
    }

    size_t first_free = find_free(blocks[(size_t) log2_size]);
    return (cmu8_t*)blocks[(size_t) log2_size].data + first_free;
}

void cfree(void* loc)
{
    // Find the block that the location is in (by iteration)
    // Free it from bitmap

    for (size_t i = 0; i < 13; i++)
    {
        if (blocks[i].data <= loc && loc < (cmu8_t*)blocks[i].data + blocks[i].num_locs * blocks[i].size)
        {
            // Find the location in the bitmap
            cmsize_t loc_in_bitmap = ((cmu8_t*)loc - (cmu8_t*)blocks[i].data) / blocks[i].size;

            // Find the location block in the bitmap
            cmsize_t loc_block = loc_in_bitmap / 64;

            // Find the location in the location block
            cmsize_t loc_in_block = loc_in_bitmap % 64;

            // Update the bitmap
            *((cmu8_t*)(blocks[i].bitmap_ptr) + loc_block) |= 1 << loc_in_block;
        }
    }
}

void init()
{
    cmsize_t num_locs = 4096; // Number of locations in the bitmap

    // Initialize the blocks
    blocks = (block*) sys_alloc(13 * sizeof(block));

    for (size_t i = 0; i < 13; i++)
    {
        blocks[i].data = sys_alloc((1 << i));
        blocks[i].bitmap_ptr = sys_alloc(1);
        blocks[i].last_found = blocks[i].bitmap_ptr;
        blocks[i].num_locs = 4096;
        blocks[i].size = 1 << i;
    }
}

void deinit()
{
    // Deinitialize the blocks
    for (size_t i = 0; i < 13; i++)
    {
        sys_free(blocks[i].data);
        sys_free(blocks[i].bitmap_ptr);
    }

    sys_free(blocks);
}

#ifdef __cplusplus
}
#endif
#endif
#endif





