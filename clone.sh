#!/bin/bash

function usage()
{
    echo "$0 prj_name"
    exit 0
}

[ $# -ne 1 ] && usage
prj_name=$1
[[ -z "$prj_name" ]] && usage

cur_dir=$(cd $(dirname $0); pwd)
cd $cur_dir
[ -d $cur_dir/$prj_name ] && rm -rf $prj_name
mkdir -p $prj_name

hello_dir=hello
hello_c=$hello_dir/hello.c
hello_makefile=$hello_dir/Makefile
prj_c=$prj_name/$prj_name.c
prj_makefile=$prj_name/Makefile

cp $hello_c $prj_c
sed -i "s/hello_init/${prj_name}_init/g" $prj_c
sed -i "s/hello_exit/${prj_name}_exit/g" $prj_c

cp $hello_makefile $prj_makefile
sed -i "s/hello.o/${prj_name}.o/g" $prj_makefile