
from remote.remote_execute import unique_execute, execute, connect, local_execute, run_once
from binascii import hexlify
import os
import random
import sqlite3

TWISTD_BIN="~/.local/bin/twistd"

def readFiles():
    import petlib.pack

    # sys.path += ["../loopix"]
    # local("rm -f example.db")

    #import databaseConnect as dc
    from database_connect import DatabaseManager
    databaseName = "example.db"
    dbManager = DatabaseManager(databaseName)
    dbManager.create_users_table("Users")
    dbManager.create_providers_table("Providers")
    dbManager.create_mixnodes_table("Mixnodes")

    for f in os.listdir('.'):
        if f.endswith(".bin"):
            with open(f, 'rb') as fileName:
                lines = petlib.pack.decode(fileName.read())
                print 'Lines: ', lines
                if lines[0] == "client":
                    dbManager.insert_row_into_table('Users',
                        [None, lines[1], lines[2], lines[3],
                        sqlite3.Binary(petlib.pack.encode(lines[4])),
                        lines[5]])
                elif lines[0] == "mixnode":
                    dbManager.insert_row_into_table('Mixnodes',
                            [None, lines[1], lines[2], lines[3],
                            sqlite3.Binary(petlib.pack.encode(lines[5])), lines[4]])
                elif lines[0] == "provider":
                    dbManager.insert_row_into_table('Providers',
                        [None, lines[1], lines[2], lines[3],
                        sqlite3.Binary(petlib.pack.encode(lines[4]))])
                else:
                    assert False
    dbManager.close_connection()

def storeProvidersNames():
    import petlib.pack
    pn = []
    for f in os.listdir('.'):
        if f.endswith(".bin"):
            with open(f, "rb") as infile:
                lines = petlib.pack.decode(infile.read())
                if lines[0] == "provider":
                    pn.append(lines[1])
    with open('providersNames.bi2', 'wb') as outfile:
        outfile.write(petlib.pack.encode(pn))

def getProvidersNames():
    import petlib.pack
    filedir = 'providersNames.bi2'
    with open(filedir, "rb") as infile:
        lines = petlib.pack.decode(infile.read())
    print lines
    return lines

def loaddir(instances):
    for instance in instances:
        with connect(instance['host'], instance['name'], False) as r:
            r.put("example.db", "loopix/loopix/")

# install dependency
# unique_execute("pip install numpy pytest twisted "\
    # "msgpack-python petlib sphinxmix==0.0.6 matplotlib scipy scapy pybloom",
    # is_shared=False)


# setup repo
# execute("git clone git@github.com:jianyu-m/loopix.git")

def deployMixnode(mixes):
    for idx, mix in enumerate(mixes):
        with connect(mix['host'], mix['name'], False) as r:
            r.execute("git clone git@github.com:jianyu-m/loopix.git")
            N = hexlify(os.urandom(8))
            r.execute("cd loopix/loopix/; python setup_mixnode.py %d %s Mix%s %s" % (mix['port'], mix['host'], N, 0))
            r.get('loopix/loopix/publicMixnode.bin', 'publicMixnode-%s.bin' % N)

def deployProvider(providers):
    for idx, provider in enumerate(providers):
        with connect(provider['host'], provider['name'], False) as r:
            r.execute("git clone git@github.com:jianyu-m/loopix.git")
            N = hexlify(os.urandom(8))
            r.execute("cd loopix/loopix/; python setup_provider.py %d %s Provider%s" % (provider['port'], provider['host'], N))
            r.get('loopix/loopix/publicProvider.bin', 'publicProvider-%s.bin' % N)

def deployClient(clients):
    for idx, client in enumerate(clients):
        with connect(client['host'], client['name'], False) as r:
            r.execute("git clone git@github.com:jianyu-m/loopix.git")
            r.execute("cd loopix; git pull")
            N = hexlify(os.urandom(8))
            all_providers = getProvidersNames()
            prvName = random.choice(all_providers)
            r.execute("cd loopix/loopix/; python setup_client.py %d %s Client%s %s" % (client['port'], client['host'], N, prvName))
            r.get('loopix/loopix/publicClient.bin', 'publicClient-%s.bin' % N)

def deploy_backend(all_nodes):
    local_execute("rm -rf *.bin *.bi2 example.db")
    deployMixnode(all_nodes[0])
    deployProvider(all_nodes[1])
    storeProvidersNames()
    deployClient(all_nodes[2])
    readFiles()
    loaddir(all_nodes[0] + all_nodes[1] + all_nodes[2])

def runMixnode(mixes):
    for mix in mixes:
        with connect(mix['host'], mix['name'] + "/loopix/loopix/", False) as r:
            r.execute("%s -y run_mixnode.py" % TWISTD_BIN)

def runProvider(providers):
    for provider in providers:
        with connect(provider['host'], provider['name'] + "/loopix/loopix/", False) as r:
            r.execute("%s -y run_provider.py" % TWISTD_BIN)

def runClient(clients):
    cnt = 0
    for client in clients:
        with connect(client['host'], client['name'] + "/loopix/loopix/", False) as r:
            ret = r.execute("%s -y run_client.py" % TWISTD_BIN, will_wait=False)
            cnt += 1
            if cnt == 50:
                cnt = 0
                ret.wait()

def run_backend(all_nodes):
    runMixnode(all_nodes[0])
    runProvider(all_nodes[1])
    runClient(all_nodes[2])

def update_config(all_nodes):
    for nodes in all_nodes:
        for node in nodes:
            with connect(node['host'], node['name'], False) as f:
                f.put("config.json", "loopix/loopix/")
