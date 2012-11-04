static char *plugerr(int perror, char *details)
{
  static char error_msg[1024];
  switch(perror)
  {
    case PLUGERR_NONE:
      strncpy(error_msg, "Why are you even here? You shouldn't be\n", 1024);
      break;;
    case PLUGERR_OPEN:
      snprintf(error_msg, 1024, "Failed to open plugin file: %s\n", dlerror() );
      break;
    case PLUGERR_LOAD:
      snprintf(error_msg, 1024, "Failed to load plugin: %s\n", dlerror() );
      break;
    case PLUGERR_UNLOAD:
      snprintf(error_msg, 1024, "Failed to unload plugin: %s\n", dlerror() );
      break;
    case PLUGERR_NUNLOAD:
      snprintf(error_msg, 1024, "You're trying to unload %s, but the plugin is not loaded\n", details);
      break;
    case PLUGERR_NLOADED:
      strncpy(error_msg, "You're trying to return a function pointer for a plugin,"
                          " but it does not appear to be loaded\n", 1024);
      break;
    case PLUGERR_LOADED:
      snprintf(error_msg, 1024, "Plugin %s already loaded. reinstalling is done with reinstall()\n", details);
      break;
    case PLUGERR_HASHCOL:
      snprintf(error_msg, 1024, "Oops! Seems like there's been a hash collision. The colliding plugin names are: %s."
                          "To fix this, rename your plugin\n", details);
      break;
    case PLUGERR_FIXHASHCOL:
      snprintf(error_msg, 1024, "There has been a fixable hash collision. The colliding plugin names are: %s."
                          "Fixing.\n", details);
      break;
    case PLUGERR_UFIXHASHCOL:
      snprintf(error_msg, 1024, "There has previously been a fixable hash collision. Now unloading %s, "
                          "which should be the correct plugin\n", details);
      break;
    case PLUGERR_SMEM:
      strncpy(error_msg, "Failed to allocate memory for plugin info. Not enough memory available\n", 1024);
      break;
    case PLUGERR_CMEM:
      strncpy(error_msg, "Failed to allocate memory for the main plugin container. Not enough memory available\n", 1024);
      break;
    case PLUGERR_CNA:
      strncpy(error_msg, "You're trying to free the main plugin container, but it's not even malloced. Aborting\n", 1024);
      break;
    default:
      strncpy(error_msg, "I have no idea WHAT the fuck is going on\n", 1024);
  }
  return error_msg;
}

/* Allocates memory space for a plugin. It is only called by install() */

static pinfo *plug_alloc(int *status)
{
  *status = 0;
  pinfo *plugin = malloc(sizeof(pinfo) );
  if(!plugin)
    *status = PLUGERR_SMEM;
  return plugin;
}

static plist *list_alloc(int *status)
{
  *status = 0;
  size_t size = sizeof(plist);
  plist *list = calloc(size, size);
  if(!list)
    *status = PLUGERR_LMEM;
  return list;
}
/* Constructs the main plugin container, and initializes all container->plugin to NULL.
 * It should be called before trying to run ANY other functions. Returns NULL on failure and a pcontainer on success. */

