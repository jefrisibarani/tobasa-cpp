@echo off

REM 
REM Run this script from project root dir
REM Author: Jefri Sibarani
REM

mkdir build

echo -------------------------------------------------------
echo -- Configuring Tobasa Framework
echo -------------------------------------------------------
cmake -S . -B build -G "Visual Studio 15 2017" -T host=x64 -A x64 


timeout 3 > NUL
echo -------------------------------------------------------
echo -- Building - Release 
echo -------------------------------------------------------
cmake --build build --config=Release


timeout 3 > NUL
echo -------------------------------------------------------
echo -- Building - Debug 
echo -------------------------------------------------------
cmake --build build --config=Debug