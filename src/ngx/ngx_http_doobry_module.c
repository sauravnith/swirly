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

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <dbr/ash.h>
#include <dbr/elm.h>
#include <dbr/fig.h>

typedef struct {
    DbrBool doobry;
    ngx_str_t mdaddr;
    ngx_str_t traddr;
    ngx_uint_t tmout;
    ngx_uint_t capacity;

    struct DbrIHandler i_handler;
    DbrCtx ctx;
    DbrAsync async;
} ngx_http_doobry_loc_conf_t;

typedef struct {
    ngx_http_request_t* request;
    struct DbrRest rest;
    size_t len;
    ngx_buf_t* buf;
} ngx_http_doobry_task_t;

static inline ngx_int_t
ngx_http_doobry_task_init(ngx_http_doobry_task_t* t, ngx_http_request_t* r)
{
    t->request = r;
    dbr_rest_init(&t->rest);
    t->len = 0;
    t->buf = NULL;

    switch (r->method) {
    case NGX_HTTP_DELETE:
        dbr_rest_set_method(&t->rest, DBR_METHOD_DELETE);
        break;
    case NGX_HTTP_GET:
        dbr_rest_set_method(&t->rest, DBR_METHOD_GET);
        break;
    case NGX_HTTP_HEAD:
        dbr_rest_set_method(&t->rest, DBR_METHOD_HEAD);
        break;
    case NGX_HTTP_POST:
        dbr_rest_set_method(&t->rest, DBR_METHOD_POST);
        break;
    case NGX_HTTP_PUT:
        dbr_rest_set_method(&t->rest, DBR_METHOD_PUT);
        break;
    default:
        return NGX_DECLINED;
    };

    switch (dbr_rest_rurl(&t->rest, (const char*)r->uri_start + 4,
                          r->uri_end - r->uri_start - 4)) {
    case -1:
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "url parse error");
        // Fallthrough.
    case 0:
        // The request cannot be fulfilled due to bad syntax.
        return NGX_HTTP_BAD_REQUEST;
    case 1:
        break;
    }
    return NGX_OK;
}

static ngx_http_doobry_loc_conf_t*
ngx_http_doobry_loc_conf(ngx_http_request_t* r);

static DbrAccnt
get_accnt(DbrClnt clnt, const char* mnem)
{
    struct DbrSlNode* node = dbr_clnt_find_rec_mnem(clnt, DBR_ENTITY_ACCNT, mnem);
    if (node == DBR_CLNT_END_REC) {
        dbr_err_setf(DBR_EINVAL, "no such account '%.16s'", mnem);
        return NULL;
    }
    return dbr_clnt_accnt(clnt, dbr_clnt_rec_entry(node));
}

#if 0
static inline ngx_http_doobry_loc_conf_t*
handler_implof(DbrHandler handler)
{
    return dbr_implof(ngx_http_doobry_loc_conf_t, i_handler, handler);
}
#endif

static void
on_close(DbrHandler handler, DbrClnt clnt)
{
    dbr_log_error("on_close()");
}

static void
on_ready(DbrHandler handler, DbrClnt clnt)
{
    dbr_log_error("on_ready()");
}

static void
on_logon(DbrHandler handler, DbrClnt clnt, DbrIden req_id, DbrIden aid)
{
    dbr_log_error("on_logon()");
}

static void
on_logoff(DbrHandler handler, DbrClnt clnt, DbrIden req_id, DbrIden aid)
{
    dbr_log_error("on_logoff()");
}

static void
on_reset(DbrHandler handler, DbrClnt clnt)
{
    dbr_log_error("on_reset()");
}

static void
on_timeout(DbrHandler handler, DbrClnt clnt, DbrIden req_id)
{
    dbr_log_error("on_timeout()");
}

static void
on_status(DbrHandler handler, DbrClnt clnt, DbrIden req_id, int num, const char* msg)
{
    dbr_log_error("on_status()");
}

static void
on_exec(DbrHandler handler, DbrClnt clnt, DbrIden req_id, struct DbrExec* exec)
{
    dbr_log_error("on_exec()");
}

static void
on_posn(DbrHandler handler, DbrClnt clnt, struct DbrPosn* posn)
{
    dbr_log_error("on_posn()");
}

static void
on_view(DbrHandler handler, DbrClnt clnt, struct DbrView* view)
{
    dbr_log_error("on_view()");
}

