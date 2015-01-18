
check_for_executable()
{
  command -v $1 >/dev/null 2>&1 || { echo "$1 is required."; exit 1; }    
}

cmake_build_install_current_dir()
{
 { mkdir -p silicon_build && cd silicon_build && cmake .. -DCMAKE_PREFIX_PATH=$1 -DCMAKE_INSTALL_PREFIX=$1 && make -j4 install; } || { echo "Cannot install $1."; exit 1; }
}

[ ! $# -eq 1 ] && echo "Usage: install.sh prefix" && exit 1
[ ! -d $1 ] && echo "The given prefix is not a directory" && exit 1

check_for_executable cmake;
check_for_executable git;

ROOT=$PWD
PREFIX=$(readlink --canonicalize $1)

cmake_build_install_current_dir $PREFIX

mkdir externals;
cd externals;

git clone http://github.com/matt-42/iod.git
cd iod
cmake_build_install_current_dir $PREFIX

cd $ROOT
cmake_build_install_current_dir $PREFIX
