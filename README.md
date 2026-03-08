# ⚡ IDM Freezer And Activator (Advanced Native Toolkit)

![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)
![Framework](https://img.shields.io/badge/Framework-Qt_6-green.svg)
![Architecture](https://img.shields.io/badge/Architecture-x86%20%7C%20x64-lightgrey.svg)
![Status](https://img.shields.io/badge/Status-Stable-success.svg)
![License](https://img.shields.io/badge/License-MIT-orange.svg)

A highly advanced, native **C++** utility built with a modern **Qt GUI** to seamlessly manage, freeze, and interact with your Internet Download Manager (IDM) installation. 

Unlike traditional batch (`.cmd`) or PowerShell scripts that trigger false-positive Antivirus alerts, display ugly black console windows, and suffer from permission errors, this project is engineered from the ground up using **Pure Windows API (WinAPI)**. It performs silent registry manipulations, deep ACL (Access Control List) permission locks, and Native Memory DLL Injection—all orchestrated beautifully in a multi-threaded Qt environment without ever freezing the UI.

---

## 🚨 IMPORTANT PREREQUISITE: OFFICIAL INSTALLATION REQUIRED
**This tool DOES NOT contain, distribute, or download any modified or cracked versions of IDM.** For this tool to work, you **must** download and install the official, untouched version of Internet Download Manager directly from the developer's official website. 

📥 **Download Official IDM Here:** [Internet Download Manager Official Site](https://www.internetdownloadmanager.com/download.html)

---

## ✨ Detailed Core Features & Methodologies

### 1. ❄️ The "Freeze Trial" Method (HIGHLY RECOMMENDED 🌟)
**If you want a permanent, stable, and pop-up-free experience, this is the option you should choose.**

* **How it works:** IDM provides a legitimate 30-day trial. It tracks this trial using hidden, dynamically generated `CLSID` registry keys. When you click "Freeze Trial", our tool launches the IDM engine silently in the background for a few seconds to generate fresh trial keys. Then, using pure WinAPI (`SetNamedSecurityInfoW`), it applies a strict `Deny-Everyone` ACL permission on those specific registry paths.
* **Why it is the BEST option:** 1. It relies on the official 30-day trial mechanism, meaning **no fake serials are used**.
  2. Because there is no fake serial, IDM will **never** connect to its servers to verify your license, which means you will **never see the "Fake Serial" popup error**.
  3. It survives IDM software updates natively.

### 2. 🚀 Full Activation (Registry + Native Memory Patch)
This is an alternative method for users who specifically want the software to display as "Registered" with their own name, rather than "Trial".
* **The Challenge:** Modern versions of IDM have an internal, hardcoded blacklist. If you simply put a generated serial into the registry, IDM will verify it online, realize it's fake, wipe your registration data, and throw a persistent "Fake Serial" popup.
* **The Solution:** Our tool executes a sophisticated 2-step bypass:
  1. **Registry Layer:** Injects valid registration metadata (Name, Email, Serial) into the Windows Registry.
  2. **Memory Layer (DLL Injection):** To prevent IDM from showing the fake serial popup, the tool extracts an embedded `aidm_patch.dll` directly from its Qt Resources into the IDM installation folder. It then launches IDM in a `CREATE_SUSPENDED` thread state. Using `VirtualAllocEx` and `CreateRemoteThread`, the DLL is injected directly into IDM's memory before it even starts. The DLL hooks into the system's `MessageBoxW` functions, effectively rendering IDM completely incapable of displaying the fake serial warning.

### 3. 🔄 Deep Factory Reset & Smart Cleaner
Over time, users might use various unverified scripts or cracks that leave messy, encrypted, or corrupted keys in the Windows Registry.
* The Reset function completely obliterates all IDM registration data and hidden CLSID trial keys.
* It smartly handles locked/encrypted registry paths by taking full system ownership (`SE_TAKE_OWNERSHIP_NAME`) and resetting DACL permissions to `GRANT_ACCESS` before safely executing a recursive tree deletion. This returns IDM to a "Clean / Factory Ready" state.

### 4. 🎨 Modern Glassmorphism GUI
* Fully responsive and animated interface built with Qt.
* **Dark & Light Mode** toggle on the fly.
* **Live Action Log:** A custom-built, syntax-highlighted logging widget (`QTextEdit`) that reports every registry change, ACL lock, and injection step in real-time. No hidden processes—you see exactly what the tool is doing to your system.

---

## 🏗️ Project Architecture & Source Map

The codebase is strictly modular, adhering to SOLID principles. The heavy lifting is separated from the UI using `QThread` to ensure the application remains perfectly fluid.

```text
📦 IDM_Tool
 ┣ 📂 resources          # Contains embedded assets, icons, and the 'aidm.dll' patch.
 ┣ 📂 src
 ┃ ┣ 📜 main.cpp         # Application entry point.
 ┃ ┣ 📜 mainwindow.cpp/h # Main Qt Window handling routing, animations, and UI states.
 ┃ ┣ 📜 idmworker.cpp/h  # The Core Engine (QThread). Handles WinAPI, RegEdit, Native Injection.
 ┃ ┣ 📜 logwidget.cpp/h  # Custom component for colored, auto-scrolling terminal logs.
 ┃ ┗ 📜 aboutdialog.cpp/h # Frameless, programmatic Qt dialog for project info.
 ┗ 📜 IDM_Tool.pro       # qmake project configuration file.

```

---

## ❓ Frequently Asked Questions (FAQ)

**Q: Which option should I choose? Activate or Freeze?**
A: **Choose Freeze.** The Freeze method is infinitely more stable. It tricks the system into keeping the 30-day trial forever. The "Activate" method uses memory injection, which might need to be re-run after every computer restart to block the popups. Freeze is a "Set it and forget it" solution.

**Q: Why does my Antivirus flag the `idm_patch.dll` or the `.exe`?**
A: This tool utilizes Memory Injection (`CreateRemoteThread`, `WriteProcessMemory`) and modifies Windows Registry ACL permissions. These are the exact same techniques used by malware, which triggers heuristic/generic flags in Antivirus software. The source code is completely open for you to review and compile yourself to ensure it is safe.

**Q: Does this tool download IDM for me?**
A: No. You must download the official release from the Internet Download Manager website yourself. This tool only interacts with an already installed official copy.

---

## 🛠️ Build Instructions for Developers

1. Clone the repository:
```bash
git clone [https://github.com/alisakkaf/IDM-Freezer-And-Activator.git](https://github.com/alisakkaf/IDM-Freezer-And-Activator.git)

```


2. Open `IDM_Tool.pro` in **Qt Creator** (Qt 5.15 or Qt 6.x supported).
3. Ensure your compiled `idm_patch.dll` is renamed to `aidm.dll` and placed in the `resources` folder before building.
4. Select the **Release Profile** (MinGW or MSVC).
5. Build and Run! *(Note: Execution requires Administrator Privileges to modify HKLM registry paths).*

---

## ⚖️ Legal Disclaimer & Terms of Use (PLEASE READ)

This project is strictly for **Educational, Academic, and Reverse Engineering Research Purposes Only**.

The goal of this software is to demonstrate:

1. Advanced C++ Windows API interactions.
2. Registry permission handling and ACL manipulation.
3. Memory manipulation and DLL Injection techniques within a graphical Qt application.

**The author (@alisakkaf) does NOT condone software piracy.** This tool does not distribute copyrighted binaries. If you regularly use Internet Download Manager and find it useful in your daily life, please support the developers (Tonec Inc.) by purchasing a legitimate license from their official website.

By using this code, you agree that the creator assumes **no responsibility** for any misuse of this tool, registry corruption, system instability, or account bans. **Use at your own risk.**

---

*Architected and Engineered with ❤️ in C++ by [Ali Sakkaf]*

```