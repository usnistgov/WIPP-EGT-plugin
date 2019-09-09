# Building a new production container

### Prerequesites

Directions are provided for installing with opencv3.4.6


### Building a fresh executable

From the root directory.

    cmake commandLineCli dist/build
    cd dist/build
    make
    
### OPENCV dependencies.

In order to avoid providing a whole development environment,
we decided not to build the opencv on the container.

We also did not want to commit 80MBs of library dependencies.

So you need to build it from sources (you can also copy the libs you used to compile the executable in the previous section)

cd /tmp ;\
wget https://github.com/opencv/opencv/archive/3.4.6.zip ;\
unzip 3.4.6.zip  ;\
cd opencv-3.4.6 ;\
mkdir build ;\
cd build ;\
cmake -DBUILD_LIST=core,highgui,improc -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. ;\
make -j7 

copy both lib/ and include/ output directories to the dist folder.
The opencv dependencies are ready.
    
### Building the docker image

    docker build -f Dockerfile-with-opencv -t wipp/wipp-egt-plugin:1.0.1 .
    docker login
    docker push wipp/wipp-egt-plugin:1.0.1