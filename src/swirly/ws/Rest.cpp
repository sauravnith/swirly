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
#include "Rest.hpp"

#include <swirly/clob/Accnt.hpp>
#include <swirly/clob/Response.hpp>

#include <swirly/fin/Exception.hpp>

#include <swirly/util/Date.hpp>

#include <algorithm>

using namespace std;

namespace swirly {
namespace detail {
namespace {

void getOrder(const Accnt& accnt, ostream& out)
{
    const auto& orders = accnt.orders();
    out << '[';
    copy(orders.begin(), orders.end(), OStreamJoiner(out, ','));
    out << ']';
}

void getExec(const Accnt& accnt, Page page, ostream& out)
{
    const auto& execs = accnt.execs();
    out << '[';
    const auto size = execs.size();
    if (page.offset < size) {
        auto first = execs.begin();
        advance(first, page.offset);
        decltype(first) last;
        if (page.limit && *page.limit < size - page.offset) {
            last = first;
            advance(last, *page.limit);
        } else {
            last = execs.end();
        }
        transform(first, last,
                  OStreamJoiner(out, ','), [](const auto& ptr) -> const auto& { return *ptr; });
    }
    out << ']';
}

void getTrade(const Accnt& accnt, ostream& out)
{
    const auto& trades = accnt.trades();
    out << '[';
    copy(trades.begin(), trades.end(), OStreamJoiner(out, ','));
    out << ']';
}

void getPosn(const Accnt& accnt, ostream& out)
{
    const auto& posns = accnt.posns();
    out << '[';
    copy(posns.begin(), posns.end(), OStreamJoiner(out, ','));
    out << ']';
}

} // anonymous
} // detail

Rest::~Rest() noexcept = default;

Rest::Rest(Rest&&) = default;

Rest& Rest::operator=(Rest&&) = default;

void Rest::getRefData(EntitySet es, Time now, ostream& out) const
{
    int i{0};
    out << '{';
    // FIXME: validate entities.
    if (es.asset()) {
        out << "\"assets\":";
        getAsset(now, out);
        ++i;
    }
    if (es.instr()) {
        if (i > 0) {
            out << ',';
        }
        out << "\"instrs\":";
        getInstr(now, out);
        ++i;
    }
    if (es.market()) {
        if (i > 0) {
            out << ',';
        }
        out << "\"markets\":";
        getMarket(now, out);
        ++i;
    }
    out << '}';
}

void Rest::getAsset(Time now, ostream& out) const
{
    const auto& assets = serv_.assets();
    out << '[';
    copy(assets.begin(), assets.end(), OStreamJoiner(out, ','));
    out << ']';
}

void Rest::getAsset(Symbol symbol, Time now, ostream& out) const
{
    const auto& assets = serv_.assets();
    auto it = assets.find(symbol);
    if (it == assets.end()) {
        throw NotFoundException{errMsg() << "asset '" << symbol << "' does not exist"};
    }
    out << *it;
}

void Rest::getInstr(Time now, ostream& out) const
{
    const auto& instrs = serv_.instrs();
    out << '[';
    copy(instrs.begin(), instrs.end(), OStreamJoiner(out, ','));
    out << ']';
}

void Rest::getInstr(Symbol symbol, Time now, ostream& out) const
{
    const auto& instrs = serv_.instrs();
    auto it = instrs.find(symbol);
    if (it == instrs.end()) {
        throw NotFoundException{errMsg() << "instr '" << symbol << "' does not exist"};
    }
    out << *it;
}

void Rest::getAccnt(Symbol symbol, EntitySet es, Page page, Time now, ostream& out) const
{
    const auto& accnt = serv_.accnt(symbol);
    int i{0};
    out << '{';
    if (es.market()) {
        out << "\"markets\":";
        getMarket(now, out);
        ++i;
    }
    if (es.order()) {
        if (i > 0) {
            out << ',';
        }
        out << "\"orders\":";
        detail::getOrder(accnt, out);
        ++i;
    }
    if (es.exec()) {
        if (i > 0) {
            out << ',';
        }
        out << "\"execs\":";
        detail::getExec(accnt, page, out);
        ++i;
    }
    if (es.trade()) {
        if (i > 0) {
            out << ',';
        }
        out << "\"trades\":";
        detail::getTrade(accnt, out);
        ++i;
    }
    if (es.posn()) {
        if (i > 0) {
            out << ',';
        }
        out << "\"posns\":";
        detail::getPosn(accnt, out);
        ++i;
    }
    out << '}';
}

void Rest::getMarket(Time now, std::ostream& out) const
{
    const auto& markets = serv_.markets();
    out << '[';
    copy(markets.begin(), markets.end(), OStreamJoiner(out, ','));
    out << ']';
}

void Rest::getMarket(Symbol instrSymbol, Time now, std::ostream& out) const
{
    const auto& markets = serv_.markets();
    out << '[';
    copy_if(markets.begin(), markets.end(), OStreamJoiner(out, ','),
            [instrSymbol](const auto& market) { return market.instr() == instrSymbol; });
    out << ']';
}

void Rest::getMarket(Symbol instrSymbol, IsoDate settlDate, Time now, std::ostream& out) const
{
    const auto id = toMarketId(serv_.instr(instrSymbol).id(), settlDate);
    out << serv_.market(id);
}

void Rest::getOrder(Symbol accntSymbol, Time now, ostream& out) const
{
    detail::getOrder(serv_.accnt(accntSymbol), out);
}

void Rest::getOrder(Symbol accntSymbol, Symbol instrSymbol, Time now, ostream& out) const
{
    const auto& accnt = serv_.accnt(accntSymbol);
    const auto& orders = accnt.orders();
    out << '[';
    copy_if(orders.begin(), orders.end(), OStreamJoiner(out, ','),
            [instrSymbol](const auto& order) { return order.instr() == instrSymbol; });
    out << ']';
}

void Rest::getOrder(Symbol accntSymbol, Symbol instrSymbol, IsoDate settlDate, Time now,
                    ostream& out) const
{
    const auto& accnt = serv_.accnt(accntSymbol);
    const auto& instr = serv_.instr(instrSymbol);
    const auto marketId = toMarketId(instr.id(), settlDate);
    const auto& orders = accnt.orders();
    out << '[';
    copy_if(orders.begin(), orders.end(), OStreamJoiner(out, ','),
            [marketId](const auto& order) { return order.marketId() == marketId; });
    out << ']';
}

void Rest::getOrder(Symbol accntSymbol, Symbol instrSymbol, IsoDate settlDate, Id64 id, Time now,
                    ostream& out) const
{
    const auto& accnt = serv_.accnt(accntSymbol);
    const auto& instr = serv_.instr(instrSymbol);
    const auto marketId = toMarketId(instr.id(), settlDate);
    const auto& orders = accnt.orders();
    auto it = orders.find(marketId, id);
    if (it == orders.end()) {
        throw OrderNotFoundException{errMsg() << "order '" << id << "' does not exist"};
    }
    out << *it;
}

void Rest::getExec(Symbol accntSymbol, Page page, Time now, ostream& out) const
{
    detail::getExec(serv_.accnt(accntSymbol), page, out);
}

void Rest::getTrade(Symbol accntSymbol, Time now, ostream& out) const
{
    detail::getTrade(serv_.accnt(accntSymbol), out);
}

void Rest::getTrade(Symbol accntSymbol, Symbol instrSymbol, Time now, std::ostream& out) const
{
    const auto& accnt = serv_.accnt(accntSymbol);
    const auto& trades = accnt.trades();
    out << '[';
    copy_if(trades.begin(), trades.end(), OStreamJoiner(out, ','),
            [instrSymbol](const auto& trade) { return trade.instr() == instrSymbol; });
    out << ']';
}

void Rest::getTrade(Symbol accntSymbol, Symbol instrSymbol, IsoDate settlDate, Time now,
                    ostream& out) const
{
    const auto& accnt = serv_.accnt(accntSymbol);
    const auto& instr = serv_.instr(instrSymbol);
    const auto marketId = toMarketId(instr.id(), settlDate);
    const auto& trades = accnt.trades();
    out << '[';
    copy_if(trades.begin(), trades.end(), OStreamJoiner(out, ','),
            [marketId](const auto& trade) { return trade.marketId() == marketId; });
    out << ']';
}

void Rest::getTrade(Symbol accntSymbol, Symbol instrSymbol, IsoDate settlDate, Id64 id, Time now,
                    ostream& out) const
{
    const auto& accnt = serv_.accnt(accntSymbol);
    const auto& instr = serv_.instr(instrSymbol);
    const auto marketId = toMarketId(instr.id(), settlDate);
    const auto& trades = accnt.trades();
    auto it = trades.find(marketId, id);
    if (it == trades.end()) {
        throw NotFoundException{errMsg() << "trade '" << id << "' does not exist"};
    }
    out << *it;
}

void Rest::getPosn(Symbol accntSymbol, Time now, ostream& out) const
{
    detail::getPosn(serv_.accnt(accntSymbol), out);
}

void Rest::getPosn(Symbol accntSymbol, Symbol instrSymbol, Time now, ostream& out) const
{
    const auto& accnt = serv_.accnt(accntSymbol);
    const auto& posns = accnt.posns();
    out << '[';
    copy_if(posns.begin(), posns.end(), OStreamJoiner(out, ','),
            [instrSymbol](const auto& posn) { return posn.instr() == instrSymbol; });
    out << ']';
}

void Rest::getPosn(Symbol accntSymbol, Symbol instrSymbol, IsoDate settlDate, Time now,
                   ostream& out) const
{
    const auto& accnt = serv_.accnt(accntSymbol);
    const auto& instr = serv_.instr(instrSymbol);
    const auto marketId = toMarketId(instr.id(), settlDate);
    const auto& posns = accnt.posns();
    auto it = posns.find(marketId);
    if (it == posns.end()) {
        throw NotFoundException{errMsg() << "posn for '" << instrSymbol << "' on " << settlDate
                                         << " does not exist"};
    }
    out << *it;
}

void Rest::postMarket(Symbol instrSymbol, IsoDate settlDate, MarketState state, Time now,
                      ostream& out)
{
    const auto& instr = serv_.instr(instrSymbol);
    const auto settlDay = maybeIsoToJd(settlDate);
    const auto& market = serv_.createMarket(instr, settlDay, state, now);
    out << market;
}

void Rest::putMarket(Symbol instrSymbol, IsoDate settlDate, MarketState state, Time now,
                     ostream& out)
{
    const auto& instr = serv_.instr(instrSymbol);
    const auto id = toMarketId(instr.id(), settlDate);
    const auto& market = serv_.market(id);
    serv_.updateMarket(market, state, now);
    out << market;
}

void Rest::postOrder(Symbol accntSymbol, Symbol instrSymbol, IsoDate settlDate, string_view ref,
                     Side side, Lots lots, Ticks ticks, Lots minLots, Time now, ostream& out)
{
    const auto& accnt = serv_.accnt(accntSymbol);
    const auto& instr = serv_.instr(instrSymbol);
    const auto marketId = toMarketId(instr.id(), settlDate);
    const auto& market = serv_.market(marketId);
    Response resp;
    serv_.createOrder(accnt, market, ref, side, lots, ticks, minLots, now, resp);
    out << resp;
}

void Rest::putOrder(Symbol accntSymbol, Symbol instrSymbol, IsoDate settlDate, ArrayView<Id64> ids,
                    Lots lots, Time now, ostream& out)
{
    const auto& accnt = serv_.accnt(accntSymbol);
    const auto& instr = serv_.instr(instrSymbol);
    const auto marketId = toMarketId(instr.id(), settlDate);
    const auto& market = serv_.market(marketId);
    Response resp;
    if (lots > 0_lts) {
        if (ids.size() == 1) {
            serv_.reviseOrder(accnt, market, ids[0], lots, now, resp);
        } else {
            serv_.reviseOrder(accnt, market, ids, lots, now, resp);
        }
    } else {
        if (ids.size() == 1) {
            serv_.cancelOrder(accnt, market, ids[0], now, resp);
        } else {
            serv_.cancelOrder(accnt, market, ids, now, resp);
        }
    }
    out << resp;
}

void Rest::postTrade(Symbol accntSymbol, Symbol instrSymbol, IsoDate settlDate, string_view ref,
                     Side side, Lots lots, Ticks ticks, LiqInd liqInd, Symbol cpty, Time now,
                     ostream& out)
{
    const auto& accnt = serv_.accnt(accntSymbol);
    const auto& instr = serv_.instr(instrSymbol);
    const auto marketId = toMarketId(instr.id(), settlDate);
    const auto& market = serv_.market(marketId);
    auto trades = serv_.createTrade(accnt, market, ref, side, lots, ticks, liqInd, cpty, now);
    out << '[' << *trades.first;
    if (trades.second) {
        out << ',' << *trades.second;
    }
    out << ']';
}

void Rest::deleteTrade(Symbol accntSymbol, Symbol instrSymbol, IsoDate settlDate,
                       ArrayView<Id64> ids, Time now)
{
    const auto& accnt = serv_.accnt(accntSymbol);
    const auto& instr = serv_.instr(instrSymbol);
    const auto marketId = toMarketId(instr.id(), settlDate);
    serv_.archiveTrade(accnt, marketId, ids, now);
}

} // swirly
