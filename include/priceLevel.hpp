#pragma once
#include "order.hpp"
#include <cassert>
#include <cstdlib>

class PriceLevel {
public:
    Price price = 0;
    Side side = Side::Buy;
    Volume total_volume = 0;
    Order* head = nullptr;
    Order* tail = nullptr;

    PriceLevel() = default;

    // Queued orders point back at their level, so a level cannot be copied.
    PriceLevel(const PriceLevel&) = delete;
    PriceLevel& operator=(const PriceLevel&) = delete;

    bool empty() const {
        return head == nullptr;
    }

    bool debug_volume_matches() const {
        Volume volume = 0;
        for (const Order* order = head; order; order = order->next) {
            volume += order->count;
        }
        return volume == total_volume;
    }

    [[nodiscard]] bool push_back(Order* order){
        if(!order || order->level || order->prev || order->next
            || order->price != price || order->side != side){
            return false;
        }

        total_volume += order->count;
        order->level = this;
        order->prev = tail;

        if(!tail) {
            head = tail = order;
        }
        else {
            tail->next = order;
            tail = order;
        }
        verify_volume();
        return true;
    }

    [[nodiscard]] bool reduce_quantity(Order* order, Quantity quantity){
        if(!order || order->level != this || quantity == 0 || quantity > order->count){
            return false;
        }

        assert(order->price == price);
        // A full reduction leaves a zero-count order linked; remove it afterward.
        order->count -= quantity;
        total_volume -= quantity;
        verify_volume();
        return true;
    }

    [[nodiscard]] bool remove( Order* order){
        if(!order || order->level != this){
            return false;
        }

        assert(order->price == price);
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
        order->level = nullptr;
        verify_volume();
        return true;
    }

private:
    void verify_volume() const {
#ifdef LOB_PARANOID
        if (!debug_volume_matches()) {
            std::abort();
        }
#endif
    }
};
