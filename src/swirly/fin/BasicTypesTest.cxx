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
#include "BasicTypes.hpp"

#include <swirly/unit/Test.hpp>

#include <cstring>

using namespace swirly;

SWIRLY_TEST_CASE(AssetType)
{
    SWIRLY_CHECK(strcmp(enumString(AssetType::Cmdty), "CMDTY") == 0);
}

SWIRLY_TEST_CASE(Direct)
{
    SWIRLY_CHECK(strcmp(enumString(Direct::Paid), "PAID") == 0);
}

SWIRLY_TEST_CASE(LiqInd)
{
    SWIRLY_CHECK(strcmp(enumString(LiqInd::Maker), "MAKER") == 0);
}

SWIRLY_TEST_CASE(Side)
{
    SWIRLY_CHECK(strcmp(enumString(Side::Buy), "BUY") == 0);
}

SWIRLY_TEST_CASE(State)
{
    SWIRLY_CHECK(strcmp(enumString(State::New), "NEW") == 0);
}

SWIRLY_TEST_CASE(StateResd)
{
    SWIRLY_CHECK(strcmp(enumString(State::Trade, 0_lts), "COMPLETE") == 0);
    SWIRLY_CHECK(strcmp(enumString(State::Trade, 1_lts), "PARTIAL") == 0);
}
