from remote.remote_execute import unique_execute, execute, connect

def tc_latency(server_ports, dev, latency, on):
    add_str = " netem delay %dms" % latency if on else ""
    print("latency on %s as %d ms - %d" % (dev, latency, on))
    for host, port in server_ports:
        with connect(host, "dae-eval/anonymous-p2p/", False, is_sudo=True) as f:
            if on:
                f.execute("device=$(route -n |grep 10.22.1|tr -s ' '|cut -d' ' -f8);"\
                    "sudo tc qdisc add dev $device root%s" % add_str, False, will_wait=False)
                f.execute("sudo tc qdisc add dev lo root handle 1:0%s" % add_str, False, will_wait=False)
            else:
                f.execute("device=$(route -n |grep 10.22.1|tr -s ' '|cut -d' ' -f8);"\
                    "sudo tc qdisc del dev $device root", False, will_wait=False)
                f.execute("sudo tc qdisc del dev lo root", False, will_wait=False)
