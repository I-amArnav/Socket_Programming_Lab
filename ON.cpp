#include <iostream>
#include <unistd.h>
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

#define MYPORT 5000
#define BACKLOG 26
#define MY_IP "10.130.154.63"
#define BUFFER_SIZE (1 + 26 * 11) // 1 byte for number of neighbors, 11 bytes for each neighbor (1+4+2+4)

// cpp is including padding so I'm using buffer to send and receive data
// struct CONNECT_MSG{
//     uint32_t ip;
//     uint16_t port;
// };

// struct LINK_STATE_MSG{
//     uint8_t alphabet;
//     uint32_t ip;
//     uint16_t port;
//     uint32_t cost;
// };

vector<vector<int>> Virtual_network(26, vector<int>(26, -1));
vector<pair<uint32_t, uint16_t>> Node_info(26); // ip, port

int Link_State_Msg(int id, uint8_t *buffer){
    int num_nei = 0;
    int prefix_len = 1; // the first byte is for the number of neighbors/tuples
    // 1 B for alphabet, 4 B for ip, 2 B for port, 4 B for cost
    for(int i=0; i<Virtual_network[id].size(); i++){
        if(Virtual_network[id][i] != -1){
            uint32_t ip = Node_info[i].first;
            uint16_t port = Node_info[i].second;
            uint32_t cost = Virtual_network[id][i];
            buffer[prefix_len+num_nei*11] = 'A' + i; // alphabet
            buffer[prefix_len+num_nei*11+1] = (ip >> 24) & 0xFF; // ip
            buffer[prefix_len+num_nei*11+2] = (ip >> 16) & 0xFF;
            buffer[prefix_len+num_nei*11+3] = (ip >> 8) & 0xFF;
            buffer[prefix_len+num_nei*11+4] = ip & 0xFF;
            buffer[prefix_len+num_nei*11+5] = (port >> 8) & 0xFF; // port
            buffer[prefix_len+num_nei*11+6] = port & 0xFF;
            buffer[prefix_len+num_nei*11+7] = (cost >> 24) & 0xFF; // cost
            buffer[prefix_len+num_nei*11+8] = (cost >> 16) & 0xFF;
            buffer[prefix_len+num_nei*11+9] = (cost >> 8) & 0xFF;
            buffer[prefix_len+num_nei*11+10] = cost & 0xFF;
            num_nei++;  
        }
    }
    buffer[0] = num_nei; 
    return prefix_len + num_nei * 11;
}

bool recv_exact(int sockfd, uint8_t* buffer, int num_bytes){
    int bytes_received = 0;
    while(bytes_received < num_bytes){
        int result = recv(sockfd, buffer + bytes_received, num_bytes - bytes_received, 0);
        if(result < 0){
            return false;
        }else if(result == 0){
            return false;
        }
        bytes_received += result;
    }
    return true;
}

bool send_exact(int sockfd, uint8_t* buffer, int num_bytes){
    int bytes_sent = 0;
    while(bytes_sent < num_bytes){
        int result = send(sockfd, buffer + bytes_sent, num_bytes - bytes_sent, 0);
        if(result < 0){
            cout << "Error in sending message to one of the nodes\n";
            return false;
        }else if(result == 0){
            cout << "Connection closed by one of the nodes\n";
            return false; //connection closed
        }
        bytes_sent += result;
    }
    return true;
}

bool send_link_state_msgs(int num_nodes, int new_fd[]){
    int id = 0;
    while(id<num_nodes){
        uint8_t LS_Msg[BUFFER_SIZE]; 
        int msg_size = Link_State_Msg(id, LS_Msg);
        if(!send_exact(new_fd[id], LS_Msg, msg_size)){
            return false;
        }
        id++;
    }
    return true;
}

char id_to_char(int id){
    return 'A' + id;
}

int char_to_id(char c){
    return c - 'A';
}

int build_virtual_network(vector<string> &valid_lines){
    
    Virtual_network.assign(26, vector<int>(26, -1));
    int num_nodes = valid_lines.size()+1;

    for(int id1 =0; id1<num_nodes-1; id1++){
        Virtual_network[id1][id1] = 0;
        string line = valid_lines[id1];
        int id2 = id1+1;
        for(long unsigned i=0; i<line.size(); i++){
            char c = line[i];
            if(c == '\t' || c == ' ' || c == '\r' || c == '\n'){
                continue;
            }else if(c=='-'){
                i++;
            }else{
                int start = i;
                while(i<line.size() && (line[i]>='0' && line[i]<='9')){
                    i++;
                }
                if(i>start){
                    int cost = stoi(line.substr(start, i-start));
                    Virtual_network[id1][id2] = cost;
                    Virtual_network[id2][id1] = cost;
                    i--;
                }else continue;
            }
            id2++;
        }
        Virtual_network[num_nodes-1][num_nodes-1] = 0;
    }
    return num_nodes;
}

