Naglfar Plugin Framework
=======================

About
-------

Naglfar is an abstract framework for plugins, based upon libdl. It's intended to abstract away the memory management of plugin loading into easily usable functions, be thread-safe and give you error messages of what goes wrong if something does go wrong. It is written to be extensible and usable in any scenario, with as few limitations as possible. All plugins should be compiled with -fPIC -shared in either gcc or clang, I do not know the compilation options for other compilers.

License
-------

Copyright (C) Yggdrasill/Wolfie

[yggdrasill@lavabit.com] [email]

[#hacking @ DatNode] [irc]

Naglfar is released under the terms of the [GNU GPLv3] [gpl3]

tl;dr:

You can use Naglfar (here-on referred to as "this software") for any purpose, redistribute it as you wish, modify it as you wish, including
redistributing your modified version, with or without compensation. You are not allowed to restrict these freedoms in
any version of this software, nor can you add any restrictions beyond those outlined by GPLv3.

Depends
------

### libdl

This one should be obvious, as Naglfar is just an abstraction over libdl. Naglfar simply removes the manual management behind dynamic code loading.

### pthread

This is not necessary unless you personally define THREADING. If THREADING is not defined it does not require linking to pthread.

### Vígríðr

Vígríðr is hosted in its own repo, and it can be found [here][vigrior]. It implements a hashing algorithm that should spread hashes out widely.

Usage
-----

_For thread-safety, you should define THREADING._

As previously stated in this README, Naglfar is intended to abstract away some frustrations with plugin loading in C. This is done by giving you only five functions (and one optional function) that are necessary to call in order to load a plugin, execute the plugin and unloading the plugin. These functions are:

### plug _ construct()

This function takes an unsigned long as its input (maximum amount of plugins). It allocates memory to a main plugin container that holds the amount of plugins loaded, the max amount of plugins and an array of datastructures that contain the plugins themselves, and various information related to it. On failure it throws an error and returns NULL, and returns the main plugin container on success.

### plug _ install()

This function takes four arguments, which are the main plugin container, the name of a plugin, the path to the plugin and the symbol to look for in the compiled plugin. It hashes the name of the plugin and makes it fit into the main plugin container, then fills the data within the correct plugin information structure. On failure it throws an error and returns a non-zero integer, on success it returns 0.

### plug _ exec()

This function takes three arguments, which are the main plugin container, the name of a plugin and the argument which you want to pass to the plugin as a void pointer. It hashes the name of the plugin, and from this executes the correct one. On failure it throws an error and returns NULL, and the function pointer of the plugin on success. It is up to you to ensure a plugins safe running, plug _ exec does not do anything beyond calling it.

### plug _ uninstall()

This function takes two arguments, which are the main plugin container and the name of a plugin. It hashes the name of the plugin, and throws an error on failure. It does not return anything.

### plug _ reinstall()

This function takes four arguments, which are the main plugin container, the name of the plugin, the path to the plugin and the symbol to look for. Internally it calls uninstall() and then calls install(). It return non-zero on failure and 0 on success.

### plug _ destruct()

This function takes the main plugin container as its argument, and frees the main plugin container. If the main plugin container is not malloced, it will throw an error. It does not return anything.

NOTE: This function _IS NOT_ thread-safe, it WILL destroy all your plugins.

Last Words
---------

I wrote this plugin framework for my own enjoyment, and to make it easier for me to load plugins in the future. I will fix bugs as I discover them (although I have tested it a lot), but if you find an error, please inform me of this. It will be much appreciated. I would also appreciate suggestions.

[gpl3]: https://www.gnu.org/licenses/gpl.html
[irc]: irc://irc.datnode.net/hacking
[email]: mailto:yggdrasill@lavabit.com
[vigrior]: https://github.com/Yggdrasill/Vigrior
