//
// Created by jianyu on 3/6/19.
//

#include "utils/Heap.hpp"

int main() {
    Heap<int> heap(16);
    int arr[] = {1, 3, 5, 7, 9, 0, 2, 4, 6, 8, 10};
    for (int i = 0;i < 10;i++) {
        heap.insert(arr[i]);
    }
    for (int i = 0;i < 10;i++) {
        heap.pop_heap();
        heap.print_all();
    }

    return 0;
}