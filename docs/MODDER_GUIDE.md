# PerfUI Modder Guide (Руководство для разработчиков модов)

Добро пожаловать в **PerfUI** — высокопроизводительный, независимый UI-фреймворк нового поколения для Skyrim Special Edition / Anniversary Edition.

PerfUI создан, чтобы избавить модмейкеров от необходимости писать громоздкие хуки DirectX 11, возиться с сырым ImGui, вручную вычислять пиксельные координаты или внедряться в WndProc.

---

## 1. Архитектурная концепция: «Просто подключи и работай»

### Что требуется игроку (Runtime):
- Единственный файл **`PerfUI.dll`** в папке `Data/SKSE/Plugins/` (устанавливается один раз через MO2 или Vortex как обычный мод).

### Что требуется мододелу (Compile-Time):
- Папка заголовочных файлов `include/` (в частности [`PerfUI/PerfUIApi.h`](../include/PerfUI/PerfUIApi.h)).
- Больше никаких зависимостей! Не нужен ни ImGui, ни DirectX 11 SDK, ни Flash/Scaleform.

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

## 8. Проверка в Sandbox

Для интерактивного тестирования функционала для мододелов запустите `PerfUI_Sandbox.exe`:
- **`[F8]`** — Показать / скрыть тестовый `CustomHealthBar` мододела.
- **`[J]`** — Нанести 15 урона (проверка анимации уменьшения HP и критического пульса).
- **`[H]`** — Восстановить 15 HP.
- **`[F10]`** — Открыть полноэкранное Главное меню.
- **`[F11]`** — Открыть Дневник квестов.
