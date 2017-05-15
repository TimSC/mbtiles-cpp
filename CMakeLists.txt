cmake_minimum_required(VERSION 3.7)
project(mbtilesCpp)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES
        Source/vector_tile21/vector_tile.pb.cc
        Include/vector_tile21/vector_tile.pb.h
        Source/MBTileReader.cpp
        Include/MBTileReader.h
        Source/VectorTile.cpp
        Include/VectorTile.h
        cppGzip/DecodeGzip.cpp
        cppGzip/DecodeGzip.h
        cppGzip/EncodeGzip.cpp
        cppGzip/EncodeGzip.h
        cppGzip/HandleDeflate.h)

#       Examples/example.cpp
#       Examples/example2.cpp

#pull in some helpers
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FindProtobuf)
include(FindZLIB)

#find sqlite
find_package (sqlite3)

include_directories(Include/)
include_directories(${SQLITE3_INCLUDE_DIR})
include_directories(${Protobuf_INCLUDE_DIR})
include_directories(${ZLIB_INCLUDE_DIR})



add_library(mbtilesCpp ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(mbtilesCpp ${SQLITE3_LIBRARY})
TARGET_LINK_LIBRARIES(mbtilesCpp ${Protobuf_LIBRARY})
TARGET_LINK_LIBRARIES(mbtilesCpp ${ZLIB_LIBRARY})

add_executable(example1 Examples/example.cpp)
TARGET_LINK_LIBRARIES(example1 mbtilesCpp)
TARGET_LINK_LIBRARIES(example1 ${SQLITE3_LIBRARY})
TARGET_LINK_LIBRARIES(example1 ${Protobuf_LIBRARY})
TARGET_LINK_LIBRARIES(example1 ${ZLIB_LIBRARY})

add_executable(example2 Examples/example2.cpp)
TARGET_LINK_LIBRARIES(example2 mbtilesCpp)
TARGET_LINK_LIBRARIES(example2 ${SQLITE3_LIBRARY})
TARGET_LINK_LIBRARIES(example2 ${Protobuf_LIBRARY})
TARGET_LINK_LIBRARIES(example2 ${ZLIB_LIBRARY})