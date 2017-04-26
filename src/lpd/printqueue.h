#include "../common/printjob.h"

#define queue
#ifndef queue
struct testStruct{
    int a;
    char b[4];
    int c[20];
};

struct queueElement{
    char* name;
    struct testStruct temp; // this is going to be a job struct
    struct queueElement *previous;
};

struct queueManager{
    int size;
    struct queueElement *head;
    struct queueElement *tail;
    char* name;
};

void addElement(struct queueManager *queue, struct queueElement *E);
struct queueElement* pop(struct queueManager *queue);
void queueEdit(struct queueManager* queue, int index);

#endif
