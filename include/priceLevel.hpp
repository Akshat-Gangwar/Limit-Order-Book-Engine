#pragma once
#include "order.hpp"

using namespace std;

class PriceLevel {
public:
    Price price = 0;
    Quantity total_volume = 0;
    Order* head = nullptr;
    Order* tail = nullptr;
    
    bool empty() const {
        return head == nullptr;
    }

    void push_back(Order* order){
        total_volume += order->count;
        order->next = nullptr;
        order->prev = tail;

        if(!tail) {
            head = tail = order;
        }
        else {
            tail->next = order;
            tail = tail->next;
        }
    }

    void remove( Order* order){
        total_volume -= order->count;

        if(order->prev){
            order->prev->next = order->next;
        }
        else{
            head = order->next;
        }

        if(order->next){
            order->next->prev = order->prev;
        }
        else{
            tail = order->prev;
        }

        order->prev = nullptr;
        order->next = nullptr;
    }
};