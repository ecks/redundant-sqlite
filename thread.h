#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
struct thr
{
  int thr_id;
  struct list * input_queue;
  struct list * output_queue;
  float p_loss;
  float p_error;
  float p_delay;
  float p_long_path;
};

#define NUM_THREADS 3

pthread_mutex_t db_mutex;

void * db_init(void * thr);
#endif
