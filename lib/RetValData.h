#ifndef RETVALDATA_H
#define RETVALDATA_H

#include <stdarg.h>
#include "Class.h"

struct RetValData
{
  const void * class;
  int retval;
  char * buf;
};

typedef bool (*RetValData_cmp_type) (int, char *, int, char *);

void * RetValData_ctor(void * _self, va_list * app);
void RetValData_xtor(void * _self, va_list * app);
bool RetValData_cmp(void * _self, void * _b, va_list * app);
void * RetValData_dup(void * _self);
void * RetValData_dtor(void * _self);

static const struct Class _RetValData = 
{
  sizeof(struct RetValData), 
  RetValData_ctor,
  RetValData_xtor,
  RetValData_cmp,
  RetValData_dup,
  RetValData_dtor
};

static const void * RetValData = &_RetValData;

#endif
