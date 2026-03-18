#!/bin/bash

# 
# Run this script from project root dir
# Author: Jefri Sibarani
#



mkdir build_d
echo -------------------------------------------------------
echo -- Building - Debug 
echo -------------------------------------------------------
cmake -S . -B build_d -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Debug"

sleep 5
cmake --build build_d
sleep 5



mkdir build_r
echo -------------------------------------------------------
echo -- Building - Release 
echo -------------------------------------------------------
cmake -S . -B build_r -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Release"

sleep 5
cmake --build build_r
sleep 5
