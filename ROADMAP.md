# Roadmap: Minecraft 1.8.9 → C++20 / DirectX 11

Исходник для переноса: `C:\MCP-919\src\minecraft` (MCP 9.19, Minecraft 1.8.9, ~278 000 строк Java, 1613 файлов).

Цель: поведенчески идентичный клиент Minecraft 1.8.9 на C++ с рендером на Direct3D 11 вместо OpenGL/LWJGL 2.
"Идентичный" значит: та же картинка GUI пиксель в пиксель, тот же мир по тому же сиду, совместимость с сохранениями Anvil и с серверами 1.8.x (протокол 47).

---

## Принципы переноса

1. **Неизменным остаётся то, что видит и чувствует игрок, а не устройство кода.** Неизменны: картинка (пиксели GUI, мира, сущностей, частиц), механики (физика, тайминги в тиках, урон, ИИ, редстоун, крафт), генерация мира по сиду, формат сейвов, сетевой протокол. Внутреннее устройство можно менять: лишние абстракции, рефлексию, копирование объектов, `instanceof`-цепочки, аллокации в горячих циклах и прочий "спагетти-код" Mojang переписываем нормально.
2. **Оптимизация разрешена сразу, если результат тот же.** Пример: мешинг чанков, батчинг GUI, кеши состояний D3D11, многопоточность. Не трогаем то, от чего зависит результат: порядок вызовов `Random`, порядок обновления блоков и сущностей, особенности и баги оригинала, которые влияют на геймплей (квази-связность поршней, дюпы, обход коллизий и т.п.). Если сомневаемся — оставляем как в оригинале.
3. **Имена из MCP там, где это помогает сверке** (`GuiMainMenu`, `drawScreen`, `ChunkProviderGenerate`), каталоги по пакетам (`client/gui`, `world/gen`, ...). В начале файла — ссылка на исходный Java-класс. Один Java-класс не обязан становиться ровно одним C++-классом.
4. **Семантика Java соблюдается явно:**
   - переполнение `int` в Java определено (wrap), в C++ это UB: для хешей, `Random`, генерации используем беззнаковую арифметику / хелперы `jint_add`, `jint_mul`;
   - `>>>` реализуется через `uint32_t`/`uint64_t`;
   - `java.util.Random` (48-битный LCG) портируется бит в бит: от него зависят сиды мира;
   - `String.hashCode()` портируется: из него получается сид из текстового поля;
   - `MathHelper.sin/cos` используют таблицу на 65536 значений, переносим так же, не `std::sin`;
   - `float`/`double` там, где они в оригинале, без замены типов.
5. **Память:** владение через `std::unique_ptr`, разделяемые объекты (сущности, чанки) через `std::shared_ptr`/`std::weak_ptr`. Без голых `new/delete`.
6. **Строки:** внутри UTF-8 (`std::string`), конвертация в UTF-16 на границах (Win32, шрифт работает по кодовым точкам).
7. **Ассеты Mojang не входят в репозиторий.** Игра читает их из лаунчерной установки пользователя: `client 1.8.9.jar` и `assets/indexes/1.8.json` + `assets/objects`. Декомпилированный код и ассеты распространять нельзя; проект для личного/учебного использования.

## Технологический стек

| Задача | Решение |
|---|---|
| Язык / сборка | C++20, MSVC, CMake + Ninja, сторонние библиотеки лежат в `third_party/` (сборка без интернета) |
| Окно и ввод | Win32 API, Raw Input для мыши (аналог `Mouse.setGrabbed`) |
| Графика | Direct3D 11 + DXGI flip-model swapchain, HLSL (компиляция `fxc`/`dxc` на этапе сборки) |
| Изображения | stb_image (PNG из jar), свой загрузчик `DynamicTexture` |
| Архивы | miniz / zlib (чтение jar, NBT, регионы, сжатие пакетов) |
| JSON | nlohmann/json (модели блоков, `sounds.json`, индекс ассетов) |
| Звук | OpenAL Soft + stb_vorbis (замена paulscode SoundSystem) |
| Сеть | Winsock2 (или standalone asio) + свой пайплайн вместо Netty |
| Криптография | Windows CNG (BCrypt): AES/CFB8, RSA, SHA-1 для логина на сервер |
| HTTP | WinHTTP (скины, авторизация) |
| Логи | свой логгер в стиле log4j (`[12:00:00] [Client thread/INFO]`) |
| Тесты | doctest + сравнение с эталонными дампами из Java-версии |

