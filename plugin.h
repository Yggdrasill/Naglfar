#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdint.h>

typedef void(*funcPtr)(void *);

typedef struct {
  void *plugHandle;
  funcPtr funcPointer;
  char plugName[128];
  uint64_t plugHash;
} plugInfo;

typedef struct {
  plugInfo **sPlugin;
  uint32_t plugCount;
  uint32_t plugMax;
} plugCont;

enum plugErrCodes {
  PLUGERR_NONE = 0,
  PLUGERR_OPEN,
  PLUGERR_LOAD,
  PLUGERR_UNLOAD,
  PLUGERR_NUNLOAD,
  PLUGERR_NLOADED,
  PLUGERR_LOADED,
  PLUGERR_HASHCOL,
  PLUGERR_SMEM,
  PLUGERR_CMEM
} plugErrCode;

char *plugGetError(int, char *);
plugInfo *plugAlloc(int *);
plugCont *plugContConstruct(uint32_t);
void plugDestroy(plugInfo *);
void plugContDestruct(plugCont *);
uint32_t genHash(char *, uint32_t);
int hashColCk(char *, char *, uint32_t);
int plugUnload(void *);
void plugFree(plugCont *, char *);
int plugLoad(plugInfo *, char *, char *);
int plugInstall(plugCont *, char *, char *, char *);
int plugReload(plugCont *, char *, char *, char *);
funcPtr plugGetPtr(plugCont *, char *);

#include "plugin.c"
