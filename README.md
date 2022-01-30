XTLua
====

XTLua is a modification of XLua which runs lua - specifically before_physics and after_physics lua functions in a worker thread aysnchronously to X-Plane.

The goal is to facilitate more complex Lua calcuations without impacting the X-Plane frame rate. The Lua bindings of XLua have been replaced with xtluaDefs in xpdatarefs.cpp which synchronize the current variable states of XTLua with X-Plane once each flight model frame.


XLua
====

XLua is a very simple Lua adapter plugin for X-Plane. It allows authors to create and modify commands and add create new datarefs for X-Plane aircraft.

XLua's functionality is in its core similar to Gizmo, SASL and FlyWithLua, but it is much smaller and has only a tiny fraction of these other plugn's functionality. The goals of XLua are simplicity, ease of use and to be light weight and minimal. XLua is meant to provide a small amount of "glue" to aircraft authors to connect art assets to the simulator itself.

XLua is developed internally by Laminar Research and is intended to help our internal art team, but anyone is free to use it, modify it, hack it, etc.; it is licensed under the MIT/X11 license and is thus Free and Open Source Software.

XLua is **not** meant to be an "official" Lua plugin for X-Plane, and it definitely does not replace any of the existing Lua plugins, all of which have significantly more features than XLua itself.

## Release Notes

**1.1.0r1 - 08/21/2021**

Features:
 * Upgraded to LuaJIT 2.1
 * LuaJIT GC64 support is now enabled so that custom allocator is no longer necessary.
 * Mac arm64 support

**1.0.0r1 - 11/6/2017**

Bug fixes:
 * Support for unicode install paths on Windows.
 * Timing source is now sim time and not user interface time. Fixes scripts breaking on pause.
 * Debug logging of missing datarefs.
 * Full back-trace of uncaught Lua exceptions.

**1.0.0b1 - 11/26/16**

Initial Release

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

2.0.2a4 - 06/27/2020
feature complete

0.0.5b1 -  05/08/2020
rewrite XTLuaChar

0.0.2b1 -  04/19/2020

XTLua Initial Release

XLua 1.0.0r1 - 11/6/2017

Bug fixes:
 * Support for unicode install paths on Windows.
 * Timing source is now sim time and not user interface time. Fixes scripts breaking on pause.
 * Debug logging of missing datarefs.
 * Full back-trace of uncaught Lua exceptions.

**1.0.0b1 - 11/26/16**

Initial Release

## Documentation

### Script packaging and basic structure

XLua scripts are organized into “modules” - each module is a fully independent script that runs inside your aircraft.

* Modules are completely independent of each other and do not share data  -they are designed for isolation.
* You do not need to use more than one module. Multiple modules are provided to let work be copied as a whole from one aircraft to each other.
* Each module can use one .lua script file or more than one .lua script file, depending on author’s preferences.

The installation of the plugin looks like this on disk:

```
/My airplane
    /plugins
        /xlua
        /lin_x64 <-- these contain the real binaries
        /mac_x64
        /win_x64
        init.lua  <-- part of the plugin, do not modify
        /scripts  <-- your aircraft specific scripts
            /adf
                adf.lua
                adf_helpers.lua
            /stec_500
                stec_500.lua
```

In this example, there are two modules installed: “stec_500” and “adf”.  **The main lua file for a module has the same name as the module itself**.

The adf module has a second lua file - for this file to be used, it must be “included” from adf.lua using `dofile("adf_helpers.lua")`. Only the “main” lua script in a module is run automatically.

Sub-folders in the scripts folder are not allowed - all modules must be within “scripts”.

The file “init.lua” is part of the XLua plugin itself and should not be edited or removed.

### How a Module Script Runs

When your aircraft is loaded (before the .acf and art files are loaded) the XLua plugin is loaded, and it loads and runs each of your module scripts.

When your module’s script is run, all Lua code that is outside of any function is run immediately. Your script should use this immediate execution only to:

* Create new datarefs and commands specific to your module and
* Find built-in datarefs and commands from the sim.

All other work should be deferred until you receive additional callbacks.

Once the aircraft itself has been loaded, your script will receive a number of major callbacks - these callbacks run a function in your script if a function with the matching name is found. You do not have to implement a function for every major callback, but you will almost certainly want to implement at least some of them.

Besides major callbacks, one other type of function in your script will run: when you create or modify commands and when you create writeable datarefs, you provide a Lua function that is called when the command is run by the user or when the dataref is written (e.g. by the panel or a manipulator).

### Major callbacks

The non-function part of your script is run top-to-bottom when your airplane is loaded - this is the right time to create new commands and datarefs.

Most work is done in response to major callbacks - functions that are automatically called in your script during X-Plane’s execution.  Those names are:

* `aircraft_load()` - run once when your aircraft is loaded. This is run after the aircraft is initialized enough to set overrides.
* `aircraft_unload()` - run once when your aircraft is unloaded.
* `flight_start()` - run once each time a flight is started.  The aircraft is already initialized and can thus be customized.  This is always called after `aircraft_load` has been run at least once.
* `flight_crash()` - called if X-Plane detects that the user has crashed the airplane.
* `before_physics()` - called every frame that the sim is not paused and not in replay, before physics are calculated
* `after_physics()` - called every frame that the sim is not paused and not in replay, after physics are calculated
* `after_replay()` - called every frame that the sim is in replay mode, regardless of pause status.

These functions take no arguments.

### Global variables provided by XLua

