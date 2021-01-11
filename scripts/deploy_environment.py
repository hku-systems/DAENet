import tc
from remote.config import servers

net_dev="enp1s0d1"
# net_dev="lo"
latency=20
default_port=3100

sp=[(host, default_port) for host in servers]

def setup_environment(server_ports=sp):
    pass
    # tc.tc_latency(server_ports, net_dev, latency, True)

def cleanup_environment(server_ports=sp):
    pass
    # tc.tc_latency(server_ports, net_dev, latency, False)