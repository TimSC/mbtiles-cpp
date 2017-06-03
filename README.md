# mbtiles-cpp
MBTiles reader, vector map pbf v2.0 reader. C++ library for decoding of mbtiles and vector data into function callbacks. It can be integrated by compiling it inline with your code by adding appropriate files to your project.

The MBTiles spec is at https://github.com/mapbox/mbtiles-spec

Vector tile spec is at https://github.com/mapbox/vector-tile-spec

UTFGrid spec is at https://github.com/mapbox/utfgrid-spec

MBTiles available at http://osm2vectortiles.org/downloads/ and https://www.mapbox.com/

This software may be redistributed under the MIT license.

   git clone --recursive git@github.com:TimSC/mbtiles-cpp.git

   sudo apt-get install libsqlite3-dev g++ libprotobuf-dev zlib1g-dev

   make

Useful?
-------

To update the protobuf files, remove the line "optimize_for = LITE_RUNTIME;" from vector_tile.proto, then

  protoc vector_tile.proto --cpp_out vector_tile20


