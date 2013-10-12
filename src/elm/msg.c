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
#include <dbr/msg.h>

#include <dbr/err.h>
#include <dbr/pack.h>
#include <dbr/proto.h>
#include <dbr/queue.h>

#include <zmq.h>

#include <stdlib.h> // abort()

static const char STATUS_REP_FORMAT[] = "is";
static const char PLACE_ORDER_REQ_FORMAT[] = "mmmisillll";
static const char REVISE_ORDER_ID_REQ_FORMAT[] = "mll";
static const char REVISE_ORDER_REF_REQ_FORMAT[] = "msl";
static const char CANCEL_ORDER_ID_REQ_FORMAT[] = "ml";
static const char CANCEL_ORDER_REF_REQ_FORMAT[] = "ms";
static const char ARCHIVE_ORDER_REQ_FORMAT[] = "ml";
static const char ARCHIVE_TRADE_REQ_FORMAT[] = "ml";

static void
free_result_posns(struct DbrSlNode* first, DbrPool pool)
{
    struct DbrSlNode* node = first;
    while (node) {
        struct DbrPosn* posn = dbr_result_posn_entry(node);
        node = node->next;
        dbr_pool_free_posn(pool, posn);
    }
}

static void
free_result_trades(struct DbrSlNode* first, DbrPool pool)
{
    struct DbrSlNode* node = first;
    while (node) {
        struct DbrTrade* trade = dbr_result_trade_entry(node);
        node = node->next;
        dbr_pool_free_trade(pool, trade);
    }
}

static void
free_trans_stmts(struct DbrSlNode* first, DbrPool pool)
{
    struct DbrSlNode* node = first;
    while (node) {
        struct DbrStmt* stmt = dbr_trans_stmt_entry(node);
        node = node->next;
        dbr_pool_free_stmt(pool, stmt);
    }
}

static const char*
read_entity_trader(const char* buf, DbrPool pool, struct DbrQueue* queue)
{
    struct DbrRec* rec = dbr_pool_alloc_rec(pool);
    if (!rec)
        goto fail1;
    if (!(buf = dbr_read_trader(buf, rec))) {
        dbr_pool_free_rec(pool, rec);
        goto fail1;
    }
    dbr_queue_insert_back(queue, &rec->entity_node_);
    return buf;
 fail1:
    return NULL;
}

static const char*
read_entity_accnt(const char* buf, DbrPool pool, struct DbrQueue* queue)
{
    struct DbrRec* rec = dbr_pool_alloc_rec(pool);
    if (!rec)
        goto fail1;
    if (!(buf = dbr_read_accnt(buf, rec))) {
        dbr_pool_free_rec(pool, rec);
        goto fail1;
    }
    dbr_queue_insert_back(queue, &rec->entity_node_);
    return buf;
 fail1:
    return NULL;
}

static const char*
read_entity_contr(const char* buf, DbrPool pool, struct DbrQueue* queue)
{
    struct DbrRec* rec = dbr_pool_alloc_rec(pool);
    if (!rec)
        goto fail1;
    if (!(buf = dbr_read_contr(buf, rec))) {
        dbr_pool_free_rec(pool, rec);
        goto fail1;
    }
    dbr_queue_insert_back(queue, &rec->entity_node_);
    return buf;
 fail1:
    return NULL;
}

static const char*
read_entity_order(const char* buf, DbrPool pool, struct DbrQueue* queue)
{
    struct DbrOrder* order = dbr_pool_alloc_order(pool);
    if (!order)
        goto fail1;
    if (!(buf = dbr_read_order(buf, order))) {
        dbr_pool_free_order(pool, order);
        goto fail1;
    }
    dbr_queue_insert_back(queue, &order->entity_node_);
    return buf;
 fail1:
    return NULL;
}

