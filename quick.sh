# Minimal build

$PWD/script/installreq.sh

if [[ $# -eq 1 ]]
then
    BUILD=$1
else
    BUILD="build"
fi

if ($PWD/script/cmake_generic.sh $PWD/$BUILD -DDRY_TOOLS=0 -DDRY_SAMPLES=0) then
    cd $PWD/$BUILD;
    make;
    cd $PWD;
fi
