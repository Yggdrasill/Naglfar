#include <stdio.h>
#include "struct.h"
#include "../naglfar.h"

/* An ugly example of how to use it, it's not clean or elegant at all, but it works. */
int main(void)
{
  char *path[2] = {"/path/to/plugin1",
                  "/path/to/plugin2"};
  char *name[2] = {"abc", "bac"};
  char *msg[2] = {"h\nh", "hhhh\nh   h\nh   h"};
  pcontainer *container = plug_construct(256);
  pdata data;
  func_ptr function_ptr;
  for(int i = 0; i < 2; i++) {
    if(!install(container, name[i], path[i], "init_func") ) {
      function_ptr = getptr(container, name[i]);
      strncpy(data.string, msg[i], 128);
      function_ptr(&data);
    }
  }
  uninstall(container, name[1]);
  uninstall(container, name[0]);
  plug_destruct(container);
  return 0;
}
