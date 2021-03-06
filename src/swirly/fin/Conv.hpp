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
#ifndef SWIRLY_FIN_CONV_HPP
#define SWIRLY_FIN_CONV_HPP

#include <swirly/fin/BasicTypes.hpp>

#include <cmath>

namespace swirly {

constexpr int64_t roundHalfAway(double real) noexcept
{
    return static_cast<int64_t>(real < 0.0 ? real - 0.5 : real + 0.5);
}

constexpr double fractToReal(int numer, int denom) noexcept
{
    return static_cast<double>(numer) / static_cast<double>(denom);
}

constexpr double fractToReal(Incs numer, Incs denom) noexcept
{
    return static_cast<double>(numer) / static_cast<double>(denom);
}

constexpr Incs realToIncs(double real, double incSize) noexcept
{
    return roundHalfAway(real / incSize);
}

constexpr double incsToReal(Incs incs, double incSize) noexcept
{
    return incs * incSize;
}

/**
 * Convert quantity to lots.
 */
constexpr Lots qtyToLots(double qty, double qtyInc) noexcept
{
    return Lots{realToIncs(qty, qtyInc)};
}

/**
 * Convert lots to quantity.
 */
constexpr double lotsToQty(Lots lots, double qtyInc) noexcept
{
    return incsToReal(lots.count(), qtyInc);
}

/**
 * Convert price to ticks.
 */
constexpr Ticks priceToTicks(double price, double priceInc) noexcept
{
    return Ticks{realToIncs(price, priceInc)};
}

/**
 * Convert ticks to price.
 */
constexpr double ticksToPrice(Ticks ticks, double priceInc) noexcept
{
    return incsToReal(ticks.count(), priceInc);
}

/**
 * Number of decimal places in real.
 */
inline int realToDp(double d) noexcept
{
    int dp{0};
    for (; dp < 9; ++dp) {
        double ip{};
        const double fp{std::modf(d, &ip)};
        if (fp < 0.000000001) {
            break;
        }
        d *= 10;
    }
    return dp;
}

/**
 * Decimal places as real.
 */
inline double dpToReal(int dp) noexcept
{
    return std::pow(10, -dp);
}

/**
 * Cost from lots and ticks.
 */
constexpr Cost cost(Lots lots, Ticks ticks) noexcept
{
    return Cost{lots.count() * ticks.count()};
}

} // swirly

#endif // SWIRLY_FIN_CONV_HPP
