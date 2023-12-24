#include <errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct s_client {
  int id;
  char msg[120000];
} t_client;

t_client clients[1200];
fd_set read_it, write_it, active;
int max;
char msg_w[120000];
int id = 0;
void fatal() {
  write(2, "Fatal error\n", strlen("Fatal error\n"));
  exit(1);
}

void sendAll(int client_fd) {
  for (int fd = 0; fd <= max; fd++) {
    if (FD_ISSET(fd, &write_it) && fd != client_fd) {
      send(fd, msg_w, strlen(msg_w), 0);
    }
  }
}

int main(int ac, char **arg) {

  if (ac != 2) {
    write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
    exit(1);
  }

	int sockfd, connfd;
  socklen_t len;
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) {
    fatal();
  }
	bzero(&servaddr, sizeof(servaddr));
  bzero(clients, sizeof(clients));

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(arg[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
    fatal();
  }

	if (listen(sockfd, 10) != 0) {
		fatal();
	}
	len = sizeof(cli);

  FD_ZERO(&active);
  FD_SET(sockfd, &active);
  max = sockfd;
  bzero(msg_w, 120000);
  while (1) {
    read_it = write_it = active;
    if (select(max + 1, &read_it, &write_it, NULL, NULL) == -1) {
      continue;
    }
    for (int fd = 0; fd <= max; fd++) {
      if (FD_ISSET(fd, &read_it)) {
        if (fd == sockfd) {
          connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
          if (connfd == -1) {
            puts("error");
            continue;
          }
          if (max < connfd) {
            max = connfd;
          }
          FD_SET(connfd, &active);
          sprintf(msg_w, "server: client %d just arrived\n", id);
          sendAll(connfd);
          clients[connfd].id = id++;
        }
        else {
          int bytes = recv(fd, clients[fd].msg, 120000, 0);
          if (bytes <= 0) {
            sprintf(msg_w, "server: client %d just left\n", clients[fd].id);
            sendAll(fd);
            FD_CLR(fd, &active);
            close(fd);
          }
          else {
            char msg[120000];
            bzero(msg, 120000);
            int j = 0;
            for (int i = 0; i < bytes ; i++) {
              msg[j++] = clients[fd].msg[i];
              if (clients[fd].msg[i + 1] == '\0' || clients[fd].msg[i] == '\n') {
                msg[j] = '\0';
                sprintf(msg_w, "client %d: %s", clients[fd].id, msg);
                sendAll(fd);
                j = 0;
                bzero(msg, 120000);
              }
            }
          }
        }
      }
    }
  }
}
