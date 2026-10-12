import sys
import socket
import struct
import select

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

    while True :
        try:
            recieved, _, _ = select.select(inputs, [], [], 2.0)
            for s in recieved :
                if s is tcp_sock :
                    data = tcp_sock.recv(1024)
                    if data:
                        tuple_sz = struct.calcsize("!ci")
                        num = len(data)//tuple_sz
                        info = []
                        for i in range(num):
                            chunk = data[i*tuple_sz : (i+1)*tuple_sz]
                            raw, cost = struct.unpack("!ci", chunk)
                            alpha = raw.decode('utf-8')
                            info.append(f"{alpha}={cost}")
                        print(", ".join(info))

                elif s is udp_sock :
                    pass

        except Exception as e :
            print(f"Error in communications : {e}")

    tcp_sock.close()
    udp_sock.close()

if __name__ == "__main__" :
    main()