#include "orderBook.hpp"
#include "orderPool.hpp"
#include "priceLevel.hpp"
#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<OrderPool>, "OrderPool must not be copyable");
static_assert(!std::is_copy_assignable_v<OrderPool>, "OrderPool must not be copyable");
static_assert(!std::is_copy_constructible_v<PriceLevel>, "PriceLevel must not be copyable");
static_assert(!std::is_copy_assignable_v<PriceLevel>, "PriceLevel must not be copyable");
static_assert(!std::is_copy_constructible_v<OrderBook>, "OrderBook must not be copyable");
static_assert(!std::is_copy_assignable_v<OrderBook>, "OrderBook must not be copyable");

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_invariant(const PriceLevel& level, Volume expected_volume) {
    require(level.total_volume == expected_volume, "unexpected total volume");
    require(level.debug_volume_matches(), "queue volume invariant failed");

    const Order* previous = nullptr;
    const Order* last = nullptr;
    for (const Order* order = level.head; order; order = order->next) {
        require(order->prev == previous, "queue previous link is incorrect");
        previous = order;
        last = order;
    }
    require(last == level.tail, "queue tail does not match the final order");
    require((level.head == nullptr) == (level.tail == nullptr),
            "head and tail emptiness do not match");
}

void test_push_and_remove_positions() {
    PriceLevel level;
    level.price = 100;
    Order first{1, 100, 10, Side::Buy};
    Order middle{2, 100, 20, Side::Buy};
    Order third{3, 100, 30, Side::Buy};
    Order last{4, 100, 40, Side::Buy};

    require(level.push_back(&first), "first push was rejected");
    require(level.head == &first && level.tail == &first, "first push failed");
    require_invariant(level, 10);

    require(level.push_back(&middle), "second push was rejected");
    require(level.head == &first && level.tail == &middle, "second push failed");
    require_invariant(level, 30);

    require(level.push_back(&third), "third push was rejected");
    require_invariant(level, 60);
    require(level.push_back(&last), "fourth push was rejected");
    require(level.tail == &last, "tail push failed");
    require_invariant(level, 100);

    require(level.remove(&middle), "middle removal was rejected");
    require(first.next == &third && third.prev == &first, "middle removal failed");
    require_invariant(level, 80);

    require(level.remove(&last), "tail removal was rejected");
    require(level.tail == &third && third.next == nullptr, "tail removal failed");
    require_invariant(level, 40);

    require(level.remove(&first), "head removal was rejected");
    require(level.head == &third && third.prev == nullptr, "head removal failed");
    require_invariant(level, 30);

    require(level.remove(&third), "final removal was rejected");
    require(level.empty(), "removing the final order should empty the level");
    require_invariant(level, 0);
}

void test_partial_reduction() {
    PriceLevel level;
    level.price = 100;
    Order order{1, 100, 10, Side::Buy};
    require(level.push_back(&order), "push was rejected");
    require_invariant(level, 10);

    require(level.reduce_quantity(&order, 4), "partial reduction failed");
    require(order.count == 6, "partial reduction did not update order count");
    require_invariant(level, 6);
}

void test_full_reduction_then_remove() {
    PriceLevel level;
    level.price = 100;
    Order order{1, 100, 10, Side::Buy};
    require(level.push_back(&order), "push was rejected");
    require_invariant(level, 10);

    require(level.reduce_quantity(&order, order.count), "full reduction failed");
    require(order.count == 0, "full reduction did not zero the order count");
    require_invariant(level, 0);

    require(level.remove(&order), "removal was rejected");
    require(level.empty(), "level should be empty after removing the reduced order");
    require_invariant(level, 0);
}

void test_invalid_reductions() {
    PriceLevel level;
    level.price = 100;
    Order order{1, 100, 10, Side::Buy};
    require(level.push_back(&order), "push was rejected");
    require_invariant(level, 10);

    require(!level.reduce_quantity(&order, 11), "oversized reduction should fail");
    require(order.count == 10, "oversized reduction changed order count");
    require_invariant(level, 10);

    require(!level.reduce_quantity(&order, 0), "zero reduction should fail");
    require(order.count == 10, "zero reduction changed order count");
    require_invariant(level, 10);
    require(!level.reduce_quantity(nullptr, 1), "null order should fail");
    require_invariant(level, 10);
}

