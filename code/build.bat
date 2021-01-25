@echo off

IF NOT EXIST ..\..\hmbuild (mkdir ..\..\hmbuild)
pushd ..\..\hmbuild
cl -Zi ..\hmtrain\code\*.cpp User32.lib Gdi32.lib Ole32.lib
popd