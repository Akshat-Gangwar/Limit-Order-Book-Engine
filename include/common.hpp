#pragma once
#include <cstdint>

using namespace std;

enum class Side : uint8_t{
    Buy = 1,
    Sell = 0
};

using Price = uint32_t;
using OrderID = uint64_t;
using Quantity = uint32_t;
