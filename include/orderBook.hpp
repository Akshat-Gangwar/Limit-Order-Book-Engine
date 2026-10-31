#pragma once

#include "priceLevel.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

class OrderBook {
public:
    OrderBook(Price first_price, std::size_t level_count)
        : first_price_(first_price),
          level_count_(validated_level_count(first_price, level_count)),
          bids_(level_count_),
          asks_(level_count_) {
        for (std::size_t i = 0; i < level_count_; ++i) {
            const Price price = static_cast<Price>(
                static_cast<std::uint64_t>(first_price_) + i);
            bids_[i].price = price;
            bids_[i].side = Side::Buy;
            asks_[i].price = price;
            asks_[i].side = Side::Sell;
        }
    }

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    PriceLevel& bid_level(Price price) {
        return bids_[index_for(price)];
    }

    const PriceLevel& bid_level(Price price) const {
        return bids_[index_for(price)];
    }

    PriceLevel& ask_level(Price price) {
        return asks_[index_for(price)];
    }

    const PriceLevel& ask_level(Price price) const {
        return asks_[index_for(price)];
    }

    Price first_price() const {
        return first_price_;
    }

    std::size_t level_count() const {
        return level_count_;
    }

private:
    static std::size_t validated_level_count(Price first_price, std::size_t level_count) {
        if (level_count == 0) {
            throw std::invalid_argument("OrderBook must have at least one price level");
        }

        const std::uint64_t available_levels =
            static_cast<std::uint64_t>(std::numeric_limits<Price>::max()) - first_price + 1;
        if (level_count > available_levels) {
            throw std::invalid_argument("OrderBook price range exceeds the Price type");
        }
        return level_count;
    }

    std::size_t index_for(Price price) const {
        if (price < first_price_) {
            throw std::out_of_range("Price is below the OrderBook range");
        }

        const std::uint64_t index =
            static_cast<std::uint64_t>(price) - first_price_;
        if (index >= level_count_) {
            throw std::out_of_range("Price is above the OrderBook range");
        }
        return static_cast<std::size_t>(index);
    }

    Price first_price_;
    std::size_t level_count_;
    std::vector<PriceLevel> bids_;
    std::vector<PriceLevel> asks_;
};
