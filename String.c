#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "String.h"

void * String_ctor(void * _self, va_list * app)
{
  struct String * self = _self;
  const char * text = va_arg(*app, const char *);

  self->text = calloc(150, sizeof(char));
  assert(self->text);
  strcpy(self->text, text);
  return self;
}

void String_xtor(void * _self, va_list * app)
{
  struct String * self = _self;
  const char * text = va_arg(*app, const char *);
  
  text = self->text;
}

bool String_cmp(void * _self, void * _b, va_list * app)
{
  struct String * self = _self;
  struct String * b = _b;
  bool (* cmp_ptr) (char *, char *) = va_arg(*app, String_cmp_type);

  return (*cmp_ptr)(self->text, b->text);
 }

void * String_dup(void * _self)
{
  struct String * self = _self;

  return new(String, self->text);
}

void * String_dtor(void * _self)
{
  struct String * self = _self;

  free(self->text);
  self->text = 0;
  return self;
}

