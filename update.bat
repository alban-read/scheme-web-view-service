xcopy SchemeWebView\x64\Release\docs docs /S /Y
copy SchemeWebView\x64\Release\scripts scripts
del release /S /Q /F
mkdir release
mkdir release\docs
mkdir release\docs\addon
mkdir release\docs\lib
mkdir release\scripts
mkdir release\boot
copy SchemeWebView\x64\Release\boot release\boot
copy SchemeWebView\x64\Release\*.exe release
copy SchemeWebView\x64\Release\*.dll release
copy scripts release\scripts
xcopy docs release\docs /S /Y
copy docs\lib release\docs\lib
copy docs\mode release\docs\mode
copy docs\addon release\docs\addon