int parse_config_file(string filename, vector<string> &valid_lines){

    std::ifstream file(filename);

    if(!file.is_open()){
        cout<<"Error opening config file\n";
        return -1;
    }

    string line;

    while(getline(file, line)){
        size_t pos = line.find_first_not_of(" \t\r\n");
        if(pos==string::npos) continue;
        if(line[pos] == '#') continue;
        valid_lines.push_back(line.substr(pos));
    }
    return 0;
}

int main( int argc, char *argv[] ){ 

    if(argc!=2){
        cout<<"Usage: " << argv[0] << "<config.txt>\n";
        return 0;
    }

    vector<string> valid_lines;

    if(parse_config_file(argv[1], valid_lines) == -1){
        return 0;
    }
    int num_nodes = build_virtual_network(valid_lines);

    int id = 0; // start listening with A

    int sockfd, new_fd[num_nodes];
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    socklen_t sin_size; 
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if( sockfd == -1){
        cout<<"Error creating socket\n";
        return 0;
    }else{
        cout << "Socket created successfully\n";
    }
    my_addr.sin_family = AF_INET;        
    my_addr.sin_port = htons(MYPORT);
    my_addr.sin_addr.s_addr = inet_addr(MY_IP);
    memset(&(my_addr.sin_zero), '\0', 8);
    // cout << inet_ntoa(my_addr.sin_addr);

    if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1){
        cout<<"Error on bind\n";
        close(sockfd);
        return 0;
    }else{
        cout << "bind succeeded\n";
    }

    if(listen(sockfd, BACKLOG) == -1){
        cout<<"Error on listen\n";
        close(sockfd);
        return 0;
    }else{
        cout << "Ha hum sun rahe hai...\n";
    }

    while(id<num_nodes){
        cout << "Yo! node " << id_to_char(id) << " :) ...\n";
        sin_size = sizeof(struct sockaddr_in); 
        new_fd[id] = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

        if(new_fd[id] == -1){
            cout<<"Error on accept\n";
            close(sockfd);
            return 0;
        }else{
            cout << "Let's stay connected, node " << id_to_char(id) << " :D\n";
        }
        id++;
    }
    
    id = 0;
    while(id<num_nodes){
        uint8_t connect_msg[6]; // 4 bytes for ip, 2 bytes for port
        if(!recv_exact(new_fd[id], connect_msg, sizeof(connect_msg))){
            cout << "Error receiving connection message from node " << id_to_char(id) << "\n";
            close(sockfd);
            return 0;
        }
        uint32_t ip = (static_cast<uint32_t>(connect_msg[0]) << 24) | (static_cast<uint32_t>(connect_msg[1]) << 16) | (static_cast<uint32_t>(connect_msg[2]) << 8)  | static_cast<uint32_t>(connect_msg[3]);
        uint16_t port = (static_cast<uint16_t>(connect_msg[4]) << 8) | static_cast<uint16_t>(connect_msg[5]);
        Node_info[id] = {ip, port};
        id++;
    }
    
    if(!send_link_state_msgs(num_nodes, new_fd)){
        cout << "Error sending link state messages\n";
        close(sockfd);
        for(int i=0; i<num_nodes; i++){
            close(new_fd[i]);
        }  
        return 0;
    }

    while(true){
        sleep(1);
        valid_lines.clear();
        if(parse_config_file(argv[1], valid_lines) == -1){
            cout << "Error parsing config file, retrying...\n";
            continue;
        }
        auto old_virtual_network = Virtual_network;
        if(build_virtual_network(valid_lines) != num_nodes){
            cout << "Number of nodes changed, ignoring this configuration...\n";
            Virtual_network = old_virtual_network;
            continue;
        }
        if(old_virtual_network != Virtual_network){
            cout << "Pados me badlaav, khabar failaani padegi :))...\n";
            if(!send_link_state_msgs(num_nodes, new_fd)){
                break;
            }
        }
    }
    

    close(sockfd);
    for(int i=0; i<num_nodes; i++){
        close(new_fd[i]);
    }  
    return 0;
}