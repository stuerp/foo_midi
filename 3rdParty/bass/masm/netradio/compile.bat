@echo off
rc /v res.rc
if errorlevel 1 goto end
cvtres /machine:ix86 res.res
if errorlevel 1 goto end
ml /c /coff netradio.asm
if errorlevel 1 goto end
link /SUBSYSTEM:WINDOWS netradio.obj res.obj
:end
if exist *.obj del *.obj
if exist *.res del *.res