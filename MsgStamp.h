#ifndef MSGSTAMP_H
#define MSGSTAMP_H

#include <stdarg.h>
#include "String.h"
#include "Class.h"

struct MsgStamp
{
  const void * class;
  char * text;
  int tid;
};

void * MsgStamp_ctor(void * _self, va_list * app);
void * MsgStamp_dup(void * _self);
void * MsgStamp_dtor(void * _self);

static const struct Class _MsgStamp = 
{
  sizeof(struct MsgStamp), 
  MsgStamp_ctor,
  MsgStamp_dup,
  MsgStamp_dtor
};

static const void * MsgStamp = &_MsgStamp;

#endif
