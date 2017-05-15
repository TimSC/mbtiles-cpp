pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        git(url: 'https://github.com/Serinox/mbtiles-cpp.git', branch: 'master')
        sh '''
if [ -d "build" ]; then
    rm -rf build
fi
mkdir build
cd build
cmake ../'''
      }
    }
  }
}