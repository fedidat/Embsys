#!/bin/bash

cleanup(){

  cd ..
  rm -rf $TEMP_PATH

}

check() {
echo -ne "\E[0;34m"$1"\E[0m"
if [ $2 -ne "0" ] ; then
	echo -e "\E[0;31m Failed.\E[0m"
	if [ $3 -ne "0" ] ; then
		cleanup
		exit 
	fi
else
	echo -e "\E[0;32m Succeed.\E[0m"
fi

}

EX=2
if [ $# -lt 1 ] ; then
	echo "usage: $0 tar_filename"
	exit
fi

TAR_FILE=$1
TEMP_PATH=`pwd`/embsys_temp
mkdir -p $TEMP_PATH
cp $TAR_FILE $TEMP_PATH
cd $TEMP_PATH

 
tar -xf $TAR_FILE
check "Extracting tar file..." $? 1

if [ -r README ] ; then
	check "README file exist:" 0 0 
	echo -e "\E[0;34mREADME file first line, make sure these are your logins: \E[0;33m" `head -1 README` "\E[0m"
	echo Press enter to continue...
	read dummy
else
	check "README file exist:" 1 0
fi

[ -r makefile ]
check "'makefile' exist:" $? 1
[ -r device.svr3 ]
check "Linker command file 'device.svr3' exist:" $? 1
make
check "'make'" $? 1
[ -r a.out ]
check "'a.out' created:" $? 1


cleanup
echo -e "\E[0;31m*** This script does not necessarily check all submission requirements ***\E[0m"
echo -e "\E[0;31m*** It is up to you to make sure your submission comply all guidelines ***\E[0m"
echo Press enter to continue...
read dummy


