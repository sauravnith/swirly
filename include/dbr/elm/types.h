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
#ifndef DBR_ASH_TYPES_H
#define DBR_ASH_TYPES_H

#include <dbr/ash/date.h>
#include <dbr/ash/list.h>
#include <dbr/ash/queue.h>
#include <dbr/ash/slnode.h>
#include <dbr/ash/tree.h>

#include <sys/types.h> // ssize_t

/**
 * @addtogroup Types
 * @{
 */

typedef long DbrIncs;

/**
 * Milliseconds since Unix epoch.
 */

typedef long DbrMillis;

typedef DbrIncs DbrLots;

/**
 * Unit representing the minimum price increment.
 */

typedef DbrIncs DbrTicks;

/**
 * Sum of lots and ticks.
 */

typedef DbrIncs DbrLicks;

enum DbrEntity {
    /**
     * Account.
     */
    DBR_ENTITY_ACCNT = 1,
    /**
     * Contract.
     */
    DBR_ENTITY_CONTR,
    /**
     * Membership.
     */
    DBR_ENTITY_MEMB,
    /**
     * Order.
     */
    DBR_ENTITY_ORDER,
    /**
     * Execution.
     */
    DBR_ENTITY_EXEC,
    /**
     * Position.
     */
    DBR_ENTITY_POSN
};

/** @} */

/**
 * @addtogroup Bank
 */

enum DbrReg {
    /**
     * Order identifier register index.
     */
    DBR_REG_ORDER = 0,
    /**
     * Execution identifier register index.
     */
    DBR_REG_EXEC,
    /**
     * Match identifier register index.
     */
    DBR_REG_MATCH,
    /**
     * Number of registers in @ref DbrBank file.
     */
    DBR_REG_LEN
};

/** @} */

/**
 * @addtogroup Pool
 */

typedef struct ElmPool* DbrPool;

/**
 * @addtogroup Accnt
 */

typedef struct FigAccnt* DbrAccnt;

/**
 * @addtogroup TypesRec
 * @{
 */

enum {
    /**
     * Maximum display characters.
     */
    DBR_DISPLAY_MAX = 64,
    /**
     * Maximum email characters.
     */
    DBR_EMAIL_MAX = 64,
    /**
     * Maximum mnemonic characters.
     */
    DBR_MNEM_MAX = 16,
    /**
     * Maximum reference characters.
     */
    DBR_REF_MAX = 64
};

/**
 * Description suitable for display on user-interface.
 */

typedef char DbrDisplay[DBR_DISPLAY_MAX];

/**
 * Email address.
 */

typedef char DbrEmail[DBR_EMAIL_MAX];

/**
 * Memorable identifier.
 */

typedef char DbrMnem[DBR_MNEM_MAX];

/**
 * Reference.
 */

typedef char DbrRef[DBR_REF_MAX];

/**
 * Union used for record enrichment.
 */

union DbrURec {
    DbrIden id_only;
    struct DbrRec* rec;
};

struct DbrRec {
    /**
     * @publicsection
     */
    int type;
    DbrIden id;
    DbrMnem mnem;
    DbrDisplay display;
    union {
        struct {
            DbrEmail email;
            DbrAccnt state;
        } accnt;
        struct {
            DbrMnem asset_type;
            DbrMnem asset;
            DbrMnem ccy;
            int tick_numer;
            int tick_denom;
            // Transient.
            double price_inc;
            int lot_numer;
            int lot_denom;
            // Transient.
            double qty_inc;
            // Transient.
            int price_dp;
            int pip_dp;
            // Transient.
            int qty_dp;
            DbrLots min_lots;
            DbrLots max_lots;
        } contr;
    };
    /**
     * @privatesection
     */
    // Singly-linked for data model.
    struct DbrSlNode shared_node_;
    // Cache nodes.
    struct DbrSlNode id_node_;
    struct DbrSlNode mnem_node_;
};

static inline void
dbr_rec_init(struct DbrRec* rec)
{
    dbr_slnode_init(&rec->shared_node_);
    dbr_slnode_init(&rec->id_node_);
    dbr_slnode_init(&rec->mnem_node_);
}

