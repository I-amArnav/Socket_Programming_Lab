#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 

using namespace std;

#define DEST_IP "172.20.80.1" 
#define DEST_PORT 3490
int main(){

    int sockfd; 
    struct sockaddr_in dest_addr; // will hold the destination addr 
    
    sockfd = socket(PF_INET, SOCK_STREAM, 0); 
    dest_addr.sin_family = AF_INET; // host byte order 
    dest_addr.sin_port = htons(DEST_PORT); // network byte order 
    dest_addr.sin_addr.s_addr = inet_addr(DEST_IP); 
    memset(&(dest_addr.sin_zero), '\0', 8); // zero the rest of the struct 
    
    if(connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr))==-1){
        cout << "Error in connecting to server\n";
    }
    
    char buffer[100];
    int bytes_read = recv(sockfd, buffer, sizeof(buffer), 0);
    if(bytes_read < 0){
        cout << "Error in receiving data from server\n";
    }else if(bytes_read == 0){
        cout << "Server closed the connection\n";
    }else{
        cout << "Client recieved : " << buffer << '\n';
    }  

    char *msg = "Hi!";
    int len, bytes_sent; 
    len = strlen(msg); 
    bytes_sent = send(sockfd, msg, len, 0);

    close(sockfd);
}