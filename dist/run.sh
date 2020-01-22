#!/usr/bin/env bash

#INPUT_DIR_HOST=/Users/gerardin/Documents/projects/wipp++/pyramid-building/resources/dataset1
#OUTPUT_DIR_HOST=/Users/gerardin/Documents/projects/wipp++/pyramid-building/outputs

#IMAGES_RELATIVE_PATH=tiled-images
#VECTOR_RELATIVE_PATH=stitching_vector/img-global-positions-1.txt

#-i  "/home/gerardin/Documents/images/egt-test-images/dataset01/images/test01-tiled.tif"  -o "/home/gerardin/CLionProjects/newEgt/outputs/" -d "8U"  --level "0" --minhole "500" --maxhole "inf" --minobject "2" -x "0"   --op "and" --minintensity "0" --maxintensity "100" -e "loader=2;tile=10;threshold=2"

# Docker requires absolute paths
# INPUT_DIR_HOST = base dir for inputs
# OUTPUT_DIR_HOST = base dir for outputs
INPUT_DIR_HOST=/home/gerardin/Documents/images/dataset2/images/
#INPUT_DIR_HOST=/home/gerardin/Documents/images/egt-test-images/dataset01/images/
OUTPUT_DIR_HOST=/home/gerardin/Documents/projects/egt++/test-outputs/

IMAGE_RELATIVE_PATH=tiled_stitched_c01t020p1_pyramid_1024.ome.tif

# Mount locations inside the container. DO NOT EDIT.
CONTAINER_DIR_INPUTS=/tmp/inputs
CONTAINER_DIR_OUTPUTS=/tmp/outputs

# image name
CONTAINER_IMAGE=wipp/wipp-egt-plugin:1.1.4
# CONTAINER_IMAGE=wipp/wipp-egt-plugin:1.0.3
#CONTAINER_IMAGE=wipp/wipp-egt-plugin:1.0.1

# Full command
#docker run -v $INPUT_DIR_HOST:$CONTAINER_DIR_INPUTS  -v $OUTPUT_DIR_HOST:$CONTAINER_DIR_OUTPUTS \
#-e GLOG_v=4 -e GLOG_logtostderr=1 $CONTAINER_IMAGE \
#  -i $CONTAINER_DIR_INPUTS/$IMAGE_RELATIVE_PATH -o $CONTAINER_DIR_OUTPUTS \
# -d "8U"  --level "0" --minhole "500" --maxhole "inf" --minobject "2" -x "0"   --op "and" --minintensity "0" --maxintensity "100" -e "loader=2;tile=10;threshold=2"

 docker run -v $INPUT_DIR_HOST:$CONTAINER_DIR_INPUTS  -v $OUTPUT_DIR_HOST:$CONTAINER_DIR_OUTPUTS \
-e GLOG_v=1 -e GLOG_logtostderr=1 $CONTAINER_IMAGE \
  -i $CONTAINER_DIR_INPUTS/$IMAGE_RELATIVE_PATH -o $CONTAINER_DIR_OUTPUTS \
 -d "16U"  --level "0" --minhole "1000" --maxhole "inf" --minobject "3000" -x "0" --label "true"  --op "and" --minintensity "0" --maxintensity "100" -e "loader=2;tile=10;intensity=0;"

# FOR DEBUGGING ONLY. Pop up a bash shell in the container.
# docker run -it -v $INPUT_DIR_HOST:$CONTAINER_DIR_INPUTS  -v $OUTPUT_DIR_HOST:$CONTAINER_DIR_OUTPUTS $CONTAINER_IMAGE bash