static inline struct DbrRec*
dbr_shared_rec_entry(struct DbrSlNode* node)
{
    return dbr_implof(struct DbrRec, shared_node_, node);
}

/** @} */

/**
 * @addtogroup TypesMemb
 * @{
 */

struct DbrMemb {
    /**
     * @publicsection
     */
    union DbrURec user;
    union DbrURec group;
    /**
     * @privatesection
     */
    // Singly-linked for data model.
    struct DbrSlNode shared_node_;
    struct DbrRbNode user_node_;
    struct DbrRbNode group_node_;
};

static inline void
dbr_memb_init(struct DbrMemb* memb)
{
    dbr_slnode_init(&memb->shared_node_);
    dbr_rbnode_init(&memb->user_node_);
    dbr_rbnode_init(&memb->group_node_);
}

static inline struct DbrMemb*
dbr_shared_memb_entry(struct DbrSlNode* node)
{
    return dbr_implof(struct DbrMemb, shared_node_, node);
}

/** @} */

/**
 * @addtogroup Types
 * @{
 */

enum DbrState {
    DBR_STATE_NEW = 1,
    DBR_STATE_REVISE,
    DBR_STATE_CANCEL,
    DBR_STATE_TRADE
};

enum DbrAction {
    DBR_ACTION_BUY = 1,
    DBR_ACTION_SELL = -1
};

/**
 * Fields common to both DbrOrder and DbrExec.
 */

struct DbrCommon {
    union DbrURec user;
    union DbrURec group;
    union DbrURec contr;
    DbrJd settl_day;
    /**
     * Ref is optional.
     */
    DbrRef ref;
    /**
     * @sa enum DbrState
     */
    int state;
    /**
     * @sa enum DbrAction
     */
    int action;
    DbrTicks ticks;
    /**
     * Must be greater than zero.
     */
    DbrLots lots;
    /**
     * Must be greater than zero.
     */
    DbrLots resd;
    /**
     * Must not be greater that lots.
     */
    DbrLots exec;
    DbrTicks last_ticks;
    DbrLots last_lots;
    /**
     * Minimum to be filled by this order.
     */
    DbrLots min_lots;
};

/** @} */

/**
 * @addtogroup TypesOrder
 * @{
 */

struct DbrOrder {
    /**
     * @publicsection
     */
    /**
     * Set when order is associated with book.
     */
    struct DbrLevel* level;
    DbrIden id;
    struct DbrCommon c;
    DbrMillis created;
    DbrMillis modified;
    /**
     * @privatesection
     */
    // Singly-linked for data model.
    struct DbrSlNode shared_node_;
    // Singly-linked for index.
    struct DbrSlNode ref_node_;
    // Doubly-linked for side.
    struct DbrDlNode side_node_;
    struct DbrRbNode accnt_node_;
};

static inline void
dbr_order_init(struct DbrOrder* order)
{
    dbr_slnode_init(&order->shared_node_);
    dbr_slnode_init(&order->ref_node_);
    dbr_dlnode_init(&order->side_node_);
    dbr_rbnode_init(&order->accnt_node_);
}

static inline DbrBool
dbr_order_done(const struct DbrOrder* order)
{
    return order->c.resd == 0;
}

static inline struct DbrOrder*
dbr_shared_order_entry(struct DbrSlNode* node)
{
    return dbr_implof(struct DbrOrder, shared_node_, node);
}

/** @} */

/**
 * @addtogroup TypesLevel
 * @{
 */

struct DbrLevel {
    /**
     * @publicsection
     */
    struct DbrOrder* first_order;
    DbrTicks ticks;
    /**
     * Must be greater than zero.
     */
    DbrLots lots;
    /**
     * Must be greater than zero.
     */
    size_t count;
    /**
     * @privatesection
     */
    struct DbrRbNode side_node_;
};

static inline void
dbr_level_init(struct DbrLevel* level)
{
    dbr_rbnode_init(&level->side_node_);
}

/** @} */

/**
 * @addtogroup TypesExec
 * @{
 */

enum DbrRole {
    DBR_ROLE_MAKER = 1,
    DBR_ROLE_TAKER
};

