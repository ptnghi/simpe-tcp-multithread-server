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
#include<errno.h>

int *ball_pool;
int number_of_ball, ball_pointer;
int no_thread;
int finished = 0;
int isFinished = 1;
int t_count =0;
int closed_count = 0;

typedef struct res_nodes {
    int sum;
    int source_id;
} res_node;

res_node *res_array;
//char msg[200];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER, countlock = PTHREAD_MUTEX_INITIALIZER, finishlock = PTHREAD_MUTEX_INITIALIZER, isFinishlock = PTHREAD_MUTEX_INITIALIZER;
void *ballgiver(void *arg);
void initpool();
void trimstr(char * inStr);
void *finishCounter(void *arg);
int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }
void sort( res_node * arr, int n);

int main(int argc, char const *argv[])
{
    initpool();
    int serverSocket;
    int *new_socket_fd;
    struct sockaddr_in serverAddr;


    serverSocket = socket(PF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    serverAddr.sin_addr.s_addr= inet_addr("127.0.0.1");

    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

    int flags = guard(fcntl(serverSocket, F_GETFL), "could not get flags on TCP listening socket");
    guard(fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK), "could not set TCP listening socket to be non-blocking");

   if ( bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != 0){
       printf("Bind failed");
       printf("Error: %s\n", strerror(errno));
       exit(1);
   }

    if (listen(serverSocket, 10)==0){
        printf("Listening\n");
    } else {
        printf("Error\n");
    }
    pthread_t tid[60];
    pthread_t watcher_id;

    if (pthread_create(&watcher_id, NULL, finishCounter, NULL) != 0){
        printf("Create counter failed\n");
    } else {
        printf("Create counter success\n");
    }

    while (1){
        new_socket_fd = malloc(sizeof(int));
        *new_socket_fd = accept(serverSocket, NULL , 0);
        
        if (*new_socket_fd == -1) {
            if(errno == EWOULDBLOCK){

            } else {
                perror("Accept error");
                exit(1);
            }
        } else {
            if (pthread_create(&tid[t_count], NULL, ballgiver, new_socket_fd) != 0){
                printf("Create thread failed\n");
            }

            t_count++;
        }
        
        if (isFinished == 0){

            pthread_join(watcher_id, NULL);
            break;
        }
    }
    printf("Program end\n");
    free(ball_pool);
    return 0;
}

void *ballgiver(void *arg){
    int newSocket = *(int *)arg;
    char *client_msg = malloc(4000 * sizeof(char));
    char *tmp_buffer =malloc(1024);
    int len;
    int id;
    pthread_mutex_lock(&countlock);
        no_thread++;
        id = no_thread;
    pthread_mutex_unlock(&countlock);
    printf("Thread %d created\n", id);
    char pending_msg[5];
    int num;
    while (1){
        memset(client_msg, '\0', 4000);
        memset(tmp_buffer, '\0', 1024);
        while ((len = recv(newSocket, tmp_buffer, 1024, 0)) > 0){
            printf("Received: %d, leng of buf: %li\n",len,strlen(tmp_buffer));
            strcat(client_msg,tmp_buffer);
            if (strlen(tmp_buffer) < 1024){
                printf("break\n");
                break;
            }
            memset(tmp_buffer, '\0', 1024);
        }
        trimstr(client_msg);
        char *token;
        char code[200];
        memset(code,'\0',200);
        token = strtok(client_msg, " ");
        strcpy(code,token);
        if (strcmp(code, "get") == 0){
            pthread_mutex_lock(&lock);
            if (ball_pointer >= number_of_ball)
                num = -1;
            else {
                num = ball_pool[ball_pointer];
                ball_pointer++;
            }
            pthread_mutex_unlock(&lock);
            sprintf(pending_msg, "%d", num);
            send(newSocket, pending_msg, strlen(pending_msg), 0);
            printf("Sent %s to %d\n", pending_msg, id);
        }
        if(strcmp(code, "result") == 0){
            int temp, sum = 0;
            while (token != NULL){  
                token = strtok(NULL, " ");
                if (token != NULL){
                    temp = atoi(token);
                    sum += temp;
                }
            }
            pthread_mutex_lock(&finishlock);
                if (finished == 0){
                    res_array = malloc(t_count * sizeof(res_node));
                }
                res_array[finished].sum = sum;
                res_array[finished].source_id = newSocket;
                finished++;
            pthread_mutex_unlock(&finishlock);  
            printf("Process %d result is: %d\n", id, sum);
        }
        if (strcmp(code, "finished") == 0){
            break;  
        }
    }
    while (1){
        pthread_mutex_lock(&isFinishlock);
        if (isFinished == 0) {
            pthread_mutex_unlock(&isFinishlock);
            break;
        }
        pthread_mutex_unlock(&isFinishlock);
    }
    printf("Thread %d closed\n", id);
    pthread_mutex_lock(&countlock);
    closed_count++;
    pthread_mutex_unlock(&countlock);
    close(newSocket);
    pthread_exit(NULL);
}


void initpool(){
    no_thread = 0;
    srand(time(NULL));
    ball_pointer = 0;
    number_of_ball = 1000;
    printf("Number of ball: %d\n", number_of_ball);
    ball_pool = malloc(number_of_ball * sizeof(int));
    for (int i = 0; i< number_of_ball; i++){
        int point = (rand() % (100)) + 1;
        printf("%d ", point);
        ball_pool[i] = point;
    }
    printf("\n");
}

void *finishCounter(void *arg){
    int no_of_c;
    FILE *fp;
    char result_msg[5000];
    memset(result_msg, '\0', 5000);
    while (1){
        pthread_mutex_lock(&finishlock);
        if ((t_count > 0) && (finished == t_count)){
            no_of_c = t_count;
            sort(res_array, t_count);
            pthread_mutex_unlock(&finishlock);
            break;
        }
        pthread_mutex_unlock(&finishlock);
    }
    fp = fopen("ranking","w");
    for (int i = 0; i< no_of_c; i++){
        char pending[500];
        memset(pending, '\0', 500);
        sprintf(pending,"Rank %d, Sum: %d\n", i+1, res_array[i].sum);
        fprintf(fp,"%s", pending);
        strcat(result_msg, pending);
    }
    for (int i = 0; i < no_of_c; i++){
        char pending_msg[5200];
        memset(pending_msg,'\0', 5200);
        sprintf(pending_msg, "My rank: %d\n", i+1);
        strcat(pending_msg,result_msg);
        int l = strlen(pending_msg);
        int offset = 0;
        while (send(res_array[i].source_id, pending_msg + offset, 1024, 0) < l){
            l-=1024;
            offset+=1024;   
        }
    }
    pthread_mutex_lock(&isFinishlock);
    isFinished = 0;
    pthread_mutex_unlock(&isFinishlock);
    while(1){
        pthread_mutex_lock(&countlock);
        if(no_of_c == closed_count){
            pthread_mutex_unlock(&countlock);
            printf("Counter exit\n");
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&countlock);
    }
}

void trimstr(char * inStr){
    int l = strlen(inStr);
    for (int i = 0; i< l; i++){
        if (inStr[i] == '\n'){
            inStr[i] = '\0';
            break;
        }
    }
}


void sort(res_node *arr, int n){
    for (int i = 0; i< n; i++){
        for (int j = i+1; j < n; j++){
            if(arr[i].sum < arr[j].sum) {
                int temp;
                temp = arr[i].sum;
                arr[i].sum = arr[j].sum;
                arr[j].sum = temp;

                temp = arr[i].source_id;
                arr[i].source_id = arr[j].source_id;
                arr[j].source_id = temp;
            }
        }
    }
}