set -e
mkdir -p .build
pushd .build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DLTO=ON -DNATIVE=ON
ninja
./dvm ../pythran
popd
