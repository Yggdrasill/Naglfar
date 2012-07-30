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
      strncpy(plugErrMsg, "You're trying to return a function pointer for a plugin, but it does not appear to be loaded\n", 1024);
      break;
    case PLUGERR_LOADED:
      snprintf(plugErrMsg, 1024, "Plugin %s already loaded. Reloading is done with plugReload()\n", plugErrorDetails);
      break;
    case PLUGERR_HASHCOL:
      snprintf(plugErrMsg, 1024, "Oops! Seems like there's been a hash collision. The colliding plugin names are: %s."
                          "To fix this, rename your plugin\n", plugErrorDetails);
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

/* Constructs the main plugin container, and initializes all sPlugCont->sPlugin to NULL. It should be called before trying to run ANY other functions. Returns NULL on failure and a plugCont on success. */

plugCont *plugContConstruct(uint32_t plugMax)
{
  plugCont *sPlugCont;
  sPlugCont = malloc(sizeof(plugCont) * sizeof(plugInfo) );
  if(!sPlugCont) {
    fputs(plugGetError(PLUGERR_CMEM, NULL), stderr);
    return NULL;
  }
  sPlugCont->sPlugin = malloc(plugMax * sizeof(plugInfo *) );
  if(!sPlugCont->sPlugin)  {
    fputs(plugGetError(PLUGERR_SMEM, NULL), stderr);
    return NULL;
  }
  for(uint32_t i = 0; i <= plugMax; i++)
    sPlugCont->sPlugin[i] = NULL;
  sPlugCont->plugMax = plugMax;
  sPlugCont->plugCount = 0;
  return sPlugCont;
}

/* Frees a sPlugCont->sPlugin. Called by plugFree() and plugContDestruct(). */

void plugDestroy(plugInfo *sPlugin)
{
  if(sPlugin)
    free(sPlugin);
}

/* Destroys the main plugin container. You should plugFree() all plugins before calling this. */

void plugContDestruct(plugCont *sPlugCont)
{
  if(sPlugCont) {
    while(sPlugCont->plugMax--)
      plugDestroy(sPlugCont->sPlugin[sPlugCont->plugMax]);
    free(sPlugCont->sPlugin);
    free(sPlugCont);
  }
  else
    fputs(plugGetError(PLUGERR_CNA, NULL), stderr);
}

/* Generates a hash from plugName, and then divides hash by plugMax, returning the remainder. */

uint32_t genHash(char *plugName, uint32_t plugMax)
{
  size_t cmdLen = strlen(plugName);
  uint32_t hash = 0;
  for(size_t i = 0; i <= cmdLen; i++)
    hash += (uint32_t)plugName[i];
  hash %= plugMax;
  return hash;
}

/* Checks for hash collisions. Returns non-zero on collision and zero otherwise. */

int hashColCk(char *s1, char *s2, uint32_t plugMax)
{
  int hashCol = 0;
  uint32_t hash = genHash(s1, plugMax);
  uint32_t hash2 = genHash(s2, plugMax);
  if(hash == hash2 && strcmp(s1, s2) ) {
    char plugNames[256];
    snprintf(plugNames, 256, "%s %s", s1, s2);
    hashCol = PLUGERR_HASHCOL;
    fputs(plugGetError(hashCol, plugNames), stderr);
  }
  return hashCol;
}

/* Unloads the plugin. You shouldn't call it, you should use plugFree(), which calls this function. Returns non-zero on
 * failure and zero on success. */

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
  if(!sPlugCont->sPlugin[hash]) {
    fputs(plugGetError(PLUGERR_NUNLOAD, plugName), stderr);
    return;
  }
  unloadStatus = hashColCk(sPlugCont->sPlugin[hash]->plugName, plugName, sPlugCont->plugMax);
  if(unloadStatus)
    return;
  unloadStatus = plugUnload(sPlugCont->sPlugin[hash]->plugHandle);
  if(unloadStatus) {
    fputs(plugGetError(unloadStatus, NULL), stderr);
    return;
  }
  plugDestroy(sPlugCont->sPlugin[hash]);
  sPlugCont->plugCount--;
  sPlugCont->sPlugin[hash] = NULL;
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

/* Installs a plugin and puts it in the correct sPlugCont->sPlugin[hash]. Returns non-zero on failure and zero on
 * success. */

int plugInstall(plugCont *sPlugCont, char *plugName, char *strPath, char *strSymbol)
{
  uint32_t hash = genHash(plugName, sPlugCont->plugMax);
  int installStatus = 0;
  if(sPlugCont->sPlugin[hash]) {
    installStatus = hashColCk(sPlugCont->sPlugin[hash]->plugName, plugName, sPlugCont->plugMax);
    if(!installStatus)
      installStatus = PLUGERR_LOADED;
    fputs(plugGetError(installStatus, plugName), stderr);
    return installStatus;
  }
  sPlugCont->sPlugin[hash] = plugAlloc(&installStatus);
  if(installStatus) {
    fputs(plugGetError(installStatus, NULL), stderr);
    return installStatus;
  }
  installStatus = plugLoad(sPlugCont->sPlugin[hash], strPath, strSymbol);
  if(installStatus) {
    fputs(plugGetError(installStatus, NULL), stderr);
    return installStatus;
  }
  sPlugCont->sPlugin[hash]->plugHash = hash;
  strncpy(sPlugCont->sPlugin[hash]->plugName, plugName, 128);
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
  if(!sPlugCont->sPlugin[hash]) {
    fputs(plugGetError(PLUGERR_NLOADED, NULL), stderr);
    return NULL;
  }
  return sPlugCont->sPlugin[hash]->funcPointer;
}
