#include <iostream>
#include <unistd.h>
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <ifstream>
#include <string>
#include <vector>

using namespace std;

#define MYPORT 5000
#define BACKLOG 10

vector<vector<int>> Virtual_network(26, vector<int>(26, -1));

char id_to_char(int id){
    return 'A' + id;
}

int char_to_id(char c){
    return c - 'A';
}

void build_virtual_network(vector<string> &valid_lines){
    
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
                while(i<line.size() && (line[i]>'0' && line[i]<='9')){
                    i++;
                }
                int cost = stoi(line.substr(start, i-start));
                Virtual_network[id1][id2] = cost;
                Virtual_network[id2][id1] = cost;
                i--;
            }
        }
        id2++;
    }
}

int parse_config_file(string filename, vector<string> &valid_lines){

    ifstream file(filename);

    if(!file.is_open()){
        cout<<"Error opening config file\n";
        return -1;
    }

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
    build_virtual_network(valid_lines);


    int id = 0;
    



    int sockfd, new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    socklen_t sin_size; 
    sockfd = socket(PF_INET, SOCK_STREAM, 0);  
    my_addr.sin_family = AF_INET;        

    my_addr.sin_port = htons(MYPORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero), '\0', 8);

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

    sin_size = sizeof(struct sockaddr_in); 
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

    if(new_fd == -1){
        cout<<"Error on accept\n";
        close(sockfd);
        return 0;
    }

    const char *msg = "hello!";
    int len, bytes_sent; 
    len = strlen(msg); 
    bytes_sent = send(new_fd, msg, len, 0);

    char buf[100];
    int bytes_recieved = recv(new_fd, buf, sizeof(buf)-1, 0);
    buf[bytes_recieved] = '\0';
    cout << "Server recieved : " << buf << '\n';

    close(sockfd);
    close(new_fd);
    return 0;
}