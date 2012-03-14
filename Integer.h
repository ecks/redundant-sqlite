#ifndef INTEGER_H
#define INTEGER_H

#include <stdarg.h>
#include "Class.h"

struct Integer
{
  const void * class;
  int val;
};

void * Integer_ctor(void * _self, va_list * app);
void * Integer_dup(void * _self);
void * Integer_dtor(void * _self);

static const struct Class _Integer = 
{
  sizeof(struct Integer), 
  Integer_ctor,
  Integer_dup,
  Integer_dtor
};

static const void * Integer = &_Integer;

#endif
