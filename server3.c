#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define CL_MAX 10
#define BUFF_SIZE 200

pthread_mutex_t mutex;
int cl_socks[CL_MAX];
int cl_cnt = 0;
char *name = NULL;

char *get_username(int sock)
{
	char *str = malloc(29);
	read(sock, str, 29);
	return strrchr(str, ' ') + 1;
}

void *handle_client(void* arg)
{
	int cl_sock = *((int *)arg);
	char buffer[BUFF_SIZE];
	char sv_msg[BUFF_SIZE];
	char *receiver, *tmp, *msg, *str;
	int cl_rsock;
	
	while(1) {
		memset(buffer, 0, sizeof(buffer));
		read(cl_sock, buffer, sizeof(buffer));

		if(strncmp(buffer, "exit", 4) == 0) {
			printf("%s\n", buffer);
			pthread_mutex_lock(&mutex);
			for(int i = 0; i < cl_cnt; i++) {
				if(cl_sock == cl_socks[i]) {
					while(i < cl_cnt - 1) {
						cl_socks[i] = cl_socks[i+1];
						i++;
					}
					break;
				}
			}
			cl_cnt--;
                        sprintf(sv_msg,"[ server: disconnected %s(user) ]", name);
                        for(int i = 0; i < cl_cnt ; i++)
                                write(cl_socks[i], sv_msg, strlen(sv_msg) + 1);
                        pthread_mutex_unlock(&mutex);
			break;
		}
		else if(strncmp(buffer, "send ", 5) == 0) {
			tmp = strrchr(buffer, ' ');
			if(tmp == NULL) 
				continue;
			receiver = tmp + 1;
			cl_rsock = atoi(receiver);

			tmp = strchr(buffer, ' ');
			if(tmp == NULL)
				continue;
			msg = tmp + 1; 
			
			str = malloc(BUFF_SIZE);
			sprintf(str, "%s >> %s", name, msg);

			pthread_mutex_lock(&mutex);
                        for(int i = 0; i < cl_cnt ; i++) {
                                if(cl_socks[i] != cl_sock && cl_socks[i] == cl_rsock) 
                                        write(cl_socks[i], str, strlen(str) + 1);
                        }
                        pthread_mutex_unlock(&mutex);
			free(str);
		}
		else {
			pthread_mutex_lock(&mutex);
			for(int i = 0; i < cl_cnt ; i++) {
				if(cl_socks[i] != cl_sock) 
					write(cl_socks[i], buffer, strlen(buffer) + 1);
			}	
			pthread_mutex_unlock(&mutex);
		}
	}
	close(cl_sock);
	return NULL;
}

int main()
{
	int sv_sock, cl_sock, len, th_id;
	struct sockaddr_in sv_addr, cl_addr;
	char sv_msg[BUFF_SIZE];

	pthread_t thread;
	pthread_mutex_init(&mutex, NULL);
	sv_sock = socket(AF_INET, SOCK_STREAM, 0);
	
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(8000);
	sv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(sv_sock, (struct sockaddr *)&sv_addr, sizeof(sv_addr));
	listen(sv_sock, 5);

	while(1) {
		len = sizeof(cl_addr);
		cl_sock = accept(sv_sock, (struct sockaddr *)&cl_addr, &len);
		pthread_mutex_lock(&mutex);
		cl_socks[cl_cnt++] = cl_sock;
		pthread_mutex_unlock(&mutex);
		th_id = pthread_create(&thread, NULL, handle_client, (void *)&cl_sock);
       	 	
		if(th_id == 0) {
			pthread_mutex_lock(&mutex);
			name = get_username(cl_sock);
			printf("connected user: %s\n", name);
			sprintf(sv_msg,"[ server: connected %s(user, %d) ]", name, cl_sock);
			for(int i = 0; i < cl_cnt ; i++) 
                                write(cl_socks[i], sv_msg, strlen(sv_msg) + 1);
			pthread_mutex_unlock(&mutex);
		}
	}			
	return 0;
}

