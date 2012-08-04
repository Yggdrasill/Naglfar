/* The Naglfar Plugin Framework is an abstract plugin architecture written in C using libdl.
 *
 * Copyright (C) 2012 Yggdrasill yggdrasill@lavabit.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdint.h>
#include "../Vigrior/vigrior.h"

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
  PLUGERR_CMEM,
  PLUGERR_CNA
} plugErrCode;

char *plugGetError(int, char *);
plugInfo *plugAlloc(int *);
plugCont *plugContConstruct(uint32_t);
void plugDestroy(plugInfo *);
void plugContDestruct(plugCont *);
int plugUnload(void *);
void plugFree(plugCont *, char *);
int plugLoad(plugInfo *, char *, char *);
int plugInstall(plugCont *, char *, char *, char *);
int plugReload(plugCont *, char *, char *, char *);
funcPtr plugGetPtr(plugCont *, char *);

#include "naglfar.c"
