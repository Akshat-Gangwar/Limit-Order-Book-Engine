#pragma once
#include "order.hpp"
#include <cstddef>
#include <algorithm>
#include <bit>
#include <vector>

// Open-addressing OrderID -> Order* table. It allocates only in the constructor
// and stays at most half full, so a holder must never insert more than max_orders.
class OrderIndex{
private:
    struct Slot {
        OrderID id = 0;
        Order* order = nullptr; // Null marks an empty slot.
    };

    std::vector<Slot> slots;
    std::size_t mask;
    int shift;

    // Fibonacci hashing spreads sequential IDs across the table.
    std::size_t home(OrderID id) const {
        return static_cast<std::size_t>((id * 0x9E3779B97F4A7C15ULL) >> shift);
    }

public:
    explicit OrderIndex(std::size_t max_orders)
        : slots(std::bit_ceil(std::max<std::size_t>(max_orders * 2, 2))),
          mask(slots.size() - 1),
          shift(64 - std::countr_zero(slots.size())) {}

    [[nodiscard]] bool insert(OrderID id,Order* order){
        if(!order){
            return false;
        }

        std::size_t slot = home(id);
        while(slots[slot].order){
            if(slots[slot].id == id){
                return false;
            }
            slot = (slot + 1) & mask;
        }

        slots[slot].id = id;
        slots[slot].order = order;
        return true;
    }

    Order* find(OrderID id) const {
        std::size_t slot = home(id);
        while(slots[slot].order){
            if(slots[slot].id == id){
                return slots[slot].order;
            }
            slot = (slot + 1) & mask;
        }
        return nullptr;
    }

    [[nodiscard]] bool erase(OrderID id){
        std::size_t hole = home(id);
        while(slots[hole].order && slots[hole].id != id){
            hole = (hole + 1) & mask;
        }
        if(!slots[hole].order){
            return false;
        }

        // Shift later entries back so every probe chain stays unbroken.
        std::size_t next = hole;
        while(true){
            next = (next + 1) & mask;
            if(!slots[next].order){
                break;
            }

            // An entry may fill the hole only if that isn't before its home slot.
            const std::size_t next_home = home(slots[next].id);
            if(((next - next_home) & mask) >= ((next - hole) & mask)){
                slots[hole] = slots[next];
                hole = next;
            }
        }

        slots[hole].order = nullptr;
        return true;
    }

    OrderIndex (const OrderIndex&)=delete;
    OrderIndex& operator=(const OrderIndex&)=delete;
};
