#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

/*Structs based of slides from recitation*/
struct Attr{
    unsigned int AttrType;
    unsigned int AttrLength;
    char        Payload[512];
};

struct Messages{
    unsigned int MessageVrsn;
    unsigned int MessageType;
    unsigned int MessageLength;
    struct Attr Attr;
};

int main(int argc, char *argv[]){

    struct Messages *Mess_to;
    struct Messages *Mess_from;
    char message[512];
    int len_of_ip, readingbytes;
    size_t size = sizeof(struct Messages);

    int sock_fd, connection;
    char *c;
    int portNum = strtol(argv[3], &c, 10);
    int i;

    fd_set master_fd;
    fd_set temp_fd;

    struct sockaddr_in serveraddress;
    sock_fd=socket(AF_INET, SOCK_STREAM,0);
    //check for error
    if(sock_fd<0){
        perror("socket error");
    }

    bzero(&serveraddress, sizeof serveraddress);
    serveraddress.sin_family=AF_INET;
    serveraddress.sin_port=htons(portNum);
    int inet = inet_pton(AF_INET, argv[2], &(serveraddress.sin_addr));
    //check for error
    if(inet <= 0){
        perror("inet error");
    }

    connection = connect(sock_fd, (struct sockaddr *)&serveraddress, sizeof(serveraddress));
    //check for error
    if(connection < 0){
        perror("connection error");
    }

    //print message
    printf("From the client: username is %s\n", argv[1]);

    Mess_to = malloc(size);
    setInitialMessage(Mess_to);
    strcpy(Mess_to->Attr.Payload, argv[1]);
    //print message
    printf("From the client: joining chat\n");

    if(write(sock_fd, Mess_to, size)==-1){
        perror("write error");
    }

    FD_SET(0, &temp_fd);
    FD_SET(sock_fd, &temp_fd);

    while(1){
        if(select(sock_fd+1, &temp_fd, NULL, NULL, NULL)==-1){
            perror("select error");
            exit(6);
        }

        for(i=0; i<=sock_fd; i++){
            if(FD_ISSET(i, &temp_fd)){
                if(i==0){
                    bzero(message, 512);
                    fgets(message, 512, stdin);
                    len_of_ip = strlen(message)-1;
                    
                    Mess_to = malloc(size);
                    strcpy(Mess_to->Attr.Payload, message);
                    setMessage(Mess_to);
                    if(write(sock_fd, Mess_to, size)==-1){
                        perror("write error");
                    }
                }

                if(i==sock_fd){
                    Mess_from = malloc(size);
                    readingbytes = read(sock_fd, Mess_from, sizeof(struct Messages));
                    //print statement
                    printf("%s", Mess_from->Attr.Payload);

                    free(Mess_from);
                    if(Mess_from->MessageType==5){
                        if(Mess_from->Attr.AttrType==1){
                            exit(7);
                        }
                    }
                }
            }
            FD_SET(0,&temp_fd);
            FD_SET(sock_fd, &temp_fd);

        }
    }
    close(sock_fd);
    return 0;
}

void setInitialMessage(struct Messages *mess){
    mess->Attr.AttrType=2;
    mess->Attr.AttrLength=20;
    mess->MessageVrsn=3;
    mess->MessageType=2;
    mess->MessageLength=24;
}

void setMessage(struct Messages *mess){
    mess->Attr.AttrType=4;
    mess->Attr.AttrLength=524;
    mess->MessageVrsn=3;
    mess->MessageType=4;
    mess->MessageLength=520;
}
