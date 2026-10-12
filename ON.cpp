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
#define MY_IP "172.20.87.112"

struct CONNECT_MSG{
    uint32_t ip;
    uint16_t port;
};

struct LINK_STATE_MSG{
    uint8_t alphabet;
    uint32_t cost;
};

vector<vector<int>> Virtual_network(26, vector<int>(26, -1));
vector<pair<uint32_t, uint16_t>> Node_info(26); // ip, port

char id_to_char(int id){
    return 'A' + id;
}

int char_to_id(char c){
    return c - 'A';
}

int build_virtual_network(vector<string> &valid_lines){
    
    int num_nodes = valid_lines.size()+1;

    for(int id1 =0; id1<num_nodes; id1++){
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
        }
        id2++;
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
    my_addr.sin_family = AF_INET;        
    my_addr.sin_port = htons(MYPORT);
    my_addr.sin_addr.s_addr = inet_addr(MY_IP);
    memset(&(my_addr.sin_zero), '\0', 8);
    // cout << inet_ntoa(my_addr.sin_addr);

    if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1){
        cout<<"Error on bind\n";
        close(sockfd);
        return 0;
    } 

    if(listen(sockfd, BACKLOG) == -1){
        cout<<"Error on listen\n";
        close(sockfd);
        return 0;
    }

    while(id<num_nodes){
        cout << "Waiting for connection from node " << id_to_char(id) << "...\n";
        sin_size = sizeof(struct sockaddr_in); 
        new_fd[id] = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

        if(new_fd[id] == -1){
            cout<<"Error on accept\n";
            close(sockfd);
            return 0;
        }
        id++;
    }
    
    while(id<num_nodes){
        CONNECT_MSG connect_msg;
        int bytes_recieved = recv(new_fd[id], &connect_msg, sizeof(connect_msg), 0);
        if(bytes_recieved < 0){
            cout << "Error in receiving data from node " << id_to_char(id) << "\n";
        }else if(bytes_recieved == 0){
            cout << "Node " << id_to_char(id) << " closed the connection\n";
        }else{
            Node_info[id] = {connect_msg.ip, connect_msg.port};
        }
        id++;
    }



    // const char *msg = "hello!";
    // int len, bytes_sent; 
    // len = strlen(msg); 
    // bytes_sent = send(new_fd[id], msg, len, 0);

    

    close(sockfd);
    for(int i=0; i<num_nodes; i++)
        close(new_fd[i]);
    return 0;
}