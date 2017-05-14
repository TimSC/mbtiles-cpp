pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        git(url: 'https://github.com/Serinox/mbtiles-cpp.git', branch: 'master')
        sh '''rm -rf build
mkdir build
cd build
cmake ../'''
      }
    }
  }
}