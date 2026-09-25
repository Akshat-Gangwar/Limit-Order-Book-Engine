#pragma once
#include "order.hpp"
#include <vector>

using namespace std;

class OrderPool {
private: 
    vector<Order> memory_block;
    vector<Order*> free_list;
    
public:
    explicit OrderPool(size_t max_orders) {
        memory_block.resize(max_orders);
        free_list.reserve(max_orders);
        for(size_t i=0;i<max_orders;++i){
            free_list.push_back(&memory_block[i]);
        }
    }
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
        order->prev= nullptr;
        order->next= nullptr;
        return order;
    }

    void release(Order* order){
        free_list.push_back(order);
    }

};