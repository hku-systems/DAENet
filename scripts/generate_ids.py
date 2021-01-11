import os
import numpy as np
all_client = 18
f = open('node_ids.txt', 'w')
node_ids = []
max_id = 2**20
while True:
    x = np.random.randint(0, max_id)
    if x not in node_ids:
        node_ids.append(x)
        f.write(str(x) + '\n')
    if len(node_ids) >= all_client:
        break
f.close()
exit(0)