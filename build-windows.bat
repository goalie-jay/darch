@echo off
echo ##### STOP #####
echo Are you using a *Developer Command Prompt*?
echo You need one unless you want the build to fail.
echo ################

mkdir build
cl .\*.c /D WINDOWS /Fe:.\build\darch.exe