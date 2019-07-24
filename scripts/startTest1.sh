#!/usr/bin/env bash

export GLOG_logtostderr=1;
export GLOG_v=3;

PATH=../cmake-build-release
#PATH=../cmake-build-relwithdebingo
#PATH=../cmake-build-debug

$PATH/commandLineCli \
-i "/home/gerardin/Documents/images/egt-test-images/egt_test/inputs/phase_image_002_tiled256_pyramid.tif" \
-o "/home/gerardin/CLionProjects/newEgt/outputs/" \
-d "16U" --level "0" --minhole "10" --maxhole "3000" --minobject "20" -x "0"