static const char*
read_entity_memb(const char* buf, DbrPool pool, struct DbrQueue* queue)
{
    struct DbrMemb* memb = dbr_pool_alloc_memb(pool);
    if (!memb)
        goto fail1;
    if (!(buf = dbr_read_memb(buf, memb))) {
        dbr_pool_free_memb(pool, memb);
        goto fail1;
    }
    dbr_queue_insert_back(queue, &memb->entity_node_);
    return buf;
 fail1:
    return NULL;
}

static const char*
read_entity_trade(const char* buf, DbrPool pool, struct DbrQueue* queue)
{
    struct DbrTrade* trade = dbr_pool_alloc_trade(pool);
    if (!trade)
        goto fail1;
    if (!(buf = dbr_read_trade(buf, trade))) {
        dbr_pool_free_trade(pool, trade);
        goto fail1;
    }
    dbr_queue_insert_back(queue, &trade->entity_node_);
    return buf;
 fail1:
    return NULL;
}

static const char*
read_entity_posn(const char* buf, DbrPool pool, struct DbrQueue* queue)
{
    struct DbrPosn* posn = dbr_pool_alloc_posn(pool);
    if (!posn)
        goto fail1;
    if (!(buf = dbr_read_posn(buf, posn))) {
        dbr_pool_free_posn(pool, posn);
        goto fail1;
    }
    dbr_queue_insert_back(queue, &posn->entity_node_);
    return buf;
 fail1:
    return NULL;
}

static const char*
read_trans_stmt(const char* buf, DbrPool pool, struct DbrQueue* queue)
{
    struct DbrStmt* stmt = dbr_pool_alloc_stmt(pool);
    if (!stmt)
        goto fail1;
    if (!(buf = dbr_read_stmt(buf, stmt))) {
        dbr_pool_free_stmt(pool, stmt);
        goto fail1;
    }
    dbr_queue_insert_back(queue, &stmt->trans_node_);
    return buf;
 fail1:
    return NULL;
}

static const char*
read_result_posn(const char* buf, DbrPool pool, struct DbrQueue* queue)
{
    struct DbrPosn* posn = dbr_pool_alloc_posn(pool);
    if (!posn)
        goto fail1;
    if (!(buf = dbr_read_posn(buf, posn))) {
        dbr_pool_free_posn(pool, posn);
        goto fail1;
    }
    dbr_queue_insert_back(queue, &posn->result_node_);
    return buf;
 fail1:
    return NULL;
}

static const char*
read_result_trade(const char* buf, DbrPool pool, struct DbrQueue* queue)
{
    struct DbrTrade* trade = dbr_pool_alloc_trade(pool);
    if (!trade)
        goto fail1;
    if (!(buf = dbr_read_trade(buf, trade))) {
        dbr_pool_free_trade(pool, trade);
        goto fail1;
    }
    dbr_queue_insert_back(queue, &trade->result_node_);
    return buf;
 fail1:
    return NULL;
}

