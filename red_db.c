#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "String.h"
#include "MsgStamp.h"
#include "Class.h"
#include "list.h"
#include "thread.h"

#include "red_db.h"

#define NUM_THREADS 3

pthread_t threads[NUM_THREADS];
pthread_attr_t attr;
int num_to_read;

struct list * ans;
struct list * real_ans;

int min(int a, int b)
{
  if(a < b)
    return a;
  else
    return b;
}

int minimum(int a, int b, int c)
{
  int temp = min(a, b);
  return min(temp, c);
}

int levenshtein(struct list * s, struct list * t)
{
  int s_size = s->size;
  int t_size = t->size;
  int d[s_size+1][t_size+1];

  int i;
  for(i = 0; i <= s_size; i++)
  {
    d[i][0] = i;
  }

  int j;
  for(j = 0; j<= t_size; j++)
  {
    d[0][j] = j;
  }

  struct listnode * s_node;
  struct listnode * t_node;
  j = 0;
  LIST_FOREACH(t, t_node)
  {
    i = 0;
    LIST_FOREACH(s, s_node)
    {
      if(strcmp(((struct MsgStamp *)t_node->data)->text,((struct MsgStamp *)s_node->data)->text) == 0)
      {
        d[i+1][j+1] = d[i][j];
      }
      else
      {
        d[i+1][j+1] = minimum
                        (
                          d[i][j+1] + 1,
                          d[i+1][j] + 1,
                          d[i][j] + 1
                        );
      }
      i++;
    }
    j++;
  }

  return d[s_size][t_size];
}


struct list * mark_from_list_by_str(char * str, struct list * list)
{
  struct list * removed = list_new();
  struct listnode * node = list->head;
  int tid_from_list; 
  while(node != NULL)
  {
    char * str_from_list = ((struct MsgStamp *)node->data)->text;
    int tid_from_list = ((struct MsgStamp *)node->data)->tid;
    struct listnode * next = node->next;
    if(strcmp(str_from_list, str) == 0)
    {
      struct listnode * node_to_add = calloc(1, sizeof(struct listnode));
      node_to_add->data = calloc(1, sizeof(int));
      memcpy(node_to_add->data, &tid_from_list, sizeof(int));
      LIST_APPEND(removed, node_to_add);

      ((struct MsgStamp *)node->data)->text = "x";
      
    }
    node = next;
  }
  return removed;
}

struct list * remove_from_list_by_str(char * str, struct list * list)
{
  struct list * removed = list_new();
  struct listnode * node = list->head;
  int tid_from_list; 
  while(node != NULL)
  {
    char * str_from_list = ((struct MsgStamp *)node->data)->text;
    int tid_from_list = ((struct MsgStamp *)node->data)->tid;
    struct listnode * prev = node->prev;
    struct listnode * next = node->next;
    if(strcmp(str_from_list, str) == 0)
    {
      if(prev != NULL)
      {
        prev = next;
      }
      if(list->head == node)
      {
        list->head = next;
      }
      if(list->tail == node)
      {
        list->tail = prev;
      }
      list->size--;

      struct listnode * node_to_add = calloc(1, sizeof(struct listnode));
      node_to_add->data = calloc(1, sizeof(int));
      memcpy(node_to_add->data, &tid_from_list, sizeof(int));
      LIST_APPEND(removed, node_to_add);

      listnode_delete(node);
      
    }
    node = next;
  }
  return removed;
}

void remove_from_list_by_tid(int tid, struct list * list)
{
  struct listnode * node = list->head;
  while(node != NULL)
  {
    int tid_from_list = ((struct MsgStamp *)node->data)->tid;
    struct listnode * prev = node->prev;
    struct listnode * next = node->next;
    if(tid == tid_from_list)
    {
      if(prev != NULL)
      {
        prev = next;
      }
      if(list->head == node)
      {
        list->head = next;
      }
      if(list->tail == node)
      {
        list->tail = prev;
      }
      list->size--;

      listnode_delete(node);
    }
    node = next;
  }
}

int data_in_output_queue(struct thr * thr_arr[])
{
  int i;
  for(i = 0; i<NUM_THREADS; i++)
  {
    if(thr_arr[i]->output_queue->size > 0)
      return 1;
  }
  return 0;
}

int num_of_messages(char * str, int tid, struct list * list_to_check)
{
  struct listnode * node_to_check;
  int similarity = 0;
  LIST_FOREACH(list_to_check, node_to_check)
  {
    if((strcmp(str, (char *)((struct MsgStamp *)node_to_check->data)->text) == 0) &&
       (tid != (int)((struct MsgStamp *)node_to_check->data)->tid))
    {
      similarity++;
    } 
  }
  return similarity;
}

struct list * vote(struct list * foq, struct list * left_behind)
{
  struct list * list_to_check = list_new();
  struct listnode * node_to_check;

//  LIST_FOREACH(foq, node_to_check)
//  {
//    printf("inside vote: %s\n", (char *)node_to_check->data);
//  }

  if(foq->size > 0)
  {
    extend(list_to_check, foq);
  }
  if(left_behind->size > 0)
  {
    extend(list_to_check, foq);
  }

