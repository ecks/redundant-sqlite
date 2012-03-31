#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
struct thr
{
  int thr_id;
  struct list * input_queue;
  struct list * output_queue;
  struct stream * input_stream;
  struct stream * output_stream;
  float p_loss;
  float p_error;
  float p_delay;
  float p_long_path;
};

#define NUM_THREADS 3

#define RET_VAL      0
#define RET_VAL_LIST 1

pthread_mutex_t db_mutex;

void * db_init(void * thr);
#endif
