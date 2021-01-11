//
// Created by jianyu on 6/21/19.
//

#ifndef ANONYMOUSP2P_EVENTTASK_HPP
#define ANONYMOUSP2P_EVENTTASK_HPP

#include <event.h>
#include <functional>

typedef void (*func)(int, short, void*);
typedef void (*f)();
typedef void (*f_time)(struct timeval *);
typedef std::function<void()> f_lambda;
typedef std::function<void(struct timeval*)> f_time_lambda;
class EventTask {
private:
    struct event_base *base;

    // init the libevent runtime
    void init();

    static void task(int sock, short s, void *arg);
    static void task_lambda(int sock, short s, void *arg);
    static void future_task(int sock, short s, void *arg);
    static void future_task_lambda(int sock, short s, void *arg);
public:

    explicit EventTask() {
        init();
    }

    // global task runtime
    static EventTask* global;

    /*
     * There are two types of function:
     * 1. void function (VoidFunc):
     *      void func() { // content }
     * 2. time function (TimeFunc):
     *      void func(struct timeval *tv) { // content }
     *      tv is the time interval of next task
     */

    /*
     * The difference of the lambda and non-lambda versions:
     * the function taken in the lambda version can capture variables
     * while the non-lambda version cannot.
     * Both of them can take lambda function without capturing variables,
     * but non-lambda version does not need to create a std::function
     * instance and thus (may) has better performance.
     */

    /*
     * add a TimeFunc task that runs in a routine of t
     */
    void add_time_task(struct timeval t, f_time f);
    void add_time_task_lambda(struct timeval t, f_time_lambda f);

    bool add_socket_task(int sock, func f);

    /*
     * add a VoidFunc task that runs after t
    */
    void future_execute(struct timeval t, f f);
    void future_execute_lambda(struct timeval t, f_lambda func);

    /*
     * start the libevent loop
     */
    void start();
};


#endif //ANONYMOUSP2P_EVENTTASK_HPP
