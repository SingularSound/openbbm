<div align="center">
  <a href="https://singularsound.com/">
    <img src="https://singularsound.com/wp-content/uploads/elementor/thumbs/SS_LOGO_LONGFORM_BLACK-01-e1531153161400-oicxykd903bhj4hochumz1mu5i906qdtgawzvp5xxc.png" alt="Singular Sound logo" width="340"/>
  </a>
</div>

# Getting Started

## Building in MacOS X

### Initial Setup
To build BBManager you'll need qt5.5; you can get it through brew (if you don't have brew, you can get it at brew.sh), by invoking the command `brew install qt@5.5` in the command line.

To successfully run BBManager, you need a local installation of libquazip and libMinini.

Those should be installed at the ~/lib directory, because the OS looks for shared libraries there without any configuration; the files to be moved into that directory can be found in BBManager/libs/quazip/macx/release and BBManager/libs/minIni/macx/release respectively.

### Building and Deploying
The project file is BBManager/project/ProjectEditor.pro. In BBManager/project you can build by running `qmake && make clean && make all`. The BBManager.app will be in the same directory, and you can run this to test the app.

While dependencies don't change, future builds can be done with just `make`.

To pack for deployment - ie: to share with other people - you need to run `BBManager/mac_deployment/deploy.sh` with `BBManager/project/BBManager.app` as an argument; this will make the dynamic libraries be loaded from the BBManager.app bundle, instead of them being sought elsewhere in the system.

I haven't yet figured out how to sign it, and that needs to be figured out before we can start shipping it as a final version.

## Building in Windows

### Initial Setup
To build BBMnager you'll need Qt 5.12.4 or up.

To successfully run BBManager, you'll need `Desktop Qt 5.12.4 MSVC2015 64bit` Kit as your kit selection. 

##### Setting your kit
On Tools->Options->Kits make sure it's using `Microsoft Visual C++ Compiler 14.0 (amd64)` or up for C and C++, if you don't see it on the drop down options you can download Microsoft Visual Studio and choose Desktop development with C++ making sure the installation includes all the MSVC optionals. Or create it manually, by setting its path to  `C:\Program Files (x86)\Wndows Kits\10\Debuggers\x64\cdb.exe`, setting the make path to `C:\Qt\Tools\QtCreator\bin` selecting the jom.exe file, and the qt mkspecs to `C:\Qt\Tools\msvc2015_64\mkspecs`. 

Finally make sure your `C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64` folder has a rc.exe and a rcdll.dll files, if they don't exists copy them from `C:\Program Files (x86)\Windows Kits\10\bin\VERSION\x64`.

### Building and Deploying
The project file is `BBManagerLean/BBManagerLean.pro`. When opening the project select the corresponding kit, Clean, Build, and Run.

## Contributing

Please read the [contribution guidelines for this project](CONTRIBUTING.md), for details on contributions and the process of submitting pull requests.
