#pragma once
#include "common.hpp"

using namespace std;

struct alignas(64) Order{

    OrderID id;
    Price price;
    Quantity count;
    Side side;

    Order* next = nullptr;
    Order* prev = nullptr;
};

static_assert(sizeof(Order) == 64, "Order must occupy one 64-byte cache line");
static_assert(alignof(Order) == 64, "Order must be 64-byte aligned");