Из Java-зависимостей не переносим: Twitch (`tv.twitch`, `client/stream`), Realms, Snooper, OSHI, ICU4J. Guava/Commons/Gson заменяются стандартной библиотекой и библиотеками выше.

## Архитектура рендера: как заменить OpenGL

Код 1.8.9 рисует через фиксированный конвейер OpenGL 1.x: стек матриц, `glBegin`-подобный `Tessellator/WorldRenderer`, display lists, альфа-тест, туман, освещение, lightmap на втором текстурном юните. Всё это проходит через `GlStateManager` и `OpenGlHelper`.

План — два уровня:
- **GUI, предметы в руке, сущности, частицы:** тонкий слой `DxStateManager` + `Tessellator` поверх D3D11 с тем же API, что у `GlStateManager`. Тут много мелкого кода с точными координатами и состояниями, и построчный перенос — самый надёжный путь к попиксельному совпадению. Внутри слоя — батчинг и кеши, снаружи — привычные вызовы.
- **Мир (чанки, небо, облака, погода):** свой D3D11-рендер без эмуляции: статические буферы чанков, меньше draw call'ов, многопоточный мешинг. Требование — та же картинка: те же вершины, UV, цвета, AO, свет, туман, порядок слоёв.

Детали слоя эмуляции:

- `DxStateManager` (замена `GlStateManager`) кеширует состояния (blend, depth, cull, alpha test, fog, color, текстуры) и собирает из них объекты `ID3D11BlendState / DepthStencilState / RasterizerState / SamplerState` (с кешем по ключу).
- Стек матриц (`pushMatrix/translate/rotate/scale`, `MODELVIEW`/`PROJECTION`/`TEXTURE`) на DirectXMath, отдаётся в constant buffer.
- Фиксированный конвейер заменяется набором "uber"-шейдеров HLSL с флагами: текстура, цвет вершины, альфа-тест (`discard`), туман (linear/exp), 2 источника света для сущностей, lightmap.
- `Tessellator/WorldRenderer` пишет в динамический vertex buffer (`D3D11_MAP_WRITE_NO_OVERWRITE`, кольцевой буфер), форматы из `DefaultVertexFormats` → `ID3D11InputLayout`.
- Display lists и VBO чанков → статические vertex/index buffers.
- Различия GL ↔ D3D, которые надо учесть: диапазон глубины клипа `[-1,1]` → `[0,1]` (правка матрицы проекции), начало текстурных координат, порядок обхода граней для culling, `GL_QUADS` нет — квады разбиваются на два треугольника общим index buffer.

---

## Этап 0. Фундамент проекта

- [x] Исходники MCP 9.19 скачаны (`C:\MCP-919\src`)
- [x] Каркас проекта: CMake, структура каталогов, `.gitignore`
- [x] Сторонние библиотеки в `third_party/`: miniz, stb_image, nlohmann/json, doctest (версии в `third_party/README.md`)
- [x] Логгер в формате log4j, крэш-хендлер с minidump в `crash-reports/` (текстовый `CrashReport` — этап 15)
- [x] Java-совместимость (`core/Java.h`): арифметика с переполнением, сдвиги, насыщающие приведения `(int)float`, `String.hashCode`, `StrictMath.log` (fdlibm)
- [x] `java.util.Random` бит в бит, `MathHelper` целиком (кроме `getPositionRandom`/`getRandomUuid` — ждут `BlockPos`/`UUID`)
- [x] `ResourceLocation`, `ZipArchive`, `ResourceIndex`, `DefaultResourcePack` (jar + `assets/objects` по индексу), декодирование PNG (`TextureUtil::readBufferedImage`)
- [x] Поиск установки 1.8.9 (`--clientJar`/`--assetsDir` или `.minecraft` / TLauncher), проверка SHA-1 jar
- [x] Тесты (`MinecraftDX11Tests`): эталоны снимаются на настоящей Java 8 (`tools/reference/generate.bat`) и сверяются побитово, в debug и release

