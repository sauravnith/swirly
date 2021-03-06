/*
 * The Restful Matching-Engine.
 * Copyright (C) 2013, 2017 Swirly Cloud Limited.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include "MemCtx.hpp"

#include "MemMap.hpp"
#include "MemPool.hpp"

#include <fcntl.h>

using namespace std;

namespace swirly {
namespace {

File reserveFile(const char* path, size_t size)
{
    File file{openFile(path, O_RDWR | O_CREAT, 0644)};
    resize(file.get(), size);
    return file;
}

} // anonymous

struct MemCtx::Impl {
    explicit Impl(size_t maxSize)
        : maxSize{maxSize},
          memMap{openMemMap(nullptr, PageSize + maxSize, PROT_READ | PROT_WRITE,
                            MAP_ANON | MAP_PRIVATE, -1, 0)},
          pool(*static_cast<MemPool*>(memMap.get().data()))
    {
    }
    Impl(const char* path, size_t maxSize)
        : maxSize{maxSize},
          file{reserveFile(path, PageSize + maxSize)},
          memMap{openMemMap(nullptr, PageSize + maxSize, PROT_READ | PROT_WRITE, MAP_SHARED,
                            file.get(), 0)},
          pool(*static_cast<MemPool*>(memMap.get().data()))
    {
    }
    void* alloc(size_t size)
    {
        void* addr;
        switch (ceilCacheLine(size)) {
        case 1 << 6:
            addr = allocBlock(pool, pool.free1, maxSize);
            break;
        case 2 << 6:
            addr = allocBlock(pool, pool.free2, maxSize);
            break;
        case 3 << 6:
            addr = allocBlock(pool, pool.free3, maxSize);
            break;
        case 4 << 6:
            addr = allocBlock(pool, pool.free4, maxSize);
            break;
        case 5 << 6:
            addr = allocBlock(pool, pool.free5, maxSize);
            break;
        case 6 << 6:
            addr = allocBlock(pool, pool.free6, maxSize);
            break;
        default:
            throw bad_alloc{};
        }
        return addr;
    }
    void dealloc(void* addr, size_t size) noexcept
    {
        switch (ceilCacheLine(size)) {
        case 1 << 6:
            deallocBlock(pool, pool.free1, addr);
            break;
        case 2 << 6:
            deallocBlock(pool, pool.free2, addr);
            break;
        case 3 << 6:
            deallocBlock(pool, pool.free3, addr);
            break;
        case 4 << 6:
            deallocBlock(pool, pool.free4, addr);
            break;
        case 5 << 6:
            deallocBlock(pool, pool.free5, addr);
            break;
        case 6 << 6:
            deallocBlock(pool, pool.free6, addr);
            break;
        default:
            abort();
        }
    }
    const size_t maxSize;
    File file;
    MemMap memMap;
    MemPool& pool;
};

MemCtx::MemCtx(size_t maxSize) : impl_{make_unique<Impl>(maxSize)}
{
}

MemCtx::MemCtx(const char* path, size_t maxSize) : impl_{make_unique<Impl>(path, maxSize)}
{
}

MemCtx::MemCtx() = default;

MemCtx::~MemCtx() noexcept = default;

// Move.
MemCtx::MemCtx(MemCtx&&) noexcept = default;
MemCtx& MemCtx::operator=(MemCtx&&) noexcept = default;

size_t MemCtx::maxSize() noexcept
{
    assert(impl_);
    return impl_->maxSize;
}

void* MemCtx::alloc(size_t size)
{
    assert(impl_);
    return impl_->alloc(size);
}

void MemCtx::dealloc(void* addr, size_t size) noexcept
{
    assert(impl_);
    impl_->dealloc(addr, size);
}

} // swirly
