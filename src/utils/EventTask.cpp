//
// Created by jianyu on 6/21/19.
//

#include <cstdlib>
#include "EventTask.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* For inet_ntoa. */
#include <arpa/inet.h>

/* Required by event.h. */
#include <sys/time.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

EventTask* EventTask::global = NULL;

void EventTask::init() {
    base = event_base_new();
}

void EventTask::task(int sock, short s, void *arg) {
    char** args = (char**)arg;
    auto time_internal = (struct timeval*)args[0];
    struct event* e = (struct event*)args[2];
    auto fun = (f_time)args[1];
    (fun)((struct timeval*)args[0]);
    event_add(e, time_internal);
}

void EventTask::task_lambda(int sock, short s, void *arg) {
    char** args = (char**)arg;
    auto time_internal = (struct timeval*)args[0];
    struct event* e = (struct event*)args[2];
    auto *fun = (f_time_lambda*)args[1];
    (*fun)((struct timeval*)args[0]);
    event_add(e, time_internal);
}

void EventTask::future_task(int sock, short s, void *arg) {
    char** args = (char**)arg;
    auto time_internal = (struct timeval*)args[0];
    struct event* e = (struct event*)args[2];
    f fun = (f)args[1];
    (fun)();
    delete args[0];
    delete args[2];
    free(args);
}

void EventTask::future_task_lambda(int sock, short s, void *arg) {
    char** args = (char**)arg;
    auto time_internal = (struct timeval*)args[0];
    struct event* e = (struct event*)args[2];
    auto *fun = (f_lambda*)args[1];
    (*fun)();
    delete args[0];
    delete args[1];
    delete args[2];
    free(args);
}

void EventTask::add_time_task(struct timeval t, f_time f) {
    struct timeval tv = t;

    struct event *e = new event();
    evtimer_set(e, task, e);

    char **arg = new char*[3];
    auto arg_time = new timeval();
    *arg_time = t;
    arg[0] = (char*)arg_time;
    arg[1] = (char*)f;
    arg[2] = (char*)e;
    e->ev_arg = arg;
    event_base_set(base, e);
    event_add(e, &tv);
}

void EventTask::add_time_task_lambda(struct timeval t, f_time_lambda func) {
    struct timeval tv = t;

    struct event *e = new event();
    evtimer_set(e, task_lambda, e);

    char **arg = new char*[3];
    auto arg_time = new timeval();
    *arg_time = t;
    auto f = new f_time_lambda(std::move(func));
    arg[0] = (char*)arg_time;
    arg[1] = (char*)f;
    arg[2] = (char*)e;
    e->ev_arg = arg;
    event_base_set(base, e);
    event_add(e, &tv);
}


void EventTask::future_execute(struct timeval t, f f) {
    struct timeval tv = t;
    struct event *e = new event();
    evtimer_set(e, future_task, e);

    char **arg = new char*[3];
    auto arg_time = new timeval();
    *arg_time = t;
    arg[0] = (char*)arg_time;
    arg[1] = (char*)f;
    arg[2] = (char*)e;
    e->ev_arg = arg;
    event_base_set(base, e);
    evtimer_add(e, &tv);
}

void EventTask::future_execute_lambda(struct timeval t, f_lambda func) {
    struct timeval tv = t;
    struct event *e = new event();
    evtimer_set(e, future_task_lambda, e);

    char **arg = new char*[3];
    auto arg_time = new timeval();
    *arg_time = t;
    auto f = new f_lambda(std::move(func));
    arg[0] = (char*)arg_time;
    arg[1] = (char*)f;
    arg[2] = (char*)e;
    e->ev_arg = arg;
    event_base_set(base, e);
    evtimer_add(e, &tv);
}

void EventTask::start() {
    event_base_loop(base, 0);
}

int
setnonblock(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL);
    if (flags < 0)
        return flags;
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0)
        return -1;

    return 0;
}

bool EventTask::add_socket_task(int sock, func f) {
    if (setnonblock(sock) < 0)
        return false;
    struct event *ev_read = new event();
    event_set(ev_read, sock, EV_READ|EV_PERSIST, f, NULL);
    event_base_set(base, ev_read);
    event_add(ev_read, NULL);
    return true;
}