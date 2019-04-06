


# <img src="https://molecular-matters.com/docs/livepp/assets/img/favicon.png" width="30" height="30" />Live++   **//**<img src="https://www.cryengine.com/assets/img/cryengine-logo.svg" width="200" height="20" />

This plugin enables the use of [Live++](https://molecular-matters.com/products_livepp.html) by Molecular Matter inside of Cryengine.   
Live++ allows for easy hot-reloading and on the fly changes to native C++ code.

Prerequirements
------
A valid License and the Live++ sdk is required.

Building from source
------
* Clone this Repository into your code/plugins folder.
* Place the Live++ SDK into the SDK folder.
* Add Live++ to your cmake setup.   
   >**add_subdirectory(Live++/Module)** in **CryPlugins/CMakeLists.txt**
* Build with PLUGIN_LIVE++ enabled, the required binaries will be copied to your output folder.
  
Usage
------
### Simple:
* Make sure the Live++ plugin dll and the Live++ folder are in your binary directory. (Building from source copies them there)
* Enable Live++ for your Plugin/Project/Library via the console commands or the module list
  
   ```cpp
   //Enables these Modules during runtime
   lpp_EnableModules "MyGame MyPlugin.dll" 
   //List of enabled modules. Can be set in cfg before startup
   lpp_enabledModules  "MyGame MyPlugin.dll" 
   //Enables Live++ for all loaded Plugins
   lpp_enableForAllPlugins 1
   ```
* Use Live++ as specefied in the documentation
  > Note: This does not enable all required compiler settings for your module. It should still work for basic code changes.
### Advanced:
* Same steps as simple
* Add this code snippet to the **CMakeLists.txt** of the project you want to enable Live++ for:  
   ```cmake
   include(PATH_TO_LIVE++_REPO/Live++.cmake)
   LIVEPP_ENABLE_FOR_TARGET(${THIS_PROJECT})
   ```
* The Live++ settings can now be enabled/disabled on a per Project basis in the CMake GUI

API
------
This Plugin offers an easy API to interact with Live++ functionality.   
Listeners for Patch and Compile events can be implemented as well.

The API is defined and documented in [ Interface/ILive++.h](Interface/ILive++.h) and can be used like any other Cryengine plugin.

Console Variables
------

```cpp
//Manually triggers a Live++ recompile
lpp_DoRecompile

/*
Enables the specified modules for live++ usage.
If no extension is specified, .dll will be assumed.
*/
lpp_EnableModules

/*
Disables the specified modules for live++ usage.
If no extension is specified, .dll will be assumed.
*/
lpp_DisableModules

//Enables the current executable to be used with Live++
lpp_enableForExecutable

//Enables Live++ for all loaded engine plugins
lpp_enableForAllPlugins

//Enables the Live++ exception handler instead of the internal one
lpp_useExceptionHandler

//Use the cryengine cmake build system for Live++. --NOT IMPLEMENTED
lpp_useExternalBuildSystem

/*
Stage at which a Live++ Sync Point should be triggered.
0: No automatic sync point
1: pre-update
2: post-update
*/
lpp_syncPointMode 

//Allow the use of external manual sync point calls through the api.
lpp_allowExternalSyncPoints 

//Sets Live++ to load modules asynchronously --Does nothing right now
lpp_enableModulesAsync

/*
List of modules that are enabled for Live++.
If read from config, these modules will be loaded on startup.
If no extension is specified, .dll will be assumed.
*/
lpp_enabledModules 

//Override name for the Live++ process group. Project name + version by default
lpp_groupNameOverride
```