#pragma once
#include <cstdint>

enum class Side : std::uint8_t{
    Buy = 1,
    Sell = 0
};

using Price = std::uint32_t;
using OrderID = std::uint64_t;
using Quantity = std::uint32_t;
using Volume = std::uint64_t;
