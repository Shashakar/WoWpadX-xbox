# **Pixel Bridge: A Simple Gamepad Interface**

**Bridging the gap between the game and your WoWpadX with zero memory-reading.**  
Pixel Bridge is a image processing feature designed to provide some specific gamepad interactions for **World of Warcraft 3.3.5a**. Unlike traditional memory readers that are prone to bans, Pixel Bridge uses a "visual handshake" to read game states instantly via the GPU output.

## **Key Features**

* **Zero Memory Reading:** 100% Anti-Cheat safe. It reads pixels from the screen buffer, not memory addresses.
* **Rumble on Damage:** Tracks the player's health percentage in real-time, allowing the controller to vibrate when losing HP.
* **Custom AOE Bindings:** Automatically detects when an AOE reticle is active, allowing WoWpadX to rearrange bindings for easier spell placement.
* **Intelligent Auto-Walk:** Maps analog stick pressure to the game's movement state. It identifies if you want to walk or run and manages the transition based on current player speed.
* **Smart Cursor Centering:**  Identifies when the game enters a "mouselook" state and automatically centers the cursor upon exit, ensuring a consistent console-like feel.

## **How to Enable Pixel Bridge**

To use this feature, the Pixel Bridge feature must be synchronized between the WoWpadX and ConsolePortLK.

### **1\. In the WoWpadX App (The Brain)**

1. Launch **WoWpadX.exe**.  
2. Navigate to the **Settings** panel.  
3. Go to Pixel Bridge and Enable it. 
4. The app will show in the main page "Pixel Bridge is waiting sync..." until the game is launched and detected.

### **2\. In-Game (ConsolePortLK / Addon)**

1. Open your game and ensure it is set to **Windowed** or **Windowed Maximized** mode.  
2. Ensure the **ConsolePortLK** addon is loaded.
3. Go to ConsolePortLK settings page and activate the experimental Pixel Bridge feature.
3. Once activated, a tiny **Blinking Beacon** (Magenta/Black) will appear at the top-left corner of your game window.f
4. If the beacon is visible, WoWpadX will be able to synchronize with the client.

## **Technical Data Layout**

This feature works by encoding game data into a single RGB pixel. The WoWpadX app "samples" this pixel 125 times per second.

| Channel | Data Encoded | Purpose |
| :---- | :---- | :---- |
| **Red** | Player Health (0-255) | Triggers Controller Rumble/Vibration on low HP. |
| **Green** | Movement & Mouselook Bits | Logic for Auto-Walk and Cursor centering |
| **Blue** | AOE State | Handles AOE state only |