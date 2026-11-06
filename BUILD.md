# Инструкция по сборке

## Требования
*   **Qt 6.8+** (с модулем Multimedia)
*   **MinGW 13.1.0** (или MSVC)
*   **CMake 3.20+**
*   **vcpkg** (для libvorbis и mp3lame)

## Команды (Windows, MinGW)
```powershell
# Установка зависимостей через vcpkg
vcpkg install libvorbis:x64-windows mp3lame:x64-windows

# Конфигурация
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_TOOLCHAIN_FILE="путь/к/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"

# Сборка
cmake --build build --config Release