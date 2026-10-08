@echo off
setlocal

set "SRC=F:\Skyrim Dev\Skyrim_SpellHotbar2\skse_plugin\build\release_with_debug_info"
set "DST=F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\SKSE\Plugins"

if not exist "%DST%" mkdir "%DST%"

copy /Y "%SRC%\SpellHotbar2.dll" "%DST%\" || goto :fail
copy /Y "%SRC%\SpellHotbar2.pdb" "%DST%\" || goto :fail

echo Copied SpellHotbar2.dll and SpellHotbar2.pdb to "%DST%"
exit /b 0

:fail
echo Copy failed.
exit /b 1
