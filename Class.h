#ifndef CLASS_H
#define CLASS_H

#include <string.h>
#include <stdarg.h>

struct Class
{
  size_t size;
  void * (* ctor) (void * self, va_list * app);
  void * (* dupl) (void * self);
  void * (* dtor) (void * self);
};

void * new(const void * _class, ...);
void * vnew(const void * _class, va_list * app);
void * dupl(void * self);
void delete(void * self);

#endif