struct DbrExec {
    /**
     * @publicsection
     */
    DbrIden id;
    DbrIden order;
    struct DbrCommon c;
    /**
     * @sa struct DbrMatch
     */
    DbrIden match;
    /**
     * @privatesection
     */
    /**
     * Reference count. Exec lifetime is managed by a shared reference count.
     */
    int refs_;
    /**
     * @publicsection
     */
    /**
     * @sa enum DbrRole
     */
    int role;
    union DbrURec cpty;
    DbrMillis created;
    /**
     * @privatesection
     */
    // Singly-linked for data model.
    struct DbrSlNode shared_node_;
    struct DbrRbNode accnt_node_;
};

static inline void
dbr_exec_init(struct DbrExec* exec)
{
    exec->refs_ = 1;
    dbr_slnode_init(&exec->shared_node_);
    dbr_rbnode_init(&exec->accnt_node_);
}

static inline DbrBool
dbr_exec_done(const struct DbrExec* exec)
{
    return exec->c.resd == 0;
}

static inline struct DbrExec*
dbr_shared_exec_entry(struct DbrSlNode* node)
{
    return dbr_implof(struct DbrExec, shared_node_, node);
}

/** @} */

/**
 * @addtogroup TypesMatch
 * @{
 */

/**
 * A Match represents two orders from opposing sides of the market that may trade.
 */

struct DbrMatch {
    /**
     * @publicsection
     */
    DbrIden id;
    DbrTicks ticks;
    DbrLots lots;
    struct DbrExec* taker_exec;
    struct DbrOrder* maker_order;
    struct DbrExec* maker_exec;
    struct DbrPosn* maker_posn;
    /**
     * @privatesection
     */
    /**
     * Pointer to next match or null.
     */
    struct DbrSlNode trans_node_;
};

static inline void
dbr_match_init(struct DbrMatch* match)
{
    dbr_slnode_init(&match->trans_node_);
}

/**
 * A single Transaction comprises many Matches.
 */
struct DbrTrans {
    struct DbrQueue matches;
    /**
     * All executions referenced in DbrTrans::matches.
     */
    struct DbrQueue execs;
    struct DbrPosn* taker_posn;
};

static inline void
dbr_trans_init(struct DbrTrans* trans)
{
    dbr_queue_init(&trans->matches);
    dbr_queue_init(&trans->execs);
    trans->taker_posn = NULL;
}

static inline struct DbrMatch*
dbr_trans_match_entry(struct DbrSlNode* node)
{
    return dbr_implof(struct DbrMatch, trans_node_, node);
}

/** @} */

/**
 * @addtogroup TypesPosn
 * @{
 */

struct DbrPosn {
    /**
     * @publicsection
     */
    union DbrURec accnt;
    union DbrURec contr;
    DbrJd settl_day;
    DbrLicks buy_licks;
    DbrLots buy_lots;
    DbrLicks sell_licks;
    DbrLots sell_lots;
    /**
     * @privatesection
     */
    // Singly-linked for data model.
    struct DbrSlNode shared_node_;
    struct DbrRbNode accnt_node_;
    struct DbrRbNode update_node_;
};

static inline void
dbr_posn_init(struct DbrPosn* posn)
{
    dbr_slnode_init(&posn->shared_node_);
    dbr_rbnode_init(&posn->accnt_node_);
    dbr_rbnode_init(&posn->update_node_);
}

/**
 * Synthetic posn key.
 */

static inline DbrKey
dbr_posn_key(DbrIden aid, DbrIden cid, DbrJd settl_day)
{
    enum {
        // 16 million ids.
        ID_MASK = (1 << 24) - 1,
        // 16 bits is sufficient for truncated Julian day.
        JD_MASK = (1 << 16) - 1
    };
    // Truncated Julian Day (TJD).
    const DbrJd tjd = dbr_jd_to_tjd(settl_day);
    return ((((DbrKey)aid) & ID_MASK) << 40) | ((((DbrKey)cid) & ID_MASK) << 16)
        | (((DbrKey)tjd) & JD_MASK);
}

static inline struct DbrPosn*
dbr_shared_posn_entry(struct DbrSlNode* node)
{
    return dbr_implof(struct DbrPosn, shared_node_, node);
}

