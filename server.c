#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
//#include <utils.h>

/*Structs based of slides from recitation*/
struct Attr{
    unsigned int Type;
    unsigned int Length;
    char        Payload[512];
};

struct Messages{
    unsigned int Vrsn;
    unsigned int Type;
    unsigned int Length;
    struct Attr Attr;
};

int main(int argc, char *argv[]){
    
    int client_cnt, max_num_clients;
    int max_fd, sock_fd, listen_fd, new_com;
    int portnum;
    char *c, *d;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    //check for error
    if (sock_fd < 0){
        perror("socket error");
    }

    portnum = strtol(argv[1], &c, 10);
    max_num_clients = strtol(argv[2], &d, 10);

    struct Messages *Mess_to;
    struct Messages *Mess_from;

    fd_set master_fd;
    fd_set temp_fd;
    int return_val;
    char names[100][16]={0};
    char who_is_online[500]={0};
    char who_has_left[500]={0};

    FD_ZERO(&master_fd);
    FD_ZERO(&temp_fd);

    struct sockaddr_in serveraddress;
    serveraddress.sin_addr.s_addr = htons(INADDR_ANY);
    serveraddress.sin_port = htons(portnum);
    serveraddress.sin_family = AF_INET;

    int binding = bind(sock_fd, (struct sockaddr *) &serveraddress, sizeof(serveraddress));
    //check for error
    if (binding <0){
        perror("binding error");
    }

    listen_fd = listen(sock_fd, 10);
    //check for error
    if(listen_fd < 0){
        perror("listening error");
    }

    FD_SET(sock_fd, &master_fd);
    max_fd = sock_fd;

    client_cnt = 0;

    while(1){

        //copy
        temp_fd = master_fd;

        if(select(max_fd+1, &temp_fd, NULL, NULL, NULL)==-1){
            perror("server select error");
            exit(1);
        }

        for(int i=0; i<=max_fd; i++){

            if(FD_ISSET(i, &temp_fd)){

                if(i==sock_fd){
                    //ADD HERE
                    if(client_cnt < max_num_clients){
                        new_com = accept(sock_fd, (struct sockaddr*)NULL, NULL);

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
                             printf("print server");
                        }
                        
                    }
                   

                }else{
                    //ADD HERE
                    Mess_from = malloc(sizeof(struct Messages));
                    printf("From Server: prepared for message from client");
                    return_val = read(i, Mess_from, sizeof(struct Messages));
                    //print message
                    if(return_val>0){
                        
                        if(Mess_from->Type ==2){
                            if(Mess_from->Attr.Type == 2){
                                if(client_cnt <= max_num_clients -1){
                                    int f = 1;
                                    for(int m = 4; m <= max_fd; m++){
                                        if(strcmp(Mess_from->Attr.Payload, names[m])==0){
                                            f=0;
                                            Mess_to = malloc(sizeof(struct Messages));
                                            //Mess_to->Attr.Type=1;
                                            //Mess_to_Clientt->Type=5;
                                            strcpy(Mess_to->Attr.Payload, "This name is already being used. Choose a different one");
                                            write(i, Mess_to, sizeof(struct Messages));
                                            client_cnt--;
                                            close(i);
                                            FD_CLR(i, &master_fd);
                                            break;
                                        }
                                    }
                                    if(f==1){
                                        //print message
                                        sprintf(names[i], "%s", Mess_from->Attr.Payload);

                                        Mess_to = malloc(sizeof(struct Messages));
                                        //Mess_to->Type=7;
                                        if(client_cnt==1){
                                            //no one else is online so update the array
                                            strcpy(who_is_online, "Howdy! No one else is online");
                                        }else{
                                            //others are online so update array with different welcome message
                                            strcpy(who_is_online, "Howdy! There are others online");
                                        }
                                        for(int n = 4; n<max_fd; n++){
                                            if(n!=i){
                                                if(n!=sock_fd){
                                                    if(client_cnt != 1){
                                                        //update who_is_online with users that are online
                                                        strcat(who_is_online, names[n]);
                                                        
                                                        //add a space to separate names
                                                        strcat(who_is_online, " ");
                                                    }
                                                }
                                            }
                                        }
                                        //add who_is_online to the payload of the message struct
                                        strcpy(Mess_to->Attr.Payload, who_is_online);
                                        printf("Message sent to the client was: %s", Mess_to->Attr.Payload);
                                        write(i, Mess_to, sizeof(struct Messages));
                                    }
                                }else{
                                    Mess_to = malloc(sizeof(struct Messages));
                                    //Mess_to->Type = 5;
                                    //Mess_to->Attr.Type=1;
                                    strcpy(Mess_to->Attr.Payload, "maximum number reached");
                                    write(i, Mess_to, sizeof(struct Messages));

                                    client_cnt--;
                                    close(i);
                                    FD_CLR(i, &master_fd);
                                    
                                }
                            }
                            
                        }
                        if(Mess_from->Type==4){
                            if(Mess_from->Attr.Type==4){
                                //print message
                                sprintf(Mess_from, "%s - %s", names[i], Mess_from->Attr.Payload);
                                for(int c=0; c<max_fd; c++){
                                    if(FD_ISSET(c, &master_fd)){
                                        if(c!=i){
                                            if(c!=sock_fd){
                                                Mess_to = malloc(sizeof(struct Messages));
                                                //Mess_to->Type = 3;
                                                //Mess_to->Attr.Type=4;
                                                strcpy(Mess_to->Attr.Payload, Mess_from);
                                                write(c, Mess_to, sizeof(struct Messages));
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
                            printf("The client side has ended");
                            names[i][0]='\0';

                            for(int l=0; l<=max_fd; l++){
                                if(FD_ISSET(l, &master_fd)){
                                    if(l!=i){
                                        if(l!=sock_fd){
                                            Mess_to = malloc(sizeof(struct Messages));
                                            strcpy(Mess_to->Attr.Payload, who_has_left);

                                            if(write(l, Mess_to, sizeof(struct Messages))==-1){
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

void new_connection(int client_cnt, int max_num_clients, int sock_fd, int max_fd, int master_fd){
    if(client_cnt < max_num_clients){
        int new_com = accept(sock_fd, (struct sockaddr*)NULL, NULL);

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
                printf("print server");
        }
        
    }
}