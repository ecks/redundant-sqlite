#include <pthread.h>
#include<sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "lib/String.h"
#include "lib/RetValData.h"
#include "lib/Integer.h"
#include "lib/list.h"
#include "lib/stream.h"
#include "thread.h"

void gen_random(char * s, const int len)
{ 
  static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

void * db_init(void * thr)
{
  char l_query[150];
  char answer[150];

  int retval;
  struct listnode * retvalnode;

  float random;
  int send;
  int delay;

  sqlite3 *handle;
  sqlite3_stmt *stmt;

  answer[0] = '\0';
  struct list * thr_input_queue = ((struct thr *)thr)->input_queue;
  struct list * thr_output_queue = ((struct thr *)thr)->output_queue;

  struct stream * thr_input_stream = ((struct thr *)thr)->input_stream;
  struct stream * thr_output_stream = ((struct thr *)thr)->output_stream;

  struct list * delay_queue = list_new();
  struct listnode * delay_node;

  float * p_loss = &(((struct thr *)thr)->p_loss);
  float * p_error = &(((struct thr *)thr)->p_error);
  float * p_delay = &(((struct thr *)thr)->p_delay);
  float * p_long_path = &(((struct thr *)thr)->p_long_path);

  char id_name[20]; // id of thread

  int stream_size; // size of data in stream

  int cols;

  sprintf(id_name, "%d", ((struct thr *)thr)->thr_id);
  retval = sqlite3_open(id_name,&handle);
  if(retval)
  {
    printf("Database connection failed\n");
    pthread_exit(NULL);
  }
  printf("Connection successful\n");
  fflush(stdout);

  srand(time(NULL));

  while(1)
  { 
    if(STREAM_READABLE(thr_input_stream) != 0)
    {
      pthread_mutex_lock(&db_mutex);
      stream_size = stream_getl(thr_input_stream);
      stream_get(l_query, thr_input_stream, stream_size);
      stream_reset(thr_input_stream);
      pthread_mutex_unlock(&db_mutex);

      l_query[stream_size] = '\0';  // null terminate our characters


      printf("%s: %s\n", id_name, l_query);
      fflush(stdout);

      if(strncmp(l_query, "SELECT", 6) == 0)
      {
        retval = sqlite3_prepare_v2(handle,l_query,-1,&stmt,0);
        if(retval)
        {
          printf("Selecting data from DB Failed\n");
        }
        else
        {
          cols = sqlite3_column_count(stmt);
          char uname[150];
          memset(answer, 0, 150 * sizeof(char));
          while((retval = sqlite3_step(stmt)) != SQLITE_DONE)
          {
            for(int col=0 ; col<cols;col++)
            {
              char loc_ans[150];
              const char * name;
              const char * text;
              name = sqlite3_column_name(stmt,col);
              text = sqlite3_column_text(stmt,col);
              if(strcmp(name, "uname") == 0)
              {
                strncpy(uname, text, 150);
              }
              sprintf(loc_ans, "%s = %s",name,text);
              strncat(answer, loc_ans, strlen(loc_ans));
              printf("%s: answer: %s\n", id_name, answer);             
            }
          }

          random = (rand() % 100) * 0.01;
          send = 1;
              
          pthread_mutex_lock(&db_mutex);
          if(random <= *p_loss)
          {
            send = 0;
          }
          pthread_mutex_unlock(&db_mutex);
              
          random = (rand() % 100) * 0.01;

          pthread_mutex_lock(&db_mutex);
          if(random <= *p_error)
          {
            gen_random(answer, strlen(answer));
          }
          pthread_mutex_unlock(&db_mutex);

          random = (rand() % 100) * 0.01;
          delay = 0;
          
          pthread_mutex_lock(&db_mutex);
          if(random <= *p_delay)
          {
            delay = 1;
          }
          pthread_mutex_unlock(&db_mutex);

          random = (rand() % 100) * 0.01;

          pthread_mutex_lock(&db_mutex);
          if(random <= *p_long_path)
          {

          }
          pthread_mutex_unlock(&db_mutex);

          if(send)
          {
            if(delay) // we delay sending of the data
            {
              struct listnode * node = listnode_new(RetValData, retval, answer);
              printf("%s: appending to delayed queue: %s\n", id_name, answer);
              LIST_APPEND(delay_queue, node);
            }
            else
            {
              if(delay_queue->size != 0) // if there is something delayed prevously, we send what was delayed previously
              {
                struct listnode * node = listnode_new(RetValData, retval, answer);
                char * delay_answer;

                delay_node = list_pop(delay_queue);
                listnode_extract(delay_node, &retval, &delay_answer);
                printf("%s: sending delayed node:  %s\n", id_name, delay_answer);
                int len = strlen(delay_answer);
                pthread_mutex_lock(&db_mutex);
                stream_putl(thr_output_stream, RET_VAL_LIST);
                stream_putl(thr_output_stream, retval);
                stream_putl(thr_output_stream, len);
                stream_put(thr_output_stream, delay_answer, strlen(delay_answer));
                pthread_mutex_unlock(&db_mutex);
      
                LIST_APPEND(delay_queue, node);
              }
              else // we can just send normally
              {
                printf("about to send data...\n");
                int len = strlen(answer);
                pthread_mutex_lock(&db_mutex);
                stream_putl(thr_output_stream, RET_VAL_LIST);
        	stream_putl(thr_output_stream, retval);
                stream_putl(thr_output_stream, len);
                stream_put(thr_output_stream, answer, strlen(answer));
                pthread_mutex_unlock(&db_mutex);
              }
            }
          }
     
            // delete the row we just sent
//            char d_query[150];
//            sprintf(d_query, "DELETE from users where uname = '%s'", uname);
//            retval = sqlite3_exec(handle,d_query,0,0,0);
//            if(retval)
//            {
//              printf("Unable to delete from users\n");
//            }

//          while(delay_queue->size > 0) // any delayed messages left over should be sent now
//          {
//            delay_node = list_pop(delay_queue);
//            pthread_mutex_lock(&db_mutex);
//            LIST_APPEND(thr_output_queue, delay_node);
//            printf("%s: sending delayed node: %s\n", id_name, ((struct String *)delay_node->data)->text);
//            pthread_mutex_unlock(&db_mutex);
//          }
        }
        
      }
      else
      {
        retval = sqlite3_exec(handle,l_query,0,0,0);
        printf("%s: answer: %d\n", id_name, retval);             

        random = (rand() % 100) * 0.01;
        send = 1;
              
        pthread_mutex_lock(&db_mutex);
        if(random <= *p_loss)
        {
          send = 0;
        }
        pthread_mutex_unlock(&db_mutex);
              
        random = (rand() % 100) * 0.01;

        pthread_mutex_lock(&db_mutex);
        if(random <= *p_error)
        {
          gen_random(answer, strlen(answer));
        }
        pthread_mutex_unlock(&db_mutex);

        random = (rand() % 100) * 0.01;
        delay = 0;
          
        pthread_mutex_lock(&db_mutex);
        if(random <= *p_delay)
        {
          delay = 1;
        }
        pthread_mutex_unlock(&db_mutex);

        random = (rand() % 100) * 0.01;

        pthread_mutex_lock(&db_mutex);
        if(random <= *p_long_path)
        {

        }
        pthread_mutex_unlock(&db_mutex);

        if(send)
        {
          pthread_mutex_lock(&db_mutex);

          stream_putl(thr_output_stream, RET_VAL);
          stream_putl(thr_output_stream, retval);
          pthread_mutex_unlock(&db_mutex);
        }
      }
    }
  }

  pthread_exit(NULL);
}
