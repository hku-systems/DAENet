import socket
import sys, signal
import numpy as np
import time

all_client = 299*9+8

'''
msg:
#1. ['error', errno, from ,to]
#2. ['relay', session, seq, from, to] 
#3. ['relay[maybe_wrong:dest_id!=this->successor(),relay_to_successor]', session, seq, from, to] 
4. [node_id, route_item*20, successor*5(or more?)]
5. [node_id, node_ip, node_port]
#6. ['arrived_at_deaddrop_0', deaddrop_id, init_sender_id, session_id, seq_id]
#7. ['arrived_at_deaddrop_1', deaddrop_id, init_sender_id, session_id, seq_id]
#8. ['arrived_at_sender', from, init_sender_id, session_id, seq_id]
9. ['successor_failure', node_id, init_successor, new_successor]
10. ['successor_failure_and(before_pop)_empty', node_id, init_successor, new_successor]
11. ['successor_failure_and(after_pop)_empty', node_id, init_successor, new_successor]
'''



class Client:
    def __init__(self, id, ip, port):
        self.id = id
        self.ip = ip
        self.port = port

    def __str__(self):
        return "id: " + str(self.id) + "    ip: " + self.ip + "    port: " + str(self.port) + "    "


address = ('10.22.1.12', 28509)
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(address)

m = 20
cur_client = []
route_table = {}
successor_list = {}
table_done = []
successor_done = []
clients = {}

pre_time = None
flag = True

all_successors = {}
all_table = {}

max_id = 2**20

f = open('node_ids.txt', 'w')
print("generate node_ids: %d" % all_client)
node_ids = []
while True:
    x = np.random.randint(0, max_id)
    if x not in node_ids:
        node_ids.append(x)
        f.write(str(x) + '\n')
    if len(node_ids) > all_client:
        break
f.close()

def signal_handler(sig, frame):
    s.close()
    print('time: %ds\n' % (time.time() - pre_time))
    with open('clients.info', 'w') as f:
        for i in cur_client:
            f.write(str(clients[i]) + '\n')
    f = open('circle.debug', 'w')
    for x in cur_client:
        if x not in successor_client:
            f.writelines(str(x) + ' ' + str(edge[x]) + '\n')
    f.close()
    with open('successors.info', 'w') as f:
        for x in cur_client:
            f.writelines(str(x) + ':  ' + str(len(all_successors[x])) + ' ' + str(all_successors[x][:10]) + '\n')
            ids = len(np.where(cur_client > x)[0])
            if ids >= 10:
                f.writelines(str(x) + ':  ' + '10' + ' ' + str(cur_client[np.where(cur_client > x)[0][:10]]) + '\n')
            else:
                f.writelines(str(x) + ':  ' + '10' + ' ' + str(cur_client[np.where(cur_client > x)[0][:ids]]) + str(cur_client[:10 - ids]) + '\n')
    with open('table_wrong.debug', 'w') as f:
        for x in cur_client:
            if x not in table_done:
                f.writelines(str(x) + ': ' + str(all_table[x]) + '\n')
                f.writelines(str(x) + ': ' + str(route_table[x]) + '\n')
    print(successor_failures)
    print(len(successor_failures))
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)


edge = {}
successor_cnt = 0
successor_client = []

successor_failures = []

cur_client = node_ids
print(len(cur_client))
cur_client = np.array(cur_client)
cur_client.sort()
for id in cur_client:
    route_table[id] = []
    for i in range(m):
        temp = 2 ** i
        tempid = np.where(cur_client >= (id + temp) % (2 ** m))[0]
        if len(tempid) == 0:
            route_table[id].append(cur_client[0])
        else:
            route_table[id].append(cur_client[tempid[0]])
    route_table[id] = np.array(route_table[id])
    tempid = np.where(cur_client >= (id + 1) % (2 ** m))[0]
    if len(tempid) < 5:
        successor_list[id] = np.concatenate((cur_client[tempid], cur_client[:5 - len(tempid)]))
    else:
        successor_list[id] = cur_client[tempid[:5]]
print("done, ready!")

while True:
    data, addr = s.recvfrom(2048)
    if pre_time is None:
        pre_time = time.time()
    if not data:
        print("client has exist")
        break
    data = data.decode('utf8')
    # print("received:", data, "from", addr)
    a = data.split()
    if 'successor_failure' in a[0]:
        print(a)
        successor_failures.append(a)
    elif len(a) == 3:
        a[0] = int(a[0])
        a[2] = int(a[2])
        if a[0] not in clients:
            clients[a[0]] = Client(a[0], a[1], a[2])
    else:
        try:
            a = [int(x) for x in a]
            if len(cur_client) != all_client:
                continue
            id = a[0]
            if id not in cur_client:
                print(a, "!!!!!!!!!!")
                continue
            table = np.array(a[1:m+1])
            successors = a[m+1:]
            all_successors[id] = successors
            all_table[id] = table
            if len(table) != 20:
                print("node: " + str(clients[id]) + " table length is wrong! ")
            #if len(successors) != 5:
                #print("node: ", clients[id], " successor length is wrong!")
            if route_table[id][0] == table[0]:
                if id not in successor_client:
                    successor_client.append(id)
                    successor_cnt += 1
            else:
                if id in successor_client:
                    successor_client.remove(id)
                    successor_cnt -= 1
            edge[id] = table[0]
            if (route_table[id] == table).sum() == len(table):
                if id not in table_done:
                    table_done.append(id)
                    print("%d's route table is right. \nroute table:[%d/%d] successor:[%d/%d]" % (id, len(table_done), all_client, successor_cnt, all_client))
            else:
                temp = (table == route_table[id]).sum()
                table[table == route_table[id]] = 0
                if id in table_done:
                    table_done.remove(id)
                    print("%d's route table become wrong again!!! \nroute table:[%d/%d] successor:[%d/%d]" % (id, len(table_done), all_client, successor_cnt, all_client))
                    print(str(temp) + '/20')
                    print("wrong:", list(table))
                    print("right:", list(route_table[id]))
                else:
                    print("node: " + str(clients[id]) + " route table is wrong:[%d/%d] successor[%d/%d]" % (len(table_done), all_client,  successor_cnt, all_client))
                    print(str(temp) + '/20')
                    print("wrong:", list(table))
                    print("right:", list(route_table[id]))
            if len(table_done) == all_client and flag:
                print('time: %ds\n' % (time.time() - pre_time))
                flag = False
            elif len(table_done) != all_client:
                flag = True
            #if successor_list[id] == successors:
            #    if id not in successor_done:
            #        successor_done.append(id)
            #        print("%d's  successor list is right. [%d/%d]" % (id, len(successor_done), all_client))
            #else:
            #    if id in successor_done:
            #        successor_done.remove(id)
            #        print("%d's successor list become wrong again!!!" % id)
            #    else:
            #        print("node: ", clients[id], " successor list is wrong:")
            #        print("wrong:", successors)
            #        print("right:", successor_list[id])
        except Exception as e:
            print("Couldn't parse")
            print('Reason:', e)
