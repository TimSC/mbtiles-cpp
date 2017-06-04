
# mbtiles-cpp
MBTiles reader/writer, vector map pbf v2.0/2.1 reader/writer. C++ library for encoding and decoding of mbtiles and vector data into function callbacks. It can be integrated by compiling it inline with your code by adding appropriate files to your project.

The MBTiles spec is at https://github.com/mapbox/mbtiles-spec

Vector tile spec is at https://github.com/mapbox/vector-tile-spec

UTFGrid spec is at https://github.com/mapbox/utfgrid-spec

MBTiles available at https://openmaptiles.org/ and https://www.mapbox.com/

This software may be redistributed under the MIT license.

    git clone --recursive git@github.com:TimSC/mbtiles-cpp.git

    sudo apt-get install libsqlite3-dev g++ libprotobuf-dev zlib1g-dev

    mkdir build

    cd build

    cmake ..

    make

Update pbf files
----------------

To update the protobuf files, get vector_tile.proto from https://github.com/mapbox/vector-tile-spec, remove the line "option optimize_for = LITE_RUNTIME;", then

    mkdir vector_tile21

    protoc vector_tile.proto --cpp_out vector_tile21

protobuf lite is avoided because it doesn't contain SerializeToOstream functionality.

