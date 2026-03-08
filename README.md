# ⚡ IDM Freezer And Activator (Pro Toolkit)

![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)
![Framework](https://img.shields.io/badge/Framework-Qt_6-green.svg)
![Platform](https://img.shields.io/badge/Platform-Windows_10%20%7C%2011-lightgrey.svg)
![License](https://img.shields.io/badge/License-MIT-orange.svg)

A highly advanced, native **C++** utility built with a modern **Qt GUI** to seamlessly manage, freeze, and activate your Internet Download Manager (IDM) installation. 

Unlike traditional batch (`.cmd`) or PowerShell scripts that trigger Antivirus alerts and display ugly console windows, this project is engineered using **Pure Windows API (WinAPI)**. It performs silent registry manipulations, ACL (Access Control List) permission locks, and Native Memory DLL Injection—all orchestrated beautifully in a multi-threaded Qt environment.

---

## ✨ Core Features

### 1. ❄️ Lifetime Trial Freezer
Bypass the need for serial keys entirely. This feature cleanly utilizes the 30-day trial logic by starting the IDM engine, allowing it to generate the CLSID trial-tracking keys, and then utilizing WinAPI (`SetNamedSecurityInfoW`) to apply a strict `Deny-Everyone` ACL permission on those specific registry paths. The trial counter is permanently frozen.

### 2. 🚀 Full Activation (Registry + Memory Patch)
A two-step, bulletproof activation sequence:
* **Registry Layer:** Injects valid registration data (Name, Email, Serial) into the appropriate 32-bit registry paths.
* **Memory Layer (DLL Injection):** IDM possesses a built-in blacklist mechanism that verifies serials online, triggering a "Fake Serial" popup. This tool extracts a stealthy `aidm_patch.dll` from its embedded Qt Resources (`.qrc`), launches IDM in a `CREATE_SUSPENDED` state, injects the DLL directly into its memory to hook and neutralize the popup functions, and safely resumes the thread. 

### 3. 🔄 Deep Factory Reset
Completely obliterates all IDM registration data, hidden CLSID trial keys, and driver settings. It smartly handles locked/encrypted registry paths by taking ownership (`SE_TAKE_OWNERSHIP_NAME`) and resetting DACL permissions before deletion.

### 4. 🎨 Modern Glassmorphism GUI
* Fully responsive interface.
* **Dark & Light Mode** toggle on the fly.
* **Live Action Log:** A custom-built, syntax-highlighted logging widget that reports every registry change, injection step, and thread status in real-time.

---

## 🏗️ Project Architecture & Source Map

The codebase is strictly modular, adhering to SOLID principles. The heavy lifting is separated from the UI to ensure the application never freezes (Zero UI Deadlocks).

```text
📦 IDM_Tool
 ┣ 📂 resources          # Contains embedded assets and the 'aidm.dll' patch.
 ┣ 📂 src
 ┃ ┣ 📜 main.cpp         # Application entry point.
 ┃ ┣ 📜 mainwindow.cpp/h # Main Qt Window handling routing, animations, and UI states.
 ┃ ┣ 📜 mainwindow.ui    # XML layout for the primary interface.
 ┃ ┣ 📜 idmworker.cpp/h  # The Core Engine (QThread). Handles WinAPI, RegEdit, Injection.
 ┃ ┣ 📜 registrymanager.cpp/h # Dedicated wrapper for Native WinAPI Registry Operations.
 ┃ ┣ 📜 logwidget.cpp/h  # Custom QTextEdit component for colored, auto-scrolling terminal logs.
 ┃ ┣ 📜 aboutdialog.cpp/h # Frameless, programmatic Qt dialog for project info.
 ┃ ┗ 📜 aboutdialog.ui   # (Deprecated in favor of programmatic layout).
 ┗ 📜 IDM_Tool.pro       # qmake project configuration file.
🧠 Under the Hood (idmworker.cpp)
This is the brain of the application. Running on a dedicated QThread, it performs:

Token Privileges Adjustment: Elevates the process to acquire SE_RESTORE_NAME and SE_TAKE_OWNERSHIP_NAME.

Smart Detection: Resolves the current user's SID dynamically to locate the correct HKEY_USERS path, preventing permission issues.

Native DLL Injector: Replaces the need for external .exe injectors. Uses OpenProcess, VirtualAllocEx, WriteProcessMemory, and CreateRemoteThread directly.

🛠️ Build Instructions
Prerequisites
Qt Framework: Version 5.15 or Qt 6.x.

Compiler: MSVC (Microsoft Visual C++) or MinGW (32-bit or 64-bit, matching your Qt setup).

Patch DLL: You must provide your compiled idm_patch.dll and rename it to aidm.dll, placing it in the resources folder before building.

Steps to Compile
Clone the repository:

Bash
git clone [https://github.com/zinzied/IDM-Freezer-And-Activator.git](https://github.com/zinzied/IDM-Freezer-And-Activator.git)
Open IDM_Tool.pro in Qt Creator.

Add your aidm.dll to the Qt Resource file (resources.qrc).

Select your Kit (Release Profile is recommended).

Build and Run!

(Note: The application must be run as Administrator to effectively modify HKEY_LOCAL_MACHINE and inject memory).

📸 Screenshots
(To be added: Place a beautiful screenshot of your Dark Mode and Light Mode UI here!)

⚠️ Disclaimer & Legal Notice
This project is strictly for Educational and Reverse Engineering Research Purposes Only.
The goal of this software is to demonstrate advanced C++ mechanics, Windows API interactions, registry permission handling, and memory manipulation within a graphical Qt application.

The author (Ali Sakkaf) does not condone software piracy. If you regularly use Internet Download Manager and find it useful, please support the developers (Tonec Inc.) by purchasing a legitimate license from their official website.

The creator assumes no responsibility for any misuse of this tool, registry corruption, or account bans. Use at your own risk.

Developed with ❤️ and C++ by Ali Sakkaf.