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
  plugCont *sPlugCont = plugContConstruct(256);
  plugData sPlugData;
  funcPtr funcPointer;
  for(int i = 0; i < 2; i++) {
    if(!plugInstall(sPlugCont, name[i], path[i], "init_func") ) {
      funcPointer = plugGetPtr(sPlugCont, name[i]);
      strncpy(sPlugData.outputString, msg[i], 128);
      funcPointer(&sPlugData);
    }
  }
  plugFree(sPlugCont, name[1]);
  plugFree(sPlugCont, name[0]);
  plugContDestruct(sPlugCont);
  return 0;
}