pcontainer *plug_construct(uint32_t max)
{
  pcontainer *container;
  int status = 0;
  container = malloc(sizeof(pcontainer) * sizeof(plist) );
  if(!container) {
    status = PLUGERR_CMEM;
    fputs(plugerr(status, NULL), stderr);
    return NULL;
  }
  container->max = max;
  container->count = 0;
  container->list = calloc(sizeof(plist *), max);
  if(!container->list)  {
    status = PLUGERR_SMEM;
    fputs(plugerr(status, NULL), stderr);
    plug_destruct(container);
    return NULL;
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
  if(*list)
    free(*list);
  *list = NULL;
}

/* Destroys the main plugin container. You should uninstall() all plugins before calling this. */

void plug_destruct(pcontainer *container)
{
  if(container) {
    while(container->max--) {
      list_destroy(&container->list[container->max]);
      container->list[container->max] = NULL;
    }
    free(container->list);
    container->list = NULL;
    free(container);
    container = NULL;
  }
  else
    fputs(plugerr(PLUGERR_CNA, NULL), stderr);
}

/* Unloads the plugin. You shouldn't call it, you should use uninstall(), which calls this function.
 * Returns non-zero on failure and zero on success. */

static int unload(void *handle)
{
  int status = dlclose(handle);
  if(status)
    status = PLUGERR_UNLOAD;
  return status;
}

/* Unloads the plugin and frees the plugin information structure. Does not return anything */

void uninstall(pcontainer *container, char *name)
{
  uint32_t hash = 0;
  hash = gen_hash(name, container->max);
  int status = 0;
  int plugin_num = 0;
  for(int i = 0; i < 2; i++) {
    if(container->list[hash]->plugin[i]) {
      status =  check_hash(container->list[hash]->plugin[i]->name, name, container->max);
      if(!status) {
        plugin_num = i;
        break;
      }
      else if(status && container->list[hash]->hash_col)
        status = PLUGERR_UFIXHASHCOL;
    }
    else if(i)
      status = PLUGERR_NUNLOAD;
  }
  switch(status) {
    case PLUGERR_NONE:
      status = unload(container->list[hash]->plugin[plugin_num]->handle);
      break;
    case PLUGERR_UFIXHASHCOL:
      fputs(plugerr(status, container->list[hash]->plugin[plugin_num]->name), stderr);
      status = unload(container->list[hash]->plugin[plugin_num]->handle);
      break;
    case PLUGERR_NUNLOAD:
      fputs(plugerr(status, name), stderr);
      break;
    default:
      fputs(plugerr(status, NULL), stderr);
  }
  plug_destroy(&container->list[hash]->plugin[plugin_num]);
  container->count--;
}

/* Loads a plugin. You shouldn't call this, you should use install(), which calls this function. Returns non-zero on
 * failure and zero on success. */

static int load(pinfo *plugin, char *path, char *symbol)
{
  int status = 0;
  plugin->handle = dlopen(path, RTLD_NOW);
  if(!plugin->handle)
    return status = PLUGERR_OPEN;
  plugin->function_ptr = (func_ptr)dlsym(plugin->handle, symbol);
  if(!plugin->function_ptr)
    status = PLUGERR_LOAD;
  return status;
}

static int prepare(pinfo **plugin, uint32_t hash, char *name, char *path, char *symbol)
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

int install(pcontainer *container, char *name, char *path, char *symbol)
{
  uint32_t hash = gen_hash(name, container->max);
  int status = 0;
  int plugin_num = 0;
  if(!container->list[hash])
    container->list[hash] = list_alloc(&status);
  else if(container->list[hash]->plugin[plugin_num]) {
    status = check_hash(container->list[hash]->plugin[plugin_num]->name, name, container->max);
    if(status && container->list[hash]->hash_col)
      status = PLUGERR_HASHCOL;
    else if(status && !container->list[hash]->hash_col) {
      status = PLUGERR_FIXHASHCOL;
      plugin_num = 1;
    }
    else if(!status)
      status = PLUGERR_LOADED;
  }
  char names[256];
  switch(status) {
    case 0:
      status = prepare(&container->list[hash]->plugin[plugin_num], hash, name,  path, symbol);
      container->list[hash]->hash_col = 0;
      break;
    case PLUGERR_FIXHASHCOL:
      status = prepare(&container->list[hash]->plugin[plugin_num], hash, name, path, symbol);
      snprintf(names, 256, "%s %s", container->list[hash]->plugin[!plugin_num]->name, name);
      fputs(plugerr(PLUGERR_FIXHASHCOL, names), stderr);
      container->list[hash]->hash_col = 1;
      break;
    case PLUGERR_HASHCOL:
      snprintf(names, 256, "%s %s", container->list[hash]->plugin[plugin_num]->name, name);
      fputs(plugerr(status, names), stderr);
      break;
    case PLUGERR_LOADED:
      fputs(plugerr(status, name), stderr);
      break;
    default:
      fputs(plugerr(status, NULL), stderr);
  }
  container->count++;
  return status;
}

int reinstall(pcontainer *container, char *name, char *path, char *symbol)
{
  int status = 0;
  uninstall(container, name);
  status = install(container, name, path, symbol);
  return status;
}

/* Gets the function pointer from the plugin you're requesting with name. Returns NULL on failure and a function
 * pointer on success. */

func_ptr getptr(pcontainer *container, char *name)
{
  uint32_t hash = gen_hash(name, container->max);
  int status = 0;
  int plugin_num = 0;
  if(container->list[hash]->hash_col) {
    status = check_hash(container->list[hash]->plugin[plugin_num]->name, name, container->max);
    if(status)
      plugin_num = 1;
  }
  if(!container->list[hash]->plugin[plugin_num]) {
    fputs(plugerr(PLUGERR_NLOADED, NULL), stderr);
    return NULL;
  }
  return container->list[hash]->plugin[plugin_num]->function_ptr;

}
