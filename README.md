XTLua
====

XTLua is a modification of XLua which runs lua - specifically before_physics and after_physics lua functions in a worker thread aysnchronously to X-Plane.

The goal is to facilitate more complex Lua calcuations without impacting the X-Plane frame rate. The Lua bindings of XLua have been replaced with xtluaDefs in xpdatarefs.cpp which synchronize the current variable states of XTLua with X-Plane once each flight model frame.

Release Notes
-----------------
2.0.2a4 - 06/27/2020
feature complete

0.0.5b1 -  05/08/2020
rewrite XTLuaChar

0.0.2b1 -  04/19/2020

Initial Release
XLua
====

XLua is a very simple Lua adapter plugin for X-Plane. It allows authors to create and modify commands and add create new datarefs for X-Plane aircraft.

XLua's functionality is in its core similar to Gizmo, SASL and FlyWithLua, but it is much smaller and has only a tiny fraction of these other plugn's functionality. The goals of XLua are simplicity, ease of use and to be light weight and minimal. XLua is meant to provide a small amount of "glue" to aircraft authors to connect art assets to the simulator itself.

XLua is developed internally by Laminar Research and is intended to help our internal art team, but anyone is free to use it, modify it, hack it, etc.; it is licensed under the MIT/X11 license and is thus Free and Open Source Software.

XLua is **not** meant to be an "official" Lua plugin for X-Plane, and it definitely does not replace any of the existing Lua plugins, all of which have significantly more features than XLua itself.

Documentation
======
XTLua includes most of the same exports as [XLua](https://forums.x-plane.org/index.php?/forums/topic/154351-xlua-scripting/&tab=comments#comment-1460039)

Script packaging and basic structure
--

XLua scripts are organized into "modules". ­ Each module is a fully independent script that runs inside your aircraft.

Modules are completely independent of each other and do not share data. They are designed for isolation.
You do not need to use more than one module. Multiple modules are provided to let work be copied as a whole from one aircraft to each other.
Each module can use one .lua script file or more than one .lua script file, depending on author’s preferences.
So, here are two methods you can use for this:

As you described, place each script in its own directory (module) with the script name matching the folder name. See the default 747 for examples.
You can also place multiple scripts in one directory (module). The "main" script file is loaded and run automatically. The other scripts in a directory will load and run based on naming convention used. If the script names include the directory name, they will load and run automatically (see the default C90 for examples), otherwise you will need to use dofile(script_name) to load and run the other scripts.
Sub­folders in the scripts folder are not allowed. All modules must be within "scripts".

The file init.lua is part of the XLua plugin itself and should not be edited or removed.


How a module script runs
--

When your aircraft is loaded (before the .acf and art files are loaded) the XLua plugin is loaded, and it loads and runs each of your module scripts.

When your module’s script is run, all Lua code that is outside of any function is run immediately. Your script should use this immediate execution only to:

Create new datarefs and commands specific to your module and
Find built­-in datarefs and commands from the sim. All other work should be deferred until you receive additional callbacks.
Once the aircraft itself has been loaded, your script will receive a number of major callbacks. These callbacks run a function in your script if a function with the matching name is found. You do not have to implement a function for every major callback, but you will almost certainly want to implement at least some of them.

Besides major callbacks, one other type of function in your script will run: when you create or modify commands and when you create writeable datarefs, you provide a Lua function that is called when the command is run by the user or when the dataref is written (e.g. by the panel or a manipulator).

Creating and using datarefs and commands
--

XTLua treats datarefs very differently than XLua due to its asynchronous nature.
Datarefs can be be created with:
- **create_dataref("name", "type", my_func)** API creates a writeable dataref.  The function is called each time a plugin other than your script writes to the dataref.  It is OK to have the function do nothing.
Type can be one of
  - "number"
  - "string"
  - "array[n]"

- **create_command(name, description, function)** API creates a custom command.

create_dataref and create_command can only be used inside a script in init/script/ 

XTLua main scripts can interface with commands and datarefs using
- **find_command(name)**
- **replace_command(name, handler)**
- **wrap_command(name, before_handler, after_handler)**

- **find_dataref("name")** creates a variable which will read and write an dataref value to use inside XP.

There are some important differences with the way find_dataref works compared with XLua
 - A dataref is only _read_ from xplane when it has been used, for XTLua created datarefs this does not cause problems, but it does mean that external datarefs (sim/ style datarefs or datarefs from another plugin) may not be fresh when used, for example within an if statement. This improves performance when reading large numbers of datarefs, however it can cause if statements to behave strangely if an external plugin/xplane is independantly setting the dataref. Always reading datarefs that need to be fresh when used can be achieved by assigning them a local variable (e.g. local updateMe = dataref)
 - A dataref update is visibile to xplane _asynchronously_, not _immediately_ 

Timers
--

You can create a timer out of any function. Timer functions take no arguments.

run_at_interval(func, interval)
Runs func every interval seconds, starting interval seconds from the call.

run_after_time(func, delay)
Runs func once after delay seconds, then timer stops.

run_timer(func, delay, interval)
Runs func after delay seconds, then every interval seconds after that.

stop_timer(func)
This ensures that func does not run again until you re­schedule it; any scheduled runs from previous calls are canceled.

is_timer_scheduled(func)
This returns true if the timer function will run at any time in the future. It returns false if the timer isn’t scheduled or if func has never been used as a timer.


Release Notes
-----------------

1.0.0r1 - 11/6/2017

Bug fixes:
 * Support for unicode install paths on Windows.
 * Timing source is now sim time and not user interface time. Fixes scripts breaking on pause.
 * Debug logging of missing datarefs.
 * Full back-trace of uncaught Lua exceptions.

1.0.0b1 - 11/26/16

Initial Release
