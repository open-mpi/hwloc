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

cd %TARBALL:~0,-7%\contrib\windows
if %errorlevel% neq 0 exit /b %errorlevel%

MSBuild /p:Configuration=Release /p:Platform=x64 hwloc.sln
if %errorlevel% neq 0 exit /b %errorlevel%

x64\Release\lstopo-no-graphics.exe -v
if %errorlevel% neq 0 exit /b %errorlevel%

x64\Release\lstopo-no-graphics.exe --windows-processor-groups
if %errorlevel% neq 0 exit /b %errorlevel%

x64\Release\hwloc-info.exe --support
if %errorlevel% neq 0 exit /b %errorlevel%

exit /b 0
