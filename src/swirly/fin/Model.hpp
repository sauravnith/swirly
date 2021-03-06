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
#ifndef SWIRLY_FIN_MODEL_HPP
#define SWIRLY_FIN_MODEL_HPP

#include <swirly/fin/Types.hpp>

#include <swirly/util/Date.hpp>
#include <swirly/util/Time.hpp>

namespace swirly {

class Conf;

template <typename ValueT>
using ModelCallback = std::function<void(ValueT)>;

class SWIRLY_API Model {
  public:
    Model() noexcept = default;
    virtual ~Model() noexcept;

    // Copy.
    constexpr Model(const Model&) noexcept = default;
    Model& operator=(const Model&) noexcept = default;

    // Move.
    constexpr Model(Model&&) noexcept = default;
    Model& operator=(Model&&) noexcept = default;

    void readAsset(const ModelCallback<AssetPtr>& cb) const { doReadAsset(cb); }
    void readInstr(const ModelCallback<InstrPtr>& cb) const { doReadInstr(cb); }
    void readAccnt(Time now, const ModelCallback<std::string_view>& cb) const
    {
        doReadAccnt(now, cb);
    }
    void readMarket(const ModelCallback<MarketPtr>& cb) const { doReadMarket(cb); }
    void readOrder(const ModelCallback<OrderPtr>& cb) const { doReadOrder(cb); }
    void readExec(std::string_view accnt, std::size_t limit, const ModelCallback<ExecPtr>& cb) const
    {
        doReadExec(accnt, limit, cb);
    }
    void readTrade(const ModelCallback<ExecPtr>& cb) const { doReadTrade(cb); }
    void readPosn(JDay busDay, const ModelCallback<PosnPtr>& cb) const { doReadPosn(busDay, cb); }

  protected:
    virtual void doReadAsset(const ModelCallback<AssetPtr>& cb) const = 0;

    virtual void doReadInstr(const ModelCallback<InstrPtr>& cb) const = 0;

    virtual void doReadAccnt(Time now, const ModelCallback<std::string_view>& cb) const = 0;

    virtual void doReadMarket(const ModelCallback<MarketPtr>& cb) const = 0;

    virtual void doReadOrder(const ModelCallback<OrderPtr>& cb) const = 0;

    virtual void doReadExec(std::string_view accnt, std::size_t limit,
                            const ModelCallback<ExecPtr>& cb) const = 0;

    virtual void doReadTrade(const ModelCallback<ExecPtr>& cb) const = 0;

    virtual void doReadPosn(JDay busDay, const ModelCallback<PosnPtr>& cb) const = 0;
};

/**
 * Make Model. Forward declaration for Model backend.
 *
 * @param conf Configuration.
 */
SWIRLY_API std::unique_ptr<Model> makeModel(const Conf& conf);

} // swirly

#endif // SWIRLY_FIN_MODEL_HPP