Xlua provides two global variables that are available from all callbacks:

* `SIM_PERIOD` - this contains the duration of the current frame in seconds (so it is alway a fraction).  Use this to normalize rates, e.g. to add 3 units of fuel per second in a per-frame callback you’d do fuel = fuel + 3 * SIM_PERIOD.
* `IN_REPLAY` - evaluates to 0 if replay is off, 1 if replay mode is on.

### Datarefs

Datarefs work just like variables - that is, once you make a variable from a dataref, you just use it and change it like a regular lua variable.  Lua supports 3 kinds of datarefs:

* **Numbers.**  Integer, float and double datarefs in the X-Plane SDK all just act as “numbers” in Lua. Furthermore, if you set up a float or int array dataref but include the subscript in the name, e.g. `sim/flightmodel2/wing/aileron[2]` then you get a number-type dataref.
* **Arrays.** Integer and float arrays that do not have the subscripts in the name act like arrays (tables, really) in Lua.  Besides supporting array access with [0] etc. the property .len tells the number of elements in the dataref.  WARNING: plugin datarefs are ZERO based, so if len == 2 then only [0] and [1] are valid.
* **Strings.**  Raw data arrays in the SDK are treated as strings in Lua.

`x = find_dataref("dataref_name")`

This creates a new lua variable `x` that is mapped to the named dataref that already exists. Its type (string, number or table) depends on the type of the named dataref.  Use this to map X-Plane’s datarefs into your plugin.

`x = create_dataref("name", "type")`

This creates a new read-only dataref `name`.  Type must be one of:

* `number` - creates a single number dataref.
* `string` - creates a string dataref.
* `array[5]` - creates an array - you specify the number of elements.

Note that this dataref can be changed by your script, e.g. by doing `x = 5`.  But it cannot be changed by X-plane or other plugins.

`x = create_dataref("name", "type", my_func)`

This creates a writeable dataref; the function `my_func` (which takes no arguments) is called each time a plugin OTHER than your script writes to the dataref.  It is okay to have the function do nothing.

### Command classes

Commands are objects that you can act on. You start by finding or created a command, e.g.

`pause_cmd = find_command("sim/operation/pause_toggle")`

You can then act on it, e.g.

`pause_cmd:once()`

Will pause the sim. Command objects have three methods:

* `once()` - runs the command exactly once.
* `start()` - starts holding down the command.
* `stop()` - stops holding down the command.

Make sure every call to `start()` is balanced by a `stop()`!!!

You can also create your own commands:

`c = create_command(name, description, function)`

The `name` is the permanent name, e.g. `my_namespace/weapons/fire_torpedoes`
The `description` is a human readable name, e.g. `"This fires the photon torpedos"`.
The `function` is a lua callback that is called when the command is run:

```
function command_handler(phase, duration) 
end
```

The `phase` is an integer that will be `0` when the command is first pressed, `1` while it is being held down, and `2` when it is released.  For any command invocation, you are guaranteed exactly one "begin" and one "end", with one or more “held down” in the middle.  But note that if the user has multiple joysticks mapped to the command you could get a second "down" while the first one is running.

The `duration` is how long the command has been held down in seconds, starting at 0.

`create_command` returns a command object but you can ignore that object if you just need to make a command and not run it yourself.

You can customize existing commands in two ways:

`c = replace_command(name, handler)`

This takes an existing command (e.g. one of X-Plane’s commands) and replaces the action the command does with your handler - the handler has the same syntax as the custom command handler - it takes a phase and duration.

`c = wrap_command(name, before_handler, after_handler)`

This takes an existing command (e.g. one of X-Plane’s commands) and installs two new handlers - the before handler runs before X-Plane, and the after handler runs after. You can use this to “listen” for a command and do extra stuff without losing X-Plane’s capabilities. You must provide both functions even if one does nothing.

### Timers

You can create a timer out of any function.  Timer functions take no arguments. Several commands are provided:

`run_at_interval(func, interval)`

Runs `func` every `interval` seconds, starting `interval` sections from the call.

`run_after_time(func, delay)`

Runs `func` once after `delay` seconds, then timer stops.

`run_timer(func, delay, interval)`

Runs `func` after `delay` seconds, then every `interval` seconds after that.

`stop_timer(func)`

This ensures that `func` does not run again until you re-schedule it; any scheduled runs from previous calls are canceled.

`is_timer_scheduled(func)`

This returns true if the timer function will run at any time in the future.  It returns false if the timer isn’t scheduled or if `func` has never been used as a timer.

### Debugging facilities

Using `print("")` will output to the terminal on Mac/Linux/Windows (provided X-Plane has been started from the command line), and will also output a line to the X-Plane log.txt file, prefixed with `LUA:`

Using "Developer > Reload the Current Aircraft and Art" X-Plane menu will also reload your XLua script files.

### FAQ

* **Q:** I'm getting a `attempt to index global '...' (a nil value)` while trying to access a member of, sort or iterate over a global table; or I'm getting unexpected results from a `pairs` operation over a global table.
* **A:** This is a known issue (#4). You need to either make your table local or declare your global table using `raw_rable(my_tablename)` before you use it. Please note that you cannot assign dataref references inside a raw table.


* **Q:** Does XLua have any graphics or screen drawing capabilities?
* **A:** There is (currently) no way to draw to the screen/window in XLua.


* **Q:** Is there a way to compile a XLua script in a binary plugin (xpl file)?
* **A:** No. If you want or need to encrypt Lua scripts, you need to use SASL.

