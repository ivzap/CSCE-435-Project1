#!/bin/bash

for dir in cali/reversed/*; do
	files=$(ls $dir)
	if ! echo $files | grep -q "p2"; then
		echo "p2 missing"
	fi
	if ! echo $files | grep -q "p4"; then
		echo "p4 missing"
	fi
	if ! echo $files | grep -q "p8"; then
		echo "p8 missing"
	fi
	if ! echo $files | grep -q "p16"; then
		echo "p16 missing"
	fi
	if ! echo $files | grep -q "p32"; then
		echo "p32 missing"
	fi
	if ! echo $files | grep -q "p64"; then
		echo "p64 missing"
	fi
	if ! echo $files | grep -q "p128"; then
		echo "p128 missing"
	fi
	if ! echo $files | grep -q "p256"; then
		echo "p256 missing"
	fi
	if ! echo $files | grep -q "p512"; then
		echo "p512 missing"
	fi
	if ! echo $files | grep -q "p1024"; then
		echo "p1024 missing"
	fi
done

