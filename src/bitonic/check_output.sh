#!/bin/bash

for file in output.*; do
	if ! grep -q "SUCCESS" "$file"; then
		echo "$file failed!!!!!!\n"
		exit 1
	fi
done
