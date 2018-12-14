REM
REM  Copyright © 2012-2018 Inria.  All rights reserved.
REM  See COPYING in top-level directory.
REM

call %JENKINS_CONFIG_DIR%\\ciprofile.bat

set TARBALL=%1

sh -c "tar xfz %TARBALL%"
if %errorlevel% neq 0 exit /b %errorlevel%

cd %TARBALL:~0,-7%\contrib\windows
if %errorlevel% neq 0 exit /b %errorlevel%

%MSBUILD_PATH%\MSBuild hwloc.sln /p:Configuration=Release /p:Platform=x64
if %errorlevel% neq 0 exit /b %errorlevel%

x64\Release\lstopo-no-graphics.exe
if %errorlevel% neq 0 exit /b %errorlevel%

x64\Release\hwloc-info.exe --support
if %errorlevel% neq 0 exit /b %errorlevel%

exit /b 0
