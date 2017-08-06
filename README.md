# Effil
[![Build Status](https://travis-ci.org/effil/effil.svg?branch=master)](https://travis-ci.org/effil/effil)

Effil is a lua module for multithreading support.
It allows to spawn native threads and safe data exchange.
Effil has been designed to provide clear and simple API for lua developers including threads, channels and shared tables.

Effil supports lua 5.1, 5.2, 5.3 and LuaJIT.
Requires C++14 compiler compliance. Tested with GCC 4.9/5.3, clang 3.8 and Visual Studio 2015.

Read the [docs](https://github.com/loud-hound/effil/blob/master/README.md) for more information

# Table Of Contents
* [How to install](#how-to-install)
* [Quick guide](#quick-guide)
* [Important notes](#important-notes)
* [API Reference](#api-reference)
   * [Thread](#thread)
      * [effil.thread()](#runner--effilthreadfunc)
    * [Thread runner](#thread-runner)
      * [runner()](#thread--runner)
      * [runner.path](#runnerpath)
      * [runner.cpath](#runnercpath)
      * [runner.step]($runnerstep)
    * [Thread handle](#thread-handle)
      * [thread:status()](#status-err--threadstatus)
      * [thread:get()](#--threadgettime-metric)
      * [thread:wait()](#threadwaittime-metric)
      * [thread:cancel()](#threadcanceltime-metric)
      * [thread:pause()](#threadpausetime-metric)
      * [thread:resume()](#threadresume)
    * [Thread helpers](#thread-helpers)
      * [effil.thread_id()](#id--effilthread_id)
      * [effil.yield()](#effilyield)
      * [effil.sleep()](#effilsleeptime-metric)
    * [Table](#table)
      * [effil.table()](#table--effiltabletbl)
      * [__newindex: table[key] = value](#tablekey--value)
      * [__index: value = table[key]](#value--tablekey)
      * [effil.setmetatable()](#tbl--effilsetmetatabletbl-mtbl)
      * [effil.getmetatable()](#mtbl--effilgetmetatabletbl)
      * [effil.rawset()](#tbl--effilrawsettbl-key-value)
      * [effil.rawget()](#value--effilrawgettbl-key)
      * [effil.G](#effilg)
    * [Channel](#channel)
      * [effil.channel()](#channel--effilchannelcapacity)
      * [channel:push()](#channelpush)
      * [channel:pop()](#channelpoptime-metric)
      * [channel:size()](#channelsize)
    * [effil.type()](#effiltype)
    * [effil.size()](#size--effilsizetbl)
    * [effil.gc](#effilgc)
      * [effil.gc.collect()](#effilgccollect)
      * [effil.gc.count()](#count--effilgccount)
      * [effil.gc.step()](#old_value--effilgcstepnew_value)
      * [effil.gc.pause()](#effilgcpause)
      * [effil.gc.resume()](#effilgcresume)
      * [effil.gc.enabled()](#enabled--effilgcenabled)
    * [Time metrics](#time-metrics)

# How to install
### Build from src on Linux and Mac
1. `git clone git@github.com:effil/effil.git effil && cd effil`
2. `mkdir build && cd build && make -j4 install`
3. Copy effil.lua and libeffil.so/libeffil.dylib to your project.

### From lua rocks
`luarocks install effil`

# Quick guide
As you may now there are not much script
languages with **real** multithreading support
(Lua/Python/Ruby and etc has global interpreter lock aka GIL).
Effil solves this problem by running independent Lua VM
in separate native thread and provides robust communicating primitives
for creating threads (VM instances) and data sharing.

Effil library provides three major functions:
1. `effil.thread` - function which creates threads.
2. `effil.table` - table that persist in all threads and behaves just like regular lua table.
3. `effil.channel` - channel to send and receive data between threads.

And bunch of utilities to handle threads and tables as well.

### Examples
<details>
   <summary><b>Spawn the thread</b></summary>
   <p>

```lua
local effil = require("effil")

function bark(name)
    print(name .. " barks from another thread!")
end

-- run funtion bark in separate thread with name "Spaky"
local thr = effil.thread(bark)("Sparky")

-- wait for completion
thr:wait()
```

**Output:**
`Sparky barks from another thread!`
   </p>
</details>

<details>
   <summary><b>Sharing data with effil.channel</b></summary>
   <p>

```lua
local effil = require("effil")

-- channel allow to push data in one thread and pop in other
local channel = effil.channel()

-- writes some numbers to channel
local function producer(channel)
    for i = 1, 5 do
        print("push " .. i)
        channel:push(i)
    end
    channel:push(nil)
end

-- read numbers from channels
local function consumer(channel)
    local i = channel:pop()
    while i do
        print("pop " .. i)
        i = channel:pop()
    end
end

-- run producer
local thr = effil.thread(producer)(channel)

-- run consumer
consumer(channel)

thr:wait()
```
**Output:**
```
push 1
push 2
pop 1
pop 2
push 3
push 4
push 5
pop 3 
pop 4
pop 5
```
   </p>
</details>

<details>
   <summary><b>Sharing data with effil.table</b></summary>
   <p>

```lua
effil = require("effil")

-- effil.table transfers data between threads
-- and behaves like regualr lua table
local storage = effil.table { string_field = "first value" }
storage.numeric_field = 100500
storage.function_field = function(a, b) return a + b end
storage.table_field = { fist = 1, second = 2 }

function check_shared_table(storage)
   print(storage.string_field)
   print(storage.numeric_field)
   print(storage.table_field.first)
   print(storage.table_field.second)
   return storage.function_field(1, 2)
end

local thr = effil.thread(check_shared_table)(storage)
local ret = thr:get()
print("Thread result: " .. ret)

```
**Output:**
```
first value
100500.0
1.0
2.0
Thread result: 3.0
```
   </p>
</details>

# Important notes
Effil allows to transmit data between threads (Lua interpreter states) using `effil.channel`, `effil.table` or directly as parameters of `effil.thread`. In all cases Effil uses a common data handling principle:
 - Primitive types are transmitted 'as is': `nil`, `boolean`, `number`, `string`
 - Functions are dumped using [`string.dump`](#https://www.lua.org/manual/5.3/manual.html#pdf-string.dump) method and currently **it does not support function upvalues**
 - **Userdata and threads** are not supported. You can extend *userdata* support using C++ API of library but it's not documented for right now. 
 - Tables are serialized into `effil.table` recursively. So, any Lua table becomes a `effil.table`. Table serialization may take a lot of time on a big table, so, it's better to put your data directly to `effil.table` avoiding a table serialization. Let's consider 2 examples:
```Lua
-- Example #1
t = {}
for i = 1, 100 do
   t[i] = i
end
shared_table = effil.table(t)

-- Example #2
t = effil.table()
for i = 1, 100 do
   t[i] = i
end
```
In the example #1 we created a regular table, fill it and pass to `effil.table` constructor. In this case Effil needs to go through all table fields one more time. Another way is example #2 where we firstly created `eiffel.table` and after that we put data right to `effil.table`. The 2nd way pretty much faster try to follow this principle.

# API Reference

## Thread
`effil.thread` is a way to create thread. Threads can be stopped, paused, resumed and canceled.
All operation with threads can be synchronous (with timeout or infinite) or asynchronous.
Each thread runs with its own lua state.
**Do not run function with upvalues in** `effil.thread`. Use `effil.table` and `effil.channel` to transmit data over threads.
See example of thread usage [here](#examples).

### `runner = effil.thread(func)`
Creates thread runner. Runner spawns new thread for each invocation. 

**input**: *func* - any Lua function without upvalues

**output**: *runner* - [thread runner](#thread-runner) object to configure and run a new thread

## Thread runner
Allows to configure and run a new thread.
### `thread = runner(...)`
Run captured function with specified arguments in separate thread and returns [thread handle](#thread-handle).

**input**: Any number of arguments required by captured function.

**output**: [Thread handle](#thread-handle) object.

### `runner.path`
Is a Lua `package.path` value for new state. Default value inherits `package.path` form parent state.
### `runner.cpath`
Is a Lua `package.cpath` value for new state. Default value inherits `package.cpath` form parent state.
### `runner.step`
Number of lua instructions lua between cancelation points (where thread can be stopped or paused). Default value is 200. If this values is 0 then thread uses only [explicit cancelation points](#effilyield).

## Thread handle
Thread handle provides API for interaction with child thread.

### `status, err = thread:status()`
Returns thread status.

**output**:
- `status` - string values describes status of thread. Possible values are: `"running", "paused", "canceled", "completed" and "failed"`
- `err` - error description occurred in separate thread. This value is specified only if thread status == `"failed"`

### `... = thread:get(time, metric)`
Waits for thread completion and returns function result or nothing in case of error.

**input**: Operation timeout in terms of [time metrics](#time-metrics)

**output**: Results of captured function invocation or nothing in case of error.

### `thread:wait(time, metric)`
Waits for thread completion and returns thread status.

**input**: Operation timeout in terms of [time metrics](#time-metrics)

**output**: Returns status of thread. The output is the same as [`thread:status()`](#status-err--threadstatus)

### `thread:cancel(time, metric)`
Interrupts thread execution. Once this function was invoked 'cancellation' flag  is set and thread can be stopped sometime in the future (even after this function call done). To be sure that thread is stopped invoke this function with infinite timeout. Cancellation of finished thread will do nothing and return `true`.

**input**: Operation timeout in terms of [time metrics](#time-metrics)

**output**: Returns `true` if thread was stopped or `false`.

### `thread:pause(time, metric)`
Pauses thread. Once this function was invoked 'pause' flag  is set and thread can be paused sometime in the future (even after this function call done). To be sure that thread is paused invoke this function with infinite timeout.

**input**: Operation timeout in terms of [time metrics](#time-metrics)

**output**: Returns `true` if thread was paused or `false`. If the thread is completed function will return `false`

### `thread:resume()`
Resumes paused thread. Function resumes thread immediately if it was paused. This function does nothing for completed thread. Function has no input and output parameters.

## Thread helpers
### `id = effil.thread_id()`
Gives unique identifier.

**output**:  returns unique string `id` for *current* thread.

### `effil.yield()`
Explicit cancellation point. Function checks *cancellation* or *pausing* flags of current thread and if it's required it performs corresponding actions (cancel or pause thread).

### `effil.sleep(time, metric)`
Suspend current thread.

**input**:  [time metrics](#time-metrics) arguments.

## Table
`effil.table` is a way to exchange data between effil threads. It behaves almost like standard lua tables.
All operations with shared table are thread safe. **Shared table stores** primitive types (number, boolean, string), function, table, light userdata and effil based userdata. **Shared table doesn't store** lua threads (coroutines) or arbitrary userdata. See examples of shared table usage [here](#examples)

### Notes: shared tables usage

Use **Shared tables with regular tables**. If you want to store regular table in shared table, effil will implicitly dump origin table into new shared table. **Shared tables always stores subtables as shared tables.**

Use **Shared tables with functions**. If you want to store function in shared table, effil will implicitly dump this function and saves it in internal representation as string. Thus, all upvalues will be lost. **Do not store function with upvalues in shared tables**.

### `table = effil.table(tbl)`
Creates new **empty** shared table.

**input**: `tbl` - is *optional* parameter, it can be only regular Lua table which entries will be **copied** to shared table.

**output**: new instance of empty shared table. It can be empty or not, depending on `tbl` content.

### `table[key] = value` 
Set a new key of table with specified value.

**input**:
- `key` - any value of supported type. See the list of [supported types](#important-notes)
- `value` - any value of supported type. See the list of [supported types](#important-notes)

### `value = table[key]` 
Get a value from table with specified key.

**input**: `key` - any value of supported type. See the list of [supported types](#important-notes)
**output**: `value` - any value of supported type. See the list of [supported types](#important-notes)

### `tbl = effil.setmetatable(tbl, mtbl)`
Sets a new metatable to shared table. Similar to standard [setmetatable](https://www.lua.org/manual/5.3/manual.html#pdf-setmetatable).

**input**:
- `tbl` should be shared table for which you want to set metatable. 
- `mtbl` should be regular table or shared table which will become a metatable. If it's a regular table *effil* will create a new shared table and copy all fields of `mtbl`. Set `mtbl` equal to `nil` to delete metatable from shared table.

**output**: just returns `tbl` with a new *metatable* value similar to standard Lua *setmetatable* method.

### `mtbl = effil.getmetatable(tbl)`
Returns current metatable. Similar to standard [getmetatable](https://www.lua.org/manual/5.3/manual.html#pdf-getmetatable)

**input**: `tbl` should be shared table. 

**output**: returns *metatable* of specified shared table. Returned table always has type `effil.table`. Default metatable is `nil`.

### `tbl = effil.rawset(tbl, key, value)`
Set table entry without invoking metamethod `__newindex`. Similar to standard [rawset](https://www.lua.org/manual/5.3/manual.html#pdf-rawset)

**input**:
- `tbl` is shared table. 
- `key` - key of table to override. The key can be of any [supported type](#important-notes).
- `value` - value to set. The value can be of any [supported type](#important-notes).

**output**: returns the same shared table `tbl`

### `value = effil.rawget(tbl, key)`
Gets table value without invoking metamethod `__index`. Similar to standard [rawget](https://www.lua.org/manual/5.3/manual.html#pdf-rawget)

**input**:
- `tbl` is shared table. 
- `key` - key of table to receive a specific value. The key can be of any [supported type](#important-notes).

**output**: returns required `value` stored under a specified `key`

### `effil.G`
Is a global predefined shared table. This table always present in any thread (any Lua state).
```lua
effil = require "effil"

function job()
   effil = require "effil"
   effil.G.key = "value"
end

effil.G.key == "value"
```

## Channel
`effil.channel` is a way to sequentially exchange data between effil threads. It allows push values from one thread and pop them from another. All operations with channels are thread safe. **Channel passes** primitive types (number, boolean, string), function, table, light userdata and effil based userdata. **Channel doesn't pass** lua threads (coroutines) or arbitrary userdata. See examples of channel usage [here](#examples)

### `channel = effil.channel(capacity)`
Channel capacity. If `capacity` equals to `0` size of channel is unlimited. Default capacity is `0`.

### `channel:push()`
Push value. Returns `true` if value fits channel capacity, `false` otherwise. Supports multiple values.

### `channel:pop()`
Pop value. If value is not present, wait for the value.

### `channel:pop(time, metric)`
Pop value with timeout. If time equals `0` then pop asynchronously.

### `channel:size()`
Get actual size of channel.

## `size = effil.size(tbl)`
Returns number of entries in shared table.

**input**: `tbl` is [shared table](#effiltable) or [channel](#effilchannel) Lua table which entries will be **copied** to shared table.

**output**: new instance of shared table

## `effil.type`
Threads, channels and tables are userdata. Thus, `type()` will return `userdata` for any type. If you want to detect type more precisely use `effil.type`. It behaves like regular `type()`, but it can detect effil specific userdata. There is a list of extra types:

```Lua
effil.type(effil.thread()) == "effil.thread"
effil.type(effil.table()) == "effil.table"
effil.type(effil.channel() == "effil.channel"
```

## effil.gc
Effil provides custom garbage collector for `effil.table` and `effil.table`. It allows safe manage cyclic references for tables and channels in multiple threads. However it may cause extra memory usage. `effil.gc` provides a set of method configure effil garbage collector. But, usually you don't need to configure it.

### Garbage collection trigger
Garbage collection may occur with new effil object creation (table or channel).
Frequency of triggering configured by GC step.
For example, if Gc step is 200, then each 200'th object creation trigger GC.    

### How to cleanup all dereferenced objects 
Each thread represented as separate state with own garbage collector.
Thus, objects will be deleted eventually.
Effil objects itself also managed by GC and uses `__gc` userdata metamethod as deserializer hook.
To force objects deletion:
1. invoke standard `collectgarbage()` in all threads.
2. invoke `effil.gc.collect()` in any thread.

### `effil.gc.collect()`
Force garbage collection, however it doesn't guarantee deletion of all effil objects.

### `count = effil.gc.count()`
Show number of allocated shared tables and channels.

**output**: returns current number of allocated objects. Minimum value is 1, `effil.G` is always present. 

### `old_value = effil.gc.step(new_value)`
Get/set GC step. Default is `200`.

**input**: `new_value` is optional value of step to set. If it's `nil` then function will just return a current value.

**output**: `old_value` is current (if `new_value == nil`) or previous (if `new_value ~= nil`) value of step.

### `effil.gc.pause()`
Pause GC. Garbage collecting will not be performed automatically. Function does not have any *input* or *output*

### `effil.gc.resume()`
Resume GC. Enable automatic garbage collecting.

### `enabled = effil.gc.enabled()`
Get GC state.

**output**: return `true` if automatic garbage collecting is enabled or `false` otherwise. By default returns `true`.

## Time metrics:
All operations which use time metrics can be bloking or non blocking and use following API:
`(time, metric)` where `metric` is time interval like and `time` is a number of intervals.

Example:
- `thread:get()` - infinitely wait for thread completion.
- `thread:get(0)` - non blocking get, just check is thread finished and return
- `thread:get(50, "ms")` - blocking wait for 50 milliseconds.

List of available time intervals:
- `ms` - milliseconds;
- `s` - seconds (default);
- `m` - minutes;
- `h` - hours.
