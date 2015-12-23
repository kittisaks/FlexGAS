#!/bin/bash

pid=(`ps aux | grep $1 | awk '{print $2}'`)
for i in ${pid[@]}
do
    kill -9 $i
done