static void
on_flush(DbrHandler handler, DbrClnt clnt)
{
    dbr_log_debug2("on_flush()");
}

static const struct DbrHandlerVtbl HANDLER_VTBL = {
    .on_close = on_close,
    .on_ready = on_ready,
    .on_logon = on_logon,
    .on_logoff = on_logoff,
    .on_reset = on_reset,
    .on_timeout = on_timeout,
    .on_status = on_status,
    .on_exec = on_exec,
    .on_posn = on_posn,
    .on_view = on_view,
    .on_flush = on_flush,
    .on_async = dbr_task_on_async
};

static void
ngx_http_doobry_log(int level, const char* msg)
{
    ngx_log_stderr(0, msg);
}

static ngx_int_t
ngx_http_doobry_send_header(ngx_http_request_t* r, size_t len)
{
    ngx_str_set(&r->headers_out.content_type, "application/json");
    r->headers_out.content_length_n = len;
    r->headers_out.status = NGX_HTTP_OK;
    return ngx_http_send_header(r);
}

static int
ngx_http_doobry_get_logon(DbrHandler handler, DbrClnt clnt, void* arg)
{
    ngx_http_doobry_task_t* t = arg;
    DbrAccnt accnt = get_accnt(clnt, t->rest.accnt);
    if (!accnt)
        return NGX_HTTP_NOT_FOUND;

    dbr_clnt_logon(clnt, accnt);
    return NGX_HTTP_ACCEPTED;
}

static ngx_int_t
ngx_http_doobry_logon(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);
    // Testing.
    dbr_rest_set_param(&t->rest, DBR_PARAM_ACCNT);
    strncpy(t->rest.accnt, "WRAMIREZ", DBR_MNEM_MAX);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_logon, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_logoff(DbrHandler handler, DbrClnt clnt, void* arg)
{
    ngx_http_doobry_task_t* t = arg;
    DbrAccnt accnt = get_accnt(clnt, t->rest.accnt);
    if (!accnt)
        return NGX_HTTP_NOT_FOUND;

    dbr_clnt_logoff(clnt, accnt);
    return NGX_HTTP_ACCEPTED;
}

static ngx_int_t
ngx_http_doobry_logoff(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);
    // Testing.
    dbr_rest_set_param(&t->rest, DBR_PARAM_ACCNT);
    strncpy(t->rest.accnt, "WRAMIREZ", DBR_MNEM_MAX);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_logoff, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_accnt(DbrHandler handler, DbrClnt clnt, void* arg)
{
    ngx_http_doobry_task_t* t = arg;

    struct DbrSlNode* first = dbr_clnt_first_rec(clnt, DBR_ENTITY_ACCNT, NULL);

    size_t len = sizeof("[]") - 1;
    for (struct DbrSlNode* node = first; node != DBR_CLNT_END_REC;
         node = dbr_slnode_next(node)) {
        struct DbrRec* arec = dbr_clnt_rec_entry(node);
        if (node != first)
            len += sizeof(",") - 1;
        len += dbr_json_accnt_len(arec);
    }

    t->len = len;
    if (dbr_rest_get_method(&t->rest) == DBR_METHOD_HEAD)
        return NGX_OK;

    ngx_buf_t* b = ngx_create_temp_buf(t->request->pool, len + 1);
    if (!b)
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    *b->last++ = '[';
    for (struct DbrSlNode* node = first; node != DBR_CLNT_END_REC;
         node = dbr_slnode_next(node)) {
        struct DbrRec* arec = dbr_clnt_rec_entry(node);
        if (node != first)
            *b->last++ = ',';
        b->last = (u_char*)dbr_json_write_accnt((char*)b->last, arec);
    }
    *b->last++ = ']';

    b->last_buf = 1;
    t->buf = b;
    return NGX_OK;
}

