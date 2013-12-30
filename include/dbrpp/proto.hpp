/*
 *  Copyright (C) 2013 Mark Aylett <mark.aylett@gmail.com>
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
#ifndef DBRPP_PROTO_HPP
#define DBRPP_PROTO_HPP

#include <dbrpp/except.hpp>

#include <dbr/proto.h>

namespace dbr {

/**
 * @addtogroup ProtoTrader
 * @{
 */

inline size_t
trader_len(const DbrRec& trec) noexcept
{
    return dbr_trader_len(&trec);
}

inline char*
write_trader(char* buf, const DbrRec& trec) noexcept
{
    return dbr_write_trader(buf, &trec);
}

inline const char*
read_trader(const char* buf, DbrRec& trec)
{
    buf = dbr_read_trader(buf, &trec);
    if (!buf)
        throw_exception();
    return buf;
}

/** @} */

/**
 * @addtogroup ProtoAccnt
 * @{
 */

inline size_t
accnt_len(const DbrRec& arec) noexcept
{
    return dbr_accnt_len(&arec);
}

inline char*
write_accnt(char* buf, const DbrRec& arec) noexcept
{
    return dbr_write_accnt(buf, &arec);
}

inline const char*
read_accnt(const char* buf, DbrRec& arec)
{
    buf = dbr_read_accnt(buf, &arec);
    if (!buf)
        throw_exception();
    return buf;
}

/** @} */

/**
 * @addtogroup ProtoContr
 * @{
 */

inline size_t
contr_len(const DbrRec& crec) noexcept
{
    return dbr_contr_len(&crec);
}

inline char*
write_contr(char* buf, const DbrRec& crec) noexcept
{
    return dbr_write_contr(buf, &crec);
}

inline const char*
read_contr(const char* buf, DbrRec& crec)
{
    buf = dbr_read_contr(buf, &crec);
    if (!buf)
        throw_exception();
    return buf;
}

/** @} */

/**
 * @addtogroup ProtoOrder
 * @{
 */

inline size_t
order_len(const DbrOrder& order, DbrBool enriched) noexcept
{
    return dbr_order_len(&order, enriched);
}

inline char*
write_order(char* buf, const DbrOrder& order, DbrBool enriched) noexcept
{
    return dbr_write_order(buf, &order, enriched);
}

inline const char*
read_order(const char* buf, DbrOrder& order)
{
    buf = dbr_read_order(buf, &order);
    if (!buf)
        throw_exception();
    return buf;
}

/** @} */

/**
 * @addtogroup ProtoExec
 * @{
 */

inline size_t
exec_len(const DbrExec& exec, DbrBool enriched) noexcept
{
    return dbr_exec_len(&exec, enriched);
}

inline char*
write_exec(char* buf, const DbrExec& exec, DbrBool enriched) noexcept
{
    return dbr_write_exec(buf, &exec, enriched);
}

inline const char*
read_exec(const char* buf, DbrExec& exec)
{
    buf = dbr_read_exec(buf, &exec);
    if (!buf)
        throw_exception();
    return buf;
}

/** @} */

/**
 * @addtogroup ProtoMemb
 * @{
 */

inline size_t
memb_len(const DbrMemb& memb, DbrBool enriched) noexcept
{
    return dbr_memb_len(&memb, enriched);
}

inline char*
write_memb(char* buf, const DbrMemb& memb, DbrBool enriched) noexcept
{
    return dbr_write_memb(buf, &memb, enriched);
}

inline const char*
read_memb(const char* buf, DbrMemb& memb)
{
    buf = dbr_read_memb(buf, &memb);
    if (!buf)
        throw_exception();
    return buf;
}

/** @} */

/**
 * @addtogroup ProtoPosn
 * @{
 */

inline size_t
posn_len(const DbrPosn& posn, DbrBool enriched) noexcept
{
    return dbr_posn_len(&posn, enriched);
}

inline char*
write_posn(char* buf, const DbrPosn& posn, DbrBool enriched) noexcept
{
    return dbr_write_posn(buf, &posn, enriched);
}

inline const char*
read_posn(const char* buf, DbrPosn& posn)
{
    buf = dbr_read_posn(buf, &posn);
    if (!buf)
        throw_exception();
    return buf;
}

/** @} */

/**
 * @addtogroup ProtoView
 * @{
 */

inline size_t
view_len(const DbrView& view, DbrBool enriched) noexcept
{
    return dbr_view_len(&view, enriched);
}

inline char*
write_view(char* buf, const DbrView& view, DbrBool enriched) noexcept
{
    return dbr_write_view(buf, &view, enriched);
}

inline const char*
read_view(const char* buf, DbrView& view)
{
    buf = dbr_read_view(buf, &view);
    if (!buf)
        throw_exception();
    return buf;
}

/** @} */

} // dbr

#endif // DBRPP_PROTO_HPP