**Готово, когда:** юнит-тест `Random(seed)` выдаёт ту же последовательность, что Java; из jar читается `assets/minecraft/textures/gui/title/minecraft.png`. — **выполнено**.

Что нашли тесты и что учтено:
- в C++ порядок вычисления операндов не определён: в `(next(32) << 32) + next(32)` MSVC вызывал правый `next` первым, `nextLong`/`nextDouble` расходились с Java;
- таблицы `atan2` в `MathHelper` строятся через `Math.asin`/`Math.cos`, которые в Java 8 отличаются от C-рантайма в последнем бите — значения берутся из Java (`src/util/MathHelperTables.inc`);
- оптимизатор превращает `x >= 0 ? x : -x` в `fabs` и теряет `-0.0f`;
- `ImageIO` меняет яркость серых PNG; в ванильном jar их нет (только RGB/RGBA/палитра), для ресурспаков — проверить на этапе 3.

## Этап 1. Окно DirectX 11

Оригинал: `Minecraft.java` (`createDisplay`, `runGameLoop`, `toggleFullscreen`), `util/Timer.java`, LWJGL `Display/Keyboard/Mouse`.

- [x] Win32-окно 854×480 (стандартный размер 1.8.9, масштабируется по DPI), заголовок `Minecraft 1.8.9`
- [x] Иконка окна из `icons/icon_16x16.png` / `icon_32x32.png` (из индекса ассетов, как в оригинале)
- [x] Окно и его цикл сообщений в отдельном потоке: перетаскивание и ресайз не останавливают игру
- [x] Device + immediate context + DXGI swapchain (flip discard), обработка `WM_SIZE` (resize буферов), минимум 1×1
- [x] Очистка экрана, Present с vsync / без (с tearing, если поддерживается), лимит FPS (`Display::sync`, `limitFramerate`, по умолчанию 120)
- [x] Полноэкранный режим по F11 (borderless, как надёжная альтернатива exclusive)
- [x] Игровой цикл: `Timer` на 20 тиков/с с `renderPartialTicks`, разделение `runTick()` / рендер
- [x] Ввод: `Keyboard`/`Mouse` с API LWJGL 2 и LWJGL-кодами клавиш, символы, повторы, мышь, колесо, захват курсора через Raw Input
- [x] Обработка потери устройства (`DXGI_ERROR_DEVICE_REMOVED`), debug layer в Debug-сборке
- [x] Счётчик FPS в заголовке (временно, до F3)
- [x] Smoke-тест `tools/smoke_test.ps1`: запуск, клавиши по скан-кодам, захват мыши, F11, ресайз, закрытие

**Готово, когда:** окно открывается, ресайзится, переключается в fullscreen, держит заданный FPS, логирует нажатия клавиш в LWJGL-кодах. — **выполнено**.

Временный код этапа 1, который уберётся на этапе 2: лог ввода `[input test]` и захват мыши по клику в `Minecraft::debugInputTick`, FPS в заголовке, заливка небесным цветом вместо рендера.

## Этап 2. Главное меню (полный перенос)

Оригинал: `client/gui/GuiMainMenu.java` и всё, от чего он зависит.

### 2.1 Рендер-ядро (минимум для 2D)
- [x] Эмуляция OpenGL 1.x на D3D11 (`render/gl/GL.h`): состояние, стеки матриц, текстуры и FBO с целочисленными именами, текущие цвет/нормаль/UV, immediate mode (`glBegin`), `glPushAttrib`, `glCopyTexSubImage2D`, `glReadPixels`/`glGetTexImage`; render target'ы хранятся снизу вверх, как в GL, в swapchain кадр переворачивается при Present
- [x] Uber-шейдер фиксированного конвейера (`shaders/FixedFunction.hlsl`, компиляция `fxc` при сборке): модуляция текстурой и lightmap, туман linear/exp/exp2, альфа-тест с любым компаратором GL, освещение 2 источниками + `GL_COLOR_MATERIAL`, `GL_FLAT` с провоцирующей вершиной GL
- [x] `DxStateManager` — замена `GlStateManager` с тем же кешем состояний (чтобы воспроизводились его особенности при `pushAttrib/popAttrib`), `OpenGlHelper`
- [x] `Tessellator` + `WorldRenderer` + `WorldVertexBufferUploader` + `VertexFormat`/`DefaultVertexFormats`; недостающие атрибуты берутся из текущих значений GL
- [x] `TextureManager`, `SimpleTexture` (+ `.mcmeta` blur/clamp), `DynamicTexture`, `TextureUtil` (загрузка, мипмапы, missing texture)
- [x] `ScaledResolution`, `Framebuffer`
- [ ] Не эмулируется (пока не нужно меню): `GL_COMBINE`-режимы текстур, texgen, logic op, толстые линии, очистка с частичной маской/scissor'ом

