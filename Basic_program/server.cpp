#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 

using namespace std;

#define MYPORT 3490
#define BACKLOG 10

main(){ 

    int sockfd, new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    int sin_size; 
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

    char *msg = "hello!";
    int len, bytes_sent; 
    len = strlen(msg); 
    bytes_sent = send(new_fd, msg, len 0);

    char buf[100];
    int bytes_recieved = recv(new_fd, buf, sizeof(buf)-1, 0);
    buf[bytes_recieved] = '\0';
    cout << "Server recieved : " << buf << '\n';

    close(sockfd);
    close(new_fd);
}