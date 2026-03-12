<div align="center">

# ⚡ IDM Freezer & Activator (Advanced Native Toolkit)

**The Ultimate C++ / WinAPI Environment Manager for Internet Download Manager**

A highly advanced, native **C++** utility built with a modern **Qt GUI** to seamlessly manage, analyze, and interact with your Internet Download Manager (IDM) installation environment.

*Designed for extreme stability, bypassing the flaws of traditional batch scripts through deep Windows API integration and asynchronous threading.*

</div>

---

## 📑 Table of Contents

1. [🛑 Legal & Prerequisite Notice](https://www.google.com/search?q=%23-legal--prerequisite-notice)
2. [📖 Philosophy: Why Native C++?](https://www.google.com/search?q=%23-philosophy-why-native-c)
3. [⚙️ Comprehensive Feature Breakdown](https://www.google.com/search?q=%23%EF%B8%8F-comprehensive-feature-breakdown)
* [Option 1: ❄️ The "Freeze Trial" Method](https://www.google.com/search?q=%23option-1-%EF%B8%8F-the-freeze-trial-method)
* [Option 2: 🚀 Full Activation (DLL Injection)](https://www.google.com/search?q=%23option-2--full-activation-registry-patch--native-memory-dll-injection)
* [Option 3: 🌐 Ultimate Browser Extension Engine](https://www.google.com/search?q=%23option-3--ultimate-browser-extension-engine)
* [Option 4: 🔄 Deep Factory Reset & Smart Cleaner](https://www.google.com/search?q=%23option-4--deep-factory-reset--smart-cleaner)
* [Option 5: 📡 In-App OTA Updater](https://www.google.com/search?q=%23option-5--in-app-ota-updater-network-module)


4. [🛠️ Under the Hood (Technical Architecture)](https://www.google.com/search?q=%23%EF%B8%8F-under-the-hood-technical-architecture)
5. [🚀 Compilation & Build Instructions](https://www.google.com/search?q=%23-compilation--build-instructions)
6. [👨‍💻 Author & Connect](https://www.google.com/search?q=%23-author--connect)

---

## 🛑 Legal & Prerequisite Notice

> **Read Before Use:** Please strictly refer to the [LEGAL_NOTICE.md](https://www.google.com/search?q=LEGAL_NOTICE.md) file before proceeding. This repository is provided strictly for educational purposes, reverse-engineering analysis of Windows permission structures, and personal system management.

**This tool DOES NOT contain, distribute, or download any modified or cracked executables of IDM.** For this software to function as intended, you **must** download and install the official, unmodified release of Internet Download Manager directly from the official developer.

📥 **Official Download:** [Internet Download Manager Official Site](https://www.internetdownloadmanager.com/download.html)

---

## 📖 Philosophy: Why Native C++?

Historically, IDM trial resetters and activators have been written in Batch (`.bat`), PowerShell, or AutoIt. These pose significant issues:

* **Antivirus False Positives:** Heuristic engines instantly flag heavily obfuscated batch scripts.
* **Ugly UX:** Intrusive black console windows popping up and freezing the system.
* **Permission Failures:** Scripts often fail to manipulate deeply secured registry keys (`HKEY_CLASSES_ROOT`), resulting in broken trials.

**The Solution:** This toolkit is engineered from the ground up using **Pure Windows API (WinAPI)** inside a multi-threaded **Qt Framework**. It directly manipulates Access Control Lists (ACLs), escalates privileges via security tokens, and injects native DLLs directly into process memory without ever blocking the user interface.

---

## ⚙️ Comprehensive Feature Breakdown

This toolkit offers several distinct operational modes. Below is a detailed technical explanation of what happens when you click each button in the UI.

### Option 1: ❄️ The "Freeze Trial" Method (🌟 Highly Recommended)

**Best for:** Users who want a permanent, stable, pop-up-free experience without registering fake names.

**How it works technically:**

1. **Background Initialization & Wipe:** IDM provides a legitimate 30-day trial. It tracks this trial using dynamically generated, heavily obfuscated `CLSID` registry keys. The tool first forcefully unlocks and deletes all existing legacy keys to avoid the infamous "27 days left" bug.
2. **Key Generation:** It silently launches the IDM engine in the background, utilizing 3 independent dummy download triggers with a dynamic micro-polling timeout to force a fresh 30-day generation safely and instantly.
3. **WinAPI Deep Lock:** Utilizing `SetNamedSecurityInfoW`, the tool aggressively applies a strict **Deny-Everyone** ACL permission directly onto these specific, newly generated registry paths.

**The Result:** IDM can no longer read or modify its own trial expiration counters. It is permanently frozen at 30 days.

**Why it is superior:**

* **Zero Fake Serials:** Relies entirely on the official trial mechanism.
* **No Server Pings:** IDM will never attempt to verify a fake license against its remote servers, effectively eliminating the infamous *"Fake Serial"* popup loop.
* **Update Resilient:** Survives official IDM software updates natively.

### Option 2: 🚀 Full Activation (Registry Patch + Native Memory DLL Injection)

**Best for:** Users requiring the software to display as "Registered" with custom metadata (Name/Email) rather than "Trial".

**How it works technically:** Modern iterations of IDM employ an internal, hardcoded blacklist mechanism. If you submit generic registration data, IDM verifies it online, wipes the registration block upon failure, and triggers a persistent warning loop. This option executes a sophisticated 2-Step Bypass:

1. **Registry Metadata Injection:** Valid registration formatting (Name, Email, Serial) is securely injected into the target `HKEY_CURRENT_USER` registry hive.
2. **Native Memory Patching (DLL Injection):** * The tool dynamically extracts an embedded payload (`aidm_patch.dll`) directly from the compiled Qt Resources (`.qrc`) into the IDM installation directory.
* It then spawns the `IDMan.exe` process in a `CREATE_SUSPENDED` thread state using `CreateProcessW`.
* Using standard native APIs (`VirtualAllocEx` to allocate memory in the target process, `WriteProcessMemory` to write the DLL path, and `CreateRemoteThread` to force the process to load the DLL), the payload is injected into IDM's memory space *prior* to execution.



**The Hook:** The injected payload hooks into the system's `MessageBoxW` calls, intercepting and completely suppressing the "Fake Serial" verification warnings at runtime.

### Option 3: 🌐 Ultimate Browser Extension Engine

**Best for:** Ensuring IDM seamlessly catches downloads across all modern web browsers without relying on buggy web stores.

**How it works technically:**

* **1-Click Universal Sideloading:** Automatically detects and injects the local IDM extension `.crx` / `.xpi` files directly into Chromium and Firefox engines.
* **Enterprise Policy Force-Install:** Completely bypasses Chromium's "Developer Mode" requirements by utilizing registry Enterprise Policies for Chrome, Edge, and Brave.
* **Dynamic Version Extraction:** Built with a smart Regex-powered locator that scans your system for IDM, extracts the exact software version, and perfectly formats it to match Chromium's strict `x.y.z.w` requirements for a silent, permanent setup.
* **Direct CRX Launch:** Securely pushes the extension to restricted browsers like Opera and Opera GX via command-line arguments.

### Option 4: 🔄 Deep Factory Reset & Smart Cleaner

**Best for:** Sanitizing corrupted environments caused by third-party scripts, legacy cracks, or broken registry states. Over time, encrypted garbage keys accumulate and break IDM.

**How it works technically:**

1. **Process Termination:** Forcefully terminates all IDM agents and browser extension hooks.
2. **Privilege Escalation:** Intelligently handles externally locked registry paths by escalating the application's access tokens to acquire full system ownership (`SE_TAKE_OWNERSHIP_NAME` & `SE_RESTORE_NAME`).
3. **DACL Rewrite & Annihilation:** Resets the Discretionary Access Control List (DACL) permissions to `GRANT_ACCESS` on all known IDM registry trees. Utilizing a sophisticated **Backward Iteration Algorithm**, it safely executes a recursive tree deletion across `HKCU\Software\DownloadManager` and hidden `HKCR\CLSID` keys without encountering infinite loops or application freezes.

**The Result:** Returns the IDM environment to a pristine, "Factory Ready" state.

### Option 5: 📡 In-App OTA Updater (Network Module)

The toolkit features a built-in, non-blocking Over-The-Air (OTA) update checker.

* Built using `QNetworkAccessManager` and `QSslSocket`.
* Securely communicates with the GitHub raw repository over HTTPS to fetch the latest version.
* Includes comprehensive SSL/TLS diagnostics, timeout handling, and seamless UI progression via Qt Signals and Slots (`emit operationStarted()`, `emit progress()`).

---

## 🛠️ Under the Hood (Technical Architecture)

* **Language:** C++14 / C++17
* **Framework:** Qt 5 / Qt 6 (Qt Core, Qt Gui, Qt Widgets, Qt Network)
* **Architecture Support:** Full detection and compatibility for `x86`, `x64`, and modern `ARM64` processors.
* **System API Integration:** * `Advapi32.dll` (Registry, Security Descriptors, and ACL manipulation)
* `Kernel32.dll` (Process, Thread memory management, and `Sysnative` routing for WOW64 bypassing)
* `User32.dll` (UI interactions and Hooks)


* **Concurrency:** Engineered with a **Zero-Freeze Asynchronous Engine**. Heavy operations (recursive registry deletion, dynamic polling, and network fetching) are offloaded to background threads (`QThread` / `QEventLoop`). The UI is equipped with an indeterminate marquee progress bar and millisecond-level live updates to guarantee a buttery-smooth, premium user experience.

---

## 🚀 Compilation & Build Instructions

To build this project from source, ensure you have the following prerequisites installed:

1. **Qt Framework:** Version 5.15 or 6.0+.
2. **Compiler:** MSVC (Microsoft Visual C++) 2019/2022 or MinGW 64-bit.
3. **OpenSSL Libraries:** Required for the OTA Update checker to function (ensure `libcrypto` and `libssl` are in your build path).

**Steps:**

1. Clone the repository: `git clone https://github.com/alisakkaf/IDM-Freezer-And-Activator.git`
2. Open the `.pro` or `CMakeLists.txt` file in **Qt Creator**.
3. Run `qmake` (or CMake) to generate the Makefiles.
4. Build the project in **Release** mode for maximum performance and standalone deployment.

---

## 👨‍💻 Author & Connect

**Ali Sakkaf**
*Software Developer | System Utilities & Reverse Engineering Enthusiast*

🌐 **Website:** [mysterious-dev.com](https://www.google.com/search?q=https://mysterious-dev.com)
🐙 **GitHub:** [@alisakkaf](https://www.google.com/search?q=https://github.com/alisakkaf)
📘 **Facebook:** [AliSakkaf.Dev](https://www.google.com/search?q=https://www.facebook.com/AliSakkaf.Dev)

If this toolkit helped you understand Windows API permissions or process management better, consider leaving a ⭐ on the repository!
