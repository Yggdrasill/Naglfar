char *plugGetError(int plugErrorCode, char *plugErrorDetails)
{
  static char plugErrMsg[1024];
  switch(plugErrorCode)
  {
    case PLUGERR_NONE:
      strncpy(plugErrMsg, "Why are you even here? You shouldn't be\n", 1024);
      break;;
    case PLUGERR_OPEN:
      snprintf(plugErrMsg, 1024, "Failed to open plugin file: %s\n", dlerror() );
      break;
    case PLUGERR_LOAD:
      snprintf(plugErrMsg, 1024, "Failed to load plugin: %s\n", dlerror() );
      break;
    case PLUGERR_UNLOAD:
      snprintf(plugErrMsg, 1024, "Failed to unload plugin: %s\n", dlerror() );
      break;
    case PLUGERR_NUNLOAD:
      snprintf(plugErrMsg, 1024, "You're trying to unload %s, but the plugin is not loaded\n", plugErrorDetails);
      break;
    case PLUGERR_NLOADED:
      strncpy(plugErrMsg, "You're trying to return a function pointer for a plugin,"
                          " but it does not appear to be loaded\n", 1024);
      break;
    case PLUGERR_LOADED:
      snprintf(plugErrMsg, 1024, "Plugin %s already loaded. Reloading is done with plugReload()\n", plugErrorDetails);
      break;
    case PLUGERR_HASHCOL:
      snprintf(plugErrMsg, 1024, "Oops! Seems like there's been a hash collision. The colliding plugin names are: %s."
                          "To fix this, rename your plugin\n", plugErrorDetails);
      break;
    case PLUGERR_FIXHASHCOL:
      snprintf(plugErrMsg, 1024, "There has been a fixable hash collision. The colliding plugin names are: %s."
                          "Fixing.\n", plugErrorDetails);
      break;
    case PLUGERR_UFIXHASHCOL:
      snprintf(plugErrMsg, 1024, "There has previously been a fixable hash collision. Now unloading %s, "
                          "which should be the correct plugin\n", plugErrorDetails);
      break;
    case PLUGERR_SMEM:
      strncpy(plugErrMsg, "Failed to allocate memory for plugin info. Not enough memory available\n", 1024);
      break;
    case PLUGERR_CMEM:
      strncpy(plugErrMsg, "Failed to allocate memory for the main plugin container. Not enough memory available\n", 1024);
      break;
    case PLUGERR_CNA:
      strncpy(plugErrMsg, "You're trying to free the main plugin container, but it's not even malloced. Aborting\n", 1024);
      break;
    default:
      strncpy(plugErrMsg, "I have no idea WHAT the fuck is going on\n", 1024);
  }
  return plugErrMsg;
}

/* Allocates memory space for a plugin. It is only called by plugInstall() */

plugInfo *plugAlloc(int *allocStatus)
{
  *allocStatus = 0;
  plugInfo *sPlugin = malloc(sizeof(plugInfo) );
  if(!sPlugin)
    *allocStatus = PLUGERR_SMEM;
  return sPlugin;
}

plugList *listAlloc(int *allocStatus)
{
  *allocStatus = 0;
  size_t size = sizeof(plugList);
  plugList *sPlugList = calloc(size, size);
  if(!sPlugList)
    *allocStatus = PLUGERR_LMEM;
  return sPlugList;
}
/* Constructs the main plugin container, and initializes all sPlugCont->sPlugin to NULL.
 * It should be called before trying to run ANY other functions. Returns NULL on failure and a plugCont on success. */

plugCont *plugContConstruct(uint32_t plugMax)
{
  plugCont *sPlugCont;
  int contStatus = 0;
  sPlugCont = malloc(sizeof(plugCont) * sizeof(plugList) );
  if(!sPlugCont) {
    contStatus = PLUGERR_CMEM;
    fputs(plugGetError(contStatus, NULL), stderr);
    return NULL;
  }
  sPlugCont->plugMax = plugMax;
  sPlugCont->plugCount = 0;
  sPlugCont->sPlugList = calloc(sizeof(plugList *), plugMax);
  if(!sPlugCont->sPlugList)  {
    contStatus = PLUGERR_SMEM;
    fputs(plugGetError(contStatus, NULL), stderr);
    plugContDestruct(sPlugCont);
    return NULL;
  }
  return sPlugCont;
}

