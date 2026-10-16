import sys
import socket
import struct
import select
import time
import heapq

## A version which is incomplete but implements dijkstra, which isn't necessary. 
## Kept this version just to keep dijkstra, if ever needed. 

def print_routing_table(start_node, lsp_db):
    distances, parents = run_dijkstra(start_node, lsp_db)
    print(f"Routing Table for Node {start_node}")
    for node, dist in sorted(distances.items()):
        if dist == float('inf'):
            print(f"To {node}: Unreachable")
        else:
            print(f"To {node}: Cost = {dist}")

def run_dijkstra(start_node, lsp_db) :
    graph = {}
    for node, info in lsp_db.items() :
        if node not in graph :
            graph[node] = {}
        for neigh, cost in info['neighbors'].items() :
            c = cost['cost'] if isinstance(cost, dict) else cost
            graph[node][neigh] = c
            if neigh not in graph :
                graph[neigh] = {}
            graph[neigh][node] = c
    distances = {node : float('inf') for node in graph}
    distances[start_node] = 0
    pq = [(0,start_node)]
    parent = {start_node : None}

    while pq :
        cdist, curr = heapq.heappop(pq)
        if cdist > distances[curr] :
            continue
        for neigh, w in graph.get(curr, {}).items() :
            dist = cdist + w
            if dist < distances[neigh] :
                distances[neigh] = dist
                parent[neigh] = curr
                heapq.heappush(pq, (dist, neigh))
    return distances, parent

def create_lsp_pkt(id, seq_num, neighbors) :
    header = struct.pack("!cIB", id.encode('utf-8'), seq_num, len(neighbors))
    body = bytearray()
    for n_id, data in neighbors.items() :
        cost = data['cost'] if isinstance(data, dict) else data
        body.extend(struct.pack("!ci", n_id.encode('utf-8'), cost))
    return header+body

def flood_lsp(sock, pkt_bytes, neighbors, exclude_ip_port=None) :
    for n_id, n_info in neighbors.items() :
        target = (n_info['ip'], n_info['port'])
        if exclude_ip_port == target :
            continue
        try :
            sock.sendto(pkt_bytes, target)
        except Exception as e :
            print(f"Error sending LSP to {n_id} : {e}")

def main():
    if len(sys.argv) != 4 :
        print("Usage: python3 VN.py ON_IP ON_PORT OWN_UDP_PORT")
        sys.exit(1)
    on_ip = sys.argv[1]
    on_port = int(sys.argv[2])
    own_udp_port = int(sys.argv[3])

    udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        udp_sock.bind(('',own_udp_port))
    except Exception as e :
        print(f"error binding socket to own UDP port : {e}")
        sys.exit(1)

    own_ip = socket.gethostbyname(socket.gethostname())
    print(f"VN initialized")

    tcp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        tcp_sock.connect((on_ip, on_port))
    except Exception as e:
        print(f"Error connecting to ON")

    try:
        ip_temp = socket.inet_aton(own_ip)
        connect_msg = struct.pack("!4sH", ip_temp, own_udp_port)
        tcp_sock.sendall(connect_msg)
        print(f"Sent CONNECT message to ON")
    except Exception as e :
        print(f"Error in sending CONNECT")
        tcp_sock.close()
        sys.exit(1)

    inputs = [tcp_sock, udp_sock]
    my_abet = None
    my_neighbors = None
    lsp = {}
    my_seq_no = 0

    last_time = time.time()
    interval = 15.0

    while True :
        try:
            curr_time = time.time()
            timeout = max(0.1, interval - (curr_time - last_time))
            recieved, _, _ = select.select(inputs, [], [], 2.0)
            if not recieved :
                if time.time() - last_time >= interval :
                    if my_abet :
                        my_seq_no += 1
                        lsp[my_abet] = {'seq' : my_seq_no, 'neighbors' : my_neighbors}
                        print_routing_table(my_abet, lsp)
                        pkt_bytes = create_lsp_pkt(my_abet, my_seq_no, my_neighbors)
                        flood_lsp(udp_sock, pkt_bytes, my_neighbors)
                        print(f"Sent periodic LSP (Seq : {my_seq_no})")
                    last_time = time.time()
                continue

            for s in recieved :
                if s is tcp_sock :
                    data = tcp_sock.recv(1024)
                    if not data:
                        print("Oracle Node closed connection.")
                        break
                    
                    tuple_sz = struct.calcsize("!c4sHi")
                    num = len(data)//tuple_sz
                    info = []
                    curr_neighbors = {}

                    for i in range(num):
                        chunk = data[i*tuple_sz : (i+1)*tuple_sz]
                        raw, ip_bytes, port, cost = struct.unpack("!c4sHi", chunk)
                        alpha = raw.decode('utf-8')
                        ip_str = socket.inet_ntoa(ip_bytes)
                        info.append(f"{alpha}={cost}")
                        if(cost==0):
                            my_abet = alpha
                        else:
                            curr_neighbors[alpha] = {'ip' : ip_str, 'port' : port, 'cost' : cost}
                    my_neighbors = curr_neighbors
                    print(f"Updated Link-State from ON: {', '.join(info)}")

                    if my_abet :
                        my_seq_no += 1
                        lsp[my_abet] = {'seq' : my_seq_no, 'neighbors' : my_neighbors}
                        print_routing_table(my_abet, lsp)
                        pkt_bytes = create_lsp_pkt(my_abet, my_seq_no, my_neighbors)
                        flood_lsp(udp_sock, pkt_bytes, my_neighbors)

                elif s is udp_sock :
                    pkt, addr = udp_sock.recvfrom(2048)
                    header_fmt = "!cIB"
                    header_sz = struct.calcsize(header_fmt)
                    if len(pkt) < header_sz :
                        continue
                    idraw, seq_num, num = struct.unpack(header_fmt, pkt[:header_sz])
                    id = idraw.decode('utf-8')
                    neigh_fmt = "!ci"
                    neigh_sz = struct.calcsize(neigh_fmt)
                    offset = header_sz
                    neighbors = {}

                    for _ in range(num) :
                        if offset + neigh_sz > len(pkt) :
                            break
                        n_bytes, n_cost = struct.unpack(neigh_fmt, pkt[offset : offset+neigh_sz])
                        neighbors[n_bytes.decode('utf-8')] = n_cost
                        offset += neigh_sz

                    if id not in lsp or seq_num > lsp[id]['seq'] :
                        print(f"Newer LSP received from node {id} (Seq: {seq_num})")
                        lsp[id] = {'seq' : seq_num, 'neighbors' : neighbors}
                        print_routing_table(my_abet, lsp)
                        pkt_bytes = create_lsp_pkt(id, seq_num, neighbors)
                        flood_lsp(udp_sock, pkt_bytes, my_neighbors, addr)
                    else :
                        # Discard duplicate
                        pass

        except Exception as e :
            print(f"Error in communications : {e}")

    tcp_sock.close()
    udp_sock.close()

if __name__ == "__main__" :
    main()