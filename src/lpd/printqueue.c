#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "printqueue.h"
#include "../common/common.h"
#include "../common/print_job.h"


void *manageQueue(void* queue);

static struct queueVector queueList;

struct queueManager* findQueue(char* queueName){
  struct queueManager *current;
  int anchor = 0;

  for(int i = 0; i<queueList.size; i++){
    current = &queueList.queues[i];
      for(int j = 0; j < (int) strlen(current->name); j++){
        if(current->name[j] == '|'){
          current->name[j] = 0;
          if(strcmp(queueName,&current->name[anchor]) == 0){
            current->name[j] = '|';
            puts(&current->name[anchor]);
            return current;
          }
          current->name[j] = '|';
          anchor = j+1;
        }
      }
      if(strcmp(queueName,&current->name[anchor]) == 0){
        puts(&current->name[anchor]);
      return current;
      }
  }

  puts("Can't find the queue.");
  return NULL;
}

// TODO add in mutexes for every function.




// 0 is success, 1 is error.
// TODO: add in a mutex for adding.
// TODO: change input to just pointer to job struct.
int addElement(struct job *input){

    struct queueManager *queue = findQueue(input->p->name);

    struct queueElement *current = malloc(sizeof(struct queueElement));
    pthread_mutex_lock(queue->lock);
    current->data = input;

    if(queue->size == 0){
        queue->head = current;
        queue->tail = current;
        pthread_mutex_unlock(queue->sleep);
    }else{
        current->previous = NULL;
        queue->tail->previous = current;
        queue->tail = current;
    }

    queue->size++;
    pthread_mutex_unlock(queue->lock);
    return 0;
}


struct queueElement* pop(struct queueManager *queue){
    struct queueElement *returnValue;

    if(queue->size == 0){
        return NULL;
    }
    queue->size--;
    returnValue=queue->head;
    queue->head = returnValue->previous;
    return returnValue;
}

// TODO: Rewrite this for the custom protocol. LPC will connect from 127.0.0.1 om
// some port, and then sends something that will uniquely identify a job. It will
// then push that job to the top of the queue.
int queueEdit(struct job *data){
    /*struct queueManager *queue = findQueue(data->p->name)
    if(queue->size == 0){
        return -1;
    }
    pthread_mutex_lock(&queue->lock);
    struct queueElement *current = queue->head;
    struct queueElement *newhead;
    for(int i=0; i<queue->size;i++){
        if()
        current = current->previous;
    }
    newhead = current->previous;
    current->previous = current->previous->previous;
    newhead->previous = queue->head;
    queue->head = newhead;
    pthread_mutex_unlock(&queue->lock);
    */
    data = NULL;
    return 0;

}

// This function will be called at the start of server.c running. It will
// go through the printcap and find all of the spooling directorie.
// use cgetfirst/cgetnext to find them. Set them up as a vector.
// TODO rebuild the queue after a crash.
void queueInit(void){
    const char *printcapdb[2] = {"/etc/printcap", 0};
    int i;
    char* printcap_buffer;
    queueList.size = 0;
    queueList.queues = malloc(sizeof(struct queueManager)*2);
    queueList.length = 2;
    puts("here");
    //i = cgetfirst(&printcap_buffer, printcapdb);
    i =cgetfirst(&printcap_buffer, printcapdb);
    if (i == 0){
      // ERROR:
      //puts("ERROR:printcap is empty/doesn't exist");
    }
    do{
        if(queueList.size == queueList.length){
            if(realloc(queueList.queues,(unsigned long) queueList.length*2*sizeof(struct queueManager)) == NULL){
                //well shit
                // TODO ERROR OUT HERE
            }
            queueList.length *= 2;
        }
        for(int j = 0; j < (int) strlen(printcap_buffer);j++){
            // extract the name of the printer.
            if(printcap_buffer[j] == ':'){
                printcap_buffer[j] ='\0';
                queueList.queues[queueList.size].name = strdup(printcap_buffer);
                printcap_buffer[j] = ':';

                //TODO: check this call.
                queueList.queues[queueList.size].lock = malloc(sizeof(pthread_mutex_t));
                queueList.queues[queueList.size].sleep = malloc(sizeof(pthread_mutex_t));
                pthread_mutex_init(queueList.queues[queueList.size].lock, NULL);
                pthread_mutex_init(queueList.queues[queueList.size].sleep, NULL);
                queueList.size++;
                break;
            }
        }
        //TODO: fix the memory leak here. cgetnext/first malloc a buffer for the string.
        //free(printcap_buffer);
    }while((i =cgetnext(&printcap_buffer, printcapdb)) == 1);
    // right here I should go search and populate the threads.
    // Start the baby sitting of the threads
    for(int j = 0; j<queueList.size;j++){
        pthread_t *thread = malloc(sizeof(pthread_t));
        if(pthread_create(thread, NULL, manageQueue, &queueList.queues[j])){
            puts("ERROR");
        }

    }

    // TODO: rebuild the queue right here.
}

//Prints out all of the queue names.
void checkQueue(void){
  printf("%d\n",queueList.size);
  for(int i =0; i<queueList.size; i++){
    puts(queueList.queues[i].name);
  }
}

void *manageQueue(void* ptr){

    struct queueManager *queue = ptr;
    struct queueElement *current;
    printf("managing queue:%s\n",queue->name);
    pthread_mutex_lock(queue->sleep);
    while(1){
        if(queue->size == 0){
            printf("Nothing in  queue:%s, going to sleep\n",queue->name);
            pthread_mutex_lock(queue->sleep);
        }

        pthread_mutex_lock(queue->lock);
        current = pop(queue);
        printf("I woke up, and now I'm doing a job for %s",current->data->username);
        //clean up
        pthread_mutex_unlock(queue->lock);

    }


    return NULL;
}
//write the function that manages the queues.
