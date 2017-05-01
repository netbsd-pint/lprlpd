#ifndef PRINTQUEUE_H
#define PRINTQUEUE_H
#include "../common/print_job.h"



struct queueElement{
    struct job *data; // this is going to be a job struct
    struct queueElement *previous;
    //char padding[8];
};

struct queueManager{
    int size;
    char padding3[4];
    struct queueElement *head;
    //char padding2[8];
    struct queueElement *tail;
    char* name;
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

void queueEdit(struct queueManager* queue, int index);
void queueInit(void);

void babysitQueue(void);
void needManagers(void);

#endif