DBR_API size_t
dbr_msg_len(struct DbrMsg* msg)
{
    size_t n = dbr_packleni(msg->type);
    switch (msg->type) {
    case DBR_STATUS_REP:
        n += dbr_packlenf(STATUS_REP_FORMAT,
                          msg->status_rep.num,
                          DBR_ERRMSG_MAX, msg->status_rep.msg);
        break;
    case DBR_ENTITY_REP:
        n += dbr_packleni(msg->entity_rep.type);
        switch (msg->entity_rep.type) {
        case DBR_TRADER:
            msg->entity_rep.count_ = 0;
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrRec* rec = dbr_rec_entry(node);
                n += dbr_trader_len(rec);
                ++msg->entity_rep.count_;
            }
            n += dbr_packlenz(msg->entity_rep.count_);
            break;
        case DBR_ACCNT:
            msg->entity_rep.count_ = 0;
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrRec* rec = dbr_rec_entry(node);
                n += dbr_accnt_len(rec);
                ++msg->entity_rep.count_;
            }
            n += dbr_packlenz(msg->entity_rep.count_);
            break;
        case DBR_CONTR:
            msg->entity_rep.count_ = 0;
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrRec* rec = dbr_rec_entry(node);
                n += dbr_contr_len(rec);
                ++msg->entity_rep.count_;
            }
            n += dbr_packlenz(msg->entity_rep.count_);
            break;
        case DBR_ORDER:
            msg->entity_rep.count_ = 0;
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrOrder* order = dbr_order_entry(node);
                n += dbr_order_len(order);
                ++msg->entity_rep.count_;
            }
            n += dbr_packlenz(msg->entity_rep.count_);
            break;
        case DBR_MEMB:
            msg->entity_rep.count_ = 0;
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrMemb* memb = dbr_memb_entry(node);
                n += dbr_memb_len(memb);
                ++msg->entity_rep.count_;
            }
            n += dbr_packlenz(msg->entity_rep.count_);
            break;
        case DBR_TRADE:
            msg->entity_rep.count_ = 0;
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrTrade* trade = dbr_trade_entry(node);
                n += dbr_trade_len(trade);
                ++msg->entity_rep.count_;
            }
            n += dbr_packlenz(msg->entity_rep.count_);
            break;
        case DBR_POSN:
            msg->entity_rep.count_ = 0;
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrPosn* posn = dbr_posn_entry(node);
                n += dbr_posn_len(posn);
                ++msg->entity_rep.count_;
            }
            n += dbr_packlenz(msg->entity_rep.count_);
            break;
        default:
            abort();
        }
        break;
    case DBR_RESULT_REP:
        n += dbr_order_len(msg->result_rep.new_order);
        msg->result_rep.posn_count_ = 0;
        for (struct DbrSlNode* node = msg->result_rep.first_posn;
             node; node = node->next) {
            struct DbrPosn* posn = dbr_result_posn_entry(node);
            n += dbr_posn_len(posn);
            ++msg->result_rep.posn_count_;
        }
        n += dbr_packlenz(msg->result_rep.posn_count_);
        msg->result_rep.trade_count_ = 0;
        for (struct DbrSlNode* node = msg->result_rep.first_trade;
             node; node = node->next) {
            struct DbrTrade* trade = dbr_result_trade_entry(node);
            n += dbr_trade_len(trade);
            ++msg->result_rep.trade_count_;
        }
        n += dbr_packlenz(msg->result_rep.trade_count_);
        break;
    case DBR_ORDER_REP:
        n += dbr_order_len(msg->order_rep.order);
        break;
    case DBR_READ_ENTITY_REQ:
        n += dbr_packleni(msg->read_entity_req.type);
        break;
    case DBR_READ_TRADER_ORDER_REQ:
        break;
    case DBR_READ_TRADER_TRADE_REQ:
        break;
    case DBR_READ_ACCNT_POSN_REQ:
        break;
    case DBR_PLACE_ORDER_REQ:
        n += dbr_packlenf(PLACE_ORDER_REQ_FORMAT,
                          msg->place_order_req.trader,
                          msg->place_order_req.accnt,
                          msg->place_order_req.contr,
                          msg->place_order_req.settl_date,
                          DBR_REF_MAX, msg->place_order_req.ref,
                          msg->place_order_req.action,
                          msg->place_order_req.ticks,
                          msg->place_order_req.lots,
                          msg->place_order_req.min,
                          msg->place_order_req.flags);
        break;
    case DBR_REVISE_ORDER_ID_REQ:
        n += dbr_packlenf(REVISE_ORDER_ID_REQ_FORMAT,
                          msg->revise_order_id_req.trader,
                          msg->revise_order_id_req.id,
                          msg->revise_order_id_req.lots);
        break;
    case DBR_REVISE_ORDER_REF_REQ:
        n += dbr_packlenf(REVISE_ORDER_REF_REQ_FORMAT,
                          msg->revise_order_ref_req.trader,
                          DBR_REF_MAX, msg->revise_order_ref_req.ref,
                          msg->revise_order_ref_req.lots);
        break;
    case DBR_CANCEL_ORDER_ID_REQ:
        n += dbr_packlenf(CANCEL_ORDER_ID_REQ_FORMAT,
                          msg->cancel_order_id_req.trader,
                          msg->cancel_order_id_req.id);
        break;
    case DBR_CANCEL_ORDER_REF_REQ:
        n += dbr_packlenf(CANCEL_ORDER_REF_REQ_FORMAT,
                          msg->cancel_order_ref_req.trader,
                          DBR_REF_MAX, msg->cancel_order_ref_req.ref);
        break;
    case DBR_ARCHIVE_ORDER_REQ:
        n += dbr_packlenf(ARCHIVE_ORDER_REQ_FORMAT,
                          msg->archive_order_req.trader,
                          msg->archive_order_req.id);
        break;
    case DBR_ARCHIVE_TRADE_REQ:
        n += dbr_packlenf(ARCHIVE_TRADE_REQ_FORMAT,
                          msg->archive_trade_req.trader,
                          msg->archive_trade_req.id);
        break;
    case DBR_WRITE_TRANS_REQ:
        msg->write_trans_req.count_ = 0;
        for (struct DbrSlNode* node = msg->write_trans_req.first; node; node = node->next) {
            struct DbrStmt* stmt = dbr_trans_stmt_entry(node);
            n += dbr_stmt_len(stmt);
            ++msg->write_trans_req.count_;
        }
        n += dbr_packlenz(msg->write_trans_req.count_);
        break;
    default:
        abort();
    }
    return n;
}

