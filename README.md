<div align="center">
  <a href="https://singularsound.com/">
    <img src="https://singularsound.com/wp-content/uploads/elementor/thumbs/SS_LOGO_LONGFORM_BLACK-01-e1531153161400-oicxykd903bhj4hochumz1mu5i906qdtgawzvp5xxc.png" alt="Singular Sound logo" width="340"/>
  </a>
</div>

# Getting Started

## Building in MacOS X

### Initial Setup
To build BBManager you'll need the [current suported version of qt](https://doc.qt.io/qt-5/macos.html) installed.

To successfully run BBManager, you need a local installation of libquazip and libMinini.

Those should be installed at the ~/lib directory after running the `make install` command, because the OS looks for shared libraries there without any configuration; the files to be moved into that directory can be found in BBManager/libs/quazip/macx/release and BBManager/libs/minIni/macx/release respectively.

### Building and Deploying
The project beatbuddy-manager-lite.pro will hold all project of the BeatBuddy Manager, so in order to run the application the project file is **BBManagerLean/BBManagerLean.pro** as this will run the BeatBuddy Manager. In BBManager/project you can build by running `qmake && make clean && make all`. The BBManager.app will be in the same directory, and you can run this to test the app.

While dependencies don't change, future builds can be done with just `make`.

To start using the BBManager you can download the default drumsets from [here](https://mybeatbuddy.s3.amazonaws.com/BeatBuddy_Default_Content_v2.0.zip).

To pack for deployment - ie: to share with other people - you need to run `BBManager/mac_deployment/deploy.sh` with `BBManager/project/BBManager.app` as an argument; this will make the dynamic libraries be loaded from the BBManager.app bundle, instead of them being sought elsewhere in the system.


## Building in Windows

### Initial Setup
To build BBMnager you'll need Qt 5.12.4 or up.

To successfully run BBManager, you'll need `Desktop Qt 5.12.4 MSVC2015 64bit` Kit as your kit selection. 

##### Setting your kit
On Tools->Options->Kits make sure it's using `Microsoft Visual C++ Compiler 14.0 (amd64)` or up for C and C++, if you don't see it on the drop down options you can download Microsoft Visual Studio and choose Desktop development with C++ making sure the installation includes all the MSVC optionals. Or create it manually, by setting its path to  `C:\Program Files (x86)\Wndows Kits\10\Debuggers\x64\cdb.exe`, setting the make path to `C:\Qt\Tools\QtCreator\bin` selecting the jom.exe file, and the qt mkspecs to `C:\Qt\Tools\msvc2015_64\mkspecs`. 

Finally make sure your `C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64` folder has a rc.exe and a rcdll.dll files, if they don't exists copy them from `C:\Program Files (x86)\Windows Kits\10\bin\VERSION\x64`.

### Building and Deploying
The project file is `BBManagerLean/BBManagerLean.pro`. When opening the project select the corresponding kit, Clean, Build, and Run.

To start using the BBManager you can download the default drumsets from [here](https://mybeatbuddy.s3.amazonaws.com/BeatBuddy_Default_Content_v2.0.zip).

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

## Contributing

Please read the [contribution guidelines for this project](CONTRIBUTING.md), for details on contributions and the process of submitting pull requests.
