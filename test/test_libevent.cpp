//
// Created by jianyu on 6/21/19.
//

#include <event.h>
#include <iostream>
#include <utils/EventTask.hpp>
#include <thread>
static struct event tickEvt;
static struct event tickEvt2;

void onTime(struct timeval *tv)
{
    std::cout << "t\n";

}

void onTime2(struct timeval *tv)
{
    std::cout << "t2\n";
}

void onTimeLambda(struct timeval *tv)
{
    tv->tv_sec *= 2;
    std::cout << "lambda\n";
}

int main() {
    struct timeval tv = {
            .tv_sec = 1,
            .tv_usec = 0
    };

    EventTask task;

    task.add_time_task(tv, onTime);
    task.add_time_task(tv, onTime2);
    task.add_time_task(tv, onTimeLambda);
    task.start();
    while (1);
}