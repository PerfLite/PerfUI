# PerfUI Modder Guide (Руководство для разработчиков модов)

[English](MODDER_GUIDE_EN.md) | [Русский](MODDER_GUIDE.md)

Добро пожаловать в **PerfUI** — высокопроизводительный, независимый UI-фреймворк нового поколения для Skyrim Special Edition / Anniversary Edition.

PerfUI создан, чтобы избавить модмейкеров от необходимости писать громоздкие хуки DirectX 11, возиться с сырым ImGui, вручную вычислять пиксельные координаты или внедряться в WndProc.

---

## 1. Архитектурная концепция: «Просто подключи и работай»

### Что требуется игроку (Runtime):
- Единственный файл **`PerfUI.dll`** в папке `Data/SKSE/Plugins/` (устанавливается один раз через MO2 или Vortex как обычный мод).

### Что реально требуется мододелу (Compile-Time / SDK):
1. **Заголовочные файлы:** папка `sdk/include/` (содержит всё публичное API фреймворка).
2. **Библиотека импорта:** `sdk/lib/PerfUI.lib` — статическая библиотека импорта для динамической линковки с `PerfUI.dll`. Каждый мод линкуется с `PerfUI.lib` и пользуется **единым экземпляром фреймворка в памяти процесса**. Никакой статической копии `PerfUI_Core.lib` в мод попадать не должно!
3. **Тулчейн компилятора:** **MSVC v143** (Visual Studio 2022, `_MSC_VER >= 1930`).
4. **C++ Runtime:** Обязательно динамический многопоточный рантайм **`/MD`** (Release). Режим Debug (`/MDd`) имеет несовместимый `_ITERATOR_DEBUG_LEVEL` — ABI-валидатор заблокирует загрузку ради стабильности игры.
5. **Стандарт C++:** **C++20** (`/std:c++20`).
6. Больше никаких внешних зависимостей! Не нужен ни ImGui, ни DirectX 11 SDK, ни Flash/Scaleform.

---

## 2. Быстрый старт (Quick Start)

Чтобы внедрить свой собственный интерфейс (HUD, окно, полоску HP или меню) в Skyrim, достаточно подключить заголовок и запросить контекст:

```cpp
#include "PerfUI/PerfUIApi.h"
#include "PerfUI/Panel.h"
#include "PerfUI/Text.h"
#include "PerfUI/Button.h"

void InitializeMyMod() {
    // 1. Получаем доступ к корневому контексту PerfUI
    auto* ctx = PerfUI::Client::GetContext();
    if (!ctx || !ctx->root()) {
        return; // PerfUI.dll еще не загружен или не установлен
    }

    // 2. Создаем и прикрепляем наш виджет прямо к корневому элементу экрана
    auto* myPanel = ctx->root()->add<PerfUI::Panel>("MyModPanel");

    // 3. Стилизуем панель
    myPanel->backgroundColor(PerfUI::Color(16, 22, 32, 220));
    myPanel->borderColor(PerfUI::Color::NordicGold());
    myPanel->cornerRadius(8.0f);
    myPanel->shadow(true, PerfUI::Color(0, 0, 0, 180), 16.0f);

    // 4. Настраиваем Flexbox-расположение
    myPanel->layout().direction(PerfUI::LayoutDirection::Vertical)
                     .padding(12.0f)
                     .gap(8.0f);

    // 5. Добавляем заголовок и кнопку
    auto* title = myPanel->add<PerfUI::Text>("Мой первый мод на PerfUI!");
    title->color(PerfUI::Color::TextPrimary()).bold(true);

    auto* btn = myPanel->add<PerfUI::Button>("Нажми меня");
    btn->onClick([]() {
        // Вызов звука Skyrim и всплывающего уведомления в 1 строку!
        PerfUI::Client::PlaySound("UIMenuOK");
        PerfUI::Client::ShowToast("Успех!", "Кнопка нажата!", PerfUI::ToastType::Success);
    });
}
```

---

## 3. Базовые компоненты (Core Widgets)

PerfUI предоставляет полный набор готовых компонентов:

