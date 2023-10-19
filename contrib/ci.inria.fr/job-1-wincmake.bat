REM
REM  Copyright © 2012-2023 Inria.  All rights reserved.
REM  See COPYING in top-level directory.
REM

call %JENKINS_CONFIG_DIR%\\ciprofile.bat

set TARBALL=%1
set BASENAME=%TARBALL:~0,-7%
REM reduce the build path name to avoid issues with very long file names in cmake/nmake
set SHORTNAME=%BASENAME:~-9%

echo ###############################
echo Running on:
hostname
ver
echo Tarball: %TARBALL%
echo Build short name: %SHORTNAME%
echo ############################

sh -c "tar xfz %TARBALL%"
if %errorlevel% neq 0 exit /b %errorlevel%

move %BASENAME% %SHORTNAME%
if %errorlevel% neq 0 exit /b %errorlevel%

cd %SHORTNAME%\contrib\windows-cmake
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --install-prefix=%cd%/install -DCMAKE_BUILD_TYPE=Release -B build
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build build --parallel
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --install build
if %errorlevel% neq 0 exit /b %errorlevel%

install\bin\lstopo-no-graphics.exe -v
if %errorlevel% neq 0 exit /b %errorlevel%

install\bin\lstopo-no-graphics.exe --windows-processor-groups
if %errorlevel% neq 0 exit /b %errorlevel%

install\bin\hwloc-info.exe --support
if %errorlevel% neq 0 exit /b %errorlevel%

ctest --test-dir build -V
if %errorlevel% neq 0 exit /b %errorlevel%

exit /b 0
