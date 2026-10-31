#pragma once
#include "common.hpp"

class PriceLevel;

struct alignas(64) Order{

    OrderID id;
    Price price;
    Quantity count;
    Side side;
    bool in_use = false; // Set while the order is checked out of an OrderPool.

    Order* next = nullptr;
    Order* prev = nullptr;
    const PriceLevel* level = nullptr; // The level currently queueing this order.
};

static_assert(sizeof(Order) == 64, "Order must occupy one 64-byte cache line");
static_assert(alignof(Order) == 64, "Order must be 64-byte aligned");