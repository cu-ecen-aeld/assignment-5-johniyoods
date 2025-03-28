#!/bin/sh

filesdir=$1
searchstr=$2

if [ -n "$filedir" -a -n "$searchstr" ];then
	exit 1;
fi

no=$(ls -1 $filesdir | wc -l)
count=$(cat $filesdir/* | grep -c $searchstr)
echo "The number of files are $no and the number of matching lines are $count"
