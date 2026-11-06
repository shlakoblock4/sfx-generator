$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;" + $env:PATH
cmake --build build --config Release
if ($LASTEXITCODE -eq 0) {
    .\build\sfx.exe
}