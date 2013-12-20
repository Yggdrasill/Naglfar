#include "naglfar.h"

static char *plugerr(const int, const char *);
static pinfo *plug_alloc(int *);
static plist *list_alloc(int *);
static void plug_destroy(pinfo **);
static void list_destroy(plist **);
static int unload(void *);
static int load(pinfo *, const char *, const char *);
static int prepare(pinfo **, const uint32_t, const char *, const char *, const char *);
static func_ptr getptr(pcontainer *, const char *, const int, const int);

static char *plugerr(const int perror, const char *details)
{
  static char error_msg[1024];

  switch(perror)
  {
    case NONE:
      strncpy(error_msg, "Why are you even here? You shouldn't be\n", 1024);
      break;;
    case OPEN:
      snprintf(error_msg, 1024, "Failed to open plugin file: %s\n", dlerror() );
      break;
    case LOAD:
      snprintf(error_msg, 1024, "Failed to load plugin: %s\n", dlerror() );
      break;
    case UNLOAD:
      snprintf(error_msg, 1024, "Failed to unload plugin: %s\n", dlerror() );
      break;
    case NUNLOAD:
      snprintf(error_msg, 1024, "You're trying to unload %s, but the plugin is not loaded\n", details);
      break;
    case NLOADED:
      strncpy(error_msg, "You're trying to return a function pointer for a plugin,"
                          " but it does not appear to be loaded\n", 1024);
      break;
    case LOADED:
      snprintf(error_msg, 1024, "Plugin %s already loaded. reinstalling is done with reinstall()\n", details);
      break;
    case HASHCOL:
      snprintf(error_msg, 1024, "Oops! Seems like there's been a hash collision. The colliding plugin names are: %s. "
                          "To fix this, rename your plugin\n", details);
      break;
    case FIXHASHCOL:
      snprintf(error_msg, 1024, "There has been a fixable hash collision. The colliding plugin names are: %s. "
                          "Fixing.\n", details);
      break;
    case UFIXHASHCOL:
      snprintf(error_msg, 1024, "There has previously been a fixable hash collision. Now unloading %s, "
                          "which should be the correct plugin\n", details);
      break;
    case SMEM:
      strncpy(error_msg, "Failed to allocate memory for plugin info. Not enough memory available\n", 1024);
      break;
    case CMEM:
      strncpy(error_msg, "Failed to allocate memory for the main plugin container. Not enough memory available\n", 1024);
      break;
    case CNA:
      strncpy(error_msg, "You're trying to free the main plugin container, but it's not even malloced. Aborting\n", 1024);
      break;
    default:
      strncpy(error_msg, "I have no idea WHAT the fuck is going on\n", 1024);
  }

  return error_msg;
}

/* Allocates memory space for a plugin. It is only called by prepare() */

static pinfo *plug_alloc(int *status)
{
  *status = 0;
  pinfo *plugin = malloc(sizeof(pinfo) );

  if(!plugin)
    *status = SMEM;

  return plugin;
}

/* Allocates memory space for a plugin list. It is only called by install() */

static plist *list_alloc(int *status)
{
  *status = 0;
  size_t size = sizeof(plist);
  plist *list = calloc(size, size);

  if(!list)
    *status = LMEM;
  #ifdef THREADING
    else
      pthread_mutex_init(&list->listmutex, NULL);
  #endif
  return list;
}

/* Constructs the main plugin container, and initializes all container->plugin to NULL.
 * It should be called before trying to run ANY other functions. Returns NULL on failure and a pcontainer on success. */

pcontainer *plug_construct(const uint32_t max)
{
  pcontainer *container;
  int status = 0;
  container = malloc(sizeof(pcontainer) * sizeof(plist) );

  #ifdef THREADING
    pthread_mutex_init(&container->allocmutex, NULL);
  #endif

  if(!container) {
    status = CMEM;
    fputs(plugerr(status, NULL), stderr);
    return NULL;
  }

  container->max = max;
  container->count = 0;
  container->list = calloc(sizeof(plist *), max);

  if(!container->list)  {
    status = SMEM;
    fputs(plugerr(status, NULL), stderr);
    plug_destruct(&container);
    container = NULL;
  }

  return container;
}