/** @} */

/**
 * @addtogroup TypesView
 * @{
 */

enum {
    DBR_LEVEL_MAX = 3
};

struct DbrView {
    union DbrURec contr;
    DbrJd settl_day;
    DbrTicks bid_ticks[DBR_LEVEL_MAX];
    DbrLots bid_lots[DBR_LEVEL_MAX];
    size_t bid_count[DBR_LEVEL_MAX];
    DbrTicks offer_ticks[DBR_LEVEL_MAX];
    DbrLots offer_lots[DBR_LEVEL_MAX];
    size_t offer_count[DBR_LEVEL_MAX];
    DbrMillis created;
    /**
     * @privatesection
     */
    // Singly-linked for data model.
    struct DbrSlNode shared_node_;
    struct DbrRbNode clnt_node_;
    struct DbrRbNode update_node_;
};

static inline void
dbr_view_init(struct DbrView* view)
{
    dbr_slnode_init(&view->shared_node_);
    dbr_rbnode_init(&view->clnt_node_);
    dbr_rbnode_init(&view->update_node_);
}

/**
 * Synthetic view key.
 */

static inline DbrKey
dbr_view_key(DbrIden cid, DbrJd settl_day)
{
    enum {
        // 16 million ids.
        ID_MASK = (1 << 24) - 1,
        // 16 bits is sufficient for truncated Julian day.
        JD_MASK = (1 << 16) - 1
    };
    // Truncated Julian Day (TJD).
    const DbrJd tjd = dbr_jd_to_tjd(settl_day);
    return ((((DbrKey)cid) & ID_MASK) << 16) | (((DbrKey)tjd) & JD_MASK);
}

static inline struct DbrView*
dbr_shared_view_entry(struct DbrSlNode* node)
{
    return dbr_implof(struct DbrView, shared_node_, node);
}

/** @} */

/**
 * @addtogroup TypesSide
 * @{
 */

struct DbrSide {
    DbrPool pool;
    struct DbrTree levels;
    struct DbrList orders;
    /**
     * Last trade information.
     */
    DbrTicks last_ticks;
    DbrLots last_lots;
    DbrMillis last_time;
};

/** @} */

/**
 * @addtogroup TypesBook
 * @{
 */

struct DbrBook {
    struct DbrRec* crec;
    DbrJd settl_day;
    struct DbrSide bid_side;
    struct DbrSide offer_side;
    /**
     * @privatesection
     */
    struct DbrRbNode serv_node_;
    struct DbrRbNode update_node_;
};

/**
 * Synthetic book key.
 */

static inline DbrKey
dbr_book_key(DbrIden cid, DbrJd settl_day)
{
    enum {
        // 16 million ids.
        ID_MASK = (1 << 24) - 1,
        // 16 bits is sufficient for truncated Julian day.
        JD_MASK = (1 << 16) - 1
    };
    // Truncated Julian Day (TJD).
    const DbrJd tjd = dbr_jd_to_tjd(settl_day);
    return ((((DbrKey)cid) & ID_MASK) << 16) | (((DbrKey)tjd) & JD_MASK);
}

/** @} */

/**
 * @addtogroup TypesSess
 * @{
 */

struct DbrSub {
    /**
     * @privatesection
     */
    int refs_;
    struct DbrRbNode sess_node_;
};

static inline void
dbr_sub_init(struct DbrSub* sub)
{
    sub->refs_ = 1;
    dbr_rbnode_init(&sub->sess_node_);
}

static inline struct DbrSub*
dbr_sess_sub_entry(struct DbrRbNode* node)
{
    return dbr_implof(struct DbrSub, sess_node_, node);
}

struct DbrSess {
    DbrUuid uuid;
    DbrPool pool;
    /**
     * The heartbeat interval requested by remote side.
     */
    DbrMillis hbint;
    struct DbrTree subs;
    struct DbrTree users;
    /**
     * @privatesection
     */
    long marker_;
    // FigSessIdx bucket list.
    struct DbrSlNode uuid_node_;
};

/** @} */

#endif // DBR_ASH_TYPES_H