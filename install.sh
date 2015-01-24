readlink_f() { # Recode readlink -f for mac osx

    # From http://stackoverflow.com/questions/1055671/how-can-i-get-the-behavior-of-gnus-readlink-f-on-a-mac
    TARGET_FILE=$1

    cd `dirname $TARGET_FILE`
    TARGET_FILE=`basename $TARGET_FILE`

    while [ -L "$TARGET_FILE" ]
    do
        TARGET_FILE=`readlink $TARGET_FILE`
        cd `dirname $TARGET_FILE`
        TARGET_FILE=`basename $TARGET_FILE`
    done

    PHYS_DIR=`pwd -P`
    RESULT=$PHYS_DIR/$TARGET_FILE
    echo $RESULT
}

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
PREFIX=$(readlink_f $1)

cmake_build_install_current_dir $PREFIX

mkdir externals;
cd externals;

git clone http://github.com/matt-42/iod.git
cd iod
cmake_build_install_current_dir $PREFIX

cd $ROOT
cmake_build_install_current_dir $PREFIX