### 2.2 Ресурсы и локализация
- [x] `IResourceManager`, `SimpleReloadableResourceManager`/`FallbackResourceManager`, `SimpleResource` с `.mcmeta`, `DefaultResourcePack` (jar + индекс ассетов)
- [x] `I18n` / `Locale` / `LanguageManager`: `.lang`-файлы (`key=value`), перезапись `%d`/`%f` в `%s` той же регуляркой, `String.format` с поведением Java (сверено с JVM), список языков из `pack.mcmeta`
- [x] `GameSettings` + `KeyBinding`: все опции, чтение/запись `options.txt` в формате оригинала (кодировка ANSI, как у `FileReader`, `Float.toString` как в Java, JSON списков как у Gson)

### 2.3 Шрифт
- [x] `FontRenderer`: `ascii.png`, ширины глифов по пикселям, тень, `§`-коды цвета/форматирования (жирный, курсив, зачёркнутый, подчёркнутый, `§k`), immediate mode как в оригинале
- [x] Unicode-страницы `unicode_page_XX.png` + `glyph_sizes.bin` (с особенностью оригинала: байты ширин ≥ 0x80 дают «мусорную» ширину)
- [x] `getStringWidth`, `trimStringToWidth`, `listFormattedStringToWidth`, `wrapFormattedStringToWidth`, `drawSplitString` (включая перенос `textColor` между строками, как в оригинале)
- [ ] `bidiReorder` (арабский/иврит через ICU) — вместе с экраном языков

### 2.4 GUI-фреймворк
- [x] `Gui`: `drawRect`, `drawGradientRect`, `drawTexturedModalRect`, `drawCenteredString`, `drawModalRectWithCustomSizedTexture`, `drawScaledCustomSizeModalRect`
- [x] `GuiScreen`: `initGui`, `drawScreen`, `updateScreen`, `actionPerformed`, `keyTyped`, `mouseClicked/Released/ClickMove`, `handleInput`, `onGuiClosed`, `drawDefaultBackground`, буфер обмена; экраны владеются через `shared_ptr` (дочерний экран держит родителя, как GC в Java)
- [x] `GuiButton`, `GuiButtonLanguage`, `GuiErrorScreen`
- [x] `GuiOptionButton`, `GuiOptionSlider`, `GuiSlot`/`GuiListExtended`, `GuiYesNo` — сделаны в этапе 3; `GuiTextField` и `GuiSlider` — в этапе 5, с первыми экранами, которым они нужны
- [x] `Minecraft::displayGuiScreen`, `setIngameFocus`/`setIngameNotInFocus`, ввод через `currentScreen->handleInput()`, `EntityRenderer` (ветка без мира + `setupOverlayRendering`), `MouseHelper`
- [x] Звук: `SoundHandler`/`SoundManager` (`sounds.json`, веса вариантов, категории громкости), вывод через XAudio2 + stb_vorbis; щелчок кнопки `gui.button.press`

