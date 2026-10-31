#pragma once
#include "order.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

class OrderPool {
private:
    std::vector<Order> memory_block;
    std::vector<Order*> free_list;

    bool owns(const Order* order) const {
        const auto address = reinterpret_cast<std::uintptr_t>(order);
        const auto first = reinterpret_cast<std::uintptr_t>(memory_block.data());
        return address >= first
            && address - first < memory_block.size() * sizeof(Order)
            && (address - first) % sizeof(Order) == 0;
    }

public:
    explicit OrderPool(std::size_t max_orders) {
        memory_block.resize(max_orders);
        free_list.reserve(max_orders);
        for(std::size_t i=0;i<max_orders;++i){
            free_list.push_back(&memory_block[i]);
        }
    }

    // free_list points into memory_block, so a copy would alias the original.
    OrderPool(const OrderPool&) = delete;
    OrderPool& operator=(const OrderPool&) = delete;

    Order* acquire(OrderID id, Price price, Quantity count, Side side){
        if(free_list.empty()) [[unlikely]] {
            return nullptr;
        }

        Order* order= free_list.back();
        free_list.pop_back();

        order->id= id;
        order->price= price;
        order->count = count;
        order->side= side;
        order->in_use= true;
        order->prev= nullptr;
        order->next= nullptr;
        order->level= nullptr;
        return order;
    }

    [[nodiscard]] bool release(Order* order){
        // Rejects null, foreign and already released orders, and orders still queued.
        if(!owns(order) || !order->in_use || order->level){
            return false;
        }

        order->in_use= false;
        free_list.push_back(order);
        return true;
    }

};