#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "printqueue.h"
#include "../common/common.h"
#include "../common/print_job.h"

static struct queueVector queueList;

struct queueManager* findQueue(char* queueName);

// TODO add in mutexes for every function.




// 0 is success, 1 is error.
// TODO: add in a mutex for adding.
// TODO: change input to just pointer to job struct.
int addElement(struct job *input){

    struct queueManager *queue = findQueue(input->p->name);

    struct queueElement *current = malloc(sizeof(struct queueElement));
    current->data = input;

    if(queue->size == 0){
        queue->head = current;
        queue->tail = current;
    }

    else{
        current->previous = NULL;
        queue->tail->previous = current;
        queue->tail = current;
    }

    queue->size++;
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

// TODO: rewrite for however we want to do this, IE queuemanager-> string.
void queueEdit(struct queueManager *queue,int index){
    if(index >= queue->size || index == 0){
        return;
    }
    struct queueElement *current = queue->head;
    struct queueElement *newhead;
    for(int i=0; i<index-1;i++){
        current = current->previous;
    }
    newhead = current->previous;
    current->previous = current->previous->previous;
    newhead->previous = queue->head;
    queue->head = newhead;


}
 // edit to take in the job struct for printer name.

struct queueManager* findQueue(char* queueName){
    //Looks through whatever I'm using to store
    queueName = " ";
    return NULL;
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

    //i = cgetfirst(&printcap_buffer, printcapdb);

    while((i =cgetnext(&printcap_buffer, printcapdb)) == 0){

        if(queueList.size == queueList.length){
            if(realloc(queueList.queues,(unsigned long) queueList.length*2*sizeof(struct queueManager)) == NULL){
                //well shit
                // TODO ERROR OUT HERE
            }
            queueList.length *= 2;
        }
        for(int j = 0; j < (int) strlen(printcap_buffer);j++){
            // extract the name of the printer.
            if(printcap_buffer[i] == ':'){
                printcap_buffer[i] ='0';
                queueList.queues[queueList.size].name = strdup(printcap_buffer);
                printcap_buffer[i] = ':';
                queueList.size++;
                break;
            }
        }
    }
}

//write the function that manages the queues.
