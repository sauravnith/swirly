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
#ifndef SWIRLYUI_POSN_HXX
#define SWIRLYUI_POSN_HXX

#include "Instr.hxx"

namespace swirly {
namespace ui {
namespace posn {

enum class Column : int { //
    CheckState, //
    Accnt, //
    MarketId, //
    Instr, //
    SettlDate, //
    BuyLots, //
    BuyAvgPrice, //
    SellLots, //
    SellAvgPrice
};
constexpr int ColumnCount{unbox(Column::SellAvgPrice) + 1};

} // posn

class Posn {
  public:
    Posn(const QString& accnt, Id64 marketId, const Instr& instr, QDate settlDate, Lots buyLots,
         Cost buyCost, Lots sellLots, Cost sellCost)
        : accnt_{accnt},
          marketId_{marketId},
          instr_{instr},
          settlDate_{settlDate},
          buyLots_{buyLots},
          buyCost_{buyCost},
          sellLots_{sellLots},
          sellCost_{sellCost}
    {
    }
    Posn() = default;
    ~Posn() noexcept = default;

    static Posn fromJson(const Instr& instr, const QJsonObject& obj);

    const QString& accnt() const noexcept { return accnt_; }
    Id64 marketId() const noexcept { return marketId_; }
    const Instr& instr() const noexcept { return instr_; }
    QDate settlDate() const noexcept { return settlDate_; }
    Lots buyLots() const noexcept { return buyLots_; }
    Cost buyCost() const noexcept { return buyCost_; }
    Lots sellLots() const noexcept { return sellLots_; }
    Cost sellCost() const noexcept { return sellCost_; }

  private:
    QString accnt_{};
    Id64 marketId_{};
    Instr instr_{};
    QDate settlDate_{};
    Lots buyLots_{};
    Cost buyCost_{};
    Lots sellLots_{};
    Cost sellCost_{};
};

QDebug operator<<(QDebug debug, const Posn& posn);

inline bool isModified(const Posn& prev, const Posn& next) noexcept
{
    return prev.buyLots() != next.buyLots() //
        || prev.buyCost() != next.buyCost() //
        || prev.sellLots() != next.sellLots() //
        || prev.sellCost() != next.sellCost();
}

} // ui
} // swirly

Q_DECLARE_METATYPE(swirly::ui::Posn)

#endif // SWIRLYUI_POSN_HXX
