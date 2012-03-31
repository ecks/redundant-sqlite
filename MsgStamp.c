#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include "MsgStamp.h"

void * MsgStamp_ctor(void * _self, va_list * app)
{
  struct MsgStamp * self = _self;
  const void * buf = va_arg(*app, const void *);
  int len = va_arg(*app, int);
  int tid = va_arg(*app, int);

  self->buf = calloc(150, sizeof(void));
  assert(self->buf);

  memcpy(self->buf, buf, sizeof(void) * len);

  self->len = len;
  self->tid = tid;
  
  return self;
}

void MsgStamp_xtor(void * _self, va_list * app)
{
  struct MsgStamp * self = _self;
  const void ** buf = va_arg(*app, const void **);
  int * len = va_arg(*app, int *);
  int * tid = va_arg(*app, int *);

  *buf = self->buf;
  *len = self->len;
  *tid = self->tid;
}

bool MsgStamp_cmp(void * _self, void * _b, va_list * app)
{
  struct MsgStamp * self = _self;
  struct MsgStamp * b = _b;
  bool (* cmp_ptr) (void *, int, int, void *, int, int) = va_arg(*app, MsgStamp_cmp_type);

  return (*cmp_ptr)(self->buf, self->len, self->tid, b->buf, b->len, b->tid);
 }

void * MsgStamp_dup(void * _self)
{
  struct MsgStamp * self = _self;

  return new(MsgStamp, self->buf, self->len, self->tid);
}

void * MsgStamp_dtor(void * _self)
{
  struct MsgStamp * self = _self;

  free(self->buf);
  self->buf = NULL;

  return self;
}
