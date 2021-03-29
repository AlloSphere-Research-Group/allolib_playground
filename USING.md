## Allolib Playground with Visual Studio Code

This document guides you to build allolib applications with Visual Studio Code on either Windows or macOS.

### Table of contents

1. [VS Code Installation](#1)
2. [Install the C++ Extension for VS Code](#2)
3. [Install the C++ Complier](#3)
4. [Install Allolib Dependencies](#4)
5. [Clone the Allolib Playground GitHub Repository](#5)
6. [Install Submodules](#6)
7. [Building and Testing an Example](#7)

<br>

### 1. VS Code Installation<a name="1"></a>

#### For Windows

- Download the [Visual Studio Code installer](https://go.microsoft.com/fwlink/?LinkID=534107) for Windows.
- Once it is downloaded, run the installer (`VSCodeUserSetup-{version}.exe`). This will only take a minute.
- By default, VS Code is installed under `C:\users\{username}\AppData\Local\Programs\Microsoft VS Code`.

More details at [here](https://code.visualstudio.com/docs/setup/windows).

<br>

#### For macOS

- [Download Visual Studio Code](https://go.microsoft.com/fwlink/?LinkID=534106) for macOS.
- Open the browser's download list and locate the downloaded archive.
- Unzip the archive in Finder.
- Drag `Visual Studio Code.app` to the `Applications` folder, making it available in the macOS Launchpad.
- Add VS Code to your Dock by right-clicking on the icon to bring up the context menu and choosing **Options**, **Keep in Dock**.

More details at [here](https://code.visualstudio.com/docs/setup/mac).

<br>

### 2. Install the C++ Extension for VS Code<a name="2"></a>

- You can install the [C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) by searching for 'c++' in the Extensions view (Windows: Ctrl+Shift+X, macOS: ⇧⌘X) in VS Code.

<br>

### 3. Install the C++ Complier<a name="3"></a>

#### For Windows: Install Visual Studio

- Install **Visual Studio 2019 Community Edition** from https://visualstudio.microsoft.com/downloads/.
- During installation options:
  - Install "**Desktop development with C++**" workload.
  - Install "Individual Components": **C++/CLI support**, **Git for Windows**, **C++ CMake tools for Windows**. (These components are necessary for allolib. See [here](https://github.com/AlloSphere-Research-Group/allolib/blob/master/readme.md) in details.)
- Restart your computer after the installation is done.

More details at [here](https://code.visualstudio.com/docs/cpp/config-msvc).

<br>

#### For macOS: Ensure XCode and Clang are installed

Clang may already be installed on your Mac. To verify that it is, you need to use **Terminal** in one of two ways: 

- Use Terminal in VS Code:
  - Click **Terminal > New Terminal** on the menu. Then, Terminal will appear at the bottom panel
- Or, use macOS's Terminal:
  - Open **Applications > Utilities > Terminal**

In Terminal, enter the following command:

```shell
clang --version
```

If Clang isn't installed, enter the following command to install the command line developer tools:

```shell
xcode-select --install
```

- You need to install [XCode](https://apps.apple.com/us/app/xcode/id497799835?mt=12) form App Store if you can't run the above command.

More details at [here](https://code.visualstudio.com/docs/cpp/config-clang-mac).

<br>

### 4. Install Allolib Dependencies<a name="4"></a>

Allolib depends on Cmake version 3.8 (as the build tool), OpenGL and glew. See platform specific instructions below. See [here](https://github.com/AlloSphere-Research-Group/allolib/blob/master/readme.md) in details.

#### For Windows

You have already installed the dependencies if you correctly followed all the instructions above for installing the C++ compiler on Windows.

<br>

#### For macOS

Install Homebrew: open Terminal and copy and paste the below text of the install command in Terminal.

```shell
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

To install necessary components, run the command below in Terminal:

```shell
brew install git git-lfs cmake libsndfile
```

<br>

### 5. Clone the Allolib Playground GitHub Repository<a name="5"></a>

You can clone the Allolib Playground code from GitHub in two ways.

#### Option 1. Through VS Code (Recommended)

- Select **"Clone Repository"** in the Explorer view (Windows: Ctrl+Shift+E, macOS: ⇧⌘E) and enter the below address:

```
https://github.com/AlloSphere-Research-Group/allolib_playground
```

- Choose your folder to down load the code and click **"Select Repository"**.

- It will ask **"Would you like to open the cloned repository?"**. Click **"Open"**.

- If it asks **"Would you like to configure project 'allolib_playground'?"**, click **"Yes"**.

- If it asks **"Always configure projects upon opening?"**, click **"Yes"**.

<br>

#### Option 2. Using git in Terminal

- Make a folder and move to the folder. For example, your folder is `/Users/abc/MUS109IA` then enter the below command in Terminal:

```shell
cd /Users/abc/MUS109IA
```

- Enter the below command to clone the source code:

``` shell
git clone https://github.com/AlloSphere-Research-Group/allolib_playground
```

- Then you will have a folder named `allolib_playground ` under your directory `/Users/abc/MUS109IA`.

- Then open the  `allolib_playground ` folder via VS Code.
- If it asks **"Would you like to configure project 'allolib_playground'?"**, click **"Yes"**.
- If it asks **"Always configure projects upon opening?"**, click **"Yes"**.

<br>

### 6. Install Submodules<a name="6"></a>

Open Terminal by clicking **Terminal > New Terminal** on the menu on VS Code. 

Run the init.sh file:

```shell
./init.sh
```

or enter the below command:

```shell
git submodule update --recursive --init
```

<br>

To check if all the setup and installation are successful, test an example by run the command in Terminal:

```shell
./run.sh tutorials/gui/01_audio_gui.cpp
```

<br>

### 7. Building and Testing an Example<a name="7"></a>

To compile and build c++ source code files with VS Code, it requires to set three files that will be created in a `.vscode` folder in the workspace (allolib_playground folder):

- `tasks.json` (build instructions)
- `launch.json` (debugger settings)
- `c_cpp_properties.json` (compiler path and IntelliSense settings)

<br>

#### tasks.json

- Open `01_SineEnv.cpp`  file under the `synthesis` folder with VS Code.

- Select **Terminal** > **Configure Default Build Task** from the VS Code menu. and choose "**create tasks.json from the default template**". It will open `tasks.json`. Change it as below:

  ```json
  {
      // See https://go.microsoft.com/fwlink/?LinkId=733558
      // for the documentation about the tasks.json format
      "version": "2.0.0",
      "tasks": [
          {
              "label": "allolib_run",
              "command": "./run.sh",
              "args": [
                "${file}"
              ],
              "type": "shell",
              "group": {
                "kind": "build",
                "isDefault": true
              }
          },
          {
              "label": "allolib_debug",
              "command": "./run.sh",
              "args": [
                "-n",
                "${file}"
              ],
              "type": "shell"
          }
      ]
  }
  ```

- To run the example `01_SineEnv.cpp` opened before, select **Terminal > Run Build Task (Control+Shift+B**) on the menu and choose **allolib_run**.

- Wait until finishing the code compiling.

- If it succeeds, a window  will appear.

<br>

#### c_cpp_properties.json (macOS only)

- When you first open the example code file above, it will ask "**Configure your IntelliSense settings to help find missing headers.**" Click "**Configure (JSON)**". 

- It will open `c_cpp_properties.json`. If the message above didn't appear or was dismissed, create the file under `.vscode` folder in the `allolib_playground` directory.

- Change it as below:

  ```json
  {
    "configurations": [
          {
              "name": "Mac",
              "includePath": [
                  "${workspaceFolder}/**"
              ],
              "browse": {
                  "limitSymbolsToIncludedHeaders": true,
                  "databaseFilename": ""
              },
              "compilerPath": "/usr/bin/clang",
              "cStandard": "c11",
              "cppStandard": "c++14",
              "intelliSenseMode": "clang-x64"
          }
      ],
      "version": 4
  }
  ```

<br>

#### launch.json (optional)

- `launch.json` is neccessary if you want to run code with a debugger.
- Select **Run > Add Configuration** on the menu and select **default configuration**. 
- It will show `launch.json`. Change it as below:

For Windows

```json
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "name": "(Windows) Launch",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "${fileDirname}/bin/${fileBasenameNoExtension}",
            "args": [],
            "preLaunchTask": "allolib_debug",
            "stopAtEntry": false,
            "cwd": "${fileDirname}/bin/",
            "environment": [],
            "console": "externalTerminal"
        }
    ]
}
```

For macOS

```json
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "name": "(lldb) Launch",
            "type": "cppdbg",
            "request": "launch",
            "program": "${fileDirname}/bin/${fileBasenameNoExtension}",
            "args": [],
            "preLaunchTask": "allolib_debug",
            "stopAtEntry": false,
            "cwd": "${fileDirname}/bin/",
            "environment": [],
            "externalConsole": false,
            "MIMode": "lldb"
        }
    ]
}
```

- To debug your code, select **Run > Start Debugging (F5)**.
- Although the debugger may not attach to your program along with debugging information (e.g. breakpoints chosen in the VS Code UI), it is another way to build and run your code.
