#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "String.h"
#include "MsgStamp.h"

void * MsgStamp_ctor(void * _self, va_list * app)
{
  struct MsgStamp * self = _self;
  const char * text = va_arg(*app, const char *);
  int tid = va_arg(*app, int);

  self->text = calloc(150, sizeof(char));
  assert(self->text);
  strcpy(self->text, text);

  self->tid = tid;
  
  return self;
}

void * MsgStamp_dup(void * _self)
{
  struct MsgStamp * self = _self;

  return new(MsgStamp, self->text, self->tid);
}

void * MsgStamp_dtor(void * _self)
{
  struct MsgStamp * self = _self;

  free(self->text);
  self->text = 0;

  return self;
}