| Виджет | Назначение | Пример использования |
| :--- | :--- | :--- |
| **`Panel`** | Фоновый контейнер со скруглением, рамкой и тенью | `auto* p = add<Panel>();` |
| **`Text`** | Векторная типографика (размер, жирность, автоперенос) | `auto* t = add<Text>("Текст");` |
| **`Button`** | Интерактивная кнопка со звуком и плавным свечением | `auto* b = add<Button>("Клик");` |
| **`ProgressBar`**| Анимированная полоса прогресса (HP, магия, опыт) | `auto* bar = add<ProgressBar>(0.75f);` |
| **`Slider`** | Ползунок числовых значений с перетаскиванием | `auto* s = add<Slider>(50.0f, 0.0f, 100.0f);` |
| **`Checkbox`** | Чекбокс-переключатель с плавной анимацией точки | `auto* c = add<Checkbox>("Включить");` |
| **`ComboBox`** | Выпадающий список с автопозиционированием и выбором | `auto* cb = add<ComboBox>(options);` |
| **`TextInput`** | Поле ввода текста с курсором и фокусом ввода | `auto* in = add<TextInput>("Поиск...");` |
| **`ScrollView`** | Прокручиваемый список с аппаратной полосой прокрутки | `auto* sv = add<ScrollView>();` |
| **`Image`** | Отрисовка текстур DX11 с сохранением пропорций | `auto* img = add<Image>(srv, w, h);` |

---

## 4. Система верстки (Flexbox Box Model)

Забудьте о высчитывании абсолютных пикселей для каждого элемента! Движок PerfUI автоматически рассчитывает геометрию:

- **Направление стека:**
  - `layout().direction(LayoutDirection::Horizontal)` — элементы выстраиваются в ряд.
  - `layout().direction(LayoutDirection::Vertical)` — элементы выстраиваются в колонку.
- **Отступы:**
  - `layout().padding(12.0f, 8.0f)` — внутренние отступы (horizontal, vertical).
  - `layout().margin(0.0f, 10.0f)` — внешние отступы.
  - `layout().gap(6.0f)` — интервал между соседними дочерними элементами.
- **Выравнивание:**
  - `layout().alignment(Alignment::Center)` — центрирование по поперечной оси.
  - `layout().justify(JustifyContent::SpaceBetween)` — распределение свободного места.
- **Размеры:**
  - `layout().width(180.0f)` — фиксированная ширина.
  - `layout().width(DimensionConstraint::Flex(1.0f))` — гибкая ширина, заполняющая оставшееся место.

---

## 5. Анимации и жизненный цикл (`update`)

Для анимации любых параметров используйте встроенный класс `AnimatedFloat`:

```cpp
class MyAnimatedCard : public PerfUI::Panel {
public:
    MyAnimatedCard() {
        m_scale.snapTo(0.0f);
        m_scale.setTarget(1.0f); // Плавное появление
    }

    void update(float deltaTime) override {
        Panel::update(deltaTime);
        m_scale.update(deltaTime); // Автоматический расчет по физической кривой
    }

private:
    PerfUI::AnimatedFloat m_scale{ 0.0f, 14.0f }; // (начальное значение, жесткость)
};
```

---

## 6. Звуки и Уведомления (Audio & Toast API)

Любой мод может вызывать звуковые эффекты движка Skyrim по их EditorID и отправлять элегантные всплывающие уведомления:

```cpp
// Звуки Skyrim
PerfUI::Client::PlaySound("UIMenuBlade");      // Звук переключения вкладок
PerfUI::Client::PlaySound("UIMenuOK");         // Подтверждение
PerfUI::Client::PlaySound("UIMenuCancel");     // Отмена / закрытие
PerfUI::Client::PlaySound("UIQuestUpdate");    // Обновление квеста

// Всплывающие тосты
PerfUI::Client::ShowToast("Заголовок", "Сообщение", PerfUI::ToastType::Info, 3.0f);
PerfUI::Client::ShowToast("Урон получен", "Здоровье критически мало!", PerfUI::ToastType::Warning, 2.5f);
```

---

## 7. Полный пример: Кастомный HUD здоровья игрока

Исходный код готового примера лежит в папке [`examples/modder_custom_hud/CustomHealthBar.h`](../examples/modder_custom_hud/CustomHealthBar.h):

