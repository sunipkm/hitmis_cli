@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
@set RC_INCLUDE_PATH="C:\Program Files\Windows Kits\10\Include\10.0.19041.0\um"
@set OUT_DIR=output
@set OUT_EXE=hitmis_cli
@set INCLUDES= /I .\include 
@set SOURCES=src/*.cpp src/*.c
@set LIBS= lib\Pvcam32.lib lib\ATMCD32M.LIB lib\cfitsio.lib
@set RESOURCES=hitmis.res
mkdir %OUT_DIR%
rc /nologo /i %RC_INCLUDE_PATH% /i .\include\ /fo %OUT_DIR%/%RESOURCES% src/hitmis_resource.rc
cl /nologo /Zi /EHsc /D NDEBUG /O2 /MD %INCLUDES% /D UNICODE /D _UNICODE %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS% %OUT_DIR%\%RESOURCES%