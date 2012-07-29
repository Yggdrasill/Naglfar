Naglfar Plugin Framework
=======================

About
-------

Naglfar is an abstract framework for plugins, based upon libdl. It's intended to abstract away the memory management of plugin loading into easily usable functions, while giving you descriptive error messages when something goes wrong. It is written to be extensible and usable in any scenario, with as few limitations as possible.

License
-------

Copyright (c) Yggdrasill/Wolfie
wolfie _ xlr@lavabit.com
[#hacking] @ irc.datnode.net

Naglfar is released under the terms of the [GNU GPLv3] [gpl3]
  [gpl3]: https://www.gnu.org/licenses/gpl.html

tl;dr You can use Naglfar (here-on referred to as "this software") for any purpose, redistribute it as you wish, modify it as you wish, including
redistributing your modified version, with or without compensation. You are not allowed to restrict these freedoms in
any version of this software, nor can you add any restrictions beyond those outlined by GPLv3.

Usage
-----

As previously stated in this README, Naglfar is intended to abstract away many frustrations with plugin loading in C.
This is done by giving you only five functions that are necessary to call should to load, get a callable function
pointer to the plugin and unloading the plugin. These functions are:
  plugContConstruct() /* This function takes an unsigned long as its input (maximum amount of plugins). It allocates memory to a main plugin
  container that holds the amount of plugins loaded, the max amount of plugins and an array of datastructures that
  contain the plugins themselves, and various information related to it. On failure it throws an error and returns NULL,
  and returns the main plugin container on success. */
  plugInstall() /* This function takes two arguments, which are the main plugin container and the name of a plugin. It
  hashes the name of the plugin and makes it fit into the main plugin container, then fills the data within
  sPlugCont->sPlugin[hash] (the plugin information structure). On failure it throws an error and returns a non-zero
  integer, on success it returns 0. */
  plugGetPtr() /* This function takes two arguments, which are the main plugin container and the name of a plugin. It
  hashes the name of the plugin, and returns the function pointer stored in sPlugCont->sPlugin[hash]. On failure it
  throws an error and returns NULL, and returns a function pointer on success.. */
  plugFree() /* This function takes two arguments, which are the main plugin container and the name of a plugin. It
  hashes the name of the plugin, and throws an error on failure. It does not return anything. */
  pluContDestruct() /* This function takes the main plugin container as its argument, and frees the main plugin
  container. If the main plugin container is not malloced, it will throw an error. It does not return anything. Note
  that you have to plugFree() your plugins, as this is not done by this function. */

Last Words
---------

I wrote this plugin framework for my own enjoyment, and to make it easier for me to load plugins in the future. I will
fix bugs as I discover them (although I have tested it a lot), but if you find an error, please inform me of this. It
will be much appreciated. I would also appreciate suggestions.
