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
    unsigned int AttrType; //2 bytes type field, indicates SBCP attribute type
    unsigned int AttrLength; //2 byte length field, indicates SBCP attribute length
    char        Payload[512]; //has the attribute payload
};

struct Messages{
    unsigned int MessageVrsn; //9 bit version field, protocol version is 3
    unsigned int MessageType; //7 bit type field, indicates SBCP message type
    unsigned int MessageLength; //2 bit length field, indicates SBCP message length
    struct Attr Payload; //contains 0 or more SBCP attributes
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
    //char who_is_online[500]={0};
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
                            perror("cannot accept new connection");
                        }else{
                            FD_SET(new_com, &master_fd);
                            //reset max
                            if(new_com > max_fd){
                                max_fd = new_com;
                            }
                            client_cnt++;
                        }
                        
                    }
                   

                }else{
                    //ADD HERE
                    Mess_from = malloc(size);
                    return_val = read(i, Mess_from, size);
                    
                    //print message
                    if(return_val>0){
                        
                        //first case: message type is 2
                        //cleint wants to join the chat session
                        if(Mess_from->MessageType ==2){
                            printf("Client trying to join chat session\n");

                            //attribute type 2 indicates nickname 
                            if(Mess_from->Payload.AttrType == 2){

                                //check that max number has not been reached
                                if(client_cnt > max_num_clients+1){
                                    Mess_to = malloc(size);
                                    Mess_to->Payload.AttrType=1;
                                    Mess_to->MessageType=5;
                                    strcpy(Mess_to->Payload.Payload, "maximum number reached, so you cannot join\n");
                                    write(i, Mess_to, size);
                                    close(i);
                                    FD_CLR(i, &master_fd);
                                    client_cnt--;
                                }else{
                                    int name_not_being_used = CheckNameAvailability(max_fd, Mess_from, Mess_to, names, size); 
                                    if(name_not_being_used != 1){
                                        write(i, Mess_to, size);
                                        client_cnt--;
                                        close(i);
                                        FD_CLR(i, &master_fd);
                                        return 0;
                                    }else{
                                        //print message
                                        Mess_to->MessageType=7;
                                        sprintf(names[i], "%s", Mess_from->Payload.Payload);

                                        Mess_to = malloc(size);
                                        PrintCurrentClients(client_cnt, max_fd, i, names, sock_fd, Mess_to, size);
                                    }
                                }
                            }
                            
                        }

                        //type 4 indicates client sending something to server. A chat text is being sent
                        if(Mess_from->MessageType==4){
                            //attribute type 4 indicates message itself
                            if(Mess_from->Payload.AttrType==4){
                                //print message
                                sprintf(Mess_from, "%s - %s", names[i], Mess_from->Payload.Payload);
                                for(c=0; c<max_fd; c++){
                                    if(FD_ISSET(c, &master_fd)){
                                        if(c!=i){
                                            if(c!=sock_fd){
                                                Mess_to = malloc(size);
                                                strcpy(Mess_to->Payload.Payload, Mess_from);
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
                                            strcpy(Mess_to->Payload.Payload, who_has_left);

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

void PrintCurrentClients(int client_cnt, int max_fd, int i, char names[100][50], int sock_fd, struct Messages *Mess_to, int size){
    char who_is_online[500];
    int n;
    
    if(client_cnt!=1){
        //others are online so update array with different welcome message
        strcpy(who_is_online, "Howdy! There are ");
        char buffer[2];
        sprintf(buffer, "%d", client_cnt-1);
        strcat(who_is_online, buffer);
        strcat(who_is_online, " others online! Here is who is on: \n");
       // strcpy(who_is_online, "Howdy! There are others online. Here is who is on: \n");
    
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
    
    }else{
        if(client_cnt==1){
        //no one else is online so update the array
        strcpy(who_is_online, "Howdy! No one else is online\n");
        }
    }

    //add who_is_online to the payload of the message struct
    strcpy(Mess_to->Payload.Payload, who_is_online);
    printf("Message sent to the client was: %s", Mess_to->Payload.Payload);
    write(i, Mess_to, size);

}

int CheckNameAvailability(int max_fd, struct Messages *Mess_from, struct Messages *Mess_to, char names[100][50], int size){
    int m;
    for(m = 0; m <= max_fd; m++){
        if(strcmp(&Mess_from->Payload.Payload, names[m])==0){
            Mess_to = malloc(size);
            strcpy(&Mess_to->Payload.Payload, "This name is already being used. Choose a different one");
            return 0;
        }
    }
    return 1;
}