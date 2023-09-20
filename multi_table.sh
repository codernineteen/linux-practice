#!/bin/sh
#20194902 전예찬

res=0

if [ -z "$1" ] || [ -z "$2" ]
then
        echo "Invalid input"
        exit 1
fi

if [ $1 -le 0 ] || [ $2 -le 0 ]
then
        echo "Input must be greater than zero"
        exit 1
fi

for i in $(seq 1 $1)
do
        for j in $(seq 1 $2)
        do
                res=`expr $i \* $j`
                printf "%d*%d = %d \t" $i $j $res
        done
        echo
done