### 2.5 Сам `GuiMainMenu`
- [x] Панорама: 6 текстур `panorama_0..5`, 64 прохода во вьюпорт 256×256 оконного буфера, копия `glCopyTexSubImage2D` в `viewportTexture`, 7 проходов `rotateAndBlurSkybox`, растяжение на экран
- [x] Логотип `minecraft.png` из двух половин; пасхалка "Minceraft" с шансом 0.0001
- [x] Сплэш из `texts/splashes.txt` (пропуск фразы с hashCode 125780783), пульсация, поворот −20°, особые даты
- [x] Кнопки: Singleplayer, Multiplayer, Minecraft Realms, Options..., Quit Game, кнопка языка; меню работает на 30 FPS, как в оригинале
- [x] Строки "Minecraft 1.8.9" и "Copyright Mojang AB. Do not distribute!"
- [x] Предупреждение `title.oldgl1/oldgl2` убрано (неприменимо к D3D11); демо-режим и уведомления Realms не переносятся
- [x] Заглушки для неперенесённых экранов — `GuiErrorScreen` с указанием этапа
- [x] Загрузочный экран Mojang перед меню

**Готово, когда:** скриншот меню в C++ совпадает с Java-версией. — **выполнено**: кнопки с надписями и нижние строки совпадают попиксельно (0 отличий; `tools/compare_screens.py` против кадра настоящего клиента из `tools/reference/run_vanilla.ps1`). Фон-панорама анимирован и сплэш случаен, поэтому их сравнить кадр в кадр нельзя; визуально они идентичны.

## Этап 3. Настройки и сопутствующие экраны

- [x] `GuiOptions`, `GuiVideoSettings` (`GuiOptionsRowList`, все пункты оригинала, VBO считаются поддержанными), `GuiControls` + `GuiKeyBindingList` (переназначение, сброс, подсветка конфликтов, 20 мс глушения клавиши), `GuiScreenOptionsSounds`, `GuiLanguage`, `ScreenChatOptions`, `GuiCustomizeSkin`, `GuiSnooper`, `GuiStreamUnavailable` (Twitch мёртв — показывается то же, что ваниль показывает в офлайне)
- [x] Арабский и иврит: `FontRenderer.bidiReorder` через ICU из Windows (`icu.dll`) — те же `ArabicShaping` + `Bidi`, что в ICU4J
- [x] Chat-компоненты (`IChatComponent` + JSON, `ChatStyle`, перевод через `StatCollector`) — нужны для описаний паков; события клика/наведения — с чатом (этап 8)
- [x] `GuiScreenResourcePacks`: `ResourcePackRepository`, zip- и папочные паки (`FileResourcePack`, `FolderResourcePack`), `pack.mcmeta` с описанием-компонентом, иконки, несовместимые паки с подтверждением, перезагрузка ресурсов; `--resourcePackDir`. Из нестрогого JSON Gson поддержаны только комментарии. Серверные паки — с мультиплеером (этап 14)
- [x] `PlayerUsageSnooper` локально, ничего не отправляет (snoop.minecraft.net мёртв): статистика клиента и простаивающего встроенного сервера, как в ванили; вместо ~150 флагов `gl_caps[...]` — сведения об адаптере D3D11
- [x] Музыка: `MusicTicker` (`music.menu` в меню, задержки и рандом как в оригинале), потоковое воспроизведение (`stream: true` в `sounds.json`) — декодирование на фоновом потоке по мере проигрывания; громкость по категориям применяется к играющим звукам (с запаздыванием на шаг для не-master, как в Java)
- [x] Mipmap-уровни — ползунок в настройках видео, сохраняется; сами мипмапы атласа — в этапе 7. Анизотропной фильтрации в 1.8.9 нет (убрана в 1.8)
- [x] Скриншоты по F2 (`ScreenShotHelper`, PNG в `screenshots/`); сообщение в чат — с чатом

**Готово, когда:** все настройки меняются, сохраняются в `options.txt` и переживают перезапуск; ресурспак меняет текстуры меню. — **выполнено**: все экраны настроек совпадают с кадрами настоящего клиента попиксельно, включая список языков с арабским (сценарий `tools/scenarios/options.txt`, отличается только снупер — значениями конкретной машины). Сценарий с ресурспаками (`tools/scenarios/resourcepacks.txt`: выбор несовместимого пака с подтверждением, выбор zip-пака, перезагрузка с новым фоном меню) совпадает попиксельно, `options.txt` после него побайтно равен ванильному.

## Этап 4. Базовые игровые данные

