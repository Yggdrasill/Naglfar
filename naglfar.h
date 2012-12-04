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

#define POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "../Vigrior/vigrior.h"

#ifdef THREADING
  #include <pthread.h>
#endif

typedef void(*func_ptr)(void *);

typedef struct {
  void *handle;
  func_ptr function_ptr;
  char name[128];
  uint32_t hash;
  #ifdef THREADING
    pthread_mutex_t pluginmutex;
  #endif
} pinfo;

typedef struct {
  pinfo *plugin[2];
  int hash_col;
  #ifdef THREADING
    pthread_mutex_t listmutex;
  #endif
} plist;

typedef struct {
  plist **list;
  uint32_t count;
  uint32_t max;
  #ifdef THREADING
    pthread_mutex_t allocmutex;
  #endif
} pcontainer;

enum plugErrCodes {
  NONE = 0,
  OPEN,
  LOAD,
  UNLOAD,
  NUNLOAD,
  NLOADED,
  LOADED,
  HASHCOL,
  FIXHASHCOL,
  UFIXHASHCOL,
  SMEM,
  LMEM,
  CMEM,
  CNA
} PLUGERR;

pcontainer *plug_construct(uint32_t);
void plug_destruct(pcontainer **);
void plug_uninstall(pcontainer *, char *);
int plug_install(pcontainer *, char *, char *, char *);
int plug_reinstall(pcontainer *, char *, char *, char *);
void* plug_exec(pcontainer *, char *, void *);