  LIST_FOREACH(list_to_check,node_to_check)
  {
    char * str = ((struct MsgStamp *)node_to_check->data)->text;
    int tid = ((struct MsgStamp *)node_to_check->data)->tid;
    if(strcmp(str, "x") != 0)
    {
      if(((float)num_of_messages(str, tid, list_to_check) / (float)NUM_THREADS) > 1.0/2.0)
      {
        struct listnode * ans_node = listnode_new(String, str);
        LIST_APPEND(ans, ans_node);
        printf("ans is %s from %d\n", str, tid );
        struct list * removed = remove_from_list_by_str(str, foq);
        removed = mark_from_list_by_str(str, list_to_check);
        struct listnode * node;
        LIST_FOREACH(removed, node)
        {
          int tid = *((int *)node->data);
          printf("%d\n", tid);
          remove_from_list_by_tid(tid, left_behind);
        }
      }
    }
  }
 
  FREE_LINKED_LIST(list_to_check);

  if(left_behind->size > 0)
  {
    extend(foq, left_behind);
  }
  return foq;
}

int init(struct thr * thr_arr[])
{  
  int i;
  int rc;
  char g_query[150];
  strncpy(g_query,"CREATE TABLE IF NOT EXISTS users (uname TEXT PRIMARY KEY,pass TEXT NOT NULL,activated INTEGER)", 150);

  for(i = 0; i<NUM_THREADS; i++)
  {
    thr_arr[i] = calloc(1, sizeof(struct thr));
    thr_arr[i]->thr_id = i;

    thr_arr[i]->input_queue = list_new();
    thr_arr[i]->output_queue = list_new();

    struct listnode * node = listnode_new(String, g_query);
    LIST_APPEND(thr_arr[i]->input_queue, node);

    thr_arr[i]->p_loss = 0.0;
    thr_arr[i]->p_error = 0.0;

    if(i == 2)
    {
      thr_arr[i]->p_delay = 1.0;
    }
    else
    {
      thr_arr[i]->p_delay = 0.0;
    }

    thr_arr[i]->p_long_path = 0.0;

   
    rc = pthread_create(&threads[i], NULL, db_init, (void *)thr_arr[i]);
    if(rc)
    {
      printf("error from pthread_create() is %d\n", rc);
      exit(-1);
    }

  }
}

void dispatch_to_threads(const char * command, struct thr * thr_arr[NUM_THREADS])
{
  int i;
  for(i = 0; i<NUM_THREADS; i++)
  {
    struct listnode * node = listnode_new(String, command);
    pthread_mutex_lock(&db_mutex);
    LIST_APPEND(thr_arr[i]->input_queue, node);
    pthread_mutex_unlock(&db_mutex);
  }
}

void get_input(char * buffer)
{
  char ch;
  int i = 0;
  while((ch = getchar()) != '\n')
  {
    buffer[i++] = ch;
  }
  buffer[i] = '\0';
}

void trim_header(char * buffer)
{
  int len = strlen(buffer);
  int i;
  for(i = 0; i < len; i++)
  {
    buffer[i] = buffer[i+3];
  }
  buffer[i] = buffer[len]; // insetrt null terminating character
}

int main(int argc, char *argv[])
{
  int rc;
  int i;
  struct thr * thr_arr[NUM_THREADS];
  struct list * foq;
  char g_query[150];
  char dispatch_msg[150];
  void *status;

  pthread_mutex_init(&db_mutex, NULL);

  init(thr_arr);
  ans = list_new();
  real_ans = list_new();

  get_input(dispatch_msg);
  while(strcmp(dispatch_msg, "SELECT * from users") != 0)
  {
    if(strncmp(dispatch_msg, "i: ", 3) ==0)
    {
      init(thr_arr);
    }
    if(strncmp(dispatch_msg, "c: ", 3) == 0)
    {
      trim_header(dispatch_msg);
      dispatch_to_threads(dispatch_msg, thr_arr);
    }
    if(strncmp(dispatch_msg, "a: ", 3) == 0)
    {
      trim_header(dispatch_msg);
      struct listnode * real_ans_node = listnode_new(String,dispatch_msg);
      LIST_APPEND(real_ans, real_ans_node);
    }
    get_input(dispatch_msg);
  }

  dispatch_to_threads(dispatch_msg, thr_arr);

  struct list * left_behind = list_new();
  foq = list_new();

  while(1)
  {
    list_clear(foq);
    for(i = 0; i<NUM_THREADS; i++)
    {
      if(thr_arr[i]->output_queue->size != 0)
      {
        pthread_mutex_lock(&db_mutex);
        struct listnode * head = list_pop(thr_arr[i]->output_queue);
        pthread_mutex_unlock(&db_mutex);

        struct listnode * answer = listnode_new(MsgStamp, ((struct String *)head->data)->text, i);

        LIST_APPEND(foq, answer);
        printf("%d: extracting %s to %s with size %d\n", i, (char *)((struct String *)head->data)->text, (char *)((struct MsgStamp *)answer->data)->text, foq->size);
      }
    }
    struct listnode * node_to_check;
    LIST_FOREACH(foq, node_to_check)
    {
      printf("outside vote: %s\n", (char *)((struct MsgStamp *)node_to_check->data)->text);
    }
    vote(foq, left_behind);
//    struct listnode * ans_node;
//    LIST_FOREACH(ans, ans_node)
//    {
//      printf("ans: %s\n", (char *)((struct MsgStamp *)ans_node->data)->text);
//    }
    sleep(1);
    printf("levenshtein: %d\n", levenshtein(ans, real_ans));

//    strncpy(g_query,"SELECT * from users", 150);
//    dispatch_to_threads(g_query, thr_arr);
  } 
  pthread_exit(NULL);
}
