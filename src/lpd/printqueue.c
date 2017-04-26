#include "printqueue.h"

#include <stdlib.h>
#include <stdio.h>
int main (){
    struct queueManager queue;
    queue.size = 0;
    struct queueElement *current = malloc(sizeof(struct queueElement));
    current->name = "1";
    addElement(&queue,current);
    current = NULL;

    current = malloc(sizeof(struct queueElement));
    current->name = "2";
    addElement(&queue,current);

    current = malloc(sizeof(struct queueElement));
    current->name = "3";
    addElement(&queue,current);

    queueEdit(&queue, 3);
    current = pop(&queue);
    puts(current->name);
    current = pop(&queue);
    puts(current->name);
    current = pop(&queue);
    puts(current->name);

    return 0;
}

void addElement(struct queueManager *queue, struct queueElement *E){

    if(queue->size == 0){
        queue->head = E;
        queue->tail = E;
    }
    else{
        E->previous = NULL;
        queue->tail->previous = E;
        queue->tail = E;
    }

    queue->size++;
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
