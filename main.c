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


// global variable; can't be avoided because
// of asynchronous signal interaction
int still_running = TRUE;
void signal_handler(int sig) {
    still_running = FALSE;
}
work_queue_item *head = NULL;
work_queue_item *tail = NULL;
pthread_mutex_t work_mutex;
pthread_mutex_t work_cond;
int queue_count = 0;

//Carrie
struct work_queue_item {
	int sock;
	struct work_queue_item *previous;	
	struct work_queue_item *next;
};


void usage(const char *progname) {
    fprintf(stderr, "usage: %s [-p port] [-t numthreads]\n", progname);
    fprintf(stderr, "\tport number defaults to 3000 if not specified.\n");
    fprintf(stderr, "\tnumber of threads is 1 by default.\n");
    exit(0);
}

//Carrie and Shreeya
void runserver(int numthreads, unsigned short serverport) {
    //////////////////////////////////////////////////
    
    // create your pool of threads here
    
 
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
        } else if (prv < 0) {
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
           /* You got a new connection.  Hand the connection off
            * to one of the threads in the pool to process the
            * request.
            *
            * Don't forget to close the socket (in the worker thread)
            * when you're done.
            */
            
            
            
            
            
            /*************************************
            
            
            //SHREEYA!!  We can delete everything I've written... was trying to get 
            //a start on the linked list, but not sure I'm implementing it in the right place...
            //have a look at the man page for pthread_create... at the bottom there is some
            //potentially useful code for make a bunch of threads.  Should we have an array of
            //them as we will always have a set number of threads (its the processes that
            //are variable)?
            
         
            
            ************************************/
            
            
            /*this will be a linked lists of the processes that need threads. Once a thread is
    freed from its process, then we will take the process off the tail.  When a new process
    needs to be added, we can add to the head.
    */
   	 
   	 //adding a process to the waiting work_queue
    	(work_queue_item* ) newitem = (work_queue_item*)malloc(sizeof(work_queue_item));
    	newitem.sock = new_sock; 
  	if(head==NULL && tail==NULL) { //1st node
    		head = newitem;
    		head.next = NULL;
    		head.previous = NULL;
    	}
    	else if(head==NULL && tail!=NULL) {
    		printf("Pointer error.\n");			
    		return;
    		//this should really never happen, but I'd figure I'd catch it just in case
    	}
    	else if(head!=NULL && tail==NULL) { //only 1 node so far
    		newitem.next = head;
    		newitem.previous = NULL;
    		head = newitem;
    		tail = newitem.next;
    		tail.previous = head; //using previous so can change tail when necessary
    	}
    	else { //otherwise, add to head
    		newitem.next = head;
    		head.previous = newitem;
    		head = newitem;
    	}
    	
    	//create loop to cycle through work_queue_item. When thread is done, takes tail
    	//of linked list, shifts tail to previous.

           ////////////////////////////////////////////////////////


        }
    }
    fprintf(stderr, "Server shutting down.\n");
        
    close(main_socket);
}


int main(int argc, char **argv) {
    unsigned short port = 3000;
    int num_threads = 1;

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

    runserver(num_threads, port);
    
    fprintf(stderr, "Server done.\n");
    exit(0);
}
