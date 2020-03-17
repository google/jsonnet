call RefreshEnv.cmd
cmake . -B build -DCMAKE_C_COMPILER="%CC%" -DCMAKE_CXX_COMPILER="%CXX%" -G "%GENERATOR%"
if %errorlevel% NEQ 0 exit /b %errorlevel%
cmake --build build --target jsonnetfmt
if %errorlevel% NEQ 0 exit /b %errorlevel%
cmake --build build --target jsonnet
if %errorlevel% NEQ 0 exit /b %errorlevel%
cmake --build build --target run_tests
if %errorlevel% NEQ 0 exit /b %errorlevel%
mkdir upload
7z -tzip a .\upload\jsonnet-bin-"%TRAVIS_TAG%"-"%COMMON_OS_NAME%".zip .\build\jsonnet*.exe
if %errorlevel% NEQ 0 exit /b %errorlevel%