- [x] NBT (`nbt/*`): все теги, чтение/запись, сжатый формат, `JsonToNBT`
- [x] Математика: `BlockPos`, `Vec3`, `AxisAlignedBB`, `EnumFacing`, `MovingObjectPosition`
- [x] Реестры: `Block` (198 блоков), `BlockState`/`IProperty`, `Item` (337 предметов), `ItemStack`, `Bootstrap`
- [x] `Material`, `MapColor`, `SoundType`, `CreativeTabs`
- [x] Дропы блоков (`getItemDropped`, `quantityDroppedWithBonus`, `createStackedBlock`), подпредметы креатива
- [x] Таблицы зелий (`Potion`, `PotionHelper`), имена и цвета яиц `EntityList`

**Сделано:** всё, что не требует мира, сверено построчно с дампами, которые пишет скомпилированный код MCP
(`tools/reference/compile_mcp.ps1` + `generate_game.ps1`). Колбэки предметов и блоков, которым нужен мир, игрок
или сущности, перенесутся на своих этапах — в коде у каждого стоит комментарий с номером этапа.

## Этап 5. Мир и чанки

- [ ] `Chunk`, `ExtendedBlockStorage`, `NibbleArray`, хранение блок-стейтов
- [ ] Формат Anvil: `RegionFile`, `AnvilChunkLoader` — **открываем существующие сейвы 1.8.9**
- [ ] `WorldInfo`/`level.dat`, `SaveFormat`, `GuiSelectWorld`, `GuiCreateWorld` (включая customized-пресеты)
- [ ] Виджеты для них: `GuiTextField` (имя мира, сид), `GuiSlider` и `GuiPageButtonList` (customized)
- [ ] Движок освещения (sky light + block light, `checkLight`)

## Этап 6. Генерация мира (бит в бит с Java)

- [ ] Шумы: `NoiseGeneratorOctaves`, `NoiseGeneratorImproved`, `NoiseGeneratorPerlin`, `NoiseGeneratorSimplex`
- [ ] Биомы: `GenLayer*` (вся цепочка слоёв), `BiomeGenBase` и наследники, `WorldChunkManager`
- [ ] `ChunkProviderGenerate`: рельеф, пещеры, каньоны, озёра, данжи
- [ ] Декораторы: руды, деревья всех типов, цветы, трава, тыквы, сахарный тростник
- [ ] Структуры: деревни, шахты, крепости, храмы пустыни/джунглей, хижины ведьм, подводные храмы
- [ ] Типы мира: Default, Flat, Large Biomes, Amplified, Customized
- [ ] Незер (`ChunkProviderHell`, крепости) и Энд (`ChunkProviderEnd`)

**Готово, когда:** для набора сидов дамп блоков N чанков из C++ совпадает с дампом из Java-версии MCP (скрипт-сверщик в `tools/`).

## Этап 7. Рендер мира

- [ ] Модели блоков: `blockstates/*.json` + `models/block/*.json`, `ModelBakery`, `BlockModelRenderer`, `BlockFluidRenderer`
- [ ] Атлас текстур: `TextureMap`, `Stitcher`, мипмапы, анимированные текстуры (вода, лава, огонь, порталы)
- [ ] Мешинг чанков: `RenderChunk`, слои SOLID / CUTOUT_MIPPED / CUTOUT / TRANSLUCENT, сортировка прозрачных
- [ ] `RenderGlobal`: frustum culling, `VisGraph` (occlusion по связности), дальность прорисовки
- [ ] Небо, солнце/луна/звёзды, облака (fast/fancy), туман, погода (дождь/снег), lightmap, smooth lighting / AO
- [ ] `EntityRenderer`: камера, FOV, view bobbing, рука игрока
- [ ] Многопоточный мешинг (`ChunkRenderDispatcher`) — уже на C++-потоках

## Этап 8. Игрок и взаимодействие

- [ ] `EntityPlayerSP`, движение, прыжки, полёт, плавание, коллизии, приседание, спринт
- [ ] Рейкаст, ломание блоков (с анимацией трещин), установка блоков
- [ ] HUD `GuiIngame`: хотбар, здоровье, голод, броня, опыт, прицел, `GuiNewChat`, оверлей игроков (Tab)
- [ ] Отладочный экран F3 (с заменой строк про OpenGL на D3D11)
- [ ] Меню паузы `GuiIngameMenu`

