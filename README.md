# WinXInputEmu

An XInput emulation layer that supports conditionally generating gamepad from keyboard and mouse input.

This is accomplished by renaming the dll provided by this project to one of the XInput dll names such as `xinput1_4.dll`, and dropping it into the application folder (where the .exe is located). This shadows the system dll, so our custom logic can intercept and process API calls from the application.

## Building

1. Install Visual Studio with Desktop C++ workload. Author used VS2022, it is not gaurenteed that lower version will work.
2. Install [vcpkg](https://github.com/microsoft/vcpkg/) and enable [MSBuild integration](https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/msbuild-integration)
3. Install the following packages in vcpkg:
   - `fmt`
   - `imgui[core,docking-experimental,win32-binding,dx11-binding]`
   - `tomlplusplus`
4. Open the .sln file, select the desired architecture, build

Note this project uses `/c++:latest`.

Note you should install packages in vcpkg with a triplet that matches the one you use in Visual Studio to build the solution. For example if you wish to build a x86 32bit dll, you should make sure that the triplet `x86-windows` is used in vcpkg.

## How to use

Check your target .exe's architecture, and build the dll with one that matches. This means if the .exe is 32bit, the dll should also be 32bit.

Check which XInput dll is loaded by the target .exe. You can do this by using something like ProcessExplorer from sysinternals and check the modules loaded by the application process. Usually, this is `XInput1_4.dll`, but some older applications may use `xinput1_3.dll` or even others.

Rename the built dll to your application's desired XInput file name, and drop it in the same directory as the .exe. Also create a file named `WinXInputEmu.toml` in the same directory, this is the config file.

(By default every API call will be forwarded directly to the system XInput: see the config docs.)

When the application is launched, a tool window will pop open. You may close it if you wish -- there is a hotkey to reopen it, though by default it is bound.

## Config file

- If an entry has a comment `#default value`, it means if such a config value is not specified, this value will be used
- If an entry has a comment `#keycode`
   - Accepts a string that represents a key
      - Use one of the strings defined in the `InitKeyCodeConv()` function of [inputdevice.cpp](WinXInputEmu/inputdevice.cpp)
   - An empty string means nothing is bound
- User profiles
   - Each user profile is defined as a subtable in the table `UserProfiles`.
   - Its name is the subtable's key, which should be a string.
      - The name may contain any valid TOML string, as long as it's not one of the reserved names.
   - Its contents determines what keys and/or mouse actions are mapped to what gamepad movements. See the comments in the snippet below.
   - Reserved names
      - The special name "NULL" means a gamepad that never has any input. This can be used to hide a real gamepad that may be conencted in the port, detected by system XInput.
      - The special name "" (an empty string) means to forward this gamepad to the system XInput.

```toml
[HotKeys]
ShowUI = "" #keycode, default value
CaptureCursor = "" #keycode, default value

[Binding]
# Value should be a UserProfile name.
Gamepad0 = "" #default value
Gamepad1 = "" #default value
Gamepad2 = "" #default value
Gamepad3 = "" #default value

# Just an example profile
[UserProfiles."myprofile"]
# ----- Joysticks -----

# either "keyboard" or "mouse" -- demonstrated in LStick and RStick respectively
LStick.Type = "keyboard"
LStick.Up = "W" #keycode
LStick.Down = "S" #keycode
LStick.Left = "A" #keycode
LStick.Right = "D" #keycode

RStick.Type = "mouse"
RStick.Sensitivity = 15.0
RStick.NonLinearSensitivity = 1.0
RStick.Deadzone = 0.0

# ----- DPad -----
DpadUp = "UpArrow" #keycode
DpadDown = "DownArrow" #keycode
DpadLeft = "LeftArrow" #keycode
DpadRight = "RightArrow" #keycode

# ----- Other -----
A = "J" #keycode
B = "K" #keycode
X = "U" #keycode
Y = "I" #keycode
LB = "LShift" #keycode
LT = "Q" #keycode
RB = "Space" #keycode
RT = "E" #keycode
Start = "1" #keycode
Back = "2" #keycode

# Another example profile
[UserProfiles."Nintendo DS-like"]
A = "Numpad6"
B = "Numpad2"
X = "Numpad8"
Y = "Numpad4"
LB = "Numpad7"
RB = "Numpad9"
DpadUp = "UpArrow"
DpadDown = "DownArrow"
DpadLeft = "LeftArrow"
DpadRight = "RightArrow"
```
