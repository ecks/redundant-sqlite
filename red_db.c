#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <regex.h>
#include "lib/String.h"
#include "lib/Integer.h"
#include "lib/MsgStamp.h"
#include "lib/Class.h"
#include "lib/list.h"
#include "lib/stream.h"
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

/*int levenshtein(struct list * s, struct list * t)
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
*/

bool compare_by_buf_and_len(void * buf_a, int len_a, int tid_a, void * buf_b, int len_b, int tid_b)
{
  if(len_a != len_b)
    return false;

  if(tid_a == tid_b)
    return false;

  if(memcmp(buf_a, buf_b, len_a) == 0)
    return true;

  return false;
}

bool compare_by_buf_and_len_only(void * buf_a, int len_a, int tid_a, void * buf_b, int len_b, int tid_b)
{
  if(len_a != len_b)
    return false;

  if(memcmp(buf_a, buf_b, len_a) == 0)
    return true;

  return false;
}

struct list * mark_from_list_by_str(struct listnode * mark_by_node, struct list * list)
{
  struct list * removed = list_new();
  struct listnode * node = list->head;
  int tid_from_list; 
  while(node != NULL)
  {
    void * str_from_list;
    int len;
    int tid_from_list;
    listnode_extract(node, &str_from_list, &len, &tid_from_list);
//    void * str_from_list = ((struct MsgStamp *)node->data)->buf;
//    int tid_from_list = ((struct MsgStamp *)node->data)->tid;
    struct listnode * next = node->next;
    if(listnode_compare_by(node, mark_by_node, compare_by_buf_and_len_only) == true)
    {
      struct listnode * node_to_add = listnode_new(Integer, tid_from_list);
      LIST_APPEND(removed, node_to_add);

      ((struct MsgStamp *)node->data)->buf = NULL;
      
    }
    node = next;
  }
  return removed;
}

