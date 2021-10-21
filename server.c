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
    char         Payload[512];
};

struct Messages{
    unsigned int MessageVrsn;
    unsigned int MessageType;
    unsigned int MessageLength;
    struct Attr Attr;
};


int main(int argc, char *argv[]){
    
    //variables for counting clients, and max number as well as fd's
    int client_cnt, max_num_clients;
    int ip_addr;
    int max_fd, sock_fd, listen_fd, new_com;
    int portnum, i, l, m, n, c;
    char *char1, *char2;
    size_t size = sizeof(struct Messages);

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    //check for error
    if (sock_fd < 0){
        perror("socket error");
    }

    //import port number and number of clients from stdin
    portnum = strtol(argv[2], &char1, 10);
    max_num_clients = strtol(argv[3], &char2, 10);

    //set up server address as it was set up in MP1
    struct sockaddr_in serveraddress;
    serveraddress.sin_addr.s_addr = htons(INADDR_ANY);
    serveraddress.sin_port = htons(portnum);
    serveraddress.sin_family = AF_INET;

    //set initial count of clients to 0
    client_cnt = 0;

    //create SBCP messages for both directions
    struct Messages *Mess_to;
    struct Messages *Mess_from;

    //create fd's
    fd_set master_fd;
    fd_set temp_fd;
    int return_val;

    //arrays to keep track of usernames and who is active on the chat
    char names[100][50]={0};
    char who_is_online[500]={0};
    char who_has_left[500]={0};

    FD_ZERO(&master_fd);
    FD_ZERO(&temp_fd);

    //binding section
    int binding = bind(sock_fd, (struct sockaddr *) &serveraddress, sizeof(serveraddress));
    //check for error
    if (binding <0){
        perror("binding error");
    }

    //listening section
    listen_fd = listen(sock_fd, 10);
    //check for error
    if(listen_fd < 0){
        perror("listening error");
    }

    FD_SET(sock_fd, &master_fd);
    max_fd = sock_fd;

    //main while loop
    while(1){

        //copy master into temporary
        temp_fd = master_fd;

        if(select(max_fd+1, &temp_fd, NULL, NULL, NULL)==-1){
            perror("server select error");
            exit(1);
        }

        for(i=0; i<=max_fd; i++){

            if(FD_ISSET(i, &temp_fd)){

                if(i==sock_fd){
                    //ADD HERE
                    if(client_cnt < max_num_clients){
                        new_com = accept(sock_fd, (struct sockaddr*)NULL, NULL);
                        printf("From server: new connection\n");
                        //check for error
                        if(new_com == -1){
                            perror("");
                        }else{
                            FD_SET(new_com, &master_fd);
                            //reset max
                            if(new_com > max_fd){
                                max_fd = new_com;
                            }
                            client_cnt++;
                             printf("print server\n");
                        }
                        
                    }
                   

                }else{
                    //ADD HERE
                    Mess_from = malloc(size);
                    printf("From Server: prepared for message from client\n");
                    return_val = read(i, Mess_from, size);
                    //print message
                    if(return_val>0){
                        
                        if(Mess_from->MessageType ==2){
                            if(Mess_from->Attr.AttrType == 2){
                                if(client_cnt <= max_num_clients -1){
                                    int f = 1;
                                    for(m = 4; m <= max_fd; m++){
                                        if(strcmp(Mess_from->Attr.Payload, names[m])==0){
                                            f=0;
                                            Mess_to = malloc(size);
                                            strcpy(Mess_to->Attr.Payload, "This name is already being used. Choose a different one");
                                            write(i, Mess_to, size);
                                            client_cnt--;
                                            close(i);
                                            FD_CLR(i, &master_fd);
                                            break;
                                        }
                                    }
                                    if(f==1){
                                        //print message
                                        sprintf(names[i], "%s", Mess_from->Attr.Payload);

                                        Mess_to = malloc(size);
                                        //Mess_to->AttrType=7;
                                        if(client_cnt==1){
                                            //no one else is online so update the array
                                            strcpy(who_is_online, "Howdy! No one else is online\n");
                                        }else{
                                            //others are online so update array with different welcome message
                                            strcpy(who_is_online, "Howdy! There are others online. Here is who is on: \n");
                                        }
                                        for(n = 4; n<max_fd; n++){
                                            if(n!=i){
                                                if(n!=sock_fd){
                                                    if(client_cnt != 1){
                                                        //update who_is_online with users that are online
                                                        strcat(who_is_online, names[n]);
                                                        
                                                        //add a space to separate names
                                                        strcat(who_is_online, "\n");
                                                    }
                                                }
                                            }
                                        }
                                        //add who_is_online to the payload of the message struct
                                        strcpy(Mess_to->Attr.Payload, who_is_online);
                                        printf("Message sent to the client was: %s", Mess_to->Attr.Payload);
                                        write(i, Mess_to, size);
                                    }
                                }else{
                                    Mess_to = malloc(size);
                                    strcpy(Mess_to->Attr.Payload, "maximum number reached\n");
                                    write(i, Mess_to, size);

                                    client_cnt--;
                                    close(i);
                                    FD_CLR(i, &master_fd);
                                    
                                }
                            }
                            
                        }
                        if(Mess_from->MessageType==4){
                            if(Mess_from->Attr.AttrType==4){
                                //print message
                                sprintf(Mess_from, "%s - %s", names[i], Mess_from->Attr.Payload);
                                for(c=0; c<max_fd; c++){
                                    if(FD_ISSET(c, &master_fd)){
                                        if(c!=i){
                                            if(c!=sock_fd){
                                                Mess_to = malloc(size);
                                                strcpy(Mess_to->Attr.Payload, Mess_from);
                                                write(c, Mess_to, size);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        free(Mess_from);
                    }else{
                        if(return_val==0){
                            //print message
                            sprintf(who_has_left, "Left: %s", names[i]);
                            printf("The client side has ended\n");
                            names[i][0]='\0';

                            for(l=0; l<=max_fd; l++){
                                if(FD_ISSET(l, &master_fd)){
                                    if(l!=i){
                                        if(l!=sock_fd){
                                            Mess_to = malloc(size);
                                            strcpy(Mess_to->Attr.Payload, who_has_left);

                                            if(write(l, Mess_to, size)==-1){
                                                perror("write error");
                                            }
                                        }
                                    }
                                }
                            }
                        }else{
                            perror("read error");
                        }
                        client_cnt--;
                        close(i);
                        FD_CLR(i, &master_fd);
                    }
                }
            }
        }
        
    }
}
