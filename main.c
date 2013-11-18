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
struct work_queue_item *head = NULL;
struct work_queue_item *tail = NULL;
pthread_mutex_t mucheck; 
//pthread_mutex_t work_mutex;
//pthread_mutex_t work_cond;
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

void add_to_queue(int new_sock) {
	struct work_queue_item * newitem = (struct work_queue_item *)malloc(sizeof(struct work_queue_item));
    	newitem->sock = new_sock; 
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

void worker(void) {
	if(tail==NULL) {
		//put thread to sleep
	}
	else { //a connection is waiting in the queue
		pthread_mutex_lock(&mucheck);
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
		//node has now been removed
		char * reqbuffer;
		int buffsize = 1024;
		int err = getrequest(temp->sock, reqbuffer, buffsize);
		//is reqbuffer malloced?  do we have to free it? ******************************
		if(err==-1) {
			printf("Couldn't obtain file.\n");
		}
		//char * present_dir = pwd;
		//int tot_size = strlen(reqbuffer) + strlen(present_dir);
		int err2 = fopen(reqbuffer);
		//if can open, concat with 202
		//if can't, concat with 404
		
		//now need thread to do something with reqbuffer
		//add to output thing
		//put thread back to sleep when finished
		pthread_mutex_unlock(&mucheck);
	}
}

//Carrie and Shreeya
void runserver(int numthreads, unsigned short serverport) {
    //////////////////////////////////////////////////
    
    // create your pool of threads here
    /*
    before creating threads, need to create/ initialize condition variable,
    mutex, and whatever other shared data worker threads need to wait on
    	--> intiially, want workers to get started up then get stuck on some
    	sort of condition variable
    	 
    want array of pthread_t to create pthreads
    when create each of the threads, need seperate function for 
    them to start up in
    	-> this needs a particular signature to match pthread_t
    	prolly takes void * and returns void *
    	look at lab for creating threads
    when go through for loop to create x number of worker threads
    
    */
    
    pthread_t thread_arr[numthreads];
    int i = 0;
    for(;i<numthreads; i++) {
    	pthread_create(&thread_arr[i], NULL, &worker, NULL);
    }
    //hypothetically, threads have now been initialized (with proper locks if need be)
 
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
            
            //adding a process to the waiting work_queue
   	 //**Should I make this a loop for a series of processes?  Currently only accepting
   	 //1 process at a time...
            add_to_queue(new_sock);
            //process has now been added to queue waiting for worker.
            i = 0;
            int thread_available = 0;
            for(;i<numthreads; i++) { //if a thread is immediately available
            	//if arr_threads[i] is waiting, unlock it and exit this loop
            	//in same if statement, make thread_available=1;
            }
            if(thread_available == 1) {
            	//pop tail off queue
            	//give arr_threads[i] the socket from the tail
            }

    	/*
    	after add to queue, kick a thread awake and pass it off
    	will have to write code to implement what worker (thread) does
    	*not passing socket directly, putting in linked list
    	
    	after main thread adds item to queue, signals worker queue.
    	worker queue grabs mutex, grabs something off linked list (removes it)
    	takes socket, reads request/ does it, then goes back and waits
    	
    	key is to coordinate producer consumer 
    	*/

           ////////////////////////////////////////////////////////


        }
    }
    fprintf(stderr, "Server shutting down.\n");
        
    close(main_socket);
}


int main(int argc, char **argv) {
    unsigned short port = 3000;
    int num_threads = 1;
   // struct work_queue_item *head;
   // struct work_queue_item *tail;
   //pthread_mutex_t mucheck;
   pthread_mutex_init(&mucheck, NULL); //initialize mutex to lock threads as
   //they are created
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
    pthread_mutex_destroy(&mucheck);
    fprintf(stderr, "Server done.\n");
    exit(0);
}
