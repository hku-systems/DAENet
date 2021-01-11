//
// Created by jianyu on 4/10/19.
//

#include "DAESimulator.hpp"
#include <list>
#include <sodium.h>
#include <chrono>
#include <cmath>
#include <map>
#include <queue>
#include <iostream>

typedef struct {
    int dest;
    int step;
    int ram;
    int t;
} DAEMsg;

bool operator<(const DAEMsg& left, const DAEMsg& right) {
    return left.ram < right.ram;
}

class DAEUser {
public:

    const static int EXP_RANDOM_MAX = 10240;
    const static int SHUFFLE_PER_GROUP = 20;
    const static int SEND_SPEED_UP = 1;

    long total_recv_msg_latencies;
    int total_recv_msg_count;

    int id;
    std::map<int, std::priority_queue<DAEMsg>> queues;
    std::queue<DAEMsg> send_queue;
    DAESimulator* simulator;
    int *neighbours;
    int count;
    int total_msg_count;

    std::queue<DAEMsg> recv_messages = std::queue<DAEMsg>();

    bool in_range(int left, int right, int val) {
        if (left < right) {
            // normal case, where the range is not crossing the 0
            return val >= left && val < right;
        }
        return (val >= left && val < simulator->num_of_users) || (val < right);
    }

    void init(int _id, DAESimulator* _sim) {
        id = _id;
        simulator = _sim;

        count = std::log2(simulator->num_of_users);

        neighbours = new int[count];

        for (int i = 0;i < count;i++) {
            neighbours[i] = (id + (1 << i)) % simulator->num_of_users;
            queues[i] = std::priority_queue<DAEMsg>();
        }

        total_msg_count = 0;

        total_recv_msg_count = 1;
        total_recv_msg_latencies = 10;
    }

    void print_info() {
        int mean = total_recv_msg_latencies / total_recv_msg_count;
        std::cout << total_msg_count << "," << mean << " ";
        total_recv_msg_latencies = mean;
        total_recv_msg_count = 1;
    }

    void recv_message(const DAEMsg &_dest) {
        if (_dest.dest == id) {
            total_recv_msg_count += 1;
            total_recv_msg_latencies += (simulator->step - _dest.t);
//            std::cout << "recv " << _dest.dest << " with " << simulator->step - _dest.t << "\n";
            return;
        }
        recv_messages.push(_dest);
    }

    std::pair<int, int> gen_random_dest() {
        int dest = randombytes_random() % simulator->num_of_users;
        int idx = count - 1;
        for (int i = count - 2;i >= 0;i--) {
            if (in_range(neighbours[i], neighbours[i + 1], dest)) {
                idx = i;
                break;
            }
        }
        return std::make_pair(dest, idx);
    }

    int get_idx(int dest) {
        int idx = count - 1;
        for (int i = count - 2;i >= 0;i--) {
            if (in_range(neighbours[i], neighbours[i + 1], dest)) {
                idx = i;
                break;
            }
        }
        return idx;
    }

    static int get_exp_random() {
        auto r = randombytes_random() % EXP_RANDOM_MAX;
        return r;
    }

    void emit_real_message() {
        auto pairs = gen_random_dest();
        int random_id = get_exp_random();
        DAEMsg msg = {.dest = pairs.first, .step = 0, .ram = random_id, .t = simulator->step};
        queues[pairs.second].push(msg);
        send_queue.push(msg);
        total_msg_count += 1;

        while (!recv_messages.empty()) {
            auto recv_msg = recv_messages.front();
            recv_messages.pop();
            int idx = get_idx(recv_msg.dest);
            recv_msg.ram = get_exp_random();
            recv_msg.step += 1;
            queues[idx].push(recv_msg);
            send_queue.push(recv_msg);
            total_msg_count += 1;
        }
    }

    void print_queue_states() {
        for (int i = 0;i < count;i++) {
            std::cout << queues[i].size() << " ";
        }
        std::cout << "\n";
    }

    void send_real_message() {
        if (!send_queue.empty()) {
            auto m = send_queue.front();
            int idx = get_idx(m.dest);
            simulator->users[neighbours[idx]].recv_message(m);
            total_msg_count -= 1;
            send_queue.pop();
            return;
        }
    }

    void send_message() {
        int idx = randombytes_random() % count;
        if (!queues[idx].empty()) {
            int ram = randombytes_random() % SHUFFLE_PER_GROUP;
            if (queues[idx].size() >= SHUFFLE_PER_GROUP || ram < queues[idx].size()) {
                simulator->users[neighbours[idx]].recv_message(queues[idx].top());
//              std::cout << "send " << queues[idx].front() << " to " << neighbours[idx] << " from " << id << "\n";
                queues[idx].pop();
                total_msg_count -= 1;
                return;
            }
        }
        auto dummy = gen_random_dest();
//        simulator->users[neighbours[dummy.second]].recv_message(dummy.first);
    }

    void send_batch_messages() {
        for (int i = 0;i < count * SEND_SPEED_UP;i++) {
            send_real_message();
        }
    }
};

void DAESimulator::start() {
    users = new DAEUser[num_of_users];
    for (int i = 0;i < num_of_users;i++) {
        users[i].init(i, this);
    }

    auto last_time = std::chrono::system_clock::now();
    auto d = std::chrono::duration<double>(0);

    while (true) {
        auto current_time = std::chrono::system_clock::now();
        auto print = false;
        d = current_time - last_time;
        if (d.count() >= 1) {
            last_time = current_time;
            print = true;
        }

        for (int i = 0;i < num_of_users;i++) {
            users[i].emit_real_message();
        }

        for (int i = 0;i < num_of_users;i++) {
            if (print && i < 1024 && i % 30 == 0 && i != 0)
                std::cout << "\n";
            users[i].send_batch_messages();
            if (print && i < 1024)
                users[i].print_info();
        }
        if (print) {
            std::cout << "\n";
            std::cout << "\033c";
        }
        step += 1;
    }
}