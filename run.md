# Run


1. pull remote repo
```
    cd script; git submodule init;git submodule update
```
2. create config.py under script/
```
# example of config.py
servers = [
    "10.22.1.1"
    "10.22.1.2"
]
# this user name and password are used only when root is required
user = "user-1"
passwd = "password"
```

3. generate node ids
```
python script/generate_ids.py  # modify parameter all_client first
```

4. deploy

modify the source code dir at line 94 in file script/deploy-dae.py
```
# delete deploy-dae.once to rebuild
python script/deploy-dae.py
```

(read README.MD for more details)


# Parameter
```
# in file script/deploy-dae.py
n=6 # number of nodes per machine
bits = 20 # ID range of nodes, [0, 1 << bits]
k=0.1  # number of sessions to pingpong (session = k * n * machines)
delay=30 # waiting time before pingpong (wait for p2p network)
request_number=5 # average latency of 5 pingpong
msg_rate=500 
pool_size=10
shuffle_P = 0.8

# some parameters for debug, or p2p internal parameter
print_shuffle=True
recheck = 20
checkwait = 4
p2p_time_b = 10
p2p_time_a = 1

```