#include <pthread.h>
#include<sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "String.h"
#include "Integer.h"
#include "list.h"
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
  struct list * delay_queue = list_new();
  struct listnode * delay_node;

  float p_loss = ((struct thr *)thr)->p_loss;
  float p_error = ((struct thr *)thr)->p_error;
  float p_delay = ((struct thr *)thr)->p_delay;
  float p_long_path = ((struct thr *)thr)->p_long_path;

  char id_name[20];
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
    if(thr_input_queue->size != 0)
    {
      pthread_mutex_lock(&db_mutex);
      struct listnode * head = list_pop(thr_input_queue);
      pthread_mutex_unlock(&db_mutex);

      struct String * str = (struct String *)head->data;
      memcpy(l_query, str->text, sizeof(char) * 150);
      listnode_delete(head);

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
          int cols = sqlite3_column_count(stmt);
          char uname[150];
          while((retval = sqlite3_step(stmt)) != SQLITE_DONE)
          {
            memset(answer, 0, 150 * sizeof(char));
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
              fflush(stdout);

              random = (rand() % 100) * 0.01;
              send = 1;
              
              if(random <= p_loss)
              {
                send = 0;
              }
              
              random = (rand() % 100) * 0.01;

              if(random <= p_error)
              {
                gen_random(answer, strlen(answer));
              }

              random = (rand() % 100) * 0.01;
              delay = 0;
          
              if(random <= p_delay)
              {
                delay = 1;
              }

              random = (rand() % 100) * 0.01;

              if(random <= p_long_path)
              {

              }
            }

            if(send)
            {
              struct listnode * node = listnode_new(String, answer);
              if(delay) // we do not send data if there is delay
              {
                printf("%s: appending to delayed queue: %s\n", id_name, ((struct String *)node->data)->text);
                LIST_APPEND(delay_queue, node);
              }
              else
              {
                if(delay_queue->size != 0) // if there is something delayed prevously, we send what was delayed previously
                {
                  delay_node = list_pop(delay_queue);
                  pthread_mutex_lock(&db_mutex);
                  LIST_APPEND(thr_output_queue, delay_node);
                  printf("%s: sending delayed node:  %s\n", id_name, ((struct String *)node->data)->text);
                  pthread_mutex_unlock(&db_mutex);
      
//                  sleep(1);

                  LIST_APPEND(delay_queue, node);
                }
                else // we can just send wormally
                {
                  pthread_mutex_lock(&db_mutex);
                  LIST_APPEND(thr_output_queue, node);
                  printf("%s: sending node: %s\n", id_name, ((struct String *)node->data)->text);
                  pthread_mutex_unlock(&db_mutex);

//                 sleep(1);
                }
              }
            }
     
            // delete the row we just sent
            char d_query[150];
            sprintf(d_query, "DELETE from users where uname = '%s'", uname);
            retval = sqlite3_exec(handle,d_query,0,0,0);
            if(retval)
            {
              printf("Unable to delete from users\n");
            }
          }

          while(delay_queue->size > 0) // andydelayed messages left over should be sent now
          {
            delay_node = list_pop(delay_queue);
            pthread_mutex_lock(&db_mutex);
            LIST_APPEND(thr_output_queue, delay_node);
            printf("%s: sending delayed node: %s\n", id_name, ((struct String *)delay_node->data)->text);
            pthread_mutex_unlock(&db_mutex);
          }
        }
        
//        printf("exiting from thread...\n");
//        break;
      }

      retval = sqlite3_exec(handle,l_query,0,0,0);
      retvalnode = listnode_new(Integer, retval);
      LIST_APPEND(thr_output_queue, retvalnode);
    
    }
  }

  pthread_exit(NULL);
}
