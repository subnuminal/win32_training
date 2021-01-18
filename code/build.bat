@echo off

mkdir y:\hmbuild
pushd y:\hmbuild
cl -Zi y:\hmtrain\code\*.cpp User32.lib Gdi32.lib
popd