## Этап 9. Встроенный сервер и одиночная игра

- [ ] `IntegratedServer` в отдельном потоке, `WorldServer`, `PlayerManager`, `ServerConfigurationManager`
- [ ] Локальное соединение клиент ↔ сервер (in-memory аналог `LocalChannel`), те же пакеты, что и по сети
- [ ] Тики блоков: scheduled ticks, random ticks, обновления соседей, жидкости, огонь, рост растений
- [ ] Редстоун: провод, факелы, повторители, компараторы, поршни, раздатчики, воронки
- [ ] Автосохранение, сохранение при выходе

## Этап 10. Сущности и ИИ

- [ ] Модели сущностей (`ModelBase`, `ModelRenderer`, кубы) и рендереры (`Render*`)
- [ ] Все мобы 1.8.9, `EntityAI*`-задачи, `PathFinder`, спаун (`SpawnerAnimals`)
- [ ] Предметы на земле, опыт, снаряды, вагонетки, лодки, падающие блоки, TNT
- [ ] Скины игроков, слои брони, рендер предметов в руке и в GUI (`RenderItem`)

## Этап 11. Инвентарь и крафт

- [ ] `Container`/`Slot`, `GuiContainer`, инвентарь игрока, креативный инвентарь с поиском
- [ ] Верстак, печь, сундуки, зачарование, наковальня, варка, маяк, воронка, раздатчик
- [ ] `CraftingManager`, `FurnaceRecipes`, все рецепты

## Этап 12. Игровые системы

- [ ] Здоровье, голод, урон, броня, зелья и эффекты, зачарования
- [ ] Команды (`command/*`, ~70 команд), автодополнение в чате
- [ ] Достижения и статистика, scoreboard и команды команд
- [ ] Режимы игры, сложность, правила (`GameRules`)
- [ ] Порталы и измерения: Незер, Энд, дракон, иссушитель
- [ ] Торговля с жителями, карты, баннеры, фейерверки

## Этап 13. Звук целиком

- [ ] Позиционный 3D-звук OpenAL, затухание, высота тона, все категории
- [ ] Музыка в игре (по измерению/режиму), пластинки в проигрывателе

## Этап 14. Мультиплеер (протокол 47)

- [ ] Сетевой пайплайн вместо Netty: VarInt-фрейминг, zlib-сжатие, AES/CFB8-шифрование
- [ ] Все пакеты `network/play`, `network/login`, `network/status`, `network/handshake`
- [ ] `GuiMultiplayer`: список серверов (`servers.dat`), пинг, MOTD, иконки, LAN-обнаружение
- [ ] Сначала offline-mode серверы, потом авторизация Microsoft (вход Mojang/Yggdrasil в 1.8.9 уже не работает)
- [ ] Открытие мира для сети (LAN)

**Готово, когда:** клиент заходит на ванильный/Spigot сервер 1.8.9 и играет без рассинхрона.

## Этап 15. Эффекты и полировка

- [ ] Частицы (`EffectRenderer`, все типы), тени сущностей, свечение зачарований
- [ ] Пост-шейдеры (`client/shader`, JSON-описания программ) — переводим GLSL-шейдеры из `assets/minecraft/shaders` в HLSL
- [ ] Super Secret Settings, спектатор-шейдеры
- [ ] Краш-репорты в формате оригинала

## Этап 16. Финальная оптимизация

- [ ] Профилирование (PIX, Superluminal / встроенный `Profiler`)
- [ ] Инстансинг, persistent-буферы, меньше смен состояний
- [ ] Многопоточный тик сервера там, где это не ломает детерминизм
- [ ] Опционально: выделенный сервер (`server/dedicated`) как отдельная цель сборки

---

## Вне проекта

Twitch-стриминг, Realms, Snooper, демо-режим, лаунчер, онлайн-сервисы Mojang (кроме загрузки скинов).

## Проверка паритета (сквозная задача)

- Java-версия MCP запускается рядом как эталон.
- `tools/` — скрипты: попиксельное сравнение скриншотов GUI, сравнение дампов чанков, сравнение последовательностей `Random`.
- Каждый перенесённый класс сопровождается ссылкой на исходный Java-файл в комментарии в начале файла.
