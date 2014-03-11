/*
 *  Copyright (C) 2013, 2014 Mark Aylett <mark.aylett@gmail.com>
 *
 *  This file is part of Doobry written by Mark Aylett.
 *
 *  Doobry is free software; you can redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Doobry is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 *  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with this program; if
 *  not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301 USA.
 */
#ifndef DBR_STORE_H
#define DBR_STORE_H

#include <dbr/defs.h>

#include <stddef.h> // size_t

/**
 * @addtogroup Store
 * @{
 */

struct DbrStore {
    int fd;
    size_t len;
    long* arr;
};

DBR_API void
dbr_store_term(struct DbrStore* store);

DBR_API DbrBool
dbr_store_init(struct DbrStore* store, const char* path, size_t len);

static inline long
dbr_store_get(const struct DbrStore* store, size_t idx)
{
    return __atomic_load_n(store->arr + idx, __ATOMIC_SEQ_CST);
}

static inline void
dbr_store_set(struct DbrStore* store, size_t idx, long val)
{
    __atomic_store_n(store->arr + idx, val, __ATOMIC_SEQ_CST);
}

static inline long
dbr_store_next(struct DbrStore* store, size_t idx)
{
    return __atomic_add_fetch(store->arr + idx, 1L, __ATOMIC_SEQ_CST);
}

/** @} */

#endif // DBR_STORE_H
