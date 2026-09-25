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