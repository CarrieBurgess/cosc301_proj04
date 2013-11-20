/*
Who: Shreeya and Carrie
What: Project 4, Concurrent Web Server
When: 20 November 2013
Where: somewhere, floating in virtual space
Why: ....we have to.
*/


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "network.h"


//Global variables:
int still_running = TRUE;
void signal_handler(int sig) {
    still_running = FALSE;
}
struct work_queue_item *head = NULL;
struct work_queue_item *tail = NULL;
pthread_mutex_t mucheck; 
pthread_cond_t thread, connection;
FILE * log_file = NULL;
int queue_count = 0;

int thread_count = 0;
int max_threads;

//Makes a queue item
struct work_queue_item {
	int sock;
    struct sockaddr_in client_address;
	struct work_queue_item *previous;	
	struct work_queue_item *next;
};

//professor made function
void usage(const char *progname) {
    fprintf(stderr, "usage: %s [-p port] [-t numthreads]\n", progname);
    fprintf(stderr, "\tport number defaults to 3000 if not specified.\n");
    fprintf(stderr, "\tnumber of threads is 1 by default.\n");
    exit(0);
}

//add an item to the queue (add to head)
void add_to_queue(int new_sock, struct sockaddr_in input_client_address) {
	struct work_queue_item * newitem = (struct work_queue_item *)malloc(sizeof(struct work_queue_item));
    	newitem->sock = new_sock; 
	newitem->client_address = input_client_address; ////added for log file
  	if(head==NULL && tail==NULL) { //1st node
    		head = newitem;
    		head->next = NULL;
    		head->previous = NULL;
    		tail = head;
    	}
    	else if(head==NULL && tail!=NULL) {
    		printf("Pointer error.\n");			
    		return;
    		//this should really never happen, but I'd figure I'd catch it just in case
    	}
    	else if(head!=NULL && head==tail) { //only 1 node so far
    		newitem->next = head;
    		newitem->previous = NULL;
    		head = newitem;
    		tail = newitem->next;
    		tail->previous = head; //using previous so can change tail when necessary
    	}
    	else { //otherwise, add to head
    		newitem->next = head;
    		head->previous = newitem;
    		head = newitem;
    	}
}

//take an item from the queue (remove from tail)
struct work_queue_item * remove_tail() {
	struct work_queue_item * temp = tail;
	if(head==tail) { //there is only 1 node
		head->next= NULL;
		head->previous=NULL;
		tail->next = NULL;
		tail->previous= NULL;
		head = NULL;
		tail = NULL;
	}
	else if(tail->previous == head) { //there are only two nodes
		tail->previous = NULL;
		tail->next = NULL;
		tail = head;
		head->previous = NULL;
		head->next = NULL;
	}
	else {
		tail = tail->previous;
		tail->next = NULL;
	}
	return temp;
}

//This removes a tail from the queue to obtain necessary information, calls getrequest to get the reqbuffer, opens the file, and concatonates the necessary string.  This is then sent to senddata and added to a file called 'stuff'
void * worker(void * arg) {
	while(still_running) {	//while the server has not shut down
		pthread_mutex_lock(&mucheck);
		while (thread_count==0 && tail==NULL) { //if there is nothing in the queue, wait
			pthread_cond_wait(&thread, &mucheck);
			if(!still_running) return NULL;
		}
		thread_count++;
    		struct work_queue_item * temp = remove_tail(); //remove tail for connection info
		pthread_mutex_unlock(&mucheck);	
		queue_count--;
		int buffersize = 1024;
		char * reqbuffer = malloc(1024*sizeof(char));
		int err = getrequest(temp->sock, reqbuffer, buffersize);
		if(err==-1) {
			printf("Couldn't obtain file.\n");
		}
		FILE *file = NULL;
		if (reqbuffer[0]=='/') 	file = fopen(reqbuffer+sizeof(char), "r");
		else file = fopen(reqbuffer, "r");
		if (file==NULL) { //cannot open, concat with 404--> make a fxn that does this
			int http_length = strlen(HTTP_404);
			time_t now = time(NULL);
			senddata(temp->sock, HTTP_404, http_length);
            		fprintf(log_file, "%s:%d  %s  'GET %s'  %d %d\n", inet_ntoa((temp->client_address).sin_addr), ntohs((temp->client_address).sin_port), ctime(&now), reqbuffer, 404, http_length);
		}
		else { //concat with 202.. send input from file to be concatenated
			
			
			fseek(file, 0, SEEK_END);
			int file_size = ftell(file);
			char http[128];
			snprintf(http, 128, HTTP_200, file_size);
			int http_length = strlen(http);
			rewind(file);			
				
			char file_info[1024];
			senddata(temp->sock, http, http_length);
			while(fread(file_info, sizeof(char), 1024, file)!=0) {
				senddata(temp->sock, file_info, 1024);
			}
			fclose(file);
			int total_size = file_size + http_length;
			time_t now = time(NULL);
			fprintf(log_file, "%s:%d  %s  'GET %s'  %d %d\n", inet_ntoa((temp->client_address).sin_addr), ntohs((temp->client_address).sin_port), ctime(&now), reqbuffer, 200, total_size);
		}
		free(reqbuffer);
		close(temp->sock);
		free(temp);
		thread_count--;		
	}
	return NULL;
}


