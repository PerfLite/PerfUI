# Skyrim UI Input & Cursor Integration Guide (PerfUI)

[English](SKYRIM_INPUT_GUIDE_EN.md) | [Русский](SKYRIM_INPUT_GUIDE.md)

Этот документ описывает критические правила и архитектурные решения для работы с вводом, мышью и курсором в кастомных DirectX 11 UI-модах для The Elder Scrolls V: Skyrim Special Edition (SKSE / CommonLibSSE).

---

## 1. Архитектура ввода в Skyrim SE: DirectInput vs Windows Messages

### Проблема:
В обычном приложении Windows события мыши приходят в функцию окна (`WndProc`) в виде сообщений `WM_LBUTTONDOWN`, `WM_LBUTTONUP` и `WM_MOUSEMOVE`.
В Skyrim SE движок опрашивает устройства мыши напрямую через **DirectInput8** (`RE::BSWin32MouseDevice`), поэтому:
- Сообщения `WM_LBUTTONDOWN` и `WM_LBUTTONUP` **НЕ генерируются** для окна игры при кликах.
- Координаты в сообщениях `WM_MOUSEMOVE` могут отсутствовать или не обновляться, так как мышь находится в exclusive/raw режиме.

### Правильное решение:
1. **Клики мыши:** Перехватывать через `RE::BSTEventSink<RE::InputEvent*>`:
   ```cpp
   if (event->GetDevice() == RE::INPUT_DEVICE::kMouse &&
       event->GetEventType() == RE::INPUT_EVENT_TYPE::kButton)
   {
       auto* btn = event->AsButtonEvent();
       uint32_t code = btn->GetIDCode(); // 0 = Left, 1 = Right, 2 = Middle, 8 = WheelUp, 9 = WheelDown
       if (btn->IsDown()) { /* клик нажат */ }
       else if (btn->IsUp()) { /* клик отпущен */ }
   }
   ```
2. **Координаты мыши:** Опрашивать каждый кадр рендера через Win32 API:
   ```cpp
   POINT pt;
   if (::GetCursorPos(&pt) && ::ScreenToClient(hWnd, &pt)) {
       Point mousePos{ static_cast<float>(pt.x), static_cast<float>(pt.y) };
       uiContext->onMouseMove(mousePos);
   }
   ```

---

## 2. Проблема курсора (Двойной курсор vs Исчезновение курсора)

### Архитектура отображения курсора в Главном Меню:
В оригинальном главном меню Скайрима драконий курсор рисуется внутри Flash-ролика `mainmenu.swf`.
Когда мы полностью скрываем ванильный интерфейс (`uiMovie->SetVisible(false)`), отрисовка дракончика во Flash отключается вместе со всеми ванильными кнопками!

Поэтому единственным и правильным курсором для кастомного DirectX 11 главного меню является **аппаратный курсор Windows** (`IDC_ARROW`).

---

### Критический баг: Пропадание курсора при клике мыши (ShowCursor Display Counter Drop)
> [!CAUTION]
> **Симптом:** Меню открывается, курсор виден и двигается. Но как только игрок делает **первый клик** мыши — курсор визуально исчезает навсегда, хотя кнопки продолжают подсвечиваться при наведении и нажиматься!
>
> **Причина:**
> 1. В Win32 API функция `SetCursor(hCursor)` отображает курсор **только если внутренний счетчик видимости `ShowCursor` равен или больше нуля (`>= 0`)**. Если счетчик равен `-1` — курсор аппаратно скрыт, сколько бы раз ни вызывался `SetCursor`.
> 2. Когда игрок нажимает кнопку мыши, DirectInput и внутренний обработчик устройств Скайрима (`BSWin32MouseDevice`) перехватывают клик и вызывают системный `ShowCursor(FALSE)`, уменьшая счетчик с `0` до `-1`.
> 3. Если вызывать `while (::ShowCursor(TRUE) < 0)` только один раз при открытии меню — после первого же клика курсор станет невидимым навсегда.

