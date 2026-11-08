# MinecraftDX11

Перенос Minecraft 1.8.9 (MCP 9.19) с Java на C++20 с рендером на Direct3D 11.

План работ: [ROADMAP.md](ROADMAP.md).

> Неофициальный фанатский проект, не связан с Mojang Studios и Microsoft. В репозитории нет ассетов и
> файлов игры Mojang: для запуска нужна своя лицензионная копия Minecraft 1.8.9. "Minecraft" — торговая
> марка Mojang Synergies AB.

## Сборка

Нужны компилятор MSVC и Windows SDK (Visual Studio или "Build Tools for Visual Studio" с C++), CMake и Ninja.
IDE не требуется: если `cmake`/`ninja` нет в `PATH`, берутся те, что поставляются вместе с MSVC.
Сторонние библиотеки лежат в `third_party/`, интернет для сборки не нужен.

```
build.bat            :: Debug          -> build\debug\MinecraftDX11.exe, MinecraftDX11Tests.exe
build.bat release    :: RelWithDebInfo -> build\release\...
```

Или вручную из "x64 Native Tools Command Prompt":

```
cmake --preset debug
cmake --build --preset debug
```

## Запуск

Нужен установленный Minecraft 1.8.9: из него берутся `1.8.9.jar` и `assets` (в репозиторий они не входят).
Игра ищет их в папке игры, `%APPDATA%\.minecraft` и `%APPDATA%\.tlauncher\legacy\Minecraft\game`,
либо пути задаются явно: `--clientJar <...\1.8.9.jar> --assetsDir <...\assets>`.
Если SHA-1 jar не совпадает с официальным, в лог пишется предупреждение.

```
run.bat                  :: debug-сборка, игровая папка run\
run.bat release --fullscreen
```

Аргументы как у оригинала: `--width`, `--height`, `--fullscreen`, `--gameDir`, `--assetsDir`, `--assetIndex`,
`--resourcePackDir`, `--version`; плюс `--clientJar`. Ресурспаки (zip и папки) лежат в `<gameDir>\resourcepacks\`. Лог пишется в `<gameDir>\logs\latest.log` и в консоль, если игра запущена
из cmd/PowerShell. При падении minidump сохраняется в `<gameDir>\crash-reports\`.

## Тесты

```
build\debug\MinecraftDX11Tests.exe
powershell -ExecutionPolicy Bypass -File tools\smoke_test.ps1    :: окно: запуск, ввод, иконка, F11, ресайз, кадры по F2
```

Сверка с Java: `tools\reference\generate.bat` снимает эталонные значения `java.util.Random`, `MathHelper` и
`String.hashCode` на Java 8 (`java` в PATH, исходники MCP в `C:\MCP-919`) и пишет
`tests\reference\JavaReference.inc` и `src\util\MathHelperTables.inc`. Тесты сверяют C++ с ними побитово.
Тесты ресурсов используют установленный Minecraft и пропускаются, если его нет.

Сравнение с оригиналом по кадрам: `tools/reference/run_vanilla.ps1 -Output run/vanilla_menu.png` запускает
настоящий клиент 1.8.9 из установки (Java 8, офлайн, отдельная папка `run_vanilla/`) и сохраняет снимок по F2;
`python tools/compare_screens.py run/vanilla_menu.png run/smoke_1_start.png` сравнивает его с кадром порта.

Сценарии по экранам: `tools/ui_scenario.ps1 -Target vanilla|port -Scenario tools/scenarios/options.txt` проходит
по экранам кликами и колесом мыши в чистой игровой папке `run/cmp/<target>/game` и снимает кадры по F2 в
`run/cmp/<target>/`; `python tools/compare_screens.py --full run/cmp/vanilla run/cmp/port` сравнивает кадры целиком
и пишет `*_diff.png` с отличающимися пикселями. Для `tools/scenarios/resourcepacks.txt` сначала нужен
`python tools/scenarios/make_test_packs.py` (самодельные тестовые паки).

## Структура

```
src/            исходники; подкаталоги повторяют пакеты net.minecraft.*
  core/         логгер, крэш-хендлер, Java-семантика (Java.h, StrictMath), zip, SHA-1
  platform/     замена LWJGL 2: Display (окно), Keyboard, Mouse, Sys
  render/       Direct3D 11: устройство, swapchain, эмуляция OpenGL 1.x (render/gl)
  client/       Minecraft, main, settings, resources, renderer (DxStateManager, Tessellator, текстуры), shader, gui
  util/         Timer, MathHelper, Random (java.util.Random), ResourceLocation, ScreenShotHelper
shaders/        HLSL фиксированного конвейера, компилируется fxc при сборке
tests/          doctest-тесты; reference/ — эталоны, снятые на Java
tools/          smoke-тест окна; reference/ — генератор эталонов на Java
third_party/    miniz, stb_image, nlohmann/json, doctest
run/            рабочая папка игры (options.txt, logs, saves), не в git
```
