@echo off

mkdir ..\..\hmbuild
pushd ..\..\hmbuild
cl -Zi ..\hmtrain\code\*.cpp User32.lib Gdi32.lib
popd