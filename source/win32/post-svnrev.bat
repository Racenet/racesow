@echo off

cd ..
xcopy "%1.svn\text-base\%2.svn-base" "%1%2" /Y
set errorlev=0
