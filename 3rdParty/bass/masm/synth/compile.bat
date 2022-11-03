@echo off
ml /c /coff synth.asm
if errorlevel 1 goto end
link /SUBSYSTEM:WINDOWS synth.obj
:end
if exist *.obj del *.obj