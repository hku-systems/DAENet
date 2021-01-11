
## Build / Deploy
1. enter the script dir and pull the remote module
```
    cd script; git submodule init;git submodule update
```
2. create a configuration file (config.py) of machines
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
3. setup ssh remote connections without password
```
    # generate or copy your private key to the master machine
    ssh-keygen

    # copy you public key to your github account
    
    # copy your private key to all machines (required for git pull)
    scp .ssh/id_rsa user@ip:~/.ssh/

    # copy public key to all machine
    ssh-copy-id user@ip
```
4. change the metadata in deploy-dae.py
The first time of the running will execute **dae_dep**() to install dependencies
and **deploy_dae**() to copy all configuration files to all machines. After execution,
dae_dep generate dae_dep.once and deploy_dae.once and these two functions
will not run until these two files are deleted. 

**Remember** to delete deploy_dae.once
every time you update your code repo, but you do not need that if you change
only the number of replicas running in each machine.

5. run deploy-dae.py and see the result!

6. (PS) If you want to execute one bash on all machine, use the script
```
python
python>>> from remote.remote_execute import unique_execute
python>>> unique_execute("sudo apt-get install libsodium-dev", is_sudo=True)
# this install the sodium library for all machines, is_sudo=True means execute it using sudo.
```


## Anonymous P2P Network

### Design
(tunnel/from tunnel_ip or from library)
| input | -> relay ->

In-Tunnel   --->    SendProcessor(padding/encrypt)  --->    ... (hops)  --->    
--->    RecvProcessor (decrypt/unpact)  --->    out-tunnel