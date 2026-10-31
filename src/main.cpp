#include<iostream>
#include "orderPool.hpp"
#include "priceLevel.hpp"

using namespace std;

int main(){
    OrderPool pool(1024);
    PriceLevel level;
    level.price = 100;

    Order* o1 = pool.acquire(1,100,50,Side::Buy);
    Order* o2 = pool.acquire(2,100,30,Side::Buy);

    if(!level.push_back(o1) || !level.push_back(o2)){
        cerr<< "Failed to queue orders" <<endl;
        return 1;
    }

    cout<< "Price Level: "<<level.price << " | Head Order ID " << level.head->id <<endl;

    if(!level.remove(o1) || !pool.release(o1)){
        cerr<< "Failed to remove Order 1" <<endl;
        return 1;
    }

    cout<< "After removing Order 1 | Total Volume: " << level.total_volume << " | Head Order ID: "
        << level.head->id<< endl;

    if(!level.remove(o2) || !pool.release(o2)){
        cerr<< "Failed to remove Order 2" <<endl;
        return 1;
    }

    return 0;

}