#!/bin/bash

for param in "$@"; do
    if [ -e $param ] 
    then if [ -r $param ] 
        then
            while read -r command; do
                ./jobCommander issueJob ${command}
            done < $param
            ./jobCommander issueJob ${command}
        fi
    fi      
done