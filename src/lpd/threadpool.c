#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<pthread.h>
#include<semaphore.h>
#include "threadpool.h"
#include "printqueue.h"

void add_thread(void);
int getFileID(void);
int getID(void);

static struct v_thread* threads;

static int init = 0;
static int ID = 0;
static pthread_mutex_t pool_lock;

static int wait = 0;



int thread_pool_init(void){
    int i;
    //int check;
    init = 1;

    threads = malloc(sizeof(struct v_thread));
    if(threads == NULL){
        return -1;
    }
    threads->size = MAX_THREAD_NUM;
    threads->current = 0;

    threads->data = malloc(sizeof(struct server_thread) * MAX_THREAD_NUM);


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
    struct job *temp = malloc(sizeof(struct job)); 
    struct printer *tempP = malloc(sizeof(struct printer));
    temp->p = tempP;
    tempP->name = "/var/spool/lpd/lp";
    //TODO make the structures the thread needs for holding data.


    //Everything above this is for setting up the thread.
    while(1){
        puts("going to sleep");
        //Each thread waits until it is unlocked.
        //pthread_mutex_lock(self->lock);
        getJobID();
        sem_wait(self->test);
        sem_wait(self->test);
        puts("I woke up");
        // Grabs the FD that was passed in.
        FD = *self->data;
        write(FD,"hello",6);
        addElement(temp);
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

int getJobID(void){
  int FD = open("job",O_RDWR|O_EXLOCK);
  if(FD <0){
    // error out here.
    puts("file not found");
  }
  char input[10];
  int JID = 0;
  read(FD,input,9);
  input[9]=0;
  JID = atoi(input);
  printf("ID is %d, setting new JID to %d", JID, JID +1);
  lseek(FD,0,SEEK_SET);
  dprintf(FD, "%d", JID+1);
  
  return JID;
  
  
  
}

// Finds a free thread, and gives it access to the input data.
// It then unlocks the thread and returns.

// TODO: put a lock in here.
int requestJob(int input){
  int i;
  for (i = 0; i < threads->current; i++){
    if(*threads->data[i].working == 0){
      *threads->data[i].working = 1;

      *threads->data[i].data = input;
      sem_post(threads->data[i].test);

      return 0;
    }

  }
  add_thread();
  return requestJob(input);
}

// This function is a mess, I need to redo it at some point.
void add_thread(void){
    puts("Spooling up new thread.");

    //struct v_thread *temp;

    // Doubling the max number of threads

    if ((int) threads->size == threads->current){
        puts("You're have too many concurrent jobs coming in.");
        /*
        temp = malloc(sizeof(struct server_thread)*threads->size*2);
        threads = memmove(temp, threads, sizeof(struct server_thread)*threads->size);
        threads->size = threads->size*2;
        puts("doubling size");
        */
    }

    int current = threads->current;



    //threads->data[current].lock = malloc(sizeof(pthread_mutex_t));
    //pthread_mutex_init(threads->data[current].lock, NULL);
    //pthread_mutex_lock(threads->data[current].lock);

    threads->data[current].test = malloc(sizeof(sem_t));
    sem_init(threads->data[current].test, 0, 0);


    threads->data[current].data = malloc(sizeof(int));
    threads->data[current].working = malloc(sizeof(int));

    threads->data[current].printer = malloc(sizeof(struct printer));

    threads->data[current].thread = malloc(sizeof(pthread_t));
    pthread_create(threads->data[current].thread, NULL, worker_thread, &threads->data[current]);

    threads->current++;
    // Waiting for the thread to be done.
    // I might want to replace this with a mutex.
    //while (globalLock == 1){}

}