struct list * remove_from_list_by_node(struct listnode * remove_by_node, struct list * list)
{
  struct list * removed = list_new();
  struct listnode * node = list->head;
  int tid_from_list; 
  while(node != NULL)
  {
    void * buf_from_list;
    int len_from_list;
    int tid_from_list;
    listnode_extract(node, &buf_from_list, &len_from_list, &tid_from_list);
    struct listnode * prev = node->prev;
    struct listnode * next = node->next;
    if(listnode_compare_by(node, remove_by_node, compare_by_buf_and_len_only) == true)
    {
      printf("about to delete node(%d) val: %d response: %s\n", tid_from_list, *((int *)buf_from_list), (((char *)buf_from_list)+4));
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

      struct listnode * node_to_add = listnode_new(Integer, tid_from_list);
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

int num_of_messages(struct listnode * compare_to_node, struct list * list_to_check)
{
  struct listnode * node_to_check;
  int similarity = 0;
  LIST_FOREACH(list_to_check, node_to_check)
  {
    if(listnode_compare_by(node_to_check, compare_to_node, compare_by_buf_and_len_only) == true)
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
    list_extend(list_to_check, foq);
  }
  if(left_behind->size > 0)
  {
    list_extend(list_to_check, foq);
  }

  LIST_FOREACH(list_to_check,node_to_check)
  {
    void * buf;
    int len;
    int tid;
    listnode_extract(node_to_check, &buf, &len, &tid);
    if(buf != NULL)
    {
      if(((float)num_of_messages(node_to_check, list_to_check) / (float)NUM_THREADS) > 1.0/2.0)
      {
//        struct listnode * ans_node = listnode_new(String, str);
//        LIST_APPEND(ans, ans_node);
//        printf("ans is %s from %d\n", str, tid );
        printf("ans node(%d) val: %d response: %s\n", tid, *((int *)buf), (((char *)buf)+4));

        struct listnode * node_to_check_dup = listnode_dupl(node_to_check);
        struct list * removed = remove_from_list_by_node(node_to_check_dup, foq);
        removed = remove_from_list_by_node(node_to_check_dup, list_to_check);
        struct listnode * node;
        LIST_FOREACH(removed, node)
        {
          int tid_to_remove;
          listnode_extract(node, &tid_to_remove);
          printf("%d\n", tid_to_remove);
          remove_from_list_by_tid(tid_to_remove, left_behind);
        }
        listnode_delete(node_to_check_dup);
      }
    }
  }
 
  FREE_LINKED_LIST(list_to_check);

  if(left_behind->size > 0)
  {
    list_extend(foq, left_behind);
  }
  return foq;
}

int vote_on_retval(struct list * retlist)
{
  int retval;

  struct listnode * retvalnode = list_pop(retlist); 
  listnode_extract(retvalnode, &retval);

  struct listnode * retvalnode_tmp;

//  ((struct Integer *)retvalnode->data)->val;
  LIST_FOREACH(retlist, retvalnode_tmp)
  {
    int retval_tmp;
    listnode_extract(retvalnode_tmp, &retval_tmp);
    if(retval_tmp == retval)
    {
      retval = retval_tmp;
    }
    else
      return -1;
  }

  return retval; 
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

    thr_arr[i]->input_stream = stream_new(4096);
    thr_arr[i]->output_stream = stream_new(4096);

    stream_putl(thr_arr[i]->input_stream, strlen(g_query));
    stream_put(thr_arr[i]->input_stream, g_query, strlen(g_query));

//    struct listnode * node = listnode_new(String, g_query);
//    LIST_APPEND(thr_arr[i]->input_queue, node);

    thr_arr[i]->p_loss = 0.0;

    if(i == 2)
    {
      thr_arr[i]->p_error = 1.0;
    }
    else
    {
      thr_arr[i]->p_error = 0.0;
    }

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

  // wait for retval (if it is there
  struct listnode * head;
  struct listnode * retvalnode;
  struct list * retlist = list_new();
  int retval;
  int nreadfrom = 0;
  sleep(1);
//  while(nreadfrom < NUM_THREADS)
//  {
    for(i = 0; i<NUM_THREADS; i++)
    {
      if(STREAM_READABLE(thr_arr[i]->output_stream) != 0)
      {
        pthread_mutex_lock(&db_mutex);
        int type = stream_getl(thr_arr[i]->output_stream);
        int val = stream_getl(thr_arr[i]->output_stream);
//        struct listnode * head = list_pop(thr_arr[i]->output_queue);
        pthread_mutex_unlock(&db_mutex);

        retvalnode = listnode_new(Integer, val);
        LIST_APPEND(retlist, retvalnode);
        nreadfrom++;
      }
    }
//  }

  if(retlist->size != 0)
  {
    retval = vote_on_retval(retlist);
    return retval;
  }
  else
  {
    return -1;
  }
}

void dispatch_to_threads(char * command, struct thr * thr_arr[NUM_THREADS])
{
  int i;
  int len = strlen(command);
  for(i = 0; i<NUM_THREADS; i++)
  {
//    struct listnode * node = listnode_new(String, command);
    pthread_mutex_lock(&db_mutex);
    stream_putl(thr_arr[i]->input_stream, strlen(command));
    stream_put(thr_arr[i]->input_stream, command, strlen(command));
//    LIST_APPEND(thr_arr[i]->input_queue, node);
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
  for(i = 0; (i+3) < len; i++)
  {
    buffer[i] = buffer[i+3];
  }
  buffer[i] = buffer[len]; // insetrt null terminating character
}

void poll_dispatch_input(char * dispatch_msg, struct thr * thr_arr[NUM_THREADS])
{
  struct pollfd fd;
  fd.fd = STDIN_FILENO;
  fd.events = POLLIN;
  fd.revents = 0;
  int ret = poll(&fd, 1, 0); // 1 fd, return right away (third argument is 0)
  if(ret > 0 && (fd.revents & POLLIN != 0))
  {
    get_input(dispatch_msg);

    // if command, then dispatch to threads
    if(strncmp(dispatch_msg, "c: ", 3) == 0)
    {
      trim_header(dispatch_msg);
      dispatch_to_threads(dispatch_msg, thr_arr);
    }

    // if request to change probability of loss change it and send update
    if(strncmp(dispatch_msg, "p: l", 4) == 0)
    {
      regex_t re;
      int num_matches = 1;
      regmatch_t matches[num_matches]; // one match for whole digit
      char dst[10];
      float dst_f;
      char * msg_p = dispatch_msg;
      const char * pattern = "([[:digit:]]+)[.]([[:digit:]]+)";
      int nomatch = 0;
      if(regcomp(&re, pattern, REG_EXTENDED) != 0)
      {
        printf("Error on a regex compilation\n");   
      }
      int i = 0;
      while(!nomatch)
      {
        nomatch = regexec(&re, msg_p, num_matches, matches, 0);
        if (nomatch)
        {
          printf("No more matches\n");
          regfree(&re);
        }
        else
        {
          printf("Start of offset: %d and end of offset: %d\n", matches[0].rm_so, matches[0].rm_eo);
          strncpy(dst, msg_p + matches[0].rm_so, matches[0].rm_eo - matches[0].rm_so);
          dst[matches[0].rm_eo - matches[0].rm_so] = '\0';
          dst_f = strtof(dst, NULL);
          printf("%f\n", dst_f);
          printf("Setting thr_arr[%d]->p_loss\n", i);
          pthread_mutex_lock(&db_mutex);
          thr_arr[i]->p_loss = dst_f;      
          pthread_mutex_unlock(&db_mutex);
          msg_p += matches[0].rm_eo;
        }
        i++;
      }
    }

    // if request to change probability of delay change it and send update
    if(strncmp(dispatch_msg, "p: d", 4) == 0)
    {
      for(int i = 0; i < NUM_THREADS; i++)
      { 
        printf("Setting thr_arr[%d]->p_delay\n", i);
        pthread_mutex_lock(&db_mutex);
        thr_arr[i]->p_delay = 0.0;      
        pthread_mutex_unlock(&db_mutex);
      }
    }
     // if answer, then record it in answer list
//    if(strncmp(dispatch_msg, "a: ", 3) == 0)
//    {
//      trim_header(dispatch_msg);
//      struct listnode * real_ans_node = listnode_new(String,dispatch_msg);
//      LIST_APPEND(real_ans, real_ans_node);
//    }

    if(strcmp(dispatch_msg, "SELECT * from users") == 0)
    {
      dispatch_to_threads(dispatch_msg, thr_arr);
    }

    if(strcmp(dispatch_msg, "quit") == 0)
    {
      exit(0);
    }
  }
  else
  { 
    // no data can be read at this time
  }
}


int main(int argc, char *argv[])
{
  int rc;
  int retval;
  int i;
  int type = -1;
  int val;
  struct thr * thr_arr[NUM_THREADS];
  struct list * foq;
  char g_query[150];
  char dispatch_msg[150];
  char response[150];
  void *status;
  void * msg_buffer;
  void * b_ptr;
  int total_len;
  
  struct listnode * answer_node; // getting answer from stream

  pthread_mutex_init(&db_mutex, NULL);

  if((retval = init(thr_arr)) < 0)
  {
    printf("Error on initialization\n");
  }
  printf("Ret on init: %d\n", retval);
 
//  ans = list_new();
  real_ans = list_new();

  struct list * left_behind = list_new();
  foq = list_new();

  // initialize message buffer
  msg_buffer = calloc(150, sizeof(void));

  while(1)
  {
    // reset values here for next round
    type = -1;

    // poll for input
    poll_dispatch_input(dispatch_msg, thr_arr);

    sleep(1);  // wait a little bit for other threads to produce result

    list_clear(foq);
    for(i = 0; i<NUM_THREADS; i++)
    {
      total_len = 0;
      if(STREAM_READABLE(thr_arr[i]->output_stream) != 0)
      {
        pthread_mutex_lock(&db_mutex);
        type = stream_getl(thr_arr[i]->output_stream);
        pthread_mutex_unlock(&db_mutex);
        if(type == RET_VAL)
        {
          pthread_mutex_lock(&db_mutex);
          val = stream_getl(thr_arr[i]->output_stream);
          stream_reset(thr_arr[i]->output_stream);
          pthread_mutex_unlock(&db_mutex);
          int len = 4; 
          memcpy(msg_buffer, (void *)&val, len);
          total_len += len;
          
          printf("ret_val(%d) val: %d\n", i, val);
         
          answer_node = listnode_new(Integer, val);
        }
        else if(type == RET_VAL_LIST)
        {
          pthread_mutex_lock(&db_mutex);
          val = stream_getl(thr_arr[i]->output_stream);
          pthread_mutex_unlock(&db_mutex);
          int len = 4;
   
          b_ptr = msg_buffer;
          memcpy(msg_buffer, (void *)&val, len);
          b_ptr += len;
          total_len += len;

          pthread_mutex_lock(&db_mutex);
          len = stream_getl(thr_arr[i]->output_stream);
          stream_get(response, thr_arr[i]->output_stream, len);
          stream_reset(thr_arr[i]->output_stream);
          pthread_mutex_unlock(&db_mutex);
          memcpy(b_ptr, (void *)response, len);
          total_len += len;     

          printf("ret_val_list(%d) val: %d response: %s\n", i, val, response);

          answer_node = listnode_new(MsgStamp, (void *)msg_buffer, total_len, i);
        }

        LIST_APPEND(foq, answer_node);
//        printf("%d: extracting %s to %s with size %d\n", i, (char *)((struct String *)head->data)->text, (char *)((struct MsgStamp *)answer->data)->text, foq->size);
      }
    }
    if(type == RET_VAL)
    {
      retval = vote_on_retval(foq);
      printf("Ret on command: %d\n", retval);
    }
    else if(type == RET_VAL_LIST)
    {
      struct listnode * node_to_check;
      LIST_FOREACH(foq, node_to_check)
      {
        void * buf;
        int len;
        int tid;
        listnode_extract(node_to_check, &buf, &len, &tid);
        printf("outside vote(%d) val: %d response: %s\n", tid, *((int *)buf), (((char *)buf)+4));
      }
      vote(foq, left_behind);
//      printf("levenshtein: %d\n", levenshtein(ans, real_ans));
    }
//    strncpy(g_query,"SELECT * from users", 150);
//    dispatch_to_threads(g_query, thr_arr);
  } 
  pthread_exit(NULL);
}