```cpp
#include "PerfUI/Panel.h"
#include "PerfUI/ProgressBar.h"
#include "PerfUI/Text.h"
#include "PerfUI/PerfUIApi.h"

class CustomHealthBar : public PerfUI::Panel {
public:
    CustomHealthBar() : Panel("CustomHealthBar") {
        backgroundColor(PerfUI::Color(14, 18, 26, 215));
        borderColor(PerfUI::Color::BorderSubtle());
        borderWidth(1.2f);
        cornerRadius(8.0f);
        shadow(true, PerfUI::Color(0, 0, 0, 180), 16.0f, { 0.0f, 4.0f });

        layout().direction(PerfUI::LayoutDirection::Horizontal)
                .alignment(PerfUI::Alignment::Center)
                .padding(12.0f, 6.0f)
                .gap(10.0f);

        m_label = add<PerfUI::Text>("HP");
        m_label->color(PerfUI::Color(255, 80, 80, 255)).fontSize(12.0f).bold(true);

        m_bar = add<PerfUI::ProgressBar>(1.0f);
        m_bar->layout().width(180.0f).height(12.0f);
        m_bar->fillColor(PerfUI::Color(210, 40, 40, 255));
        m_bar->trackColor(PerfUI::Color(30, 16, 16, 220));

        m_valText = add<PerfUI::Text>("100 / 100");
        m_valText->color(PerfUI::Color::TextSecondary()).fontSize(11.0f);
    }

    void setHealth(float current, float max) {
        float ratio = std::clamp(current / max, 0.0f, 1.0f);
        m_bar->progress(ratio);

        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f", current, max);
        m_valText->text(buf);
    }

private:
    PerfUI::Text* m_label{ nullptr };
    PerfUI::ProgressBar* m_bar{ nullptr };
    PerfUI::Text* m_valText{ nullptr };
};
```

---

## 8. Правила потоков (Threading Model)

Движок PerfUI выполняет расчет макета, обновление таймеров (`update`) и отрисовку (`render`) строго в **UI-потоке** (D3D11 Present Hook).

- **Главное правило:** Никогда не модифицируйте элементы интерфейса из произвольных фоновых потоков (события Papyrus, асинхронные SKSE-потоки, таймеры или рабочие воркеры).
- Для безопасной передачи команд в интерфейс используйте функцию:
  ```cpp
  PerfUI::Client::RunOnUIThread([=]() {
      // Этот лямбда-блок гарантированно выполнится в UI-потоке
      // в самом начале UIContext::update перед отрисовкой следующего кадра!
      if (g_customHud) {
          g_customHud->setHealth(newHp, maxHp);
      }
  });
  ```
- **Защита в Debug:** В Debug-сборке срабатывает внутренний `assertUIThread()`. Если какой-либо компонент попытается вызваться из стороннего потока без `RunOnUIThread`, игра сразу остановится в отладчике, указывая точное место ошибки.

---

## 9. Частые проблемы (Troubleshooting / FAQ)

### Ошибка `LNK2019 / LNK2001: ссылка на неразрешенный внешний символ UIElement::... Panel::...`
- **Причина:** В настройках сборки мода (`CMakeLists.txt` или свойствах проекта Visual Studio) забыли подключить библиотеку импорта `PerfUI.lib`.
- **Решение:** Добавьте путь к `sdk/lib/` и линкуйте `PerfUI.lib`:
  ```cmake
  find_library(PERFUI_IMPORT_LIB NAMES PerfUI PATHS "$ENV{PERFUI_SDK}/lib")
  target_link_libraries(MyMod PRIVATE ${PERFUI_IMPORT_LIB})
  ```
  *(Не линкуйте `PerfUI_Core.lib` — каждый плагин должен использовать общую DLL!)*

### `GetApi()` или `GetContext()` возвращают `nullptr` (или в логе `_ITERATOR_DEBUG_LEVEL mismatch`)
- **Причина:** Мод собран в конфигурации Debug с флагом `/MDd` (`_ITERATOR_DEBUG_LEVEL == 2`), тогда как установленный `PerfUI.dll` собран в Release (`_ITERATOR_DEBUG_LEVEL == 0`). В MSVC STL структуры `std::string` и `std::function` имеют совершенно разный размер и лейаут в памяти между Debug и Release. Чтобы предотвратить мгновенный вылет игры, PerfUI ABI-валидатор отклоняет вызов API.
- **Решение:** Собирайте релизную версию вашего мода с флагом `/MD` (Release).

### В логе: `MSVC toolchain version mismatch`
- **Причина:** Версия компилятора Visual Studio ниже v143 (VS 2022, `_MSC_VER < 1930`).
- **Решение:** Соберите мод с помощью инструментария Visual Studio 2022 (MSVC v143).

### Падения или артефакты при изменении UI из SKSE Message Listener / Event Sink
- **Причина:** Модификация дерева UI происходит из потока игрового цикла Skyrim в момент, когда UI-поток отрисовывает кадр в DirectX 11.
- **Решение:** Оберните вызов в `PerfUI::Client::RunOnUIThread([=]() { ... })`.

---

## 10. Проверка в Sandbox

