/*
 * The Restful Matching-Engine.
 * Copyright (C) 2013, 2016 Swirly Cloud Limited.
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
#include "PosnModel.hpp"

#include <algorithm>

using namespace std;

namespace swirly {
namespace ui {
using namespace posn;

PosnModel::PosnModel(QObject* parent) : QAbstractTableModel{parent}
{
  header_[unbox(Column::MarketId)] = tr("Market Id");
  header_[unbox(Column::Contr)] = tr("Contr");
  header_[unbox(Column::SettlDate)] = tr("Settl Date");
  header_[unbox(Column::Accnt)] = tr("Accnt");
  header_[unbox(Column::BuyLots)] = tr("Buy Lots");
  header_[unbox(Column::BuyAvgPrice)] = tr("Buy Avg Price");
  header_[unbox(Column::SellLots)] = tr("Sell Lots");
  header_[unbox(Column::SellAvgPrice)] = tr("Sell Avg Price");
}

PosnModel::~PosnModel() noexcept = default;

int PosnModel::rowCount(const QModelIndex& parent) const
{
  return rows_.size();
}

int PosnModel::columnCount(const QModelIndex& parent) const
{
  return ColumnCount;
}

QVariant PosnModel::data(const QModelIndex& index, int role) const
{
  QVariant var{};
  if (!index.isValid()) {
    // No-op.
  } else if (role == Qt::DisplayRole) {
    const auto& posn = rows_.nth(index.row())->second;
    switch (box<Column>(index.column())) {
    case Column::MarketId:
      var = toVariant(posn.marketId());
      break;
    case Column::Contr:
      var = posn.contr().mnem();
      break;
    case Column::SettlDate:
      var = posn.settlDate();
      break;
    case Column::Accnt:
      var = posn.accnt();
      break;
    case Column::BuyLots:
      var = toVariant(posn.buyLots());
      break;
    case Column::BuyAvgPrice:
      var = ticksToAvgPriceString(posn.buyLots(), posn.buyCost(), posn.contr());
      break;
    case Column::SellLots:
      var = toVariant(posn.sellLots());
      break;
    case Column::SellAvgPrice:
      var = ticksToAvgPriceString(posn.sellLots(), posn.sellCost(), posn.contr());
      break;
    }
  } else if (role == Qt::TextAlignmentRole) {
    switch (box<Column>(index.column())) {
    case Column::Contr:
    case Column::Accnt:
      var = QVariant{Qt::AlignLeft | Qt::AlignVCenter};
      break;
    case Column::MarketId:
    case Column::SettlDate:
    case Column::BuyLots:
    case Column::BuyAvgPrice:
    case Column::SellLots:
    case Column::SellAvgPrice:
      var = QVariant{Qt::AlignRight | Qt::AlignVCenter};
      break;
    }
  } else if (role == Qt::UserRole) {
    var = QVariant::fromValue(rows_.nth(index.row())->second);
  }
  return var;
}

QVariant PosnModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  QVariant var{};
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    var = header_[section];
  }
  return var;
}

void PosnModel::updateRow(const Posn& posn)
{
  auto it = rows_.lower_bound(posn.marketId());
  const int i = distance(rows_.begin(), it);

  const bool found{it != rows_.end() && !rows_.key_comp()(posn.marketId(), it->first)};
  if (found) {
    it->second = posn;
    emit dataChanged(index(i, 0), index(i, ColumnCount - 1));
  } else {
    // If not found then insert.
    beginInsertRows(QModelIndex{}, i, i);
    rows_.emplace_hint(it, posn.marketId(), posn);
    endInsertRows();
  }
}

} // ui
} // swirly
