#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include "threadpool.h"
#include<pthread.h>

void add_thread(void);                  
int getFileID(void);


static struct v_thread* threads;

static int init = 0;
static int ID = 0;
static int fileID = 1;
static pthread_mutex_t pool_lock;

static int wait = 0;
static int globalLock = 1;



int thread_pool_init(void){
    int i;
    //int check;
    init = 1;

    threads = malloc(sizeof(struct v_thread));
    if(threads == NULL){
        return -1;
    }
    threads->size = STARTING_THREAD_NUM;
    threads->current = 0;

    threads->data = malloc(sizeof(struct server_thread) * STARTING_THREAD_NUM);


    // This is grabbing memory for the thread pool.

    pthread_mutex_init(&pool_lock, NULL);

    for(i = 0; i < STARTING_THREAD_NUM; i++){
        add_thread();
    }


    return 0;
}
// TODO: Replace this with actual logic.
// Most of this code is just filler.
void* worker_thread (void* dataPointer){
    //int threadnum = getID(); might need this later
    int FD;
    struct server_thread* self = malloc(sizeof(struct server_thread));
    self = memcpy(self, dataPointer, sizeof(struct server_thread));
    //^important v not important
    
    //TODO make the structures the thread needs for holding data.

    //Syncing
    globalLock = 0;

    //Everything above this is for setting up the thread.
    while(1){

        //Each thread waits until it is unlocked.
        pthread_mutex_lock(self->lock);

        // Grabs the FD that was passed in.
        FD = *self->data;

        // Get the printcap data and setup where you are going to write to.
        // Set up function set

        // call bens monitor function.

        //Closes port, and set self to no longer working.
        close(*self->data);


        *self->working = 0;
    }
    return NULL;
}
// This function is called by threads as they created.
// It returns an ID for each thread to index into arrays.
int getID(void){
    int returnValue;

    returnValue = ID;
    ID++;
    pthread_mutex_unlock(&pool_lock);
    wait = 0;
    return returnValue;
}


int getFileID(void){
    int returnValue;
    pthread_mutex_lock(&pool_lock);
    returnValue = fileID;
    fileID++;
    pthread_mutex_unlock(&pool_lock);
    return returnValue;
}

// Finds a free thread, and gives it access to the input data.
// It then unlocks the thread and returns.
int requestJob(int input){
  int i;
  for (i = 0; i < threads->current; i++){
    if(*threads->data[i].working == 0){
      *threads->data[i].working = 1;

      *threads->data[i].data = input;
      pthread_mutex_unlock(threads->data[i].lock);

      return 0;
    }

  }
  add_thread();
  return -1;
}

void add_thread(void){
    puts("Spooling up new thread.");

    struct v_thread *temp;

    // Doubling the max number of threads
    if ((int) threads->size == threads->current){
        temp = malloc(sizeof(struct server_thread)*threads->size*2);
        threads = memmove(temp, threads, sizeof(struct server_thread)*threads->size);
        threads->size = threads->size*2;
    }
    globalLock = 1;
    int current = threads->current;

    threads->data[current].thread = malloc(sizeof(pthread_t));
    pthread_create(threads->data[current].thread, NULL, worker_thread, &threads->data[current]);

    threads->data[current].lock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(threads->data[current].lock, NULL);
    pthread_mutex_lock(threads->data[current].lock);

    threads->data[current].data = malloc(sizeof(int));
    threads->data[current].working = malloc(sizeof(int));

    threads->data[current].printer = malloc(sizeof(struct printer));

    threads->current++;
    // Waiting for the thread to be done.
    // I might want to replace this with a mutex.
    while (globalLock == 1){}

}
