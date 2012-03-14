#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include "Class.h"

void * new(const void * _class, ...)
{
  va_list ap;
  va_start(ap, _class);
  void * new = vnew(_class, &ap);
  va_end(ap);

  return new;
}

void * vnew(const void * _class, va_list * app)
{
  const struct Class * class = _class;
  void * p = calloc(1, class->size);
  
  assert(p);
  *(const struct Class **) p = class;
  
  if(class->ctor)
  {
    p = class->ctor(p, app);
  }

  return p;
}

void * dupl(void * self)
{
  const struct Class ** cp = self;

  if(self && *cp && (*cp)->dupl)
    return (*cp)->dupl(self);
  else
    return NULL;
}

void delete(void * self)
{
  const struct Class ** cp = self;
 
  if(self && *cp && (*cp)->dtor)
  {
    self = (*cp)->dtor(self);
  }

  free(self);
}