### Железное правило решения (The Ironclad Fix):
Поднимать счетчик `ShowCursor` обратно в `0` **на каждом кадре рендера и в каждом обработчике клика**:
1. **На каждом кадре DirectX 11 Present (`D3D11Hook::Hooked_Present`)**:
   ```cpp
   // Даже если Скайрим сбросил счетчик на клике, в ближайшие 16 мс он будет восстановлен
   while (::ShowCursor(TRUE) < 0);
   ::SetCursor(::LoadCursorA(nullptr, IDC_ARROW));
   ```
2. **В обработчике DirectInput кликов (`InputHook::ProcessEvent`)**:
   ```cpp
   if (keyCode <= 2) {
       while (::ShowCursor(TRUE) < 0);
       ::SetCursor(::LoadCursorA(nullptr, IDC_ARROW));
       // ... обработка клика ...
   }
   ```
3. **В оконных сообщениях мыши (`WndProc` / `WM_SETCURSOR` / `WM_LBUTTONDOWN`)**:
   ```cpp
   if (msg == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT) {
       while (::ShowCursor(TRUE) < 0);
       ::SetCursor(::LoadCursorA(nullptr, IDC_ARROW));
       return TRUE;
   }
   ```
4. **Условие `while (... < 0)` критически важно**:
   Оно гарантирует, что счетчик никогда не превысит `0`. Счетчик остановится ровно на `0` (порог видимости) и никогда не раздуется до огромных положительных чисел.
5. **Отключение софтверного курсора ImGui**:
   ```cpp
   ImGui::GetIO().MouseDrawCursor = false;
   ```
Это навсегда гарантирует: 0 задержки, отсутствие раздвоения и 100% стабильную видимость курсора при любых кликах и движениях!

---

## 3. Захват мыши ImGui (`SetCapture` / `ReleaseCapture`)

### Причина потери кликов:
Стандартная реализация бэкенда `ImGui_ImplWin32_WndProcHandler`:
- При клике левой кнопкой мыши вызывает `::SetCapture(hWnd)`.
- При отпускании вызывает `::ReleaseCapture()`.

В Skyrim это конфликтует с внутренним опросом `BSInputDeviceManager`:
- Генерируются сообщения `WM_CAPTURECHANGED`.
- Сообщения `WM_LBUTTONUP` теряются.
- Кнопка интерфейса «залипает» в нажатом состоянии, а событие клика (`onClick`) никогда не срабатывает.

### Правильное решение:
**Не пропускать события мыши через `ImGui_ImplWin32_WndProcHandler`.**
ImGui в плагинах Скайрима должен использоваться **исключительно как бэкенд рендеринга** геометрии, а весь ввод должен идти через чистый хук `PerfUI`.

---

## 4. Потокобезопасность (Thread-Safe Event Queue)

### Причина случайных вылетов (Race Conditions):
- `BSInputDeviceManager::ProcessEvent` вызывается из **главного потока логики игры / потока ввода**.
- `IDXGISwapChain::Present` вызывается из **потока рендеринга DirectX 11**.

Если из `ProcessEvent` напрямую вызывать методы интерфейса (`onMouseDown`, `onMouseUp`, переключение панелей `switchRightPanel`), то два потока одновременно модифицируют и читают дерево виджетов `UIElement`. В 95% случаев это работает, но в 5% случаев приводит к чтению невалидных указателей и вылету на рабочий стол (CTD).

### Правильное решение:
Использовать потокобезопасную очередь:
1. `ProcessEvent` только складывает события в очередь под мьютексом:
   ```cpp
   PushInput({ QueuedInput::Type::MouseDown, keyCode, mouseX, mouseY });
   ```
2. В начале кадра `Hooked_Present` (в потоке D3D11) очередь выкачивается:
   ```cpp
   auto events = InputHook::GetSingleton().DrainInputQueue();
   for (const auto& ev : events) {
       uiContext->onMouseDown(...);
   }
   uiContext->update(deltaTime);
   uiContext->render(backend);
   ```
