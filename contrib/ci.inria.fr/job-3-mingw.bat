REM
REM  Copyright © 2012-2020 Inria.  All rights reserved.
REM  See COPYING in top-level directory.
REM

call %JENKINS_CONFIG_DIR%\\ciprofile.bat

REM %MINGW32SH% and %MINGW64SH% should be defined on the slave to something like:
REM   C:\msys64\usr\bin\env MSYSTEM=MINGW32 HOME=%cd% /usr/bin/bash -li
REM (either in ciprofile.bat or custom Windows environment variables)

REM bashrc should then define $MSLIB32_PATH and $MSLIB64_PATH pointing to MSVC
REM lib.exe directory for 32/64 builds, such as:
REM export MSLIB32_PATH=/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.27.29110/bin/Hostx86/x86/
REM export MSLIB64_PATH=/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.27.29110/bin/Hostx64/x64/

sh -c "chmod 755 job-3-mingw.sh"

if %2 equ 32 set MINGWSH=%MINGW32SH%
if %2 equ 64 set MINGWSH=%MINGW64SH%

%MINGWSH% -c "./job-3-mingw.sh %1 %2"
if %errorlevel% neq 0 exit /b %errorlevel%

exit /b 0
