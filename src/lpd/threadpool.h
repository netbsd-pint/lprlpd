#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include "common.h"

#define MAX_THREAD_NUM 16
#define STARTING_THREAD_NUM 4
#define THREAD_NUM 4
#define FILENAME_MAXLENGTH 32
// I need a condition variable for thread avail ablity.

int thread_pool_init(void);
int job_request(void* data); //I don't know what structure will go in
int return_thread(pthread_t* worker);
int getID(void);
void* worker_thread(void* data_pointer);
int requestJob(int input);

struct test_Struct{
  int a;
};
struct v_thread{
  struct server_thread *data;
  unsigned long size;
  int current;
  int padding;
};
struct server_thread{
    pthread_t *thread;
    pthread_mutex_t *lock;
    struct printer_st *printer;
    int *data;
    int *working;
};

#endif

// I need to write a vector array for holding the data.
