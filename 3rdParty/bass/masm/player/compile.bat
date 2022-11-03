@echo off
rc /v rsrc.rc
if errorlevel 1 goto end
cvtres /machine:ix86 rsrc.res
if errorlevel 1 goto end
ml /c /coff player.asm
if errorlevel 1 goto end
link /SUBSYSTEM:WINDOWS player.obj rsrc.obj
:end
if exist *.obj del *.obj
if exist *.res del *.res