#include<pthread.h>
#include"../common/common.h"
#ifndef threadpool_t
#define threadpool_t

#define MAX_THREAD_NUM 16
#define STARTING_THREAD_NUM 4
#define THREAD_NUM 4
#define FILENAME_MAXLENGTH 32
// I need a condition variable for thread availablity.

struct test_Struct{
  int a;
};
struct v_thread{
  struct server_thread *data;
  int size;
  int current;
};
struct server_thread{
    pthread_t *thread;
    pthread_mutex_t *lock;
    struct printer_st *printer;
    int *data;
    int *working;
};

int thread_pool_init();
int job_request(void* data); //I don't know what structure will go in
int return_thread(pthread_t* worker);
int getID();
void* worker_thread(void* data_pointer);
int requestJob(int input);

#endif

// I need to write a vector array for holding the data.
