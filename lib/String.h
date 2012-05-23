#ifndef STRING_H
#define STRING_H

#include <stdarg.h>
#include "Class.h"

struct String
{
  const void * class;
  char * text;
};

typedef bool (*String_cmp_type) (char *, char *);

void * String_ctor(void * _self, va_list * app);
void String_xtor(void * _self, va_list * app);
bool String_cmp(void * _self, void * _b, va_list * app);
void * String_dup(void * _self);
void * String_dtor(void * _self);

static const struct Class _String = 
{
  sizeof(struct String), 
  String_ctor,
  String_xtor,
  String_cmp,
  String_dup,
  String_dtor
};

static const void * String = &_String;

#endif
