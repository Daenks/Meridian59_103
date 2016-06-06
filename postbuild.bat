@echo off
setlocal EnableDelayedExpansion

rem Create a shortcut that can connect to localhost.
call :makeshortcut

echo Post-Build started.
set foundcli=0

rem Step 1: Try 64-bit Program Files
set m59path=%ProgramW6432%\Open Meridian\Meridian 105

if EXIST !m59path! (
   call :copying
   if %ERRORLEVEL% LSS 8 goto found
)

rem Step 2: try 32-bit Program Files(x86)
set m59path=%ProgramFiles(x86)%\Open Meridian\Meridian 105

if EXIST !m59path! (
   call :copying
   if %ERRORLEVEL% LSS 8 goto found
)

rem Step 3: try AppData\Local
set m59path=%LocalAppData%\Open Meridian\Meridian 105

if EXIST !m59path! (
   call :copying
   if %ERRORLEVEL% LSS 8 goto found
)

echo No graphics found, Please download the classic client from https://www.meridiannext.com/play/.
exit /b 0

:copying
echo Copying live graphics from !m59path! to client folder.
rem The error check after this is currently redundant, but kept in case
rem further copying code is added.
robocopy "!m59path!\resource" ".\run\localclient\resource" *.bsf *.bgf *.ogg /R:0 /MT /XO > postbuild.log
if %ERRORLEVEL% GTR 7 goto:eof
rem These extensions aren't used.
rem robocopy "!m59path!\resource" ".\run\localclient\resource" *.mid *.xmi *.wav *.mp3 /R:0 /MT /XO > nul
rem if %ERRORLEVEL% GTR 7 goto:eof
goto:eof

:found 
echo Post-Build Finished.
echo Please remember to update blakserv.cfg if server is being copied to production.
goto:eof

:makeshortcut
echo Creating meridian.exe shortcut to connect to localhost.
set CURPATH=%~dp0
set cSctVBS=CreateShortcut.vbs
(
   echo Set oWS = WScript.CreateObject^("WScript.Shell"^) 
   echo sLinkFile = oWS.ExpandEnvironmentStrings^("!CURPATH!run\localclient\meridian.exe - Shortcut.lnk"^)
   echo Set oLink = oWS.CreateShortcut^(sLinkFile^)
   echo oLink.Arguments = "/H:127.0.0.1 /P:5959"
   echo oLink.TargetPath = oWS.ExpandEnvironmentStrings^("!CURPATH!run\localclient\meridian.exe"^)
   echo oLink.WorkingDirectory = oWS.ExpandEnvironmentStrings^("!CURPATH!run\localclient"^)
   echo oLink.Save
)1>!cSctVBS!
cscript //nologo .\!cSctVBS!
DEL !cSctVBS! /f /q
