Readme for it8tool
==================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/logctool/blob/master/README.md)

Introduction
------------

it8tool is a set of utilities for computation of color correction matrices from it8 charts

![Sample image or figure.](images/image.png 'it8tool')

Building
--------

The it8tool app can be built both from commandline or using optional Xcode `-GXcode`.

```shell
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=<path> -DCMAKE_PREFIX_PATH=<path> -DCMAKE_CXX_FLAGS="-I<eigen3 include>" -DCMAKE_CXX_STANDARD=17 -GXcode
cmake --build . --config Release -j 8
```

Rawtoaces uses an old version of eigen while eigen3 is installed with the eigen3 suffix in the include path, add to CMAKE_CXX_FLAGS if necessary.

**Example using 3rdparty on arm64:**

```shell
mkdir build
cd build
cmake ..
cmake .. -DCMAKE_INSTALL_PREFIX=<path>/3rdparty/build/macosx/arm64.debug -DCMAKE_INSTALL_PREFIX=<path>/3rdparty/build/macosx/arm64.debug -DCMAKE_CXX_FLAGS="-I<path>/3rdparty/build/macosx/arm64.debug/include/eigen3" -DBUILD_SHARED_LIBS=TRUE -DCMAKE_CXX_STANDARD=17 -GXcode
```


Usage
--------

Print i8tool help message with flag ```--help```.

```shell
it8tool -- a set of utilities for computation of color correction matrices from it8 chart

Usage: it8tool [options] filename...

Commands that read images:
    -i FILENAME                         Input chart file
General flags:
    --help                              Print help message
    -v                                  Verbose status messages
    -d                                  Debug status messages
Input flags:
    --referencefile IT8FILE             Input it8 reference file
    --offset IT8OFFSET                  Input it8 chart offset (default: 0.0, 0.0)
    --scale IT8SCALE                    Input it8 chart scale (default: 0.8, 0.8)
    --rotate90                          Rotate it8 chart 90 degrees
    --rotate180                         Rotate it8 chart 180 degrees
    --rotate270                         Rotate it8 chart 270 degrees
Output flags:
    --outputfile FILE                   Output it8 chart file
    --outputdatafile FILE               Output it8 data file
    --outputrawpatchfile FILE           Output raw patch file
    --outputrawreferencefile FILE       Output raw reference file
    --outputcalibrationmatrixfile FILE  Output calibration matrix file
    --outputcalibrationlutfile FILE     Output calibration lut file
```

**Input flags**

The input flags are used to set-up the it8 charts for meaurement. The ```--referencefile``` is the it8 reference data file. Target datafiles can be found in ```data/it8``` or directly at ```https://www.xrite.com/service-support/downloads/m/monaco-it8-reference-files```.

To position and align the measurement geometry the ```--offset```, ```--scale```, ```--rotate90```, ```--rotate180```, ```--rotate270``` can be used for calibration of placement.

**Output flags**

The output file will contain the it8 chart file with the measurement geometry applied for verification of accuracy and placement. 

The it8 data file ```--outputdatafile```contains all color space calculations and measurements per patch for verification.

The raw patch file ```--outputrawpatchfile``` contains measure D65 CIE XYZ per patch for verification in csv.

The raw reference file ```--outputrawreferencefile``` contains reference D65 CIE XYZ per patch for verification in csv.

The calibration matrix file ```--outputcalibrationmatrixfile``` contains the 3x3 color calibration matrix in csv.

The calibration LUT file ```--outputcalibrationlutfile``` contains the 32x32 color calibration LUT for input and output colorspace as Aces2065-1 AP0 Linear with D65 white point.


Example using ```canon_eos_5d_mark_iii_it8_71_1993_mont45_2021_03.cr2```
--------

```shell
./it8tool
./data/charts/canon_eos_5d_mark_iii_it8_71_1993_mont45_2021_03.cr2
-d 
--rotate90
--referencefile ./data/it8/4x5_Transparency_Ref_Files_2007_Forward/MONT45.2021.03.txt
--offset '0.0, -135'
--outputfile mont45_2021_03_chart.exr
--outputdatafile mont45_2021_03_datafile.csv
--outputrawpatchfile  mont45_2021_03_rawpatches.csv
--outputrawreferencefile  mont45_2021_03_rawreferences.csv
--outputcalibrationmatrixfile mont45_2021_03_ccm.csv
--outputcalibrationlutfile mont45_2021_03_ccm.lut
```


Packaging
---------

The `macdeploy.sh` script will deploy mac bundle to dmg including dependencies.

```shell
./macdeploy.sh -e <path>/it8tool -d <path> -p <path>
```

Dependencies
-------------

| Project     | Description |
| ----------- | ----------- |
| Imath       | [Imath project @ Github](https://github.com/AcademySoftwareFoundation/Imath)
| OpenImageIO | [OpenImageIO project @ Github](https://github.com/OpenImageIO/oiio)
| Eigen       | [Eigen @ Gitlab](https://gitlab.com/libeigen/eigen)
| Rawtoaces   | [Rawtoaces @ Gitlab](https://github.com/AcademySoftwareFoundation/rawtoaces)
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
Monaco Acquisition Company and X-Rite, Incorporated.

* Roboto font   
https://fonts.google.com/specimen/Roboto   
Designed by Christian Robertson