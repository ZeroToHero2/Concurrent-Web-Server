#include <stdio.h>
#include "request.h"
#include "io_helper.h"
#include <pthread.h>

char default_root[] = ".";

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t needToEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t needToFill = PTHREAD_COND_INITIALIZER;


struct {
	int count;
	int buff_size;
	int nb_threads;
	int* buff;
} shared = {0};

int get_fd () {
	// décrémente count de 1, signal que le buffer n'est pas plein, return fd
	shared.count --;
	int fd = shared.buff[shared.count];
	pthread_cond_signal(&needToFill);
	return fd;
}
void put_fd (int fd) {
	//place fd dans le buffer, incrémente count de 1, signal buffer n'est pas vide
	shared.buff[shared.count]=fd;
	shared.count ++;
	pthread_cond_signal(&needToEmpty);
}

void *worker_thread_consumer(void *arg) {
	//fonction utilisée par les worker_threads
	while (1){
		pthread_mutex_lock(&lock); //par défaut le thread crée est blocké
		while (shared.count==0)
			pthread_cond_wait (&needToEmpty, &lock);
		int fd = get_fd();
		pthread_mutex_unlock(&lock);
		request_handle(fd);
		close_or_die(fd);
		//sleep(1);
	}
}

void master_thread_producer (int fd) {
	//fonction appelée à chaque fois qu'une nouvelle requête client est recu
	pthread_mutex_lock(&lock);
	while (shared.count == shared.buff_size) {
		printf("############# BUFFER FULL #############\n");
		pthread_cond_wait(&needToFill, &lock);
	}
	put_fd(fd);
	pthread_mutex_unlock(&lock);
}

int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
    int port = 10000;
	int threads = 10;
	int buffers = 40;
    
    while ((c = getopt(argc, argv, "d:p:t:b:")) != -1)
	switch (c) {
	case 'd':
	    root_dir = optarg;
	    break;
	case 'p':
	    port = atoi(optarg);
	    break;
	case 't':
		threads = atoi(optarg);
		if (threads<=0){
	    	fprintf(stderr, "Number of threads has to be a positive integer\n");
			exit(1);
		}
		break;
	case 'b':
		buffers = atoi(optarg);
		if (buffers<=0){
			fprintf(stderr, "Number of buffers has to be a positive integer\n");
			exit(1);
		}
		break;
	default:
	    fprintf(stderr, "usage: wserver [-d basedir] [-p port] [-t threads] [-b buffers]\n");
	    exit(1);
	}

    // run out of this directory
    chdir_or_die(root_dir);

	//fill the structure with infos
	shared.buff = malloc( sizeof(int)*buffers);
	shared.buff_size = buffers;
	shared.nb_threads = threads;
  

    // create pool of threads before the while
	pthread_t pool[threads];
	printf("nb of threads = %d \n", threads);
    for (int i=0; i<threads; i++) {
        if( pthread_create(&pool[i], NULL, worker_thread_consumer, &i) != 0 )
           printf("Failed to create thread\n");
    }

    int listen_fd = open_listen_fd_or_die(port);

    while (1) {
		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);
		int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
		master_thread_producer(conn_fd);		
	}
    return 0;
}