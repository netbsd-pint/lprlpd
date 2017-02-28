#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include"threadpool.h"

static struct v_thread* threads;

static int init = 0;
static int ID = 0;
static int fileID = 1;
static pthread_mutex_t pool_lock;

int wait = 0;
int globalLock = 1;

void add_thread();
int getFileID();

int thread_pool_init(){
    int i;
    //int check;
    init = 1;

    threads = malloc(sizeof(struct v_thread));
    if(threads == NULL){
        return -1;
    }
    threads->size = STARTING_THREAD_NUM;
    threads->current = 0;

    threads->data= malloc(sizeof(struct st_thread) * STARTING_THREAD_NUM);



    // This is grabbing memory for the thread pool.

    pthread_mutex_init(&pool_lock, NULL);

    for(i = 0; i < STARTING_THREAD_NUM; i++){
        add_thread();
    }


    return 0;
}

void* worker_thread (void* dataPointer){
    int threadnum = getID();
    int FD;
    char input[512];
    int justRead = 0;
    int total = 0;
    int fileID = 0;
    int fileFD = 0;
    char fileName[FILENAME_MAXLENGTH];
    char newName[FILENAME_MAXLENGTH];
    struct st_thread* self = malloc(sizeof(struct st_thread));
    self = memcpy(self, dataPointer, sizeof(struct st_thread));

    globalLock = 0;

    while(1){
        //puts("here1");
        //printf("%d\n", threadnum);

        pthread_mutex_lock(self->lock);

        FD = *self->data;
        // Get the printcap data and setup where you are going to write to.


        fileID = getFileID();

        // I need find where to spool the file to.
        memset(fileName,0,FILENAME_MAXLENGTH);
        snprintf(&fileName[0],FILENAME_MAXLENGTH-1,"%d.N",fileID);
        printf("The file name is '%s'\n",fileName);


        fileFD = open(fileName,O_WRONLY|O_CREAT, 0777); // edit permisions

        if(fileFD < 0){
          perror("open");
        }
        //DEBUG STUFF
        printf("File FD is %d\n", fileFD);
        printf("Thread %d is starting a job\n", threadnum);
        printf("Servicing socket %d\n", FD);
        dprintf(*self->data,"Servicing you\n");

        // Main read loop, I need to clean up if something goes wrong/
        while((justRead = read(FD, &input, 512)) != 0){
            if(justRead == 0){
                printf("closing socket %d\n", FD);
                break;
            }
            printf("I read %d\n", justRead);
            justRead = write(fileFD, &input, justRead);
            if(justRead<1){
              perror("write");
            }
            printf("I wrote %d\n", justRead);
        }
        // open a file
        // print to the file


        puts("Closing the file");
        close(fileFD);

        memset(newName,0,FILENAME_MAXLENGTH);
        snprintf(&newName[0],FILENAME_MAXLENGTH-1,"%d.D",fileID);
        rename(fileName, newName);
        // Right now I need to start writing out the job.

        //function(dataPointer)
        //cleanup()

        close(*self->data);
        // delete newName


        *self->working = 0;
    }
    return NULL;
}
// This function is called by threads as they created.
// It returns an ID for each thread to index into arrays.
int getID(){
    int returnValue;

    returnValue = ID;
    ID++;
    pthread_mutex_unlock(&pool_lock);
    wait = 0;
    return returnValue;
}


int getFileID(){
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

void add_thread(){
    puts("Spooling up new thread.");

    struct v_thread *temp;

    // Doubling the max number of threads
    if (threads->size == threads->current){
        temp = malloc(sizeof(struct st_thread)*threads->size*2);
        threads = memmove(temp, threads, sizeof(struct st_thread)*threads->size);
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

    threads->current++;
    // Waiting for the thread to be done.
    // I might want to replace this with a mutex.
    while (globalLock == 1){}

}