/* Frees a sPlugCont->sPlugin. Called by plugFree() and plugContDestruct(). */

void plugDestroy(plugInfo **sPlugin)
{
   if(*sPlugin) {
    free(*sPlugin);
  }
  *sPlugin = NULL;
}

void listDestroy(plugList **sPlugList)
{
  if(*sPlugList)
    free(*sPlugList);
  *sPlugList = NULL;
}

/* Destroys the main plugin container. You should plugFree() all plugins before calling this. */

void plugContDestruct(plugCont *sPlugCont)
{
  if(sPlugCont) {
    while(sPlugCont->plugMax--) {
      listDestroy(&sPlugCont->sPlugList[sPlugCont->plugMax]);
      sPlugCont->sPlugList[sPlugCont->plugMax] = NULL;
    }
    free(sPlugCont->sPlugList);
    sPlugCont->sPlugList = NULL;
    free(sPlugCont);
    sPlugCont = NULL;
  }
  else
    fputs(plugGetError(PLUGERR_CNA, NULL), stderr);
}

/* Unloads the plugin. You shouldn't call it, you should use plugFree(), which calls this function.
 * Returns non-zero on failure and zero on success. */

int plugUnload(void *plugHandle)
{
  int unloadStatus = dlclose(plugHandle);
  if(unloadStatus)
    unloadStatus = PLUGERR_UNLOAD;
  return unloadStatus;
}

/* Unloads the plugin and frees the plugin information structure. Does not return anything */

void plugFree(plugCont *sPlugCont, char *plugName)
{
  uint32_t hash = 0;
  hash = genHash(plugName, sPlugCont->plugMax);
  int unloadStatus = 0;
  int plugin = 0;
  for(int i = 0; i < 2; i++) {
    if(sPlugCont->sPlugList[hash]->sPlugin[i]) {
      unloadStatus =  hashColCk(sPlugCont->sPlugList[hash]->sPlugin[i]->plugName, plugName, sPlugCont->plugMax);
      if(!unloadStatus) {
        plugin = i;
        break;
      }
      else if(unloadStatus && sPlugCont->sPlugList[hash]->hashCol)
        unloadStatus = PLUGERR_UFIXHASHCOL;
    }
    else if(i)
      unloadStatus = PLUGERR_NUNLOAD;
  }
  switch(unloadStatus) {
    case PLUGERR_NONE:
      unloadStatus = plugUnload(sPlugCont->sPlugList[hash]->sPlugin[plugin]->plugHandle);
      break;
    case PLUGERR_UFIXHASHCOL:
      fputs(plugGetError(unloadStatus, sPlugCont->sPlugList[hash]->sPlugin[plugin]->plugName), stderr);
      unloadStatus = plugUnload(sPlugCont->sPlugList[hash]->sPlugin[plugin]->plugHandle);
      break;
    case PLUGERR_NUNLOAD:
      fputs(plugGetError(unloadStatus, plugName), stderr);
      break;
    default:
      fputs(plugGetError(unloadStatus, NULL), stderr);
  }
  plugDestroy(&sPlugCont->sPlugList[hash]->sPlugin[plugin]);
  sPlugCont->plugCount--;
}

/* Loads a plugin. You shouldn't call this, you should use plugInstall(), which calls this function. Returns non-zero on
 * failure and zero on success. */

int plugLoad(plugInfo *sPlugin, char *strPath, char *strSymbol)
{
  int loadStatus = 0;
  sPlugin->plugHandle = dlopen(strPath, RTLD_NOW);
  if(!sPlugin->plugHandle)
    return loadStatus = PLUGERR_OPEN;
  sPlugin->funcPointer = (funcPtr)dlsym(sPlugin->plugHandle, strSymbol);
  if(!sPlugin->funcPointer)
    loadStatus = PLUGERR_LOAD;
  return loadStatus;
}

