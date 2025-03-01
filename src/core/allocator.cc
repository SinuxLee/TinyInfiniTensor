#include "core/allocator.h"
#include <utility>

namespace infini
{
    Allocator::Allocator(Runtime runtime) : runtime(runtime)
    {
        used = 0;
        peak = 0;
        ptr = nullptr;

        // 'alignment' defaults to sizeof(uint64_t), because it is the length of
        // the longest data type currently supported by the DataType field of
        // the tensor
        alignment = sizeof(uint64_t);
    }

    Allocator::~Allocator()
    {
        if (this->ptr != nullptr)
        {
            runtime->dealloc(this->ptr);
        }
    }

    size_t Allocator::alloc(size_t size)
    {
        IT_ASSERT(this->ptr == nullptr);
        // pad the size to the multiple of alignment
        size = this->getAlignedSize(size);

        auto offset = size_t(0);
        auto it = free_blocks.begin();

        while (it != free_blocks.end()) {
            if (it->second >= size) {
                offset = it->first;
                free_blocks.erase(it);
                if (it->second > size) {
                    free_blocks[offset + size] = it->second - size;
                }
                break;
            }
            it++;
        }
        
        if (it == free_blocks.end()) {
            offset = used;
            used += size;
            if (used > peak) {
                peak = used;
            }
        }

        std::cout << "alloc: " << offset << " " << size << std::endl;
        return offset;
    }

    void Allocator::free(size_t addr, size_t size)
    {
        IT_ASSERT(this->ptr == nullptr);
        size = getAlignedSize(size);

        auto it = free_blocks.begin(); 
        while (it != free_blocks.end()) {
            if (it->first + it->second == addr) {
                addr = it->first;
                size += it->second;
                it = free_blocks.erase(it);
                continue;
            }
            
            if (it->first == addr + size) {
                size += it->second;
                it = free_blocks.erase(it);
                continue;
            }

            it++;
        }

        std::cout << "free: " << addr << " " << size << std::endl;
        if (addr + size >= used) {
            used -= size;
        }else{
            free_blocks[addr] = size;
        }
    }

    void *Allocator::getPtr()
    {
        if (this->ptr == nullptr)
        {
            this->ptr = runtime->alloc(this->peak);
            printf("Allocator really alloc: %p %lu bytes\n", this->ptr, peak);
        }
        return this->ptr;
    }

    size_t Allocator::getAlignedSize(size_t size)
    {
        return ((size - 1) / this->alignment + 1) * this->alignment;
    }

    void Allocator::info()
    {
        std::cout << "Used memory: " << this->used
                  << ", peak memory: " << this->peak << std::endl;
    }
}
