#include <stdio.h>
#include "request.h"
#include "io_helper.h"

char default_root[] = ".";

//
// ./wserver [-p <portnum>] [-t threads] [-b buffers] 
// 
// e.g.
// ./wserver -p 2022 -t 5 -b 10
// 
int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
    int port = 10000;
	int threads = 2;
	int buffer_size = 5;
    
    while ((c = getopt(argc, argv, "p:t:b:")) != -1)
		switch (c) {
		case 'p':
			port = atoi(optarg);
			break;
		case 't':
			threads = atoi(optarg);
			break;
		case 'b':
			buffer_size = atoi(optarg);
			break;
		default:
			fprintf(stderr, "usage: ./wserver [-p <portnum>] [-t threads] [-b buffers] \n");
			exit(1);
		}

	
	
	printf("Server running on port: %d, threads: %d, buffer: %d\n", port, threads, buffer_size);

    // run out of this directory
    chdir_or_die(root_dir);

    // now, get to work
    int listen_fd = open_listen_fd_or_die(port);
    while (1) {



		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);
		int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
		pthread_t Nthread;
		int *pclient=malloc(sizeof(int));
		*pclient = conn_fd;
		pthread_create(&Nthread,NULL,request_handle,(*pclient));
		printf("new thread created");


		///request_handle(conn_fd); // The request is handled by this function 
	}
    return 0;
}


    


 
