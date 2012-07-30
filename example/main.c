#include <stdio.h>
#include "struct.h"
#include "../plugin.h"

/* An ugly example of how to use it. It doesn't have to be done this way. */

int main(void)
{
  char *path[2] = {"/path/to/plugin1",
                  "/path/to/plugin2"};
  char *name[2] = {"herp", "derp"};
  char *msg[2] = {"Herp", "Derp"};
  plugCont *sPlugCont = plugContConstruct(256);
  plugData sPlugData;
  funcPtr funcPointer;
  for(int i = 0; i < 2; i++) {
    plugInstall(sPlugCont, name[i], path[i], "init_func");
    funcPointer = plugGetPtr(sPlugCont, name[i]);
    strncpy(sPlugData.outputString, msg[i], 128);
    funcPointer(&sPlugData);
    plugFree(sPlugCont, name[i]);
  }
  plugContDestruct(sPlugCont);
  return 0;
}
