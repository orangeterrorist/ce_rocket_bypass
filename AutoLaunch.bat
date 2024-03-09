 @echo off
 CLS
 ECHO.
 ECHO =============================
 ECHO Running Admin shell
 ECHO =============================

:init
 setlocal DisableDelayedExpansion
 set cmdInvoke=1
 set winSysFolder=System32
 set "batchPath=%~dpnx0"
 for %%k in (%0) do set batchName=%%~nk
 set "vbsGetPrivileges=%temp%\OEgetPriv_%batchName%.vbs"
 setlocal EnableDelayedExpansion

:checkPrivileges
  NET FILE 1>NUL 2>NUL
  if '%errorlevel%' == '0' ( goto gotPrivileges ) else ( goto getPrivileges )

:getPrivileges
  if '%1'=='ELEV' (echo ELEV & shift /1 & goto gotPrivileges)
  ECHO.
  ECHO **************************************
  ECHO Invoking UAC for Privilege Escalation
  ECHO **************************************

  ECHO Set UAC = CreateObject^("Shell.Application"^) > "%vbsGetPrivileges%"
  ECHO args = "ELEV " >> "%vbsGetPrivileges%"
  ECHO For Each strArg in WScript.Arguments >> "%vbsGetPrivileges%"
  ECHO args = args ^& strArg ^& " "  >> "%vbsGetPrivileges%"
  ECHO Next >> "%vbsGetPrivileges%"
  
if '%cmdInvoke%'=='1' goto InvokeCmd 

ECHO UAC.ShellExecute "!batchPath!", args, "", "runas", 1 >> "%vbsGetPrivileges%"
goto ExecElevation

:InvokeCmd
ECHO args = "/c """ + "!batchPath!" + """ " + args >> "%vbsGetPrivileges%"
ECHO UAC.ShellExecute "%SystemRoot%\%winSysFolder%\cmd.exe", args, "", "runas", 1 >> "%vbsGetPrivileges%"

:ExecElevation
"%SystemRoot%\%winSysFolder%\WScript.exe" "%vbsGetPrivileges%" %*
exit /B

:gotPrivileges
setlocal & cd /d %~dp0
if '%1'=='ELEV' (del "%vbsGetPrivileges%" 1>nul 2>nul  &  shift /1)

@echo off
KDU-1.2.6\Bin\kdu.exe -dse 0
COLOR C
ECHO =======================================================================================
ECHO KDU DSE 0 (DSE OFF)
ECHO =======================================================================================
timeout /t 2 /nobreak > nul
COLOR F
start Lunar-Engine\Anomalyengine-x86_64.exe
COLOR 9
ECHO 
ECHO =======================================================================================
ECHO Launched Lunar Engine
ECHO =======================================================================================
ECHO launch has been paused, click ok on any errors wand wait for DBK overlay, then continue.
PAUSE
COLOR F
KDU-1.2.6\Bin\kdu.exe -dse 6
COLOR A
ECHO =======================================================================================
ECHO KDU DSE 6 (DSE ON)
ECHO =======================================================================================
timeout /t 2 /nobreak > nul
ECHO Attempting to stop "%username%"'s KDU Driver
ECHO =======================================================================================
timeout /t 1 /nobreak > nul
sc stop CEDRIVER73
ECHO =======================================================================================
ECHO END
ECHO =======================================================================================
cmd /k