# README

## Installation Instructions

### Dependencies

Dependencies can be build from sources or obtained from package managers when available.

1. g++/gcc version  7.3.0+
2. [htgs](https://github.com/usnistgov/htgs)
3. [fast image](https://github.com/usnistgov/FastImage)
4. [LibTIFF](http://www.simplesystems.org/libtiff/)
5. [openCV](https://opencv.org/releases.html) version 3.4.2+
6. [tclap](http://tclap.sourceforge.net/)
7. [glog](https://github.com/google/glog)


### Building EGT

     mkdir build && cd build
     ccmake ../ (or cmake-gui)
     make


## Distribution with Docker Container

A docker image can also be build from source.

The ```dist/Dockerfile``` contains the description of the container.


## Running the executable

#### Command line

Example:

    ./commandLineCli
    -i  "/path/to/images/pyramidal-tiled-image.tiff" \
    -o "/path/to/output/" \
    -d "16U"
    --level "0"
    --minhole "1000" --maxhole "inf" --minobject "3000" 
    
The -e flag can be used for advanced controls :
    
    -e "loader=2;tile=10;threshold=87;intensitylevel=0"


Extra parameters can be run from

#### Logging

Environment variables can be set to generate logs.

GLOG_logtostderr=1; #print tostderr instead of log file
GLOG_v=[0..4] #set log level between 0 and 4 for more detailed logs

Log Levels :

0/ no logs.
1/ basic info and execution time.
2/ algorithmic steps.
3/ detailed logs.
4/ very detailed logs.
    


    