static ngx_int_t
ngx_http_doobry_accnt(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_accnt, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_accnt_with_accnt(DbrHandler handler, DbrClnt clnt, void* arg)
{
    return NGX_HTTP_NO_CONTENT;
}

static ngx_int_t
ngx_http_doobry_accnt_with_accnt(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_accnt_with_accnt, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_contr(DbrHandler handler, DbrClnt clnt, void* arg)
{
    //ngx_http_doobry_loc_conf_t* lcf = handler_implof(handler);
    ngx_http_doobry_task_t* t = arg;

    struct DbrSlNode* first = dbr_clnt_first_rec(clnt, DBR_ENTITY_CONTR, NULL);

    size_t len = sizeof("[]") - 1;
    for (struct DbrSlNode* node = first; node != DBR_CLNT_END_REC;
         node = dbr_slnode_next(node)) {
        struct DbrRec* crec = dbr_clnt_rec_entry(node);
        if (node != first)
            len += sizeof(",") - 1;
        len += dbr_json_contr_len(crec);
    }

    t->len = len;
    if (dbr_rest_get_method(&t->rest) == DBR_METHOD_HEAD)
        return NGX_OK;

    ngx_buf_t* b = ngx_create_temp_buf(t->request->pool, len + 1);
    if (!b)
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    *b->last++ = '[';
    for (struct DbrSlNode* node = first; node != DBR_CLNT_END_REC;
         node = dbr_slnode_next(node)) {
        struct DbrRec* crec = dbr_clnt_rec_entry(node);
        if (node != first)
            *b->last++ = ',';
        b->last = (u_char*)dbr_json_write_contr((char*)b->last, crec);
    }
    *b->last++ = ']';

    b->last_buf = 1;
    t->buf = b;
    return NGX_OK;
}

static ngx_int_t
ngx_http_doobry_contr(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_contr, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_contr_with_contr(DbrHandler handler, DbrClnt clnt, void* arg)
{
    return NGX_HTTP_NO_CONTENT;
}

static ngx_int_t
ngx_http_doobry_contr_with_contr(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_contr_with_contr, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_trader_with_accnt(DbrHandler handler, DbrClnt clnt, void* arg)
{
    ngx_http_doobry_task_t* t = arg;
    DbrAccnt accnt = get_accnt(clnt, t->rest.accnt);
    if (!accnt)
        return NGX_HTTP_NOT_FOUND;

    struct DbrRbNode* first = dbr_accnt_first_trader(accnt);

    size_t len = sizeof("[]") - 1;
    for (struct DbrRbNode* node = first; node != DBR_ACCNT_END_TRADER;
         node = dbr_rbnode_next(node)) {
        struct DbrPerm* perm = dbr_accnt_trader_entry(node);
        if (node != first)
            len += sizeof(",") - 1;
        len += dbr_json_perm_len(perm);
    }

    t->len = len;
    if (dbr_rest_get_method(&t->rest) == DBR_METHOD_HEAD)
        return NGX_OK;

    ngx_buf_t* b = ngx_create_temp_buf(t->request->pool, len + 1);
    if (!b)
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    *b->last++ = '[';
    for (struct DbrRbNode* node = first; node != DBR_ACCNT_END_TRADER;
         node = dbr_rbnode_next(node)) {
        struct DbrPerm* perm = dbr_accnt_trader_entry(node);
        if (node != first)
            *b->last++ = ',';
        b->last = (u_char*)dbr_json_write_perm((char*)b->last, perm);
    }
    *b->last++ = ']';

    b->last_buf = 1;
    t->buf = b;
    return NGX_OK;
}

static ngx_int_t
ngx_http_doobry_trader_with_accnt(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_trader_with_accnt, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_giveup_with_accnt(DbrHandler handler, DbrClnt clnt, void* arg)
{
    ngx_http_doobry_task_t* t = arg;
    DbrAccnt accnt = get_accnt(clnt, t->rest.accnt);
    if (!accnt)
        return NGX_HTTP_NOT_FOUND;

    struct DbrRbNode* first = dbr_accnt_first_giveup(accnt);

    size_t len = sizeof("[]") - 1;
    for (struct DbrRbNode* node = first; node != DBR_ACCNT_END_GIVEUP;
         node = dbr_rbnode_next(node)) {
        struct DbrPerm* perm = dbr_accnt_giveup_entry(node);
        if (node != first)
            len += sizeof(",") - 1;
        len += dbr_json_perm_len(perm);
    }

    t->len = len;
    if (dbr_rest_get_method(&t->rest) == DBR_METHOD_HEAD)
        return NGX_OK;

    ngx_buf_t* b = ngx_create_temp_buf(t->request->pool, len + 1);
    if (!b)
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    *b->last++ = '[';
    for (struct DbrRbNode* node = first; node != DBR_ACCNT_END_GIVEUP;
         node = dbr_rbnode_next(node)) {
        struct DbrPerm* perm = dbr_accnt_giveup_entry(node);
        if (node != first)
            *b->last++ = ',';
        b->last = (u_char*)dbr_json_write_perm((char*)b->last, perm);
    }
    *b->last++ = ']';

    b->last_buf = 1;
    t->buf = b;
    return NGX_OK;
}

static ngx_int_t
ngx_http_doobry_giveup_with_accnt(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_giveup_with_accnt, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_order_with_accnt(DbrHandler handler, DbrClnt clnt, void* arg)
{
    ngx_http_doobry_task_t* t = arg;
    DbrAccnt accnt = get_accnt(clnt, t->rest.accnt);
    if (!accnt)
        return NGX_HTTP_NOT_FOUND;

    struct DbrRbNode* first = dbr_accnt_first_order(accnt);

    size_t len = sizeof("[]") - 1;
    for (struct DbrRbNode* node = first; node != DBR_ACCNT_END_ORDER;
         node = dbr_rbnode_next(node)) {
        struct DbrOrder* order = dbr_accnt_order_entry(node);
        if (node != first)
            len += sizeof(",") - 1;
        len += dbr_json_order_len(order);
    }

    t->len = len;
    if (dbr_rest_get_method(&t->rest) == DBR_METHOD_HEAD)
        return NGX_OK;

    ngx_buf_t* b = ngx_create_temp_buf(t->request->pool, len + 1);
    if (!b)
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    *b->last++ = '[';
    for (struct DbrRbNode* node = first; node != DBR_ACCNT_END_ORDER;
         node = dbr_rbnode_next(node)) {
        struct DbrOrder* order = dbr_accnt_order_entry(node);
        if (node != first)
            *b->last++ = ',';
        b->last = (u_char*)dbr_json_write_order((char*)b->last, order);
    }
    *b->last++ = ']';

    b->last_buf = 1;
    t->buf = b;
    return NGX_OK;
}

static ngx_int_t
ngx_http_doobry_order_with_accnt(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_order_with_accnt, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_order_with_accnt_and_id(DbrHandler handler, DbrClnt clnt, void* arg)
{
    return NGX_HTTP_NO_CONTENT;
}

static ngx_int_t
ngx_http_doobry_order_with_accnt_and_id(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_order_with_accnt_and_id, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_trade_with_accnt(DbrHandler handler, DbrClnt clnt, void* arg)
{
    ngx_http_doobry_task_t* t = arg;
    DbrAccnt accnt = get_accnt(clnt, t->rest.accnt);
    if (!accnt)
        return NGX_HTTP_NOT_FOUND;

    struct DbrRbNode* first = dbr_accnt_first_trade(accnt);

    size_t len = sizeof("[]") - 1;
    for (struct DbrRbNode* node = first; node != DBR_ACCNT_END_TRADE;
         node = dbr_rbnode_next(node)) {
        struct DbrExec* trade = dbr_accnt_trade_entry(node);
        if (node != first)
            len += sizeof(",") - 1;
        len += dbr_json_exec_len(trade);
    }

    t->len = len;
    if (dbr_rest_get_method(&t->rest) == DBR_METHOD_HEAD)
        return NGX_OK;

    ngx_buf_t* b = ngx_create_temp_buf(t->request->pool, len + 1);
    if (!b)
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    *b->last++ = '[';
    for (struct DbrRbNode* node = first; node != DBR_ACCNT_END_TRADE;
         node = dbr_rbnode_next(node)) {
        struct DbrExec* trade = dbr_accnt_trade_entry(node);
        if (node != first)
            *b->last++ = ',';
        b->last = (u_char*)dbr_json_write_exec((char*)b->last, trade);
    }
    *b->last++ = ']';

    b->last_buf = 1;
    t->buf = b;
    return NGX_OK;
}

static ngx_int_t
ngx_http_doobry_trade_with_accnt(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_trade_with_accnt, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_trade_with_accnt_and_id(DbrHandler handler, DbrClnt clnt, void* arg)
{
    return NGX_HTTP_NO_CONTENT;
}

static ngx_int_t
ngx_http_doobry_trade_with_accnt_and_id(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_trade_with_accnt_and_id, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_posn_with_accnt(DbrHandler handler, DbrClnt clnt, void* arg)
{
    ngx_http_doobry_task_t* t = arg;
    DbrAccnt accnt = get_accnt(clnt, t->rest.accnt);
    if (!accnt)
        return NGX_HTTP_NOT_FOUND;

    struct DbrRbNode* first = dbr_accnt_first_posn(accnt);

    size_t len = sizeof("[]") - 1;
    for (struct DbrRbNode* node = first; node != DBR_ACCNT_END_POSN;
         node = dbr_rbnode_next(node)) {
        struct DbrPosn* posn = dbr_accnt_posn_entry(node);
        if (node != first)
            len += sizeof(",") - 1;
        len += dbr_json_posn_len(posn);
    }

    t->len = len;
    if (dbr_rest_get_method(&t->rest) == DBR_METHOD_HEAD)
        return NGX_OK;

    ngx_buf_t* b = ngx_create_temp_buf(t->request->pool, len + 1);
    if (!b)
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    *b->last++ = '[';
    for (struct DbrRbNode* node = first; node != DBR_ACCNT_END_POSN;
         node = dbr_rbnode_next(node)) {
        struct DbrPosn* posn = dbr_accnt_posn_entry(node);
        if (node != first)
            *b->last++ = ',';
        b->last = (u_char*)dbr_json_write_posn((char*)b->last, posn);
    }
    *b->last++ = ']';

    b->last_buf = 1;
    t->buf = b;
    return NGX_OK;
}

static ngx_int_t
ngx_http_doobry_posn_with_accnt(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_posn_with_accnt, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_market(DbrHandler handler, DbrClnt clnt, void* arg)
{
    ngx_http_doobry_task_t* t = arg;

    struct DbrRbNode* first = dbr_clnt_first_view(clnt);

    size_t len = sizeof("[]") - 1;
    for (struct DbrRbNode* node = first; node != DBR_CLNT_END_VIEW;
         node = dbr_rbnode_next(node)) {
        struct DbrView* view = dbr_clnt_view_entry(node);
        if (node != first)
            len += sizeof(",") - 1;
        len += dbr_json_view_len(view);
    }

    t->len = len;
    if (dbr_rest_get_method(&t->rest) == DBR_METHOD_HEAD)
        return NGX_OK;

    ngx_buf_t* b = ngx_create_temp_buf(t->request->pool, len + 1);
    if (!b)
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    *b->last++ = '[';
    for (struct DbrRbNode* node = first; node != DBR_CLNT_END_VIEW;
         node = dbr_rbnode_next(node)) {
        struct DbrView* view = dbr_clnt_view_entry(node);
        if (node != first)
            *b->last++ = ',';
        b->last = (u_char*)dbr_json_write_view((char*)b->last, view);
    }
    *b->last++ = ']';

    b->last_buf = 1;
    t->buf = b;
    return NGX_OK;
}

static ngx_int_t
ngx_http_doobry_market(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_market, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static int
ngx_http_doobry_get_market_with_contr(DbrHandler handler, DbrClnt clnt, void* arg)
{
    return NGX_HTTP_NO_CONTENT;
}

static ngx_int_t
ngx_http_doobry_market_with_contr(ngx_http_doobry_task_t* t)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_http_doobry_loc_conf(t->request);

    ngx_int_t rc;
    switch (dbr_rest_get_method(&t->rest)) {
    case DBR_METHOD_GET:
    case DBR_METHOD_HEAD:
        rc = dbr_task_call(lcf->async, ngx_http_doobry_get_market_with_contr, t);
        break;
    default:
        rc = NGX_HTTP_NOT_ALLOWED;
        break;
    }
    return rc;
}

static ngx_int_t
ngx_http_doobry_handler(ngx_http_request_t* r)
{
    ngx_log_debug2(NGX_LOG_INFO, r->connection->log, 0, "uri: %*s",
                   r->uri_end - r->uri_start, r->uri_start);

    ngx_int_t rc;
    if (!(r->method & (NGX_HTTP_POST | NGX_HTTP_PUT))) {
        rc = ngx_http_discard_request_body(r);
        if (rc != NGX_OK)
            return rc;
    }

    ngx_http_doobry_task_t t;
    rc = ngx_http_doobry_task_init(&t, r);
    if (rc != NGX_OK)
        return rc;

    switch (t.rest.fields & ~DBR_METHOD_MASK) {
    case DBR_RESRC_LOGON:
        rc = ngx_http_doobry_logon(&t);
        break;
    case DBR_RESRC_LOGOFF:
        rc = ngx_http_doobry_logoff(&t);
        break;
    case DBR_RESRC_ACCNT:
        rc = ngx_http_doobry_accnt(&t);
        break;
    case DBR_RESRC_ACCNT | DBR_PARAM_ACCNT:
        rc = ngx_http_doobry_accnt_with_accnt(&t);
        break;
    case DBR_RESRC_CONTR:
        rc = ngx_http_doobry_contr(&t);
        break;
    case DBR_RESRC_CONTR | DBR_PARAM_CONTR:
        rc = ngx_http_doobry_contr_with_contr(&t);
        break;
    case DBR_RESRC_TRADER | DBR_PARAM_ACCNT:
        rc = ngx_http_doobry_trader_with_accnt(&t);
        break;
    case DBR_RESRC_GIVEUP | DBR_PARAM_ACCNT:
        rc = ngx_http_doobry_giveup_with_accnt(&t);
        break;
    case DBR_RESRC_ORDER | DBR_PARAM_ACCNT:
        rc = ngx_http_doobry_order_with_accnt(&t);
        break;
    case DBR_RESRC_ORDER | DBR_PARAM_ACCNT | DBR_PARAM_ID:
        rc = ngx_http_doobry_order_with_accnt_and_id(&t);
        break;
    case DBR_RESRC_TRADE | DBR_PARAM_ACCNT:
        rc = ngx_http_doobry_trade_with_accnt(&t);
        break;
    case DBR_RESRC_TRADE | DBR_PARAM_ACCNT | DBR_PARAM_ID:
        rc = ngx_http_doobry_trade_with_accnt_and_id(&t);
        break;
    case DBR_RESRC_POSN | DBR_PARAM_ACCNT:
        rc = ngx_http_doobry_posn_with_accnt(&t);
        break;
    case DBR_RESRC_MARKET:
        rc = ngx_http_doobry_market(&t);
        break;
    case DBR_RESRC_MARKET | DBR_PARAM_CONTR:
        rc = ngx_http_doobry_market_with_contr(&t);
        break;
    default:
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "unsupported fields");
        if (r->method & (NGX_HTTP_POST | NGX_HTTP_PUT))
            ngx_http_discard_request_body(r);
        // The request cannot be fulfilled due to bad syntax.
        return NGX_HTTP_BAD_REQUEST;
    }

    if (rc != NGX_OK)
        return rc;

    rc = ngx_http_doobry_send_header(r, t.len);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        ngx_pfree(r->pool, t.buf->start);
        ngx_pfree(r->pool, t.buf);
    }

    if (t.buf) {

        ngx_chain_t out;
        out.buf = t.buf;
        out.next = NULL;

        rc = ngx_http_output_filter(r, &out);
        if (rc != NGX_OK) {
            ngx_pfree(r->pool, t.buf->start);
            ngx_pfree(r->pool, t.buf);
        }
    }
    return rc;
}

static void
ngx_http_doobry_cleanup(void* data)
{
    ngx_http_doobry_loc_conf_t* lcf = data;
    dbr_async_destroy(lcf->async);
    dbr_ctx_destroy(lcf->ctx);
}

static void*
ngx_http_doobry_create_loc_conf(ngx_conf_t* cf)
{
    ngx_http_doobry_loc_conf_t* lcf = ngx_palloc(cf->pool, sizeof(ngx_http_doobry_loc_conf_t));
    if (!lcf) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "failed to allocate local config");
        return NULL;
    }

    lcf->doobry = DBR_FALSE;
    ngx_str_null(&lcf->mdaddr);
    ngx_str_null(&lcf->traddr);
    lcf->tmout = NGX_CONF_UNSET_UINT;
    lcf->capacity = NGX_CONF_UNSET_UINT;

    lcf->i_handler.vtbl = &HANDLER_VTBL;
    lcf->ctx = NULL;
    lcf->async = NULL;

    return lcf;
}

static char*
ngx_http_doobry_merge_loc_conf(ngx_conf_t* cf, void* prev, void* conf)
{
    dbr_log_setlogger(ngx_http_doobry_log);

    // Install doobry handler.
    ngx_http_core_loc_conf_t* clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_doobry_handler;

    ngx_http_doobry_loc_conf_t* pcf = prev;
    ngx_http_doobry_loc_conf_t* lcf = conf;

    ngx_conf_merge_str_value(lcf->mdaddr, pcf->mdaddr, "tcp://localhost:3270");
    ngx_conf_merge_str_value(lcf->traddr, pcf->traddr, "tcp://localhost:3271");
    ngx_conf_merge_uint_value(lcf->tmout, pcf->tmout, 5000);
    ngx_conf_merge_uint_value(lcf->capacity, pcf->capacity, 8 * 1024 * 1024);

    if (lcf->doobry) {

        // Null terminate strings.

        char mdaddr[lcf->mdaddr.len + 1];
        ngx_memcpy(mdaddr, lcf->mdaddr.data, lcf->mdaddr.len);
        mdaddr[lcf->mdaddr.len] = '\0';

        char traddr[lcf->traddr.len + 1];
        ngx_memcpy(traddr, lcf->traddr.data, lcf->traddr.len);
        traddr[lcf->traddr.len] = '\0';

        lcf->ctx = dbr_ctx_create(mdaddr, traddr, lcf->tmout, lcf->capacity, &lcf->i_handler);
        if (!lcf->ctx) {
            ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "failed to create context");
            goto fail1;
        }

        lcf->async = dbr_ctx_async(lcf->ctx);
        if (!lcf->async) {
            ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "failed to create async");
            goto fail2;
        }

        ngx_pool_cleanup_t* cln = ngx_pool_cleanup_add(cf->pool, 0);
        if (!cln) {
            ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "failed to allocate pool cleanup");
            goto fail3;
        }

        cln->handler = ngx_http_doobry_cleanup;
        cln->data = lcf;
    }
    return NGX_CONF_OK;
 fail3:
    dbr_async_destroy(lcf->async);
 fail2:
    dbr_ctx_destroy(lcf->ctx);
 fail1:
    return NGX_CONF_ERROR;
}

static char*
ngx_http_doobry(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
    ngx_http_core_loc_conf_t* clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    // Handler for the 'doobry' directive.
    clcf->handler = ngx_http_doobry_handler;

    ngx_http_doobry_loc_conf_t* lcf = conf;
    lcf->doobry = DBR_TRUE;

    return NGX_CONF_OK;
}

static char*
ngx_http_doobry_mdaddr(ngx_conf_t* cf, void* post, void* data)
{
    return NGX_CONF_OK;
}

static char*
ngx_http_doobry_traddr(ngx_conf_t* cf, void* post, void* data)
{
    return NGX_CONF_OK;
}

static char*
ngx_http_doobry_tmout(ngx_conf_t* cf, void* post, void* data)
{
    return NGX_CONF_OK;
}

static char*
ngx_http_doobry_capacity(ngx_conf_t* cf, void* post, void* data)
{
    return NGX_CONF_OK;
}

static ngx_conf_post_handler_pt ngx_http_doobry_mdaddr_p = ngx_http_doobry_mdaddr;
static ngx_conf_post_handler_pt ngx_http_doobry_traddr_p = ngx_http_doobry_traddr;
static ngx_conf_post_handler_pt ngx_http_doobry_tmout_p = ngx_http_doobry_tmout;
static ngx_conf_post_handler_pt ngx_http_doobry_capacity_p = ngx_http_doobry_capacity;

// Never called?
static ngx_int_t
ngx_http_doobry_init_master(ngx_log_t* log)
{
    ngx_log_error(NGX_LOG_NOTICE, log, 0, "init_master()");
    return NGX_OK;
}

static ngx_int_t
ngx_http_doobry_init_module(ngx_cycle_t* cycle)
{
    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "init_module()");
    return NGX_OK;
}

static ngx_int_t
ngx_http_doobry_init_process(ngx_cycle_t* cycle)
{
    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "init_process()");
    return NGX_OK;
}

static ngx_int_t
ngx_http_doobry_init_thread(ngx_cycle_t* cycle)
{
    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "term_thread()");
    return NGX_OK;
}

static void
ngx_http_doobry_exit_thread(ngx_cycle_t* cycle)
{
    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "exit_thread()");
}

