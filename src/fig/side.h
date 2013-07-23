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
#ifndef FIG_SIDE_H
#define FIG_SIDE_H

#include <dbr/list.h>
#include <dbr/tree.h>
#include <dbr/types.h>

#include <assert.h>
#include <stdbool.h>

struct FigSide {
    struct FigPool* pool;
    struct DbrTree levels;
    struct DbrList orders;
    // Last trade information.
    DbrTicks last_ticks;
    DbrLots last_lots;
    DbrMillis last_time;
};

DBR_EXTERN void
fig_side_init(struct FigSide* side, struct FigPool* pool);

// Levels associated with side are also freed.
// Assumes that pointer is not null.

DBR_EXTERN void
fig_side_term(struct FigSide* side);

// Insert order without affecting revision information.
// Assumes that order does not already belong to a side. I.e. it assumes that level is null.
// Assumes that order-id and party-ref (if ref is not empty) are unique. The fig_side_find_ref() can
// be used to detect duplicate party-refs. Returns false if level allocation fails.

DBR_EXTERN DbrBool
fig_side_insert_order(struct FigSide* side, struct DbrOrder* order);

// Internal housekeeping aside, the state of the order is not affected by this function.

DBR_EXTERN void
fig_side_remove_order(struct FigSide* side, struct DbrOrder* order);

// Reduce residual lots by delta. If the resulting residual is zero, then the order is removed from
// the side.

DBR_EXTERN void
fig_side_take_order(struct FigSide* side, struct DbrOrder* order, DbrLots delta, DbrMillis now);

DBR_EXTERN DbrBool
fig_side_revise_order(struct FigSide* side, struct DbrOrder* order, DbrLots lots, DbrMillis now);

static inline void
fig_side_cancel_order(struct FigSide* side, struct DbrOrder* order, DbrMillis now)
{
    fig_side_remove_order(side, order);
    ++order->rev;
    order->status = DBR_CANCELLED;
    // Note that executed lots is not affected.
    order->resd = 0;
    order->modified = now;
}

// Side Order.

static inline struct DbrDlNode*
fig_side_first_order(const struct FigSide* side)
{
    return dbr_list_first(&side->orders);
}

static inline struct DbrDlNode*
fig_side_last_order(const struct FigSide* side)
{
    return dbr_list_last(&side->orders);
}

static inline struct DbrDlNode*
fig_side_end_order(const struct FigSide* side)
{
    return dbr_list_end(&side->orders);
}

static inline DbrBool
fig_side_empty_order(const struct FigSide* side)
{
    return dbr_list_empty(&side->orders);
}

// Side Level.

static inline struct DbrRbNode*
fig_side_find_level(const struct FigSide* side, DbrIden id)
{
    return dbr_tree_find(&side->levels, id);
}

static inline struct DbrRbNode*
fig_side_first_level(const struct FigSide* side)
{
    return dbr_tree_first(&side->levels);
}

static inline struct DbrRbNode*
fig_side_last_level(const struct FigSide* side)
{
    return dbr_tree_last(&side->levels);
}

static inline struct DbrRbNode*
fig_side_end_level(const struct FigSide* side)
{
    return dbr_tree_end(&side->levels);
}

static inline DbrBool
fig_side_empty_level(const struct FigSide* side)
{
    return dbr_tree_empty(&side->levels);
}

static inline DbrTicks
fig_side_last_ticks(const struct FigSide* side)
{
    return side->last_ticks;
}

static inline DbrLots
fig_side_last_lots(const struct FigSide* side)
{
    return side->last_lots;
}

static inline DbrMillis
fig_side_last_time(const struct FigSide* side)
{
    return side->last_time;
}

#endif // FIG_SIDE_H