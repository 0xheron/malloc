#include "malloc.h"
#include <iostream>

int main(void)
{
    init();
    int* alloc = (int*) cmalloc(sizeof(int));
    *alloc = 5;
    std::cout << *alloc << std::endl;
    cfree(alloc);
    deinit();
}