DBR_API char*
dbr_write_msg(char* buf, const struct DbrMsg* msg)
{
    buf = dbr_packi(buf, msg->type);
    switch (msg->type) {
    case DBR_STATUS_REP:
        buf = dbr_packf(buf, STATUS_REP_FORMAT,
                        msg->status_rep.num,
                        DBR_ERRMSG_MAX, msg->status_rep.msg);
        break;
    case DBR_ENTITY_REP:
        buf = dbr_packi(buf, msg->entity_rep.type);
        switch (msg->entity_rep.type) {
        case DBR_TRADER:
            buf = dbr_packz(buf, msg->entity_rep.count_);
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrRec* rec = dbr_rec_entry(node);
                buf = dbr_write_trader(buf, rec);
            }
            break;
        case DBR_ACCNT:
            buf = dbr_packz(buf, msg->entity_rep.count_);
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrRec* rec = dbr_rec_entry(node);
                buf = dbr_write_accnt(buf, rec);
            }
            break;
        case DBR_CONTR:
            buf = dbr_packz(buf, msg->entity_rep.count_);
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrRec* rec = dbr_rec_entry(node);
                buf = dbr_write_contr(buf, rec);
            }
            break;
        case DBR_ORDER:
            buf = dbr_packz(buf, msg->entity_rep.count_);
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrOrder* order = dbr_order_entry(node);
                buf = dbr_write_order(buf, order);
            }
            break;
        case DBR_MEMB:
            buf = dbr_packz(buf, msg->entity_rep.count_);
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrMemb* memb = dbr_memb_entry(node);
                buf = dbr_write_memb(buf, memb);
            }
            break;
        case DBR_TRADE:
            buf = dbr_packz(buf, msg->entity_rep.count_);
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrTrade* trade = dbr_trade_entry(node);
                buf = dbr_write_trade(buf, trade);
            }
            break;
        case DBR_POSN:
            buf = dbr_packz(buf, msg->entity_rep.count_);
            for (struct DbrSlNode* node = msg->entity_rep.first; node; node = node->next) {
                struct DbrPosn* posn = dbr_posn_entry(node);
                buf = dbr_write_posn(buf, posn);
            }
            break;
        default:
            abort();
        }
        break;
    case DBR_RESULT_REP:
        buf = dbr_write_order(buf, msg->result_rep.new_order);
        buf = dbr_packz(buf, msg->result_rep.posn_count_);
        for (struct DbrSlNode* node = msg->result_rep.first_posn;
             node; node = node->next) {
            struct DbrPosn* posn = dbr_result_posn_entry(node);
            buf = dbr_write_posn(buf, posn);
        }
        buf = dbr_packz(buf, msg->result_rep.trade_count_);
        for (struct DbrSlNode* node = msg->result_rep.first_trade;
             node; node = node->next) {
            struct DbrTrade* trade = dbr_result_trade_entry(node);
            buf = dbr_write_trade(buf, trade);
        }
        break;
    case DBR_ORDER_REP:
        buf = dbr_write_order(buf, msg->order_rep.order);
        break;
    case DBR_READ_ENTITY_REQ:
        buf = dbr_packi(buf, msg->read_entity_req.type);
        break;
    case DBR_READ_TRADER_ORDER_REQ:
        break;
    case DBR_READ_TRADER_TRADE_REQ:
        break;
    case DBR_READ_ACCNT_POSN_REQ:
        break;
    case DBR_PLACE_ORDER_REQ:
        buf = dbr_packf(buf, PLACE_ORDER_REQ_FORMAT,
                        msg->place_order_req.trader,
                        msg->place_order_req.accnt,
                        msg->place_order_req.contr,
                        msg->place_order_req.settl_date,
                        DBR_REF_MAX, msg->place_order_req.ref,
                        msg->place_order_req.action,
                        msg->place_order_req.ticks,
                        msg->place_order_req.lots,
                        msg->place_order_req.min,
                        msg->place_order_req.flags);
        break;
    case DBR_REVISE_ORDER_ID_REQ:
        buf = dbr_packf(buf, REVISE_ORDER_ID_REQ_FORMAT,
                        msg->revise_order_id_req.trader,
                        msg->revise_order_id_req.id,
                        msg->revise_order_id_req.lots);
        break;
    case DBR_REVISE_ORDER_REF_REQ:
        buf = dbr_packf(buf, REVISE_ORDER_REF_REQ_FORMAT,
                        msg->revise_order_ref_req.trader,
                        DBR_REF_MAX, msg->revise_order_ref_req.ref,
                        msg->revise_order_ref_req.lots);
        break;
    case DBR_CANCEL_ORDER_ID_REQ:
        buf = dbr_packf(buf, CANCEL_ORDER_ID_REQ_FORMAT,
                        msg->cancel_order_id_req.trader,
                        msg->cancel_order_id_req.id);
        break;
    case DBR_CANCEL_ORDER_REF_REQ:
        buf = dbr_packf(buf, CANCEL_ORDER_REF_REQ_FORMAT,
                        msg->cancel_order_ref_req.trader,
                        DBR_REF_MAX, msg->cancel_order_ref_req.ref);
        break;
    case DBR_ARCHIVE_ORDER_REQ:
        buf = dbr_packf(buf, ARCHIVE_ORDER_REQ_FORMAT,
                        msg->archive_order_req.trader,
                        msg->archive_order_req.id);
        break;
    case DBR_ARCHIVE_TRADE_REQ:
        buf = dbr_packf(buf, ARCHIVE_TRADE_REQ_FORMAT,
                        msg->archive_trade_req.trader,
                        msg->archive_trade_req.id);
        break;
    case DBR_WRITE_TRANS_REQ:
        buf = dbr_packz(buf, msg->write_trans_req.count_);
        for (struct DbrSlNode* node = msg->write_trans_req.first; node; node = node->next) {
            struct DbrStmt* stmt = dbr_trans_stmt_entry(node);
            buf = dbr_write_stmt(buf, stmt);
        }
        break;
    }
    return buf;
}

