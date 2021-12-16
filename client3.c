#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFF_SIZE 200
#define NAME_SIZE 20

char name[NAME_SIZE];
pthread_cond_t console_cv;
pthread_mutex_t console_cv_lock;

void *receive(void *arg)
{
	char buffer[BUFF_SIZE];
	int sock = *(int *)arg;
	int readlen;

	while(1) {
		memset(buffer, 0, sizeof(buffer));
		readlen = read(sock, buffer, sizeof(buffer));
		if(readlen < 1)
			continue;
		printf("%s\n", buffer);
	}
}

void console(int sock)
{
	char buffer[BUFF_SIZE];
	char msg[NAME_SIZE + BUFF_SIZE];
	char *tmp;

	memset(buffer, 0, sizeof(buffer));
	while(1) {
		fgets(buffer, BUFF_SIZE, stdin);
		buffer[strlen(buffer) - 1] = '\0';

		if (strcmp(buffer, "") == 0)
			continue;
		else if (strncmp(buffer, "exit", 4) == 0) {
			sprintf(msg, "exit user: %s", name);
			write(sock, msg, strlen(msg) + 1);
			pthread_mutex_destroy(&console_cv_lock);
			pthread_cond_destroy(&console_cv);
			_exit(EXIT_SUCCESS);
		}
		else if (strncmp(buffer, "send ", 5) == 0) {
			tmp = strchr(buffer, ' ');
			if(tmp == NULL) {
				continue;
			}
			tmp = strrchr(buffer, ' ');
			if(tmp == NULL) {
				continue;
			}
			write(sock, buffer, sizeof(buffer));
			continue;
		}
		else {
			sprintf(msg, "[%s] %s", name, buffer);
			write(sock, msg, strlen(msg) + 1);
			continue;
		}
		
	}
}

void username(int sock)
{
	char *namestr = malloc(NAME_SIZE + 9);
	sprintf(namestr, "username %s", name);
	write(sock, namestr, strlen(namestr));
	free(namestr);
}

int main()
{
	int sock;
	struct sockaddr_in addr;
	pthread_t recv_thread;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8000);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	connect(sock, (struct sockaddr*)&addr, sizeof(addr));

	printf("enter name: ");
	fgets(name, sizeof(name), stdin);
	name[strlen(name) - 1] = '\0';
	username(sock);
	pthread_create(&recv_thread, NULL, receive, (void *)&sock);
	console(sock);

	return 0;
}

