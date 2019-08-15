FROM ubuntu:18.04

RUN apt-get update -y;\
apt-get install -y libtiff5 ;\
apt-get install -y libgoogle-glog-dev ;\
apt-get install -y libpng16-16 ;\
apt-get install -y libopenexr22 ;\
# We will mount the host input dir in this directory.
INPUTS=/tmp/inputs ;\
OUTPUTS=/tmp/outputs ;\
#make sure the user running the algorithm can read
mkdir $INPUTS ;\
#make sure the user running the algorithm can read and write
mkdir $OUTPUTS ;

ADD ./build/commandLineCli /tmp/commandLineCli

CMD /tmp/commandLineCli