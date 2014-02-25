#!/usr/bin/env python

from dbrpy import *
from threading import Thread

import json
import time
import web

SESS = 'TEST'
TMOUT = 2000

class WorkerHandler(Handler):
    def __init__(self, clnt):
        super(WorkerHandler, self).__init__()
        self.clnt = clnt
    def on_async(self, fn):
        return fn(self.clnt)

def worker(ctx):
    pool = Pool(8 * 1024 * 1024)
    clnt = Clnt(ctx, SESS, 'tcp://localhost:3270', 'tcp://localhost:3271',
                millis(), pool)
    handler = WorkerHandler(clnt)
    while clnt.is_open():
        clnt.poll(TMOUT, handler)

class CloseRequest(object):
    def __call__(self, clnt):
        return clnt.close(TMOUT)

class TraderRequest(object):
    @staticmethod
    def traderDict(trec):
        return {
            'id': trec.id,
            'mnem': trec.mnem,
            'display': trec.display,
            'email': trec.email
        }
    def __init__(self, mnems):
        self.mnems = set(mnems)
    def __call__(self, clnt):
        traders = []
        if self.mnems:
            for mnem in self.mnems:
                trec = clnt.find_rec_mnem(ENTITY_TRADER, mnem)
                if trec:
                    traders.append(TraderRequest.traderDict(trec))
        else:
            traders = [TraderRequest.traderDict(trec)
                       for trec in clnt.list_rec(ENTITY_TRADER)]
        return traders

class AccntRequest(object):
    @staticmethod
    def accntDict(arec):
        return {
            'id': arec.id,
            'mnem': arec.mnem,
            'display': arec.display,
            'email': arec.email
        }
    def __init__(self, mnems):
        self.mnems = set(mnems)
    def __call__(self, clnt):
        accnts = []
        if self.mnems:
            for mnem in self.mnems:
                arec = clnt.find_rec_mnem(ENTITY_ACCNT, mnem)
                if arec:
                    accnts.append(AccntRequest.accntDict(arec))
        else:
            accnts = [AccntRequest.accntDict(arec)
                      for arec in clnt.list_rec(ENTITY_ACCNT)]
        return accnts

class ContrRequest(object):
    @staticmethod
    def contrDict(crec):
        return {
            'id': crec.id,
            'mnem': crec.mnem,
            'display': crec.display,
            'asset_type': crec.asset_type,
            'asset': crec.asset,
            'ccy': crec.ccy,
            'tick_numer': crec.tick_numer,
            'tick_denom': crec.tick_denom,
            'lot_numer': crec.lot_numer,
            'lot_denom': crec.lot_denom,
            'price_dp': crec.price_dp,
            'pip_dp': crec.pip_dp,
            'qty_dp': crec.qty_dp,
            'min_lots': crec.min_lots,
            'max_lots': crec.max_lots
        }
    def __init__(self, mnems):
        self.mnems = set(mnems)
    def __call__(self, clnt):
        contrs = []
        if self.mnems:
            for mnem in self.mnems:
                crec = clnt.find_rec_mnem(ENTITY_CONTR, mnem)
                if crec:
                    contrs.append(ContrRequest.contrDict(crec))
        else:
            contrs = [ContrRequest.contrDict(crec)
                      for crec in clnt.list_rec(ENTITY_CONTR)]
        return contrs

class ViewRequest(object):
    @staticmethod
    def levelDict(level):
        return {
            'ticks': level.ticks,
            'lots': level.lots,
            'count': level.count
        }
    @staticmethod
    def viewDict(view):
        return {
            'cid': view.cid,
            'settl_date': view.settl_date,
            'list_bid': [ViewRequest.levelDict(level) for level in view.list_bid],
            'list_ask': [ViewRequest.levelDict(level) for level in view.list_ask]
        }
    def __init__(self, mnems, settl_dates):
        self.mnems = set(mnems)
        self.settl_dates = {int(x) for x in settl_dates}
    def __call__(self, clnt):
        cids = set()
        for mnem in self.mnems:
            crec = clnt.find_rec_mnem(ENTITY_CONTR, mnem)
            if crec:
                cids.add(crec.id)
        views = []
        if cids:
            if self.settl_dates:
                # Cids and settl_dates.
                for cid in cids:
                    for settl_date in self.settl_dates:
                        view = clnt.find_view(crec.id, settl_date)
                        if view:
                            views.append(ViewRequest.viewDict(view))
            else:
                # Cids only.
                views = [ViewRequest.viewDict(view) for view in clnt.list_view()
                         if view.cid in cids]
        else:
            if self.settl_dates:
                # Settl_dates only.
                views = [ViewRequest.viewDict(view) for view in clnt.list_view()
                         if view.settl_date in self.settl_dates]
            else:
                # Neither cids nor settl_dates.
                views = [ViewRequest.viewDict(view) for view in clnt.list_view()]
        return views

urls = (
    '/api/trader', 'TraderHandler',
    '/api/accnt',  'AccntHandler',
    '/api/contr',  'ContrHandler',
    '/api/view',   'ViewHandler'
)

class TraderHandler:
    def GET(self):
        async = web.ctx.async
        async.send(TraderRequest(web.input(mnem = []).mnem))
        web.header('Content-Type', 'application/json')
        return json.dumps(async.recv())

class AccntHandler:
    def GET(self):
        async = web.ctx.async
        async.send(AccntRequest(web.input(mnem = []).mnem))
        web.header('Content-Type', 'application/json')
        return json.dumps(async.recv())

class ContrHandler:
    def GET(self):
        async = web.ctx.async
        async.send(ContrRequest(web.input(mnem = []).mnem))
        web.header('Content-Type', 'application/json')
        return json.dumps(async.recv())

class ViewHandler:
    def GET(self):
        async = web.ctx.async
        params = web.input(mnem = [], settl_date = [])
        async.send(ViewRequest(params.mnem, params.settl_date))
        web.header('Content-Type', 'application/json')
        return json.dumps(async.recv())

def run():
    ctx = ZmqCtx()
    thread = Thread(target = worker, args = (ctx,))
    thread.start()
    async = Async(ctx, SESS)
    app = web.application(urls, globals())
    def loadhook():
        web.ctx.async = async
    app.add_processor(web.loadhook(loadhook))
    app.run()
    print('shutdown')
    async.send(CloseRequest())
    async.recv()
    thread.join()

if __name__ == '__main__':
    try:
        run()
    except Error as e:
        print 'error: ' + str(e)
