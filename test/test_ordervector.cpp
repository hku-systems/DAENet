//
// Created by jianyu on 2/23/19.
//

#include "utils/OrderedVector.hpp"
#include <iostream>

int main() {
    int a[] = {1, 3, 5, 7, 9, 55, 57, 59, 61, 63};
    auto v = new OrderedVector<int>(10, 50);
    for (int &g : a) {
        v->insert(g);
    }

    int size = 4;
    int* arr = v->near_neighbour(4, size);

    for (int i = 0;i < size;i++) {
        std::cout << arr[i] << ",";
    }
}