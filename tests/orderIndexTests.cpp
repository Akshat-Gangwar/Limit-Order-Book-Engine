#include "orderIndex.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

static_assert(!std::is_copy_constructible_v<OrderIndex>, "OrderIndex must not be copyable");
static_assert(!std::is_copy_assignable_v<OrderIndex>, "OrderIndex must not be copyable");

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void test_empty_index_finds_nothing() {
    OrderIndex index(4);
    require(index.find(1) == nullptr, "an empty index should find nothing");
    require(!index.erase(1), "erasing from an empty index should fail");
}

void test_zero_capacity_is_safe() {
    OrderIndex index(0);
    require(index.find(1) == nullptr, "a zero-capacity index should find nothing");
    require(!index.erase(1), "erasing from a zero-capacity index should fail");
}

void test_insert_then_find() {
    OrderIndex index(4);
    Order first{};
    Order second{};

    require(index.insert(1, &first), "first insert failed");
    require(index.insert(2, &second), "second insert failed");
    require(index.find(1) == &first, "first order was not found");
    require(index.find(2) == &second, "second order was not found");
    require(index.find(3) == nullptr, "a missing ID should not be found");
}

void test_id_zero_is_a_valid_key() {
    OrderIndex index(4);
    Order order{};

    require(index.find(0) == nullptr, "ID 0 should not be found before insert");
    require(index.insert(0, &order), "inserting ID 0 failed");
    require(index.find(0) == &order, "ID 0 was not found");
    require(index.erase(0), "erasing ID 0 failed");
    require(index.find(0) == nullptr, "ID 0 should not be found after erase");
}

void test_duplicate_insert_is_rejected() {
    OrderIndex index(4);
    Order first{};
    Order second{};

    require(index.insert(1, &first), "first insert failed");
    require(!index.insert(1, &second), "duplicate insert should fail");
    require(index.find(1) == &first, "duplicate insert changed the stored order");
}

void test_null_order_is_rejected() {
    OrderIndex index(4);
    require(!index.insert(1, nullptr), "inserting a null order should fail");
    require(index.find(1) == nullptr, "a null order should not be stored");
}

void test_erase_then_reinsert() {
    OrderIndex index(4);
    Order first{};
    Order second{};

    require(index.insert(1, &first), "insert failed");
    require(index.erase(1), "erase failed");
    require(index.find(1) == nullptr, "erased ID should not be found");
    require(!index.erase(1), "second erase should fail");
    require(index.insert(1, &second), "erased ID should be free to reuse");
    require(index.find(1) == &second, "reinserted order was not found");
}

// Small tables make collisions and wraparound common, and every ID in the range
// is checked after each step so a broken probe chain is noticed immediately.
void run_against_reference(std::size_t max_orders, std::uint64_t seed, int steps) {
    const OrderID id_range = max_orders * 4;
    OrderIndex index(max_orders);
    std::unordered_map<OrderID, Order*> reference;
    std::vector<Order> orders(max_orders);
    std::mt19937_64 random(seed);

    for (int step = 0; step < steps; ++step) {
        const OrderID id = random() % id_range;
        const auto found = reference.find(id);
        const bool present = found != reference.end();

        if (random() % 2 == 0) {
            if (reference.size() < max_orders) {
                Order* order = &orders[random() % max_orders];
                require(index.insert(id, order) == !present, "insert result was wrong");
                reference.emplace(id, order);
            }
        } else {
            require(index.erase(id) == present, "erase result was wrong");
            reference.erase(id);
        }

        for (OrderID check = 0; check < id_range; ++check) {
            const auto expected = reference.find(check);
            require(index.find(check) == (expected == reference.end() ? nullptr : expected->second),
                    "find disagreed with the reference map");
        }
    }
}

void test_random_operations_match_reference_map() {
    for (const std::size_t max_orders : {1, 2, 3, 4, 8, 64}) {
        for (std::uint64_t seed = 1; seed <= 4; ++seed) {
            try {
                run_against_reference(max_orders, seed, 20000);
            } catch (const std::runtime_error& error) {
                throw std::runtime_error(std::string(error.what()) + " (max_orders "
                                         + std::to_string(max_orders) + ", seed "
                                         + std::to_string(seed) + ")");
            }
        }
    }
}

} // namespace

int main() {
    try {
        test_empty_index_finds_nothing();
        test_zero_capacity_is_safe();
        test_insert_then_find();
        test_id_zero_is_a_valid_key();
        test_duplicate_insert_is_rejected();
        test_null_order_is_rejected();
        test_erase_then_reinsert();
        test_random_operations_match_reference_map();
    } catch (const std::exception& error) {
        std::cerr << "order_index_tests: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
