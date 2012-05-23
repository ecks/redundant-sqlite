#ifndef MSGSTAMP_H
#define MSGSTAMP_H

#include <stdarg.h>
#include "Class.h"

struct MsgStamp
{
  const void * class;
  void * buf;
  int len;
  int tid;
};

typedef bool (*MsgStamp_cmp_type) (void *, int, int, void *, int, int);

void * MsgStamp_ctor(void * _self, va_list * app);
void MsgStamp_xtor(void * _self, va_list * app);
bool MsgStamp_cmp(void * _self, void * _b, va_list * app);
void * MsgStamp_dup(void * _self);
void * MsgStamp_dtor(void * _self);

static const struct Class _MsgStamp = 
{
  sizeof(struct MsgStamp), 
  MsgStamp_ctor,
  MsgStamp_xtor,
  MsgStamp_cmp,
  MsgStamp_dup,
  MsgStamp_dtor
};

static const void * MsgStamp = &_MsgStamp;

#endif