Для интерактивного тестирования функционала для мододелов запустите `PerfUI_Sandbox.exe`:
- **`[F8]`** — Показать / скрыть тестовый `CustomHealthBar` мододела.
- **`[J]`** — Нанести 15 урона (проверка анимации уменьшения HP и критического пульса).
- **`[H]`** — Восстановить 15 HP.
- **`[F10]`** — Открыть полноэкранное Главное меню.
- **`[F11]`** — Открыть Дневник квестов.

---

## 11. Архитектура «Чистого клиента» (Clean Client Model)

В версии PerfUI 1.0 реализована модель **«Чистого клиента»**, позволяющая создавать моды на HUD, оверлеи и интерактивные меню **без единого собственного хука DirectX 11 или WndProc**.

### Преимущества модели «Чистого клиента»:
1. **Единый владелец хуков:** В игре остаётся строго один хук `Present`, один `ResizeBuffers` и один `WndProc` (в `PerfUI.dll`). Это исключает конфликты хуков, падения при ресайзе буферов или смене полноэкранного режима.
2. **Нулевой ImGui в коде мода:** Мод не компилирует ImGui, не создаёт второй `ImGui::Context` и не тратит лишнюю память.
3. **Безопасная задержка загрузки:** С флагом `/DELAYLOAD:PerfUI.dll` мод никогда не упадёт при старте, если `PerfUI.dll` временно отсутствует — он сможет корректно отключить свой UI и уведомить пользователя в логе.

### 11.1 Регистрация оверлеев (HUD, компас, индикаторы)

Для рисования элементов HUD поверх игры используйте `PerfUI::Client::RegisterOverlay`:

```cpp
#include <PerfUI/PerfUIApi.h>

// Регистрируем оверлей: имя, колбэк, z-порядок, флаг alwaysVisible
auto overlayId = PerfUI::Client::RegisterOverlay(
    "MyCustomCompass",
    [](PerfUI::OverlayContext& ctx) {
        // ctx.renderer предоставляет чистый UIRenderBackend:
        // drawLine, drawCircle, drawRect, drawImage, drawText и др.
        float cx = ctx.screenSize.width * 0.5f;
        float cy = 30.0f;
        ctx.renderer.drawCircle(PerfUI::Point(cx, cy), 16.0f, PerfUI::Color(255, 215, 0, 200));
        ctx.renderer.drawText("N", PerfUI::Point(cx - 5.0f, cy - 8.0f), { 16.0f, PerfUI::Color::White() });
    },
    /*zOrder=*/10,
    /*alwaysVisible=*/true
);
```

### 11.2 Горячие клавиши и блокировка ввода

```cpp
// 1. Регистрация глобальной горячей клавиши (по умолчанию F7, код 0x76)
PerfUI::Client::RegisterHotkey("MyMod_Toggle", 0x76, []() {
    SKSE::log::info("F7 pressed!");
});

// 2. Блокировка ввода игры при открытии полноэкранного меню мода
// При true: курсор мыши синхронизируется с аппаратным стрелочным курсором Windows,
// а игровой ввод (камера, кнопки) блокируется через kStop в ProcessEvent.
PerfUI::Client::CaptureInput(true);
```

### 11.3 Текстуры и спрайты без прямого DirectX 11

```cpp
// Загрузка текстуры (PNG, JPG, BMP) через PerfUI:
PerfUI::TextureId myTex = PerfUI::Client::LoadTexture("Data/Textures/MyMod/icon.png");

// Отрисовка в оверлее:
ctx.renderer.drawImage(myTex, PerfUI::Rect(100.0f, 100.0f, 48.0f, 48.0f), PerfUI::Color::White());

// Выгрузка текстуры при завершении работы:
PerfUI::Client::DestroyTexture(myTex);
```

### 11.4 Безопасная предзагрузка PerfUI.dll по полному пути

```cpp
bool PreloadPerfUI() {
    HMODULE hSelf = nullptr;
    if (::GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&PreloadPerfUI),
            &hSelf)) {
        wchar_t selfPath[MAX_PATH];
        DWORD len = ::GetModuleFileNameW(hSelf, selfPath, MAX_PATH);
        if (len > 0 && len < MAX_PATH) {
            std::filesystem::path p(selfPath);
            std::filesystem::path perfUIPath = p.parent_path() / L"PerfUI.dll";
            if (std::filesystem::exists(perfUIPath)) {
                HMODULE hPerf = ::LoadLibraryExW(perfUIPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
                if (hPerf) return true;
            }
        }
    }
    return ::GetModuleHandleW(L"PerfUI.dll") != nullptr;
}
```

