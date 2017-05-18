#ifndef PRINTQUEUE_H
#define PRINTQUEUE_H
//change this back
#include<pthread.h>
#include<semaphore.h>
#include <sys/stat.h>
#include "print_job.h"

struct queueElement{
    struct job *data; // this is going to be a job struct
    struct queueElement *previous;
    //char padding[8];
};

//TODO: check if the queue manager needs data/settings from the printcap as well
struct queueManager{
    struct queueElement *head;
    struct queueElement *tail;
    pthread_mutex_t *lock;
    //pthread_mutex_t *sleep;
    sem_t *test;
    char* name;  
    time_t last_access;
    int size;
    char padding[4];
};

struct queueVector{
    struct queueManager *queues;
    int length;
    int size;
    int watched;
    char padding[4];
};

struct queueElement* pop(struct queueManager *queue);
int addElement(struct job *input);

int queueEdit(struct job *data);
void queueInit(void);


void babysitQueue(void);
void needManagers(void);

// stuff added for testing
void checkQueue(void);
struct queueManager* findQueue(char* queueName);
void checkPrintcap(struct queueManager* queue);
#endif