//aside from the professor written code, this creates an array of threads.  A little further down,
//this adds connections to the queue
void runserver(int numthreads, unsigned short serverport) {
    //////////////////////////////////////////////////
    
    pthread_t thread_arr[numthreads];
    int i = 0;
    for(;i<numthreads; i++) {
    	pthread_create(&(thread_arr[i]), NULL, worker, NULL);
    }
 
    //////////////////////////////////////////////////
    
    
    int main_socket = prepare_server_socket(serverport);
    if (main_socket < 0) {
        exit(-1);
    }
    signal(SIGINT, signal_handler);

    struct sockaddr_in client_address;
    socklen_t addr_len;

    fprintf(stderr, "Server listening on port %d.  Going into request loop.\n", serverport);
    while (still_running) {
        struct pollfd pfd = {main_socket, POLLIN};
        int prv = poll(&pfd, 1, 10000);

        if (prv == 0) {
            continue;
        } 
        else if (prv < 0) {
            PRINT_ERROR("poll");
            still_running = FALSE;
            continue;
        }
        
        addr_len = sizeof(client_address);
        memset(&client_address, 0, addr_len);

        int new_sock = accept(main_socket, (struct sockaddr *)&client_address, &addr_len);
        if (new_sock > 0) {
            
            time_t now = time(NULL);
            fprintf(stderr, "Got connection from %s:%d at %s\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), ctime(&now));

           ////////////////////////////////////////////////////////            
            pthread_mutex_lock(&mucheck);
            add_to_queue(new_sock, client_address);
            queue_count++;
            pthread_cond_signal(&thread);
            pthread_mutex_unlock(&mucheck);
            }
           ////////////////////////////////////////////////
       
    }
	i = 0;
	pthread_cond_broadcast(&thread); //wake all up
	for(;i<numthreads; i++) { ;
    		pthread_join((thread_arr[i]), NULL);
    	}	
    fprintf(stderr, "Server shutting down.\n");
        
    close(main_socket);
	
}

//mutexes and condition variables are initialized here
int main(int argc, char **argv) {
    unsigned short port = 3000;
    int num_threads = 1;
    pthread_mutex_init(&mucheck, NULL);
    pthread_cond_init(&connection, NULL);
    pthread_cond_init(&thread, NULL);
    int c;
    while (-1 != (c = getopt(argc, argv, "hp:t:"))) {
        switch(c) {
            case 'p':
                port = atoi(optarg);
                if (port < 1024) {
                    usage(argv[0]);
                }
                break;

            case 't':
                num_threads = atoi(optarg);
                if (num_threads < 1) {
                    usage(argv[0]);
                }
                break;
            case 'h':
            default:
                usage(argv[0]);
                break;
        }
    }
    max_threads = num_threads;
    log_file = fopen("weblog.txt","a");	
    runserver(num_threads, port);
    pthread_mutex_destroy(&mucheck);
    pthread_cond_destroy(&connection);    
    pthread_cond_destroy(&thread);
    fprintf(stderr, "Server done.\n");
    fclose(log_file);
    exit(0);
}
