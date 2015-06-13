#!/bin/sh

## @file cat.sh
## cat(1) wrapper for cat'ing to the ev3 LCD
## calls cat on input, builds a bytecode executable
## and pipes it to the VM via netcat
## @author Ahmad Fatoum

#consts
LINE_WIDTH = 20


CAT = "$(cat $@)"

IFS = '\n' read -ra LINE <<< "$IN"
for i in "${LINE[@]}"; do
    # loop for each < LINE_WIDTH
	# generate bytecode
	
done