int plugPrepare(plugInfo **sPlugin, uint32_t plugHash, char *plugName, char *strPath, char *strSymbol)
{
  int prepStatus = 0;
  *sPlugin = plugAlloc(&prepStatus);
  if(prepStatus) {
    fputs(plugGetError(prepStatus, NULL), stderr);
    return prepStatus;
  }
  prepStatus = plugLoad(*sPlugin, strPath, strSymbol);
  if(prepStatus) {
    fputs(plugGetError(prepStatus, NULL), stderr);
    return prepStatus;
  }
  (*sPlugin)->plugHash = plugHash;
  strncpy( (*sPlugin)->plugName, plugName, 128);
  return prepStatus;
}

/* Installs a plugin and puts it in the correct sPlugCont->sPlugin[hash]. Returns non-zero on failure and zero on
 * success. */

int plugInstall(plugCont *sPlugCont, char *plugName, char *strPath, char *strSymbol)
{
  uint32_t hash = genHash(plugName, sPlugCont->plugMax);
  int installStatus = 0;
  int plugin = 0;
  if(!sPlugCont->sPlugList[hash])
    sPlugCont->sPlugList[hash] = listAlloc(&installStatus);
  else if(sPlugCont->sPlugList[hash]->sPlugin[plugin]) {
    installStatus = hashColCk(sPlugCont->sPlugList[hash]->sPlugin[plugin]->plugName, plugName, sPlugCont->plugMax);
    if(installStatus && sPlugCont->sPlugList[hash]->hashCol)
      installStatus = PLUGERR_HASHCOL;
    else if(installStatus && !sPlugCont->sPlugList[hash]->hashCol) {
      installStatus = PLUGERR_FIXHASHCOL;
      plugin = 1;
    }
    else if(!installStatus)
      installStatus = PLUGERR_LOADED;
  }
  char plugNames[256];
  switch(installStatus) {
    case 0:
      installStatus = plugPrepare(&sPlugCont->sPlugList[hash]->sPlugin[plugin], hash, plugName,  strPath, strSymbol);
      sPlugCont->sPlugList[hash]->hashCol = 0;
      break;
    case PLUGERR_FIXHASHCOL:
      installStatus = plugPrepare(&sPlugCont->sPlugList[hash]->sPlugin[plugin], hash, plugName, strPath, strSymbol);
      snprintf(plugNames, 256, "%s %s", sPlugCont->sPlugList[hash]->sPlugin[!plugin]->plugName, plugName);
      fputs(plugGetError(PLUGERR_FIXHASHCOL, plugNames), stderr);
      sPlugCont->sPlugList[hash]->hashCol = 1;
      break;
    case PLUGERR_HASHCOL:
      snprintf(plugNames, 256, "%s %s", sPlugCont->sPlugList[hash]->sPlugin[plugin]->plugName, plugName);
      fputs(plugGetError(installStatus, plugNames), stderr);
      break;
    case PLUGERR_LOADED:
      fputs(plugGetError(installStatus, plugName), stderr);
      break;
    default:
      fputs(plugGetError(installStatus, NULL), stderr);
  }
  sPlugCont->plugCount++;
  return installStatus;
}

int plugReload(plugCont *sPlugCont, char *plugName, char *strPath, char *strSymbol)
{
  int reloadStatus = 0;
  plugFree(sPlugCont, plugName);
  reloadStatus = plugInstall(sPlugCont, plugName, strPath, strSymbol);
  return reloadStatus;
}

/* Gets the function pointer from the plugin you're requesting with plugName. Returns NULL on failure and a function
 * pointer on success. */

funcPtr plugGetPtr(plugCont *sPlugCont, char *plugName)
{
  uint32_t hash = genHash(plugName, sPlugCont->plugMax);
  int status = 0;
  int plugin = 0;
  if(sPlugCont->sPlugList[hash]->hashCol) {
    status = hashColCk(sPlugCont->sPlugList[hash]->sPlugin[plugin]->plugName, plugName, sPlugCont->plugMax);
    if(status)
      plugin = 1;
  }
  if(!sPlugCont->sPlugList[hash]->sPlugin[plugin]) {
    fputs(plugGetError(PLUGERR_NLOADED, NULL), stderr);
    return NULL;
  }
  return sPlugCont->sPlugList[hash]->sPlugin[plugin]->funcPointer;

}