static void
ngx_http_doobry_exit_process(ngx_cycle_t* cycle)
{
    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "exit_process()");
}

static void
ngx_http_doobry_exit_master(ngx_cycle_t* cycle)
{
    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "exit_master()");
}

// The context type required for http modules.

static ngx_http_module_t ngx_http_doobry_module_ctx = {
    // ngx_int_t (*preconfiguration)(ngx_conf_t* cf);
    .preconfiguration = NULL,
    // ngx_int_t (*postconfiguration)(ngx_conf_t* cf);
    .postconfiguration = NULL,
    // void* (*create_main_conf)(ngx_conf_t* cf);
    .create_main_conf = NULL,
    // char* (*init_main_conf)(ngx_conf_t* cf, void* conf);
    .init_main_conf = NULL,
    // void* (*create_srv_conf)(ngx_conf_t* cf);
    .create_srv_conf = NULL,
    // char* (*merge_srv_conf)(ngx_conf_t* cf, void* prev, void* conf);
    .merge_srv_conf = NULL,
    // void* (*create_loc_conf)(ngx_conf_t* cf);
    .create_loc_conf = ngx_http_doobry_create_loc_conf,
    // char* (*merge_loc_conf)(ngx_conf_t* cf, void* prev, void* conf);
    .merge_loc_conf = ngx_http_doobry_merge_loc_conf
};