/* Frees a container->plugin. Called by uninstall() and plug_destruct(). */

static void plug_destroy(pinfo **plugin)
{
   if(*plugin) {
    free(*plugin);
  }
  *plugin = NULL;
}

static void list_destroy(plist **list)
{
  if(*list) {
    #ifdef THREADING
      pthread_mutex_destroy(&(*list)->listmutex);
    #endif
    free(*list);
  }
  *list = NULL;
}

/* Destroys the main plugin container. You should uninstall() all plugins before calling this. */

void plug_destruct(pcontainer **container)
{
  pcontainer *cont_ptr = *container;
  if(!cont_ptr)
    fputs(plugerr(CNA, NULL), stderr);

  #ifdef THREADING
    pthread_mutex_destroy(&container->allocmutex);
  #endif
  for(; cont_ptr->max; cont_ptr->max--) {
    for(int i = 0; i < PLUGINS && cont_ptr->list[cont_ptr->max]; i++) {
      if(cont_ptr->list[cont_ptr->max]->plugin[i]) {
        dlclose(cont_ptr->list[cont_ptr->max]->plugin[i]->handle);
        free(cont_ptr->list[cont_ptr->max]->plugin[i]);
      }
    }
    list_destroy(&cont_ptr->list[cont_ptr->max]);
    cont_ptr->list[cont_ptr->max] = NULL;
  }
  free(cont_ptr->list);
  cont_ptr->list = NULL;
  free(cont_ptr);
  cont_ptr = NULL;
}

/* Unloads the plugin. You shouldn't call it, you should use uninstall(), which calls this function.
 * Returns non-zero on failure and zero on success. */

static int unload(void *handle)
{
  int status = dlclose(handle);

  if(status)
    status = UNLOAD;

  return status;
}

/* Unloads the plugin and frees the plugin information structure. Does not return anything */

void plug_uninstall(pcontainer *container, const char *name)
{
  uint32_t hash = 0;
  hash = gen_hash(name, container->max);
  int status = 0;
  int plugin_num = 0;

  #ifdef THREADING
    pthread_mutex_lock(&container->list[hash]->listmutex);
  #endif

  for(int i = 0; i < PLUGINS; i++) {
    if(container->list[hash]->plugin[i]) {
      status =  check_hash(container->list[hash]->plugin[i]->name, name, container->max);
      if(!status) {
        plugin_num = i;
        break;
      }
      else if(status && container->list[hash]->hash_col)
        status = UFIXHASHCOL;
    }
    else if(i)
      status = NUNLOAD;
  }

  switch(status) {
    case NONE:
      status = unload(container->list[hash]->plugin[plugin_num]->handle);
      break;
    case UFIXHASHCOL:
      fputs(plugerr(status, container->list[hash]->plugin[plugin_num]->name), stderr);
      status = unload(container->list[hash]->plugin[plugin_num]->handle);
      break;
    case NUNLOAD:
      fputs(plugerr(status, name), stderr);
      break;
    default:
      fputs(plugerr(status, NULL), stderr);
  }

  plug_destroy(&container->list[hash]->plugin[plugin_num]);
  container->count--;

  #ifdef THREADING
    pthread_mutex_unlock(&container->list[hash]->listmutex);
  #endif
}

/* Loads a plugin. You shouldn't call this, you should use install(), which calls this function. Returns non-zero on
 * failure and zero on success. */

static int load(pinfo *plugin, const char *path, const char *symbol)
{
  int status = 0;
  plugin->handle = dlopen(path, RTLD_NOW);

  if(!plugin->handle)
    return status = OPEN;

  plugin->function_ptr = (func_ptr)dlsym(plugin->handle, symbol);

  if(!plugin->function_ptr)
    status = LOAD;

  return status;
}

/* Prepares memory for plugin loading and loads it. Install() calls this function. Returns non-zero on failure and zero
 * on success */

