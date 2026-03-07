@echo off
REM Generate compile_commands.json for clangd IntelliSense
REM Run this script whenever you add/remove source files or change Build.cs

set UBT="D:/UE/UE/UE_5.4/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe"
set PROJECT="%~dp0MyCustomCam.uproject"
set ENGINE_DB="D:/UE/UE/UE_5.4/compile_commands.json"
set OUTPUT="%~dp0compile_commands.json"

echo Generating compile_commands.json ...
%UBT% -mode=GenerateClangDatabase -project=%PROJECT% -game -engine MyCustomCamEditor Win64 Development

if exist %ENGINE_DB% (
    copy /Y %ENGINE_DB% %OUTPUT% >nul
    echo Done: compile_commands.json copied to project root.
    echo Restart VSCode or run "clangd: Restart language server" to apply.
) else (
    echo Error: UBT did not produce compile_commands.json
)
pause
