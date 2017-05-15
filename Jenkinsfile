pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        git(url: 'https://github.com/Serinox/mbtiles-cpp.git', branch: 'master')
        sh '''git submodule init
git submodule update


if [ -d "build" ]; then
rm -rf build
fi
mkdir build
cd build

export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/libsqlite3.so.0:$LD_LIBRARY_PATH
cmake ../

make mbtilesCpp'''
      }
    }
  }
}