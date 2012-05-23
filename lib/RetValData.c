#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include "RetValData.h"

void * RetValData_ctor(void * _self, va_list * app)
{
  struct RetValData * self = _self;
  int retval = va_arg(*app, int);
  const char * buf = va_arg(*app, const char *);

  self->buf = calloc(150, sizeof(char));
  assert(self->buf);

  self->retval = retval;
  strcpy(self->buf, buf);

  return self;
}

void RetValData_xtor(void * _self, va_list * app)
{
  struct RetValData * self = _self;
  int * retval = va_arg(*app, int *);
  const char ** buf = va_arg(*app, const char **);

  *retval = self->retval;
  *buf = self->buf;
}

bool RetValData_cmp(void * _self, void * _b, va_list * app)
{
  struct RetValData * self = _self;
  struct RetValData * b = _b;
  bool (* cmp_ptr)(int, char *, int, char *) = va_arg(*app, RetValData_cmp_type);

  return (*cmp_ptr)(self->retval, self->buf, b->retval, b->buf);
 }

void * RetValData_dup(void * _self)
{
  struct RetValData * self = _self;

  return new(RetValData, self->retval, self->buf);
}

void * RetValData_dtor(void * _self)
{
  struct RetValData * self = _self;

  free(self->buf);
  self->buf = NULL;

  return self;
}