Все манипуляции с деревом виджетов происходят строго в одном потоке.

---

## 5. Загрузка сохранений в Skyrim SE

### Особенности `BGSSaveLoadManager`:
- Функция `LoadMostRecentSaveGame()` ищет сейв во внутреннем массиве `saveManager->saveGameList`.
- Внутренний массив `saveGameList` **пуст** до тех пор, пока не будет открыто ванильное меню сохранений Scaleform.
- Если вызвать `LoadMostRecentSaveGame()` при пустом списке, функция молча вернёт `false` и ничего не загрузит.
- **Правильный подход:** Найти имя реального последнего файла `.ess` на диске и вызывать `saveManager->Load(fileName.c_str(), false)`.
- **Никогда не вызывать две команды загрузки одновременно:** Вызов `Load()` и параллельный вызов консольной команды `load` приводит к крашу движка на экране загрузки.

---

## 6. Критический вылет при хуке WndProc: ANSI vs Unicode (`0xFFFFxxxx` Pseudo-Handles)

### Симптом:
Вылет с ошибкой `Unhandled exception EXCEPTION_ACCESS_VIOLATION at 0x0000FFFF03D3` в сторонних модах (например, `SKSEMenuFramework.dll+00E6DA0`).

### Причина:
1. Окно Skyrim SE создаётся как ANSI (`CreateWindowExA`).
2. Если плагин вызывает `SetWindowLongPtrW` (Unicode) на ANSI-окне, подсистема `USER32.dll` возвращает **не настоящий указатель на функцию**, а псевдо-хэндл трансляции Unicode->ANSI вида `0xFFFFxxxx` (например, `0xFFFF03D3`).
3. Другие моды (SKSEMenuFramework, TrueHUD и др.) перехватывают оконную процедуру и вызывают полученный адрес напрямую как указатель `func(...)`. Инструкция `jmp rax` падает на адресе `0x0000FFFFxxxx`.

### Железные правила:
1. **Всегда использовать ANSI-версии Windows API:**
   ```cpp
   // ПРАВИЛЬНО:
   m_originalWndProc = reinterpret_cast<WNDPROC>(
       ::SetWindowLongPtrA(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Hooked_WndProc))
   );
   
   // Внутри Hooked_WndProc:
   return CallWindowProcA(hook.m_originalWndProc, hWnd, msg, wParam, lParam);
   ```
2. **Не устанавливать хук WndProc слишком рано:**
   - Графический хук D3D11 можно ставить рано (в `BSGraphics::InitD3D` для кадра 0).
   - Хук `WndProc` и `BSInputDeviceManager` следует подключать **строго на `kDataLoaded`**, когда окно и сторонние моды уже стабильны.

---

## 7. Бесшовный экран загрузки при клике «Продолжить» (Zero-Frame Dragon Flash)

### Проблема:
При клике по кнопке в Главном меню на 1 секунду мелькал ванильный 3D-дракон, пока движок считывал сохранение с диска.

### Причина:
Клик обрабатывается в середине кадра D3D11 `Hooked_Present`. Если Главное меню скрывается (`m_isVisible = false`), а проверка экрана загрузки уже прошла в начале кадра, то в текущем кадре ничего не рисуется. Движок успевает вывести свой бэкбуфер с драконом на экран и мгновенно блокирует основной поток функцией чтения с диска `saveManager->Load(...)`.

### Решение:
В `Hooked_Present` сразу после цикла разбора очереди ввода проверять флаг `IsLoadingMenuOpen()`:
```cpp
// Если в процессе обработки ввода было инициировано действие (Continue, Load, New Game),
// мы обязаны немедленно в ЭТОМ ЖЕ КАДРЕ отрисовать чёрный экран загрузки:
if (MainMenuManager::GetSingleton().IsLoadingMenuOpen()) {
    renderLoadingScreen();
} else {
    uiContext->update(deltaTime);
    uiContext->render(backend);
}
```
Кадр перед блокировкой гарантированно уходит на экран залитым глубоким чёрным цветом со спинером.