static int prepare(pinfo **plugin, const uint32_t hash, const char *name, const char *path, const char *symbol)
{
  int status = 0;
  *plugin = plug_alloc(&status);

  if(status) {
    fputs(plugerr(status, NULL), stderr);
    return status;
  }
  status = load(*plugin, path, symbol);
  if(status) {
    fputs(plugerr(status, NULL), stderr);
    return status;
  }

  (*plugin)->hash = hash;
  strncpy( (*plugin)->name, name, 128);

  return status;
}

/* Installs a plugin and puts it in the correct container->plugin[hash]. Returns non-zero on failure and zero on
 * success. */

int plug_install(pcontainer *container, const char *name, const char *path, const char *symbol)
{
  uint32_t hash = gen_hash(name, container->max);
  int status = 0;
  int plugin_num = 0;
  int hasalloced = 0;
  char names[256];

  #ifdef THREADING
    pthread_mutex_lock(&container->allocmutex);
  #endif

  if(!container->list[hash]) {
    if( (container->list[hash] = list_alloc(&status) ) )
      hasalloced = 1;
  }

  #ifdef THREADING
    pthread_mutex_unlock(&container->allocmutex);
    pthread_mutex_lock(&container->list[hash]->listmutex);
  #endif

  if(!hasalloced && container->list[hash]->plugin[plugin_num]) {
    status = check_hash(container->list[hash]->plugin[plugin_num]->name, name, container->max);
    if(status && container->list[hash]->hash_col)
      status = HASHCOL;
    else if(status && !container->list[hash]->hash_col) {
      status = FIXHASHCOL;
      plugin_num = 1;
    }
    else if(!status)
      status = LOADED;
  }

  switch(status) {
    case 0:
      status = prepare(&container->list[hash]->plugin[plugin_num], hash, name,  path, symbol);
      container->list[hash]->hash_col = 0;
      container->count++;
      break;
    case FIXHASHCOL:
      status = prepare(&container->list[hash]->plugin[plugin_num], hash, name, path, symbol);
      snprintf(names, 256, "%s %s", container->list[hash]->plugin[!plugin_num]->name, name);
      fputs(plugerr(FIXHASHCOL, names), stderr);
      container->list[hash]->hash_col = 1;
      container->count++;
      break;
    case HASHCOL:
      snprintf(names, 256, "%s %s", container->list[hash]->plugin[plugin_num]->name, name);
      fputs(plugerr(status, names), stderr);
      break;
    case LOADED:
      fputs(plugerr(status, name), stderr);
      break;
    default:
      fputs(plugerr(status, NULL), stderr);
  }

  #ifdef THREADING
    pthread_mutex_unlock(&container->list[hash]->listmutex);
  #endif

  return status;
}

/* Reinstalls the plugin. Use this if you need to reload a plugin. */

int plug_reinstall(pcontainer *container, const char *name, const char *path, const char *symbol)
{
  int status = 0;

  plug_uninstall(container, name);
  status = plug_install(container, name, path, symbol);

  return status;
}

/* Gets the function pointer from the plugin you're requesting with name. Returns NULL on failure and a function
 * pointer on success. */

static func_ptr getptr(pcontainer *container, const char *name, const int hash, const int plugin_num)
{
  return container->list[hash]->plugin[plugin_num]->function_ptr;
}

void* plug_exec(pcontainer *container, const char *name, void *data)
{
  uint32_t hash = gen_hash(name, container->max);
  int status = 0;
  int plugin_num = 0;

  func_ptr fptr;

  if(container->list[hash]->hash_col) {
    status = check_hash(container->list[hash]->plugin[plugin_num]->name, name, container->max);
    if(status) plugin_num = 1;
  }
  if(!container->list[hash]->plugin[plugin_num]) {
    fputs(plugerr(NLOADED, NULL), stderr);
    return NULL;
  }

  fptr = getptr(container, name, hash, plugin_num);
  fptr(data);

  return fptr;
}

