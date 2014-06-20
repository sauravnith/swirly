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
#ifndef DBR_ASH_DEFS_H
#define DBR_ASH_DEFS_H

/**
 * @addtogroup Defs
 * @{
 */

#if !defined(__cplusplus)
# define DBR_EXTERN extern
#else  // defined(__cplusplus)
# define restrict __restrict__
# define DBR_EXTERN extern "C"
#endif // defined(__cplusplus)

#define DBR_API DBR_EXTERN __attribute__((visibility ("default")))

enum {
    /**
     * Boolean logical false.
     */
    DBR_FALSE,
    /**
     * Boolean logical true.
     */
    DBR_TRUE
};

enum {
    /**
     * Maximum length of error messages.
     */
    DBR_ERRMSG_MAX = 128,
    /**
     * Maximum length of log messages.
     */
    DBR_LOGMSG_MAX = 512,
    /**
     * A UUID is represented by 32 lowercase hexadecimal digits, displayed in five groups separated
     * by hyphens.
     */
    DBR_UUID_MAX = 36
};

/**
 * Boolean logic type.
 */
typedef int DbrBool;
/**
 * Identifier type.
 */
typedef long DbrIden;
/**
 * Key type used by associative containers.
 * @sa struct RbNode
 */
typedef unsigned long DbrKey;
/**
 * Universally unique identifier.
 */
typedef unsigned char DbrUuid[16];

/**
 * @return the smallest of @a a and @a b. If both are equivalent, @a a is returned.
 */
#define dbr_min(a, b) ((a) <= (b) ? (a) : (b))

/**
 * @return the largest of @a a and @a b. If both are equivalent, @a a is returned.
 */
#define dbr_max(a, b) ((a) >= (b) ? (a) : (b))

/**
 * Expected branch.
 */
#if !defined(__GNUC__)
#define dbr_likely(x)   (x)
#else  // defined(__GNUC__)
#define dbr_likely(x)   __builtin_expect(!!(x), 1)
#endif // defined(__GNUC__)

/**
 * Unexpected branch.
 */
#if !defined(__GNUC__)
#define dbr_unlikely(x) (x)
#else  // defined(__GNUC__)
#define dbr_unlikely(x) __builtin_expect(!!(x), 0)
#endif // defined(__GNUC__)

/**
 * http://en.wikipedia.org/wiki/Offsetof
 */
#define dbr_offsetof(st, m) __builtin_offsetof(st, m)
#define dbr_implof(s, m, p) (s*)((char*)(p) - dbr_offsetof(s, m))

// FIXME: Review memory barrier usage.
#define dbr_wmb() do { } while (0)
#define dbr_rmb() do { } while (0)

/** @} */

#endif // DBR_ASH_DEFS_H