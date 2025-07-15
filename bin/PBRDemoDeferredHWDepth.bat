@echo off
setlocal
set "dirname=%~dp0"
if %0 == "%~dpnx0" where /q "%cd%:%~nx0" && set "dirname=%cd%\"
if exist "%dirname%DryPlayer.exe" (set "DEBUG=") else (set "DEBUG=_d")
"%dirname%DryPlayer%DEBUG%" Scripts/42_PBRMaterials.as -renderpath CoreData/RenderPaths/PBRDeferredHWDepth.xml %*