static ngx_command_t ngx_http_doobry_commands[] = {
    {
        .name = ngx_string("doobry"),
        .type = NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
        .set = ngx_http_doobry,
        .conf = NGX_HTTP_LOC_CONF_OFFSET,
        .offset = offsetof(ngx_http_doobry_loc_conf_t, doobry),
        .post = NULL
    },
    {
        .name = ngx_string("doobry_mdaddr"),
        .type = NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        .set = ngx_conf_set_str_slot,
        .conf = NGX_HTTP_LOC_CONF_OFFSET,
        .offset = offsetof(ngx_http_doobry_loc_conf_t, mdaddr),
        .post = &ngx_http_doobry_mdaddr_p
    },
    {
        .name = ngx_string("doobry_traddr"),
        .type = NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        .set = ngx_conf_set_str_slot,
        .conf = NGX_HTTP_LOC_CONF_OFFSET,
        .offset = offsetof(ngx_http_doobry_loc_conf_t, traddr),
        .post = &ngx_http_doobry_traddr_p
    },
    {
        .name = ngx_string("doobry_tmout"),
        .type = NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        .set = ngx_conf_set_num_slot,
        .conf = NGX_HTTP_LOC_CONF_OFFSET,
        .offset = offsetof(ngx_http_doobry_loc_conf_t, tmout),
        .post = &ngx_http_doobry_tmout_p
    },
    {
        .name = ngx_string("doobry_capacity"),
        .type = NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        .set = ngx_conf_set_num_slot,
        .conf = NGX_HTTP_LOC_CONF_OFFSET,
        .offset = offsetof(ngx_http_doobry_loc_conf_t, capacity),
        .post = &ngx_http_doobry_capacity_p
    },
    ngx_null_command
};

ngx_module_t ngx_http_doobry_module = {
    // version - contains the version of Nginx API (currently it is 1).
    NGX_MODULE_V1,
    // ctx - points to a global context of the module (the actual type of structure this pointer
    // points to depends on the value of type field).
    // void* ctx;
    .ctx = &ngx_http_doobry_module_ctx,
    // commands - points to a vector of descriptors of module commands.
    // ngx_command_t* commands;
    .commands = ngx_http_doobry_commands,
    // type - contains the type of the module: NGX_HTTP_MODULE, NGX_EVENT_MODULE, NGX_MAIL_MODULE
    // and others.
    // ngx_uint_t type;
    .type = NGX_HTTP_MODULE,
    // ngx_int_t (*init_master)(ngx_log_t* log);
    .init_master = ngx_http_doobry_init_master,
    // init_module - points to a handler, that is called upon initialisation of the module in the
    // main process before entering the event loop.
    // ngx_int_t (*init_module)(ngx_cycle_t* cycle);
    .init_module = ngx_http_doobry_init_module,
    // init_process - points to a handler, that is called upon initialisation of the module in a
    // worker process.
    // ngx_int_t (*init_process)(ngx_cycle_t* cycle);
    .init_process = ngx_http_doobry_init_process,
    // ngx_int_t (*init_thread)(ngx_cycle_t* cycle);
    .init_thread = ngx_http_doobry_init_thread,
    // void (*exit_thread)(ngx_cycle_t* cycle);
    .exit_thread = ngx_http_doobry_exit_thread,
    // exit_process - points to a handler, that is called upon termination of a worker process.
    // void (*exit_process)(ngx_cycle_t* cycle);
    .exit_process = ngx_http_doobry_exit_process,
    // exit_master - points to a handler, that is called upon termination of the master process.
    // void (*exit_master)(ngx_cycle_t* cycle);
    .exit_master = ngx_http_doobry_exit_master,
    NGX_MODULE_V1_PADDING
};

static ngx_http_doobry_loc_conf_t*
ngx_http_doobry_loc_conf(ngx_http_request_t* r)
{
    return ngx_http_get_module_loc_conf(r, ngx_http_doobry_module);
}