void test_middle_reduction_then_remove_head() {
    PriceLevel level;
    level.price = 100;
    Order head{1, 100, 10, Side::Buy};
    Order middle{2, 100, 20, Side::Buy};
    Order tail{3, 100, 30, Side::Buy};
    require(level.push_back(&head), "head push was rejected");
    require_invariant(level, 10);
    require(level.push_back(&middle), "middle push was rejected");
    require_invariant(level, 30);
    require(level.push_back(&tail), "tail push was rejected");
    require_invariant(level, 60);

    require(level.reduce_quantity(&middle, 5), "middle reduction failed");
    require(middle.count == 15, "middle reduction did not update order count");
    require_invariant(level, 55);

    require(level.remove(&head), "head removal was rejected");
    require(level.head == &middle, "middle order should become the head");
    require_invariant(level, 45);
}

void test_order_book_initializes_level_prices() {
    OrderBook book(100, 3);
    require(book.first_price() == 100, "OrderBook first price is incorrect");
    require(book.level_count() == 3, "OrderBook level count is incorrect");

    for (Price price = 100; price <= 102; ++price) {
        require(book.bid_level(price).price == price, "bid level price was not initialized");
        require(book.bid_level(price).side == Side::Buy, "bid level side was not initialized");
        require(book.ask_level(price).price == price, "ask level price was not initialized");
        require(book.ask_level(price).side == Side::Sell, "ask level side was not initialized");
    }
}

void test_order_book_rejects_invalid_ranges() {
    const Price max_price = std::numeric_limits<Price>::max();

    OrderBook top_of_range(max_price, 1);
    require(top_of_range.bid_level(max_price).price == max_price,
            "the highest price should be a valid level");

    const auto rejects = [](Price first_price, std::size_t level_count) {
        try {
            OrderBook book(first_price, level_count);
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    };

    require(rejects(100, 0), "zero levels should be rejected");
    require(rejects(max_price, 2), "a range past the highest price should be rejected");
    require(rejects(2, std::numeric_limits<std::size_t>::max()),
            "a level count that overflows should be rejected");
}

void test_double_remove_is_rejected() {
    PriceLevel level;
    level.price = 100;
    Order first{1, 100, 10, Side::Buy};
    Order second{2, 100, 20, Side::Buy};
    require(level.push_back(&first), "first push was rejected");
    require(level.push_back(&second), "second push was rejected");

    require(level.remove(&first), "first removal was rejected");
    require(!level.remove(&first), "second removal should be rejected");
    require(level.head == &second && level.tail == &second, "queued order was lost");
    require_invariant(level, 20);
}

void test_double_push_is_rejected() {
    PriceLevel level;
    level.price = 100;
    Order only{1, 100, 10, Side::Buy};
    Order other{2, 100, 20, Side::Buy};
    require(level.push_back(&only), "push was rejected");

    require(!level.push_back(&only), "pushing a queued order should be rejected");
    require(only.next == nullptr && only.prev == nullptr, "rejected push changed the links");
    require_invariant(level, 10);

    require(level.push_back(&other), "push was rejected");
    require(!level.push_back(&only), "pushing a queued head should be rejected");
    require_invariant(level, 30);
}

void test_invalid_pushes_are_rejected() {
    PriceLevel level;
    level.price = 100;
    Order wrong_price{1, 101, 10, Side::Buy};
    Order wrong_side{2, 100, 10, Side::Sell};
    Order stale_links{3, 100, 10, Side::Buy};
    stale_links.next = &wrong_price;

    require(!level.push_back(nullptr), "null order should be rejected");
    require(!level.push_back(&wrong_price), "wrong price should be rejected");
    require(!level.push_back(&wrong_side), "wrong side should be rejected");
    require(!level.push_back(&stale_links), "order with stale links should be rejected");
    require(level.empty(), "rejected pushes should leave the level empty");
    require_invariant(level, 0);
}

void test_wrong_level_operations_are_rejected() {
    OrderBook book(100, 2);
    PriceLevel& bid = book.bid_level(100);
    PriceLevel& ask = book.ask_level(100);
    Order order{1, 100, 10, Side::Buy};
    require(bid.push_back(&order), "bid push was rejected");

    require(!ask.push_back(&order), "a bid should not join an ask level");
    require(!ask.remove(&order), "removing through the wrong side should be rejected");
    require(!ask.reduce_quantity(&order, 5), "reducing through the wrong side should be rejected");
    require(order.count == 10, "rejected reduction changed order count");
    require(bid.head == &order, "rejected removal unlinked the order");
    require_invariant(bid, 10);
    require_invariant(ask, 0);
}

void test_pool_reuses_released_orders() {
    OrderPool pool(1);
    Order* order = pool.acquire(1, 100, 10, Side::Buy);
    require(order != nullptr, "acquire failed");
    require(pool.release(order), "release was rejected");

    Order* reused = pool.acquire(2, 101, 20, Side::Sell);
    require(reused == order, "pool should reuse the released slot");
    require(reused->id == 2 && reused->price == 101 && reused->count == 20,
            "acquire did not reset the order");
}

void test_pool_exhaustion_is_safe() {
    OrderPool pool(1);
    PriceLevel level;
    level.price = 100;
    require(pool.acquire(1, 100, 10, Side::Buy) != nullptr, "first acquire failed");

    Order* exhausted = pool.acquire(2, 100, 10, Side::Buy);
    require(exhausted == nullptr, "an empty pool should return null");
    require(!level.push_back(exhausted), "the null from a full pool should be rejected");
    require_invariant(level, 0);
}

void test_double_release_is_rejected() {
    OrderPool pool(2);
    Order* order = pool.acquire(1, 100, 10, Side::Buy);
    require(pool.release(order), "first release was rejected");
    require(!pool.release(order), "second release should be rejected");

    Order* first = pool.acquire(2, 100, 10, Side::Buy);
    Order* second = pool.acquire(3, 100, 10, Side::Buy);
    require(first != nullptr && second != nullptr, "acquire failed");
    require(first != second, "two live orders share a slot");
    require(pool.acquire(4, 100, 10, Side::Buy) == nullptr, "pool grew past its capacity");
}

void test_foreign_release_is_rejected() {
    OrderPool pool(1);
    OrderPool other(1);
    Order outside{1, 100, 10, Side::Buy};
    outside.in_use = true;
    Order* other_order = other.acquire(2, 100, 10, Side::Buy);

    require(!pool.release(nullptr), "null should be rejected");
    require(!pool.release(&outside), "an outside order should be rejected");
    require(!pool.release(other_order), "another pool's order should be rejected");

    require(pool.acquire(3, 100, 10, Side::Buy) != nullptr, "acquire failed");
    require(pool.acquire(4, 100, 10, Side::Buy) == nullptr, "pool grew past its capacity");
}

void test_release_while_queued_is_rejected() {
    OrderPool pool(1);
    PriceLevel level;
    level.price = 100;
    Order* order = pool.acquire(1, 100, 10, Side::Buy);
    require(level.push_back(order), "push was rejected");

    require(!pool.release(order), "releasing a queued order should be rejected");
    require(pool.acquire(2, 100, 10, Side::Buy) == nullptr, "queued order was handed out");
    require(level.head == order, "queued order was lost");
    require_invariant(level, 10);

    require(level.remove(order), "removal was rejected");
    require(pool.release(order), "release after removal was rejected");
}

} // namespace

int main() {
    try {
        test_push_and_remove_positions();
        test_partial_reduction();
        test_full_reduction_then_remove();
        test_invalid_reductions();
        test_middle_reduction_then_remove_head();
        test_order_book_initializes_level_prices();
        test_order_book_rejects_invalid_ranges();
        test_double_remove_is_rejected();
        test_double_push_is_rejected();
        test_invalid_pushes_are_rejected();
        test_wrong_level_operations_are_rejected();
        test_pool_reuses_released_orders();
        test_pool_exhaustion_is_safe();
        test_double_release_is_rejected();
        test_foreign_release_is_rejected();
        test_release_while_queued_is_rejected();
    } catch (const std::exception& error) {
        std::cerr << "price_level_tests: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
