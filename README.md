# **WoWpadX**

**A high-performance controller mapping utility for ConsolePortLK**

## **What is WoWpadX?**

<img width="408" height="720" alt="Image" src="https://github.com/user-attachments/assets/cd98f59d-6054-4377-b80a-346f03a6adf9" />
<img width="395" height="710" alt="Image" src="https://github.com/user-attachments/assets/33b8a4d0-5b43-4a8b-83ad-e2d875a6eba8" />

WoWpadX is a ground-up rewrite of the original controller mapping concepts found in [WoWmapper](https://github.com/topher-au/WoWmapper) and [WoWmapperX](https://github.com/leoaviana/WoWmapperX).  
It handles input from modern controllers via **SDL3** and converts them into precise button presses and mouse movements. These are then processed by [ConsolePortLK](https://github.com/leoaviana/ConsolePortLK) to provide a native console-like experience in World of Warcraft. WoWpadX includes advanced features such as rumble feedback, automatic cursor centering, and input assistance utilities aiming to make controller gameplay efficient and responsive in the World of Warcraft environment.

### **What is the difference between WoWpadX and WoWmapperX?**

WoWpadX is a complete architectural rebuild designed aiming better performance and compatibility:

1. **Native C++ Codebase:** Built using C++ rather than C\#/.NET, eliminating runtime dependencies and (hopefully) increasing it's overall compatibility and performance.
2. **SDL3 Backend:** Leverages the latest SDL3 for universal controller support (DualSense, Xbox Series, Switch Pro, etc.).  
3. **Qt Framework:** Provides a robust, responsive GUI to make the experience closer to WoWmapperX

## **Xbox Full Screen Experience / ROG Ally X**

This fork adds a Windows Raw Input backend for the built-in controller on the Xbox ROG Ally X. It allows WoWpadX to receive buttons, sticks, D-pad, and trigger input when Windows boots directly into Xbox Full Screen Experience, without first switching to Desktop mode.

For startup in Xbox mode, use Windows Task Scheduler with an **At log on** trigger, **Run with highest privileges**, and set **Start in** to the folder containing `WoWpadX.exe`.

### **Windows Defender note**

The portable executable is currently unsigned. Windows Defender may block or terminate it because WoWpadX reads controller HID input and generates keyboard/mouse input. This can appear in Task Scheduler as `0x8007042B` even when the application itself is valid.

Do not disable Defender permanently. Keep Defender enabled and, only when necessary:

1. Extract WoWpadX into a dedicated folder such as `C:\Tools\WoWpadX`.
2. Review **Windows Security → Virus & threat protection → Protection history**.
3. Add an exclusion for only the WoWpadX executable or its dedicated folder.
4. Avoid excluding broad locations such as the Desktop or Downloads folders.

## **Command Line Arguments**

WoWpadX supports several flags for automation and background execution.

### **Actions**

| Argument | Description |
| :---- | :---- |
| \-l \<path\> | **Launch & Map:** Launches the specified game executable and starts mapping immediately. |
| \--stop | **Shutdown:** Sends a signal to the currently running instance of WoWpadX to exit. |

### **Modifiers (Used with \-l)**

| Argument | Description |
| :---- | :---- |
| \-n | **Headless Mode:** Runs as a background driver with no GUI. |
| \-m | **Minimized Mode:** Launches the game and hides WoWpadX in the system tray. |
| \-k | **Keep-Alive:** Prevents WoWpadX from closing after the game process exits. |

### **General**

| Argument | Description |
| :---- | :---- |
| \-v | Displays current version information. |
| \-h, \--help | Shows the help documentation in the console. |

**Example Usage:**  
To launch the game in Headless mode (No GUI) and close WoWpadX automatically when the game finishes:  
WoWpadX.exe \-l "C:\\Games\\WoW\\Wow.exe" \-n

To stop a running background instance:  
WoWpadX.exe \--stop

## **Privileges & Process Detection**

If the controller is not working inside the game window, it is likely due to a privilege mismatch. If your game launcher requires Administrator privileges, you must also **Run WoWpadX as Administrator**.  
If your game process uses a non-standard name, you can add it to the settings.json file generated upon the first launch:  

```
"GameProcessNames": [  
    "wow",  
    "wow-64",  
    "ascension",  
    "custom-client-goes-here"  
]
```

## **Setup & Configuration**

WoWpadX and **ConsolePortLK** are designed to be "Plug-and-Play."

1. Launch WoWpadX (manually or via \-l).  
2. Launch World of Warcraft. 
3. That's it.

## **Optional Components: Overlay & Loader**

In the project root you will find two extra projects:

* **WoWpadXOverlay**
* **WoWpadXOverlayLoader**

These projects implement a basic Direct3D9 overlay with functionality similar to WoWmapper’s overlay. The overlay displays controller battery status, connection details, and a crosshair showing the mouse position prior to entering mouselook mode.

Please note that while the source code is included, the overlay is not compiled or packaged in the releases. Because it relies on DLL injection, using it could potentially trigger Warden. For this reason, if you want to use or test the overlay, you’ll need to build it yourself.

After building, simply place both WoWpadXOverlay.dll and WoWpadXOverlayLoader.exe inside the WoWpadX executable folder. Once they are in place, the overlay option within WoWpadX settings will become available.

## **Alternatives**

### **Linux** 
* AntiMicroX
* Steam Input

### **Windows**
* [WoWmapperX](https://github.com/leoaviana/WoWmapperX) (Legacy C\# version)
* [WoWmapper](https://github.com/topher-au/WoWmapper)
* AntiMicroX 
* Steam Input

[Download Latest Release](https://github.com/Shashakar/WoWpadX-xbox/releases/latest)
