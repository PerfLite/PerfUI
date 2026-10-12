# MapCanvas — виджет карты

`PerfUI::MapCanvas` (`include/PerfUI/MapCanvas.h`, `src/widgets/MapCanvas.cpp`) — круглая карта
с игроком в центре. Виджет не знает про Skyrim: всё, что он рисует, приходит через `IMapDataSource`.
Мод `QuestUI` наполняет этот источник из движка, `PerfUI` остаётся переносимым.

Живой пример с ходьбой по кругу — `examples/map_demo/` (в песочнице клавиша `F9`).

---

## 1. Модель данных

```cpp
enum class MapMarkerKind { Quest, Location, Enemy, Custom };

struct MapMarker {
    std::uint64_t id{ 0 };                 // уникальный в пределах карты
    std::string   label;                   // подпись; пустая — рисуем только дистанцию
    MapMarkerKind kind{ MapMarkerKind::Custom };
    float x{ 0.0f };                       // метры, +X = восток
    float y{ 0.0f };                       // метры, +Y = север
    bool  active{ false };                 // текущая цель выбранного задания
    bool  discovered{ true };              // false: полупрозрачный маркер
    bool  sameSpace{ true };               // false: на карте не рисуется, только бейдж
    Color color{ Color::Transparent() };   // alpha == 0: цвет из темы по kind
};

struct MapPlayerState {
    float x{ 0.0f };
    float y{ 0.0f };
    float headingDeg{ 0.0f };              // 0 = север, растёт по часовой стрелке
};

class IMapDataSource {
public:
    virtual MapPlayerState player() const = 0;
    virtual std::span<const MapMarker> markers() const = 0;
    virtual std::uint64_t revision() const = 0;   // меняется при любом изменении данных
};
```

Цвета по `kind`, если цвет не задан явно: `Quest` — `theme.colors.textAccent`, `Location` —
голубой `{126,200,227}`, `Enemy` — красный `{214,86,78}`, `Custom` — `theme.colors.borderFocus`.

Формы: `Quest` — ромб, `Location` — круг, `Enemy` — треугольник, `Custom` — маленький круг.
Активная цель крупнее, плюс пунктирная линия от игрока и пульсирующее кольцо.

### Готовый источник

`VectorMapDataSource` — для тестов, примеров и одно-поточных сценариев:

```cpp
PerfUI::VectorMapDataSource src;
src.setPlayer(0.0f, 0.0f, 90.0f);          // x, y, курс в градусах
src.setMarkers({ markerA, markerB });      // revision++ сам
map.setDataSource(&src);                   // виджет источнику не владеет
```

### Своё приложение (мод)

Источник живёт в моде, виджет — в ядре. Правило: `markers()` обязан вернуть снимок, валидный
на всё время рендера кадра.

```cpp
class SkyrimMapDataSource final : public PerfUI::IMapDataSource {
public:
    PerfUI::MapPlayerState player() const override { return m_player; }
    std::span<const PerfUI::MapMarker> markers() const override { return m_markers; }
    std::uint64_t revision() const override { return m_revision; }

    // Вызывается с игрового потока, копирует собранные данные в UI-очередь.
    void pushFromGameThread(PerfUI::MapPlayerState p, std::vector<PerfUI::MapMarker> m) {
        PerfUI::Client::RunOnUIThread([this, p, m = std::move(m)]() mutable {
            m_player = p;
            m_markers = std::move(m);
            ++m_revision;
        });
    }

private:
    PerfUI::MapPlayerState m_player;
    std::vector<PerfUI::MapMarker> m_markers;      // только UI-поток трогает
    std::uint64_t m_revision{ 1 };
};
```

Данные собираются на игровом потока в снимок, в дерево UI попадаются только через
`Client::RunOnUIThread` — ядро не читает объекты игры из колбэка рендера.

---

## 2. Система координат и углов

Это единственное место, где легко ошибиться знаком, поэтому — явно.

- **Ось X растёт на восток, ось Y — на север** (не «вниз», как на экране).
- **Экранная Y растёт вниз.** Отсюда все «−» в формулах проекции.
- **`headingDeg`: 0 = север, растёт по часовой стрелке.** 90 = восток, 180 = юг, 270 = запад.
- **Азимут цели** тоже от севера по часовой: `atan2(dx, dy)`, а не привычный `atan2(dy, dx)`.

```
units → meters:  m = units * 0.01428                 (в Skyrim 1 м = 70 юнитов)
bearing(P) [deg] β = deg(atan2(P.x - p.x, P.y - p.y)) → нормализовать в [0, 360)
курс игрока:     h = headingDeg
относительный:   θ = β − h

radar, rotateWithPlayer = true:   screenX = cx + R · sin θ
                                  screenY = cy − R · cos θ
radar, rotateWithPlayer = false:  θ = β (север всегда вверх)
world (этап 2):                   screenX = cx + Δx_м · zoom
                                  screenY = cy − Δy_м · zoom
```

Смысл: при `rotateWithPlayer = true` курс игрока вычитается из азимута, поэтому «стрелка игрока»
всегда смотрит вверх экрана, а карта под ней крутится. При `false` угол не вычитается — север
сверху, маркеры стоят по абсолютным азимутам, игрок-треугольник рисуёт свой курс.

Проверено в тестах (`tests/MapCanvasTest.cpp`, группа 1): цель на севере при курсе 0 — над центром;
при курсе 90° — слева от центра; цель на востоке при курсе 90° — над центром.

---

## 3. Режимы

| Режим | Что это | Статус |
| --- | --- | --- |
| `MapMode::Radar` | игрок в центре, масштаб — радиус радара | готов (этап 1) |
| `MapMode::World` | схематичная карта мира: камера, зум, панорамирование | этап 2 |

До этапа 2 `World` рисуется как радар с форсированным «север вверх» (курс из проекции не вычитается),
то есть переключатель уже работает и меняет картинку, но без камеры и зума.

---

## 4. Настройки

| Метод | По умолчанию | Эффект |
| --- | --- | --- |
| `setDataSource(IMapDataSource*)` | `nullptr` | источник; `nullptr` безопасен — пустой диск |
| `mode(MapMode)` | `Radar` | режим |
| `radarRange(float meters)` | `300` | метр → пиксель; `<= 0` отключает проекцию совсем |
| `rotateWithPlayer(bool)` | `true` | только `Radar`: крутить карту под курсом или держать север вверх |
| `showGrid(bool)` | `false` | четыре луча сторон света |
| `showDistances(bool)` | `true` | подписи колец в метрах и «· 240 м» у маркеров |
| `onMarkerClicked(cb)` | пусто | колбэк с копией маркера; `false` из `onPointerUp`, если мимо |

Размер: `measure()` даёт 160×160, если в `layout()` не заданы Fixed/Percent ширина и высота;
padding и min/max соблюдаются. Геометрия диска — `min(width, height)/2 − 3` px, всё остальное
(толщины, кегли, размеры маркеров) масштабируется коэффициентом `clamp(radius/80, 0.75, 1.75)`.

### Кольца и подписи

Три концентрических кольца на 1/3, 2/3 и 3/3 радиуса. Дальняя цель (`distance > radarRange`)
превращается в стрелку на границе с подписью реальной дистанции в метрах.

Подписи раскладывает жадно: кандидаты — до 64 ближайших маркеров, выводятся до 12, перекрывающиеся
пропускаются. Порог — по прямоугольнику, измеренному через `backend.measureText`.

### Клик

`onPointerUp` ищет ближайший маркер в пределах `14 · scale` px. У дальней цели кликается стрелка
на границе (позиция на радиусе `radius − 8·scale`), а не её реальное «несуществующее» место.
Промахов не касается — возвращает `false`, событие уходит дальше по дереву.

### Цель вне текущего пространства

`sameSpace = false` на карте не рисуется. Если такая цель активна, внизу диска появляется строка
«&lt;имя&gt; — в другом месте» (или «Цель в другом месте» без имени).

---

## 5. Кэш и стоимость кадра

Проекция пересчитывается только когда изменён ключ:

```cpp
struct CacheKey { source ptr, revision, width, height, mode, radarRange,
                  rotateWithPlayer, showGrid; };
```

Совпал ключ — переиспользуется `m_projected`, `recompute()` не вызывается. Счётчик
`recomputeCount()` это наглядно показывает в демо: 100 кадров с неподвижными данными — один пересчёт.
Единственное исключение — пульсация активной цели: она живёт в `update()` (`AnimatedFloat`)
и в параметрах кольца, а не в проекции, поэтому на кэш не влияет.

Ссылки на объекты источника между кадрами виджет не держит: маркеры копируются в `m_snapshot`,
а `Projected` указывает на них индексом. Точность: 1000 маркеров × 100 кадров — ~4.9 мс на кадр
при полном пересчёте (`PerfUI_Test_MapCanvas`, группа 8), в реальной работе пересчёт — на изменение.

Диагностика: `recomputeCount()`, `projectedCount()`.

---

## 6. Тесты

`tests/MapCanvasTest.cpp` на `MockRenderBackend` (без ImGui и D3D), регистрация в CTest и xmake:

```
cmake --build build --config Debug && ctest -C Debug -R MapCanvasTest
xmake f -p windows -m debug --builddir="../.builds/PerfUI" -y && xmake run PerfUI_Test_MapCanvas
```

Покрытие: проекция мир→экран (включая курс 90° и «север вверх»), пропорциональность расстояний,
переход дальней цели в стрелку и обратно, вырожденные входы (пустой источник, `radarRange(0)`,
пустой список, диск 2×2, снятие источника до уничтожения), `sameSpace`, кэш по revision,
обработка клика и бенчмарк.

---

## 7. Ограничения этапа 1

- Нет камеры: зум, панорамирование, геймпад — этап 2 вместе с `MapMode::World`.
- Нет кластеризации близких маркеров и «фокуса на цель по навигации».
- Масштаб плоский: кривизной мира между маркерами пренебрегаем (в пределах десятков километров
  ошибка меньше толщины линии).
- `MapMarker.id` — то, как мод сопоставляет маркеры между кадрами; сам виджет его не интерпретирует.
