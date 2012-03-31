#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include "Integer.h"

void * Integer_ctor(void * _self, va_list * app)
{
  struct Integer * self = _self;
  const int val = va_arg(*app, const int);

  self->val = val;
  return self;
}

void Integer_xtor(void * _self, va_list * app)
{
  struct Integer * self = _self;
  int * val = va_arg(*app, int *);
  
  *val = self->val;
}

bool Integer_cmp(void * _self, void * _b, va_list * app)
{
  struct Integer * self = _self;
  struct Integer * b = _b;
  bool (* cmp_ptr) (int, int) = va_arg(*app, Integer_cmp_type);

  return (*cmp_ptr)(self->val, b->val);
 }

void * Integer_dup(void * _self)
{
  struct Integer * self = _self;

  return new(Integer, self->val);
}

void * Integer_dtor(void * _self)
{
  struct Integer * self = _self;

  return self;
}

