REM
REM  Copyright © 2012-2021 Inria.  All rights reserved.
REM  See COPYING in top-level directory.
REM

call %JENKINS_CONFIG_DIR%\\ciprofile.bat

set TARBALL=%1

echo ###############################
echo Running on:
hostname
ver
echo Tarball: %TARBALL%
echo ############################

sh -c "tar xfz %TARBALL%"
if %errorlevel% neq 0 exit /b %errorlevel%

cd %TARBALL:~0,-7%\contrib\windows-cmake
if %errorlevel% neq 0 exit /b %errorlevel%

cmake -DCMAKE_INSTALL_PREFIX=%cd%\install -DCMAKE_BUILD_TYPE=Release .
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build .
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build . --target INSTALL
if %errorlevel% neq 0 exit /b %errorlevel%

install\bin\lstopo-no-graphics.exe -v
if %errorlevel% neq 0 exit /b %errorlevel%

install\bin\lstopo-no-graphics.exe --windows-processor-groups
if %errorlevel% neq 0 exit /b %errorlevel%

install\bin\hwloc-info.exe --support
if %errorlevel% neq 0 exit /b %errorlevel%

exit /b 0