DBR_API const char*
dbr_read_msg(const char* buf, DbrPool pool, struct DbrMsg* msg)
{
    int type;
    if (!(buf = dbr_unpacki(buf, &type)))
        goto fail1;
    msg->type = type;
    struct DbrQueue q;
    switch (type) {
    case DBR_STATUS_REP:
        buf = dbr_unpackf(buf, STATUS_REP_FORMAT,
                          &msg->status_rep.num,
                          DBR_ERRMSG_MAX, msg->status_rep.msg);
        break;
    case DBR_ENTITY_REP:
        if (!(buf = dbr_unpacki(buf, &msg->entity_rep.type)))
            goto fail1;
        switch (msg->entity_rep.type) {
        case DBR_TRADER:
            if (!(buf = dbr_unpackz(buf, &msg->entity_rep.count_)))
                goto fail1;
            dbr_queue_init(&q);
            for (size_t i = 0; i < msg->entity_rep.count_; ++i) {
                if (!read_entity_trader(buf, pool, &q)) {
                    dbr_pool_free_entities(pool, DBR_TRADER, dbr_queue_first(&q));
                    goto fail1;
                }
            }
            msg->entity_rep.first = dbr_queue_first(&q);
            break;
        case DBR_ACCNT:
            if (!(buf = dbr_unpackz(buf, &msg->entity_rep.count_)))
                goto fail1;
            dbr_queue_init(&q);
            for (size_t i = 0; i < msg->entity_rep.count_; ++i) {
                if (!read_entity_accnt(buf, pool, &q)) {
                    dbr_pool_free_entities(pool, DBR_ACCNT, dbr_queue_first(&q));
                    goto fail1;
                }
            }
            msg->entity_rep.first = dbr_queue_first(&q);
            break;
        case DBR_CONTR:
            if (!(buf = dbr_unpackz(buf, &msg->entity_rep.count_)))
                goto fail1;
            dbr_queue_init(&q);
            for (size_t i = 0; i < msg->entity_rep.count_; ++i) {
                if (!read_entity_contr(buf, pool, &q)) {
                    dbr_pool_free_entities(pool, DBR_CONTR, dbr_queue_first(&q));
                    goto fail1;
                }
            }
            msg->entity_rep.first = dbr_queue_first(&q);
            break;
        case DBR_ORDER:
            if (!(buf = dbr_unpackz(buf, &msg->entity_rep.count_)))
                goto fail1;
            dbr_queue_init(&q);
            for (size_t i = 0; i < msg->entity_rep.count_; ++i) {
                if (!read_entity_order(buf, pool, &q)) {
                    dbr_pool_free_entities(pool, DBR_ORDER, dbr_queue_first(&q));
                    goto fail1;
                }
            }
            msg->entity_rep.first = dbr_queue_first(&q);
            break;
        case DBR_MEMB:
            if (!(buf = dbr_unpackz(buf, &msg->entity_rep.count_)))
                goto fail1;
            dbr_queue_init(&q);
            for (size_t i = 0; i < msg->entity_rep.count_; ++i) {
                if (!read_entity_memb(buf, pool, &q)) {
                    dbr_pool_free_entities(pool, DBR_MEMB, dbr_queue_first(&q));
                    goto fail1;
                }
            }
            msg->entity_rep.first = dbr_queue_first(&q);
            break;
        case DBR_TRADE:
            if (!(buf = dbr_unpackz(buf, &msg->entity_rep.count_)))
                goto fail1;
            dbr_queue_init(&q);
            for (size_t i = 0; i < msg->entity_rep.count_; ++i) {
                if (!read_entity_trade(buf, pool, &q)) {
                    dbr_pool_free_entities(pool, DBR_TRADE, dbr_queue_first(&q));
                    goto fail1;
                }
            }
            msg->entity_rep.first = dbr_queue_first(&q);
            break;
        case DBR_POSN:
            if (!(buf = dbr_unpackz(buf, &msg->entity_rep.count_)))
                goto fail1;
            dbr_queue_init(&q);
            for (size_t i = 0; i < msg->entity_rep.count_; ++i) {
                if (!read_entity_posn(buf, pool, &q)) {
                    dbr_pool_free_entities(pool, DBR_POSN, dbr_queue_first(&q));
                    goto fail1;
                }
            }
            msg->entity_rep.first = dbr_queue_first(&q);
            break;
        default:
            dbr_err_setf(DBR_EIO, "invalid entity-type '%d'", type);
            goto fail1;
        }
        break;
    case DBR_RESULT_REP:
        // Order.
        msg->result_rep.new_order = dbr_pool_alloc_order(pool);
        if (!msg->result_rep.new_order)
            goto fail1;
        if (!(buf = dbr_read_order(buf, msg->result_rep.new_order))) {
            dbr_pool_free_order(pool, msg->result_rep.new_order);
            goto fail1;
        }
        // Posns.
        if (!(buf = dbr_unpackz(buf, &msg->result_rep.posn_count_))) {
            dbr_pool_free_order(pool, msg->result_rep.new_order);
            goto fail1;
        }
        dbr_queue_init(&q);
        for (size_t i = 0; i < msg->result_rep.posn_count_; ++i) {
            if (!read_result_posn(buf, pool, &q)) {
                free_result_posns(msg->result_rep.first_posn, pool);
                dbr_pool_free_order(pool, msg->result_rep.new_order);
                goto fail1;
            }
        }
        msg->result_rep.first_posn = dbr_queue_first(&q);
        // Trades.
        if (!(buf = dbr_unpackz(buf, &msg->result_rep.trade_count_))) {
            free_result_posns(msg->result_rep.first_posn, pool);
            dbr_pool_free_order(pool, msg->result_rep.new_order);
            goto fail1;
        }
        dbr_queue_init(&q);
        for (size_t i = 0; i < msg->result_rep.trade_count_; ++i) {
            if (!read_result_trade(buf, pool, &q)) {
                free_result_trades(msg->result_rep.first_trade, pool);
                free_result_posns(msg->result_rep.first_posn, pool);
                dbr_pool_free_order(pool, msg->result_rep.new_order);
                goto fail1;
            }
        }
        msg->result_rep.first_trade = dbr_queue_first(&q);
        break;
    case DBR_ORDER_REP:
        // Order.
        msg->order_rep.order = dbr_pool_alloc_order(pool);
        if (!msg->order_rep.order)
            goto fail1;
        if (!(buf = dbr_read_order(buf, msg->order_rep.order))) {
            dbr_pool_free_order(pool, msg->order_rep.order);
            goto fail1;
        }
        break;
    case DBR_READ_ENTITY_REQ:
        if (!(buf = dbr_unpacki(buf, &msg->read_entity_req.type)))
            goto fail1;
        break;
    case DBR_READ_TRADER_ORDER_REQ:
        break;
    case DBR_READ_TRADER_TRADE_REQ:
        break;
    case DBR_READ_ACCNT_POSN_REQ:
        break;
    case DBR_PLACE_ORDER_REQ:
        buf = dbr_unpackf(buf, PLACE_ORDER_REQ_FORMAT,
                          msg->place_order_req.trader,
                          msg->place_order_req.accnt,
                          msg->place_order_req.contr,
                          &msg->place_order_req.settl_date,
                          DBR_REF_MAX, msg->place_order_req.ref,
                          &msg->place_order_req.action,
                          &msg->place_order_req.ticks,
                          &msg->place_order_req.lots,
                          &msg->place_order_req.min,
                          &msg->place_order_req.flags);
        break;
    case DBR_REVISE_ORDER_ID_REQ:
        buf = dbr_unpackf(buf, REVISE_ORDER_ID_REQ_FORMAT,
                          msg->revise_order_id_req.trader,
                          &msg->revise_order_id_req.id,
                          &msg->revise_order_id_req.lots);
        break;
    case DBR_REVISE_ORDER_REF_REQ:
        buf = dbr_unpackf(buf, REVISE_ORDER_REF_REQ_FORMAT,
                          msg->revise_order_ref_req.trader,
                          DBR_REF_MAX, msg->revise_order_ref_req.ref,
                          &msg->revise_order_ref_req.lots);
        break;
    case DBR_CANCEL_ORDER_ID_REQ:
        buf = dbr_unpackf(buf, CANCEL_ORDER_ID_REQ_FORMAT,
                          msg->cancel_order_id_req.trader,
                          &msg->cancel_order_id_req.id);
        break;
    case DBR_CANCEL_ORDER_REF_REQ:
        buf = dbr_unpackf(buf, CANCEL_ORDER_REF_REQ_FORMAT,
                          msg->cancel_order_ref_req.trader,
                          DBR_REF_MAX, msg->cancel_order_ref_req.ref);
        break;
    case DBR_ARCHIVE_ORDER_REQ:
        buf = dbr_unpackf(buf, ARCHIVE_ORDER_REQ_FORMAT,
                          msg->archive_order_req.trader,
                          &msg->archive_order_req.id);
        break;
    case DBR_ARCHIVE_TRADE_REQ:
        buf = dbr_unpackf(buf, ARCHIVE_TRADE_REQ_FORMAT,
                          msg->archive_trade_req.trader,
                          &msg->archive_trade_req.id);
        break;
    case DBR_WRITE_TRANS_REQ:
        if (!(buf = dbr_unpackz(buf, &msg->write_trans_req.count_)))
            goto fail1;
        dbr_queue_init(&q);
        for (size_t i = 0; i < msg->write_trans_req.count_; ++i) {
            if (!read_trans_stmt(buf, pool, &q)) {
                free_trans_stmts(dbr_queue_first(&q), pool);
                goto fail1;
            }
        }
        msg->write_trans_req.first = dbr_queue_first(&q);
        break;
    default:
        dbr_err_setf(DBR_EIO, "invalid msg-type '%d'", type);
        goto fail1;
    }
    return buf;
 fail1:
    return NULL;
}

DBR_EXTERN DbrBool
dbr_recv_msg(void* sock, DbrPool pool, struct DbrMsg* msg)
{
    zmq_msg_t zmsg;
    if (zmq_msg_init(&zmsg) < 0) {
        dbr_err_setf(DBR_EIO, "zmq_msg_init() failed: %s", zmq_strerror(zmq_errno()));
        goto fail1;
    }
    if (zmq_msg_recv(&zmsg, sock, 0) < 0) {
        const int num = zmq_errno() == EINTR ? DBR_EINTR : DBR_EIO;
        dbr_err_setf(num, "zmq_msg_recv() failed: %s", zmq_strerror(zmq_errno()));
        goto fail2;
    }
    if (!dbr_read_msg(zmq_msg_data(&zmsg), pool, msg)) {
        dbr_err_set(DBR_EIO, "dbr_read_msg() failed");
        goto fail2;
    }
    zmq_msg_close(&zmsg);
    return DBR_TRUE;
 fail2:
    zmq_msg_close(&zmsg);
 fail1:
    return DBR_FALSE;
}

DBR_EXTERN DbrBool
dbr_send_msg(void* sock, struct DbrMsg* msg)
{
    zmq_msg_t zmsg;
    if (zmq_msg_init_size(&zmsg, dbr_msg_len(msg)) < 0) {
        dbr_err_setf(DBR_EIO, "zmq_msg_init_size() failed: %s", zmq_strerror(zmq_errno()));
        goto fail1;
    }
    if (!dbr_write_msg(zmq_msg_data(&zmsg), msg)) {
        dbr_err_set(DBR_EIO, "dbr_write_msg() failed");
        goto fail2;
    }
    if (zmq_msg_send(&zmsg, sock, 0) < 0) {
        dbr_err_setf(DBR_EIO, "zmq_msg_send() failed: %s", zmq_strerror(zmq_errno()));
        goto fail2;
    }
    zmq_msg_close(&zmsg);
    return DBR_TRUE;
 fail2:
    zmq_msg_close(&zmsg);
 fail1:
    return DBR_FALSE;
}