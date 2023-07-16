Readme for it8tool
==================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/logctool/blob/master/README.md)

Introduction
------------

it8tool is a set of utilities for processing it8 calibration charts.

Documentation
-------------

Building
--------

The it8tool app can be built both from commandline or using optional Xcode `-GXcode`.

```shell
mkdir build
cd build
cmake .. -DCMAKE_MODULE_PATH=<path>/logctool/modules -DCMAKE_PREFIX_PATH=<path>/3rdparty/build/macosx/arm64.debug -GXcode
cmake --build . --config Release -j 8
```

Packaging
---------

The `macdeploy.sh` script will deploy mac bundle to dmg including dependencies.

```shell
./macdeploy.sh -e <path>/logctool -d <path>/dependencies -p <path>/path to deploy
```

Dependencies
-------------

| Project     | Description |
| ----------- | ----------- |
| Imath       | [Imath project @ Github](https://github.com/AcademySoftwareFoundation/Imath)
| OpenImageIO | [OpenImageIO project @ Github](https://github.com/OpenImageIO/oiio)
| Eigen       | [Eigen @ Gitlab](https://gitlab.com/libeigen/eigen)
| 3rdparty    | [3rdparty project containing all dependencies @ Github](https://github.com/mikaelsundell/3rdparty)


Project
-------------

* GitHub page   
https://github.com/mikaelsundell/it8tool
* Issues   
https://github.com/mikaelsundell/it8tool/issues


Resources
---------

* Aces-dev Matrices    
https://github.com/ampas/aces-dev/blob/master/transforms/ctl/README-MATRIX.md


Copyright
---------

* IT8 references   
Monaco Acquisition Company

* Roboto font   
https://fonts.google.com/specimen/Roboto   
Designed by Christian Robertson