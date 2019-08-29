# Building a new production container

### Building a fresh executable

From the root directory.

    cmake commandLineCli dist/build
    cd dist/build
    make
    
    
### Building the docker image

    docker build -t wipp/wipp-egt-plugin:1.0.1 .
    docker login
    docker push wipp/wipp-egt-plugin:1.0.1