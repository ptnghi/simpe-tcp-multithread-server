#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include<pthread.h>
#include<time.h>
#define PORT 8888

void configure_socket( struct sockaddr_in *sock);
void send_result(int sock, char filename[]);
int intcmp(const void *a, const void *b);
void trim_result(char msg[]);
void save_ball(int num, char filename[]);

int ball_array[1000], count = 0;

int main(int argc, char const *argv[])
{
    FILE *fp;
    int clientSocket;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    char got_msg[20], pending_msg[20];
    char filename[50] = "result";
    int byte_received = 0;

    strcat(filename, argv[1]);

    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    memset(ball_array, 0, 1000*sizeof(int));

    configure_socket(&serverAddr);
    addr_size = sizeof(serverAddr);

    connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);
    strcpy(pending_msg,"get");

    while(1){
        if ( send(clientSocket, "get", strlen(pending_msg), 0) < 0){
            printf("Send failed\n");
        }
        printf("Process %s sent: get\n", argv[1] );
        byte_received = recv(clientSocket, got_msg, 20, 0);
        if (byte_received < 0){
            printf("Process %s received failed\n", argv[1]);
        }
        if(strlen(got_msg) > byte_received){
            got_msg[byte_received] = '\0';
        }
        if (strcmp(got_msg,"-1") == 0){
            break;
        }
        int num = atoi(got_msg);
        save_ball(num, filename);
    }
    send_result(clientSocket, filename);

    
    char *server_msg = malloc(4000 * sizeof(char));
    char *tmp_buffer =malloc(1024);
    int len = 0; 
    memset(server_msg, '\0', 4000);
    memset(tmp_buffer, '\0', 1024);
    while ((len = recv(clientSocket, tmp_buffer, 1024, 0)) > 0){

        strcat(server_msg,tmp_buffer);
        if (strlen(tmp_buffer) < 1024){
            printf("break\n");
            break;
        }
        memset(tmp_buffer, '\0', 1024);
    }
    strcpy(filename,"ranking");
    strcat(filename,argv[1]);
    fp = fopen(filename, "w");
    fprintf(fp,"%s",server_msg);
    if ( send(clientSocket, "finished", 8, 0) < 0){
        printf("Send failed\n");
    }
    printf("Process %s sent finished\n", argv[1]);  
    close(clientSocket);
    return 0;
}

void send_result(int sock, char filename[]){

    char * pending_msg = malloc(4000);
    strcpy(pending_msg, "result ");
    char char_num[10];
    for (int i = 0 ; i< count; i++){
        sprintf(char_num,"%d ", ball_array[i]);
        strcat(pending_msg, char_num);
    }
    printf("%s\n", pending_msg);
    int l = strlen(pending_msg);
    int offset = 0;
    while (send(sock, pending_msg + offset, 1024, 0) < l){
        l-=1024;
        offset+=1024;   
    }
    printf("Sent\n" );
}


void configure_socket(struct sockaddr_in *sock){
    sock->sin_family = AF_INET;
    sock->sin_port = htons(PORT);
    sock->sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(sock->sin_zero, '\0', sizeof(sock->sin_zero));

}

int intcmp(const void *a, const void *b){
    return ( *(int *)a - *(int*)b);
}

void trim_result(char msg[]){
    if (msg[0] ==  '-'){
        msg[2] = '\0';
    }
}

void save_ball(int num,  char filename[]){
    FILE *fp;
    fp = fopen(filename, "w");
    ball_array[count++] = num;
    printf("%d\n", num);
    qsort(ball_array, count, sizeof(int), intcmp);
    for (int i = 0; i < count; i++){
        fprintf(fp, "%d\n", ball_array[i]);
    }
    fclose(fp);
}