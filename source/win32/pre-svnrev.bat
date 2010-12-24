@echo off

cd ..

for /F "skip=3 tokens=1 delims=" %%f in (.svn\entries) do call :print %1 %2 %%f
exit

:print
echo #define SVN_REV %3>"%1%2"
echo #define SVN_REVSTR "%3">>"%1%2"
exit
