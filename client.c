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

struct Messages{
    unsigned int MessageVrsn; //9 bit version field, protocol version is 3
    unsigned int MessageType; //7 bit type field, indicates SBCP message type
    unsigned int MessageLength; //2 bit length field, indicates SBCP message length
    struct Attr Payload; //contains 0 or more SBCP attributes
};

struct Attr{
    unsigned int AttrType; //2 bytes type field, indicates SBCP attribute type
    unsigned int AttrLength; //2 byte length field, indicates SBCP attribute length
    char        Payload[512]; //has the attribute payload
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
    setInitialClient(Mess_to);
    strcpy(Mess_to->Payload.Payload, argv[1]);
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
                    strcpy(Mess_to->Payload.Payload, message);
                    setMessage(Mess_to);
                    if(write(sock_fd, Mess_to, size)==-1){
                        perror("write error");
                    }
                }

                if(i==sock_fd){
                    Mess_from = malloc(size);
                    readingbytes = read(sock_fd, Mess_from, sizeof(struct Messages));
                    //print statement
                    printf("%s", Mess_from->Payload.Payload);

                    free(Mess_from);
                    if(Mess_from->MessageType==5){
                        if(Mess_from->Payload.AttrType==1){
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

void setInitialClient(struct Messages *mess){
    //setting username and possible length of username 
    mess->Payload.AttrType=2;
    mess->Payload.AttrLength=25;

    //indicates client wishing to join session
    mess->MessageVrsn=3;
    mess->MessageType=2;
    mess->MessageLength=24;
}

void setMessage(struct Messages *mess){
    //indicates messages being set
    mess->Payload.AttrType=4;
    mess->Payload.AttrLength=524;

    //indicates client sending something to the server
    mess->MessageVrsn=3;
    mess->MessageType=4;
    mess->MessageLength=520;
}
