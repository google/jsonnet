# Vendoring

git clone --depth=1 --branch=master --recursive git@github.com:jbeder/yaml-cpp.git third_party/yaml-cpp/yaml-cpp
rm -rf third_party/yaml-cpp/yaml-cpp/.git
rm third_party/yaml-cpp/yaml-cpp/WORKSPACE
rm third_party/yaml-cpp/yaml-cpp/BUILD.bazel
