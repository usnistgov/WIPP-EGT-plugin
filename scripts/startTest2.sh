#!/usr/bin/env bash

export GLOG_logtostderr=1;
export GLOG_v=3;

PATH=../cmake-build-release
#PATH=../cmake-build-relwithdebingo
#PATH=../cmake-build-debug

$PATH/commandLineCli \
-i "/home/gerardin/Documents/images/dataset2/images/tiled_stitched_c01t020p1_pyramid_1024.ome.tif" \
-o "/home/gerardin/CLionProjects/newEgt/outputs/" \
-d "16U" --level "0" --minhole "1000" --maxhole "3000" --minobject "3000" -x "0"

