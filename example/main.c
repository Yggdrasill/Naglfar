#define THREADING

#include <stdio.h>
#include "../naglfar.h"

#include <pthread.h>
#include <unistd.h>

typedef struct {
  pcontainer *container;

  char string[128];
  char name[128];
  char path[128];
} threadinfo;

/* An ugly example of how to use it, it's not clean or elegant at all, but it works. */

void start_thread(threadinfo *info)
{
  if(!install(info->container, info->name, info->path, "init_func") ) {
    plug_exec(info->container, info->name, (void *)&info->string);
    uninstall(info->container, info->name);
  }
}

int main(void)
{
  char *path[2] = {"/path/to/plugin1",
                  "/path/to/plugin2"};
  char *name[2] = {"abc", "bac"};
  char *msg[2] = {"h\nh", "hhhh\nh   h\nh   h"};
  pcontainer *container = plug_construct(256);
  threadinfo info[2];
  pthread_t threads[2];
  for(int i = 0; i < 2; i++) {
    strncpy(info[i].string, msg[i], 128);
    strncpy(info[i].name, name[i], 128);
    strncpy(info[i].path, path[i], 128);
    info[i].container = container;
    pthread_create(&threads[i], NULL, (void *) start_thread, (void *) &info[i]);
  }
  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);
  plug_destruct(container);
  return 0;
}
