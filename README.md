<div align="center">
  <a href="https://singularsound.com/">
    <img src="https://singularsound.com/wp-content/uploads/elementor/thumbs/SS_LOGO_LONGFORM_BLACK-01-e1531153161400-oicxykd903bhj4hochumz1mu5i906qdtgawzvp5xxc.png" alt="Singular Sound logo" width="340"/>
  </a>
</div>

# Getting Started

## Building in MacOS X

### Initial Setup
You will need the latest version of qt to build the BeatBuddy Manager. You can get it from homebrew using `brew install qt`. Make sure to follow all instructions, as there may be issues with symlink creation that require you to add the binary directory to your path manually.

Ideally, set up Qt Creator and configure it to work with your installation of Qt. This will allow you to run the project from the IDE without any further work.

If yo decide not to set up Qt Creator, you will need to provide suitable binaries of libquazip and libminIni to be loaded dynamically.

### Building and Deploying

The deployment script has been temporarily decomissioned; we will bring it back soon. In order to distribute the app you need to make sure to build it and include the libraries in the bundle, making sure they point at the included dependencies as needed.

## Setting Up in Windows

### Setting Up Qt
To build BBMnager you'll need Qt 5.12.4 or up.
To obtain Qt, download the installer from [their official site](https://www.qt.io/download-qt-installer) and install. This includes Qt and its IDE, Qt Creator.

#### Setting your build kit
To successfully run BBManager, you'll need `Desktop Qt 5.12.4 MSVC2015 64bit` as your kit selection; this can be found on the Kits menu which you can find by navigating through the menu bar: Tools->Options->Kits.

Make sure the selected kit's compilers for both C and C++ are `Microsoft Visual C++ Compiler 14.0 (amd64)`. If they are missing from the drop down options, you may download them through Microsoft Visual Studio's installer.

##### Setting Microsoft Visual C++ Compiler
Get Microsoft Visual Studio's Installer from [their official site](https://visualstudio.microsoft.com/downloads/) and execute it. On the installer, when prompted, select Desktop development with C++, and make sure the installation includes all the MSVC optionals. This should automatically add the desired compiler to Qt's kit's compilers' dropdowns.

You may also add the compiler manually (in the same Options->Kits window); setting its path to `C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe`, setting the make path to `C:\Qt\Tools\QtCreator\bin` selecting the jom.exe file, and the qt mkspecs to `C:\Qt\Tools\msvc2015_64\mkspecs`. 

Finally make sure your `C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64` folder has a rc.exe and a rcdll.dll files, if they don't exists copy them from `C:\Program Files (x86)\Windows Kits\10\bin\VERSION\x64`.

### Building and Deploying
The project file is `BBManagerLean/BBManagerLean.pro`. When opening the project select the corresponding kit, Clean, Build, and Run.

To pack for deployment - ie: to share with other people make the .pro file is set for release, and set the Build(The computer above the Run button on the Qt Creator) to 'Release' and build the application. Go to the folder containing the .exe and delete all but the .exe file, open the command prompt and run `windeployqt.exe --quick .` this will generate all the necessary .dll, and it is ready.


## Building for Ubuntu

Build currently only performed on Ubuntu 18.04.

### Pre-requisites

All build toolchain installed:
* Compiler (`clang++`)
* Qt libraries and development tools  For Qt, you can use the default Qt provided by the distro, i.e. `5.9.5`. (Probably a newer version with Ubuntu 20.04).

### Install the dependencies

Basically dependencies remain the same as for other systems.
You can install `libquazip` directly from Ubuntu official repo:

    $ sudo apt install libquazip-dev

Unfortunately Ubuntu 18.04 doesn't provide libMinini, but apparently Ubuntu 20.04 does, so you can download the deb files for Ubuntu 20.04. Here is an example, as version may evolve (this is exactly why you should prefer apt...). If it does'nt work, check packages versions.

```shell
# Get & install the library
$ wget http://archive.ubuntu.com/ubuntu/pool/universe/libm/libminini/libminini1_1.2.a+ds-4build1_amd64.deb
$ sudo dpkg -i ./libminini1_1.2.a+ds-4build1_amd64.deb
# Get and install the development files
$ wget http://archive.ubuntu.com/ubuntu/pool/universe/libm/libminini/libminini-dev_1.2.a+ds-4build1_amd64.deb
$ sudo dpkg -i ./libminini-dev_1.2.a+ds-4build1_amd64.deb
```

If you already migrated to the Ubuntu 20.04, you obviously just have to do:

    $ sudo apt install libminini-dev

### Building

Specify that you are using the `clang` compiler for C++:

    $ qmake bbmanager.pro -spec linux-clang && make qmake_all

And then build:

    make clean && make all

It will produce the `BBManagerLean/BBManagerLean`.


## Default Files
Once OpenBBManager compiles correctly, you must add the default drumsets.
You may get all of the default files, including drumsets, from [here](https://mybeatbuddy.s3.amazonaws.com/BeatBuddy_Default_Content_v2.0.zip).

That file also contains the BeatBuddy's bundled songs' midi sources, which are also handy for comparing the virtual machine to the Beat Buddy.

To install these files, you must find the OpenBBManager's `user_lib` folder, and drag the zipped files inside that folder. (You can find this folder through the OpenBBManager by navigating through File->Open Project, and on the dialog going one folder up).

To use a drumset (to listen to the songs), you must then check its checkmark (on the left panel, on the Drum Sets tab). Once checked, the drumset will appear on the list at the top bar, and playing a song will use that selected drumset.

## Contributing

Please read the [contribution guidelines for this project](CONTRIBUTING.md), for details on contributions and the process of submitting pull requests.
