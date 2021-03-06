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
#include "HttpClient.hxx"

#include "Asset.hxx"
#include "Exec.hxx"
#include "Instr.hxx"
#include "Market.hxx"
#include "Order.hxx"
#include "Posn.hxx"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrlQuery>

#include <boost/range/adaptor/reversed.hpp>

namespace swirly {
namespace ui {
namespace {

enum : int { GetRefData = 1, GetAccnt, PostMarket, PostOrder, PutOrder };

} // anonymous

HttpClient::HttpClient(QObject* parent) : Client{parent}
{
    connect(&nam_, &QNetworkAccessManager::finished, this, &HttpClient::slotFinished);
    connect(&nam_, &QNetworkAccessManager::networkAccessibleChanged, this,
            &HttpClient::slotNetworkAccessibleChanged);
    getRefData();
    startTimer(2000);
}

HttpClient::~HttpClient() noexcept = default;

void HttpClient::timerEvent(QTimerEvent* event)
{
    if (errors_ > 0) {
        if (!pending_) {
            errors_ = 0;
            reset();
            getRefData();
        }
    } else {
        getAccnt();
    }
}

void HttpClient::createMarket(const Instr& instr, QDate settlDate)
{
    QNetworkRequest request{QUrl{"http://127.0.0.1:8080/markets"}};
    request.setAttribute(QNetworkRequest::User, PostMarket);
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Swirly-Accnt", "MARAYL");
    request.setRawHeader("Swirly-Perm", "1");

    QJsonObject obj;
    obj["instr"] = instr.symbol();
    obj["settlDate"] = toJson(settlDate);
    obj["state"] = 0;

    QJsonDocument doc(obj);
    nam_.post(request, doc.toJson());
    ++pending_;
}

void HttpClient::createOrder(const Instr& instr, QDate settlDate, const QString& ref, Side side,
                             Lots lots, Ticks ticks)
{
    QNetworkRequest request{QUrl{"http://127.0.0.1:8080/accnt/orders"}};
    request.setAttribute(QNetworkRequest::User, PostOrder);
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Swirly-Accnt", "MARAYL");
    request.setRawHeader("Swirly-Perm", "2");

    QJsonObject obj;
    obj["instr"] = instr.symbol();
    obj["settlDate"] = toJson(settlDate);
    obj["ref"] = ref;
    obj["side"] = toJson(side);
    obj["lots"] = toJson(lots);
    obj["ticks"] = toJson(ticks);

    QJsonDocument doc(obj);
    nam_.post(request, doc.toJson());
    ++pending_;
}

void HttpClient::cancelOrders(const OrderKeys& keys)
{
    QString str;
    QTextStream out{&str};
    Market market;

    for (const auto& key : keys) {
        const auto id = key.second.count();
        if (key.first != market.id()) {
            if (!str.isEmpty()) {
                putOrder(QUrl{str});
                str.clear();
                out.reset();
            }
            market = marketModel().find(key.first);
            out << "http://127.0.0.1:8080/accnt/orders/" << market.instr().symbol() //
                << '/' << dateToIso(market.settlDate()) //
                << '/' << id;
        } else {
            out << ',' << id;
        }
    }
    if (!str.isEmpty()) {
        putOrder(QUrl{str});
    }
}

void HttpClient::slotFinished(QNetworkReply* reply)
{
    --pending_;
    reply->deleteLater();

    const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (reply->error() != QNetworkReply::NoError) {
        if (statusCode.isNull()) {
            ++errors_;
        }
        emit serviceError(reply->errorString());
        return;
    }

    const auto attr = reply->request().attribute(QNetworkRequest::User);
    switch (attr.value<int>()) {
    case GetRefData:
        onRefDataReply(*reply);
        break;
    case GetAccnt:
        onAccntReply(*reply);
        break;
    case PostMarket:
        onMarketReply(*reply);
        break;
    case PostOrder:
        onOrderReply(*reply);
        break;
    case PutOrder:
        onOrderReply(*reply);
        break;
    }
}

void HttpClient::slotNetworkAccessibleChanged(
    QNetworkAccessManager::NetworkAccessibility accessible)
{
    switch (accessible) {
    case QNetworkAccessManager::UnknownAccessibility:
        qDebug() << "network accessibility unknown";
        break;
    case QNetworkAccessManager::NotAccessible:
        qDebug() << "network is not accessible";
        break;
    case QNetworkAccessManager::Accessible:
        qDebug() << "network is accessible";
        break;
    };
}

Instr HttpClient::findInstr(const QJsonObject& obj) const
{
    return instrModel().find(fromJson<QString>(obj["instr"]));
}

void HttpClient::getRefData()
{
    QNetworkRequest request{QUrl{"http://127.0.0.1:8080/refdata"}};
    request.setAttribute(QNetworkRequest::User, GetRefData);
    request.setRawHeader("Swirly-Accnt", "MARAYL");
    request.setRawHeader("Swirly-Perm", "2");
    nam_.get(request);
    ++pending_;
}

void HttpClient::getAccnt()
{
    QUrl url{"http://127.0.0.1:8080/accnt"};
    QUrlQuery query;
    query.addQueryItem("limit", QString::number(MaxExecs));
    url.setQuery(query.query());

    QNetworkRequest request{url};
    request.setAttribute(QNetworkRequest::User, GetAccnt);
    request.setRawHeader("Swirly-Accnt", "MARAYL");
    request.setRawHeader("Swirly-Perm", "2");
    nam_.get(request);
    ++pending_;
}

void HttpClient::putOrder(const QUrl& url)
{
    QNetworkRequest request{url};
    request.setAttribute(QNetworkRequest::User, PutOrder);
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Swirly-Accnt", "MARAYL");
    request.setRawHeader("Swirly-Perm", "2");

    nam_.put(request, "{\"lots\":0}");
    ++pending_;
}

void HttpClient::onRefDataReply(QNetworkReply& reply)
{
    auto body = reply.readAll();

    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(body, &error);
    if (error.error != QJsonParseError::NoError) {
        emit serviceError(error.errorString());
        return;
    }

    // New tag for mark and sweep.
    ++tag_;

    const auto obj = doc.object();
    for (const auto elem : obj["assets"].toArray()) {
        const auto asset = Asset::fromJson(elem.toObject());
        qDebug().nospace() << "asset: " << asset;
        assetModel().updateRow(tag_, asset);
    }
    assetModel().sweep(tag_);
    for (const auto elem : obj["instrs"].toArray()) {
        const auto instr = Instr::fromJson(elem.toObject());
        qDebug().nospace() << "instr: " << instr;
        instrModel().updateRow(tag_, instr);
    }
    instrModel().sweep(tag_);
    emit refDataComplete();

    getAccnt();
}

void HttpClient::onAccntReply(QNetworkReply& reply)
{
    auto body = reply.readAll();

    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(body, &error);
    if (error.error != QJsonParseError::NoError) {
        emit serviceError(error.errorString());
        return;
    }

    // New tag for mark and sweep.
    ++tag_;

    const auto obj = doc.object();
    for (const auto elem : obj["markets"].toArray()) {
        const auto obj = elem.toObject();
        const auto instr = findInstr(obj);
        marketModel().updateRow(tag_, Market::fromJson(instr, obj));
    }
    marketModel().sweep(tag_);
    for (const auto elem : obj["orders"].toArray()) {
        const auto obj = elem.toObject();
        const auto instr = findInstr(obj);
        const auto order = Order::fromJson(instr, obj);
        if (!order.done()) {
            orderModel().updateRow(tag_, order);
        } else {
            orderModel().removeRow(order);
        }
    }
    orderModel().sweep(tag_);
    {
        using boost::adaptors::reverse;
        auto arr = obj["execs"].toArray();
        for (const auto elem : reverse(arr)) {
            const auto obj = elem.toObject();
            const auto instr = findInstr(obj);
            execModel().updateRow(tag_, Exec::fromJson(instr, obj));
        }
    }
    for (const auto elem : obj["trades"].toArray()) {
        const auto obj = elem.toObject();
        const auto instr = findInstr(obj);
        tradeModel().updateRow(tag_, Exec::fromJson(instr, obj));
    }
    tradeModel().sweep(tag_);
    for (const auto elem : obj["posns"].toArray()) {
        const auto obj = elem.toObject();
        const auto instr = findInstr(obj);
        posnModel().updateRow(tag_, Posn::fromJson(instr, obj));
    }
    posnModel().sweep(tag_);
}

void HttpClient::onMarketReply(QNetworkReply& reply)
{
    auto body = reply.readAll();

    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(body, &error);
    if (error.error != QJsonParseError::NoError) {
        emit serviceError(error.errorString());
        return;
    }

    const auto obj = doc.object();
    const auto instr = findInstr(obj);
    marketModel().updateRow(tag_, Market::fromJson(instr, obj));
}

void HttpClient::onOrderReply(QNetworkReply& reply)
{
    auto body = reply.readAll();

    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(body, &error);
    if (error.error != QJsonParseError::NoError) {
        emit serviceError(error.errorString());
        return;
    }

    const auto obj = doc.object();
    {
        const auto elem = obj["market"];
        if (!elem.isNull()) {
            const auto obj = elem.toObject();
            const auto instr = findInstr(obj);
            marketModel().updateRow(tag_, Market::fromJson(instr, obj));
        }
    }
    for (const auto elem : obj["orders"].toArray()) {
        const auto obj = elem.toObject();
        const auto instr = findInstr(obj);
        const auto order = Order::fromJson(instr, obj);
        if (!order.done()) {
            orderModel().updateRow(tag_, order);
        } else {
            orderModel().removeRow(order);
        }
    }
    {
        using boost::adaptors::reverse;
        auto arr = obj["execs"].toArray();
        for (const auto elem : reverse(arr)) {
            const auto obj = elem.toObject();
            const auto instr = findInstr(obj);
            const auto exec = Exec::fromJson(instr, obj);
            execModel().updateRow(tag_, exec);
            if (exec.state() == State::Trade) {
                tradeModel().updateRow(tag_, exec);
            }
        }
    }
    {
        const auto elem = obj["posn"];
        if (!elem.isNull()) {
            const auto obj = elem.toObject();
            const auto instr = findInstr(obj);
            posnModel().updateRow(tag_, Posn::fromJson(instr, obj));
        }
    }
}

} // ui
} // swirly
