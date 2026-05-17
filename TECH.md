# Smart Alarm: технические требования к реализации

Этот документ дополняет `PLAN.md` и фиксирует именно инженерные решения: архитектуру, слои, контракты между модулями, ограничения реализации и правила, которые должен соблюдать агент при написании кода.

`PLAN.md` остается главным источником функциональных требований. Если в `PLAN.md` описано поведение пользователя, а здесь описана структура кода, нужно выполнить оба документа. Если обнаружен конфликт, не угадывать: сначала привести документы к одному смыслу.

## 1. Стек и сборка

- Язык: C++20.
- UI framework: Qt 6.8.3, Qt Widgets.
- Build system: CMake.
- QML не использовать.
- `.ui` файлы не использовать. Интерфейс создавать вручную в C++.
- Production dependencies: только Qt 6.8.3 и C++ standard library.
- Обязательные Qt modules:
  - `Core`
  - `Gui`
  - `Widgets`
  - `Multimedia`
- Тесты используют `Qt::Test`.
- Сторонние библиотеки и package managers не добавлять.
- JSON реализовать через `QJsonDocument`, `QJsonObject`, `QJsonArray`.
- Не использовать Boost, fmt, spdlog, nlohmann/json.

Базовые CMake-настройки:

```cmake
cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.8.3 REQUIRED COMPONENTS Core Gui Widgets Multimedia Test)
```

Для приложения сделать один executable `SmartAlarm`, но основную реализацию собрать в библиотечный target, например `smart_alarm_lib`, чтобы тесты могли линковаться к той же логике.

## 2. Физическая структура проекта

Не складывать все файлы в корень. Использовать такую структуру:

```text
CMakeLists.txt
src/
  app/
  audio/
  core/
  storage/
  ui/
tests/
  CMakeLists.txt
  core/
  storage/
  audio/
```

Headers и `.cpp` лежат рядом в соответствующем слое:

```text
src/core/notification.h
src/core/schedule.h
src/core/schedule_evaluator.cpp
src/storage/config_store.cpp
src/audio/sound_pattern_parser.cpp
src/ui/main_window.cpp
src/app/app_controller.cpp
```

## 3. Naming convention

Использовать современный Qt-like стиль:

- classes/types/enums: `PascalCase`.
- methods/functions/local variables: `camelCase`.
- private members: `m_` prefix, например `m_notifications`.
- files: `snake_case.h/.cpp`.
- namespace проекта: `smartalarm`.
- Не делать чрезмерно глубокие namespace-ы. Допустимо `smartalarm`, при необходимости `smartalarm::core`, `smartalarm::audio`.
- Constants в `.cpp` держать в anonymous namespace как `constexpr`, например `constexpr int PopupMargin = 16;`.
- QObject signals/slots называть в Qt-стиле: `notificationDue`, `settingsChanged`, `saveRequested`.
- Имена классов, функций, переменных и диагностических сообщений писать на английском.
- Комментарии в коде писать только когда они реально помогают. Если комментарий нужен, он должен быть на русском.

## 4. Архитектурные слои

Код должен быть разделен на слои. Не смешивать бизнес-логику расписаний с виджетами.

### 4.1 `core`

Содержит доменную модель и чистую логику:

- `Notification`
- `GlobalSettings`
- `Schedule`
- `SoundSpec`
- `RuntimeState`
- `ScheduleEvaluator`
- `ScheduleFormatter`
- `NotificationValidator`
- `TimeInputNormalizer`

`core` не должен зависеть от UI. Внутри `core` запрещены:

- `QWidget`
- `QDialog`
- `QMessageBox`
- `QSystemTrayIcon`
- `QAudioSink`

Qt value types использовать можно:

- `QString`
- `QDate`
- `QTime`
- `QDateTime`
- `QUuid`
- `QColor`, если это не усложняет сериализацию

Доменная логика должна быть тестируемой без запуска GUI.

### 4.2 `storage`

Содержит загрузку, repair и сохранение JSON:

- `ConfigStore`
- JSON serializers/deserializers
- backup corrupted/wrong-root config
- atomic save через `QSaveFile`

`storage` не показывает UI. Все пользовательские сообщения об ошибках показывает вызывающий слой.

### 4.3 `audio`

Содержит:

- `SoundPresetRegistry`
- `SoundPatternParser`
- `ToneGenerator`
- `LoopingAudioSource`
- `AudioPlayer`
- `AudioQueue`
- `PreviewPlayer`

Parser и generator должны быть тестируемыми без аудиоустройства. `QAudioSink` изолировать в тонком runtime-классе.

### 4.4 `ui`

Содержит только виджеты, модели и delegates:

- `MainWindow`
- `NotificationTableModel`
- `NotificationActionsDelegate`
- `NotificationEditorDialog`
- `GlobalSettingsDialog`
- `NotificationPopup`
- `PopupManager`
- `TimeEdit`
- `DateEdit`
- `ColorPaletteWidget`
- `DayOfWeekSelector`
- `SlideToDismiss`
- `PositionPickerWidget`
- `TrayController`

UI не пишет JSON напрямую. Все изменения проходят через `AppController`.

### 4.5 `app`

Содержит wiring и lifecycle:

- `AppController`
- `MinuteScheduler`
- startup/shutdown orchestration
- single-instance lock
- application metadata

`AppController` является центральным координатором между model/storage/scheduler/ui/audio.

## 5. Доменная модель

Доменная модель должна быть value-type oriented, без наследования и без QObject.

Рекомендуемые типы:

```cpp
struct GlobalSettings;
struct Notification;
struct RuntimeState;

struct OnceSchedule;
struct WeeklySchedule;
struct NthWeekSchedule;
struct IntervalSchedule;
using Schedule = std::variant<OnceSchedule, WeeklySchedule, NthWeekSchedule, IntervalSchedule>;

struct PresetSound;
struct CustomSound;
using SoundSpec = std::variant<PresetSound, CustomSound>;
```

Использовать:

- `QUuid` для `Notification::id`.
- `std::optional` для optional-полей.
- `enum class` для:
  - `Weekday`
  - `NotificationPosition`
  - `CountFrom`
  - `SoundPreset`
  - `Waveform`
- Не использовать пустые строки как замену optional-значениям.

Для цвета допустимы два варианта:

- хранить `QString "#RRGGBB"` в доменной модели;
- хранить `QColor`, но сериализовать строго в uppercase/lowercase canonical hex.

Главное правило: JSON должен получать валидный `#RRGGBB`, а не произвольное имя цвета.

## 6. Валидация

Валидация не должна жить внутри UI.

Сделать слой:

```cpp
struct ValidationError {
    ValidationErrorCode code;
    QString fieldPath;
};

struct ValidationResult {
    bool ok() const;
    QVector<ValidationError> errors;
};
```

Примеры `fieldPath`:

- `message`
- `playCount`
- `schedule.once.time`
- `schedule.weekly.days`
- `schedule.nthWeek.referenceDate`
- `schedule.interval.schedule.days`
- `sound.custom.pattern`

UI мапит `fieldPath` на конкретный widget и подсвечивает его красным. Тесты должны проверять enum/path, а не английский текст ошибки.

## 7. Persistence contracts

Файл настроек:

- metadata:
  - organization name: `SmartAlarm`
  - application name: `SmartAlarm`
  - application display name: `Smart Alarm`
- config dir: `QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)`
- filename: `settings.json`
- full path: `<AppConfigLocation>/settings.json`
- corrupted/wrong-root backup: `settings.corrupt-YYYYMMDD-HHMMSS.json` в той же директории

Если директории нет, создать через `QDir().mkpath(...)`.

### 7.1 LoadResult

`storage` возвращает структурированный результат:

```cpp
enum class LoadStatus {
    MissingDefaults,
    Loaded,
    CorruptedBackedUp,
    WrongRootBackedUp,
    LoadedWithRepairs
};

struct LoadResult {
    AppConfig config;
    LoadStatus status;
    QString backupPath;
    QStringList warnings;
};
```

`storage` пишет диагностику через `qWarning()` / `qInfo()`, но не показывает `QMessageBox`.

### 7.2 Поведение JSON repair

- Если файла нет: старт с defaults, файл создается при первом изменении.
- Если JSON синтаксически поврежден: сделать backup, стартовать с defaults.
- Если root неверный: сделать backup, заменить конфиг новым default-состоянием.
- Если root валидный, но есть лишние поля: игнорировать их; при следующем сохранении писать canonical JSON только с `_ver`, `settings`, `notifications`.
- Если `_ver` отсутствует или не строка: при repair записать `_ver: "1"`, но не менять логику.
- Если `settings` содержит невалидные поля: чинить по полям defaults-значениями, затем сохранить repaired JSON.
- Если `notifications` содержит сломанную запись: выкинуть всю notification целиком, затем сохранить очищенный JSON.
- Unknown fields внутри notification игнорировать, если все обязательные поля валидны.

### 7.3 Save policy

Все persistent-изменения считаются успешными только после успешного `QSaveFile::commit()`.

Если сохранение не удалось:

- изменение в памяти откатывается;
- UI возвращается в предыдущее состояние;
- показывается `QMessageBox` с ошибкой;
- runtime cleanup не выполняется для операции, которая не была сохранена.

Примеры:

- toggle `enabled` откатывает checkbox при save failure;
- delete не удаляет запись из модели и не закрывает popup при save failure;
- settings dialog не закрывается при save failure;
- editor dialog не применяет изменения при save failure.

## 8. AppController contract

`AppController` координирует все операции, которые затрагивают больше одного слоя.

Он отвечает за:

- добавление notification;
- редактирование notification;
- удаление notification;
- переключение individual `enabled`;
- переключение global runtime toggle;
- сохранение JSON после persistent-изменений;
- отмену pending snooze;
- закрытие popup;
- остановку звука;
- синхронизацию tray menu и main window;
- обработку scheduler due events.

UI не должен напрямую:

- менять JSON;
- удалять pending snooze;
- останавливать audio queue;
- закрывать popup по business rules;
- менять `RuntimeState` в обход controller.

Методы persistent mutations возвращают `OperationResult`:

```cpp
struct OperationResult {
    bool ok;
    QString errorMessage;
};
```

Не использовать exceptions для штатных ошибок валидации, загрузки и сохранения.

## 9. Scheduler и runtime

Разделить pure-логику и таймер:

- `ScheduleEvaluator`: чистые функции, проверяет due-состояние на заданный `now`.
- `MinuteScheduler`: QObject с `QTimer`, синхронизируется на границу минуты и эмитит `notificationDue`.

`MinuteScheduler` не создает popup и не запускает звук напрямую.

### 9.1 Minute timer

Поведение:

- при старте сразу проверить текущую минуту;
- затем поставить single-shot timer до следующей границы минуты, где секунды `00`;
- после каждого tick снова вычислить задержку до следующей границы минуты;
- не полагаться на fixed interval timer без пересинхронизации.

Если приложение стартовало в `10:55:40`, immediate check считается проверкой минуты `10:55`. `Once` на `10:55` должен сработать, если еще не был показан в этой сессии.

Все сравнения due делаются с точностью до минуты. `now` нормализуется до локальной минуты.

### 9.2 Пропущенные срабатывания

Не догонять пропущенные минуты.

Если приложение спало, зависло или было выключено и проснулось в `10:37`, scheduler проверяет только текущую локальную минуту `10:37`.

Timezone/DST/смену часового пояса специально не обрабатывать. MVP работает по текущему локальному времени ОС и не компенсирует переходы времени.

### 9.3 lastTriggeredMinute

Хранить в runtime по `notificationId`, значение - локальная минута `yyyy-MM-dd HH:mm` или нормализованный `QDateTime`.

После перезапуска состояние теряется.

### 9.4 Snooze priority

Порядок проверки для каждой notification:

1. Проверить normal schedule due.
2. Если normal due:
   - отменить pending snooze этой notification;
   - показать normal trigger.
3. Если normal не due, проверить pending snooze.
4. Если snooze due:
   - показать snooze trigger;
   - удалить pending snooze.

Normal trigger побеждает snooze trigger в ту же минуту.

Snooze time считать как `dismiss/snooze click time + minutes`, затем округлять вверх до следующей минутной границы. Пример: `10:55:40 + 1 minute = 10:56:40`, фактическая проверка покажет в `10:57:00`.

### 9.5 Global runtime toggle

- Не сохраняется.
- После перезапуска всегда включен.
- Управляет только будущими показами.
- Не закрывает уже открытые popup.
- Не останавливает уже играющий звук.
- Не удаляет pending snooze.
- Если snooze стал due пока toggle выключен, он остается pending и может показаться после включения, когда scheduler снова выполнит проверку.
- Плановые срабатывания, пропущенные пока toggle выключен, не догоняются.

### 9.6 Individual enabled

Если пользователь выключает конкретную notification (`enabled=false`):

- закрыть уже открытый popup этой notification;
- остановить звук этой notification;
- удалить задачи этой notification из audio queue;
- отменить pending snooze этой notification;
- блокировать будущие плановые срабатывания.

Если notification включают обратно, пропущенные события не догоняются.

### 9.7 Edit/delete runtime semantics

Edit-save существующей notification:

- не закрывает уже открытый popup;
- не меняет текст/цвет/звук уже открытого popup;
- не останавливает уже играющий звук открытого popup;
- отменяет pending snooze;
- будущие срабатывания используют новую версию notification.

Delete после успешного save:

- удалить запись из модели;
- закрыть popup этой notification;
- остановить звук;
- удалить pending snooze;
- удалить ожидающие audio queue задачи.

Delete при save failure ничего runtime-destructive не делает.

## 10. Schedule algorithms

### 10.1 Once

- Due, если `currentLocalDate == date` и `currentLocalTime HH:mm == time`.
- На startup в середине той же минуты событие считается due.
- После срабатывания запись не меняется.
- Через следующую минуту прошедший `Once` считается overdue для display.
- Overdue styling применяется только в таблице.

### 10.2 Nth Week

Алгоритм:

1. Найти понедельник календарной недели `referenceDate`.
2. Найти понедельник календарной недели `candidateDate`.
3. Если `candidateWeekStart < referenceWeekStart`, не due.
4. `weekDiff = referenceWeekStart.daysTo(candidateWeekStart) / 7`.
5. Неделя подходит, если `weekDiff % everyWeeks == 0`.
6. Дата подходит, если `candidateDate.dayOfWeek == selectedWeekday`.
7. В reference-неделе дополнительно: если `candidateDate < referenceDate`, не due.
8. Если `endDate` задан и `candidateDate > endDate`, не due.
9. Время due только при точном совпадении `HH:mm`.

### 10.3 Interval confirmation

Для `Count from = confirmation`:

- `lastDismissedAt` хранится по `notificationId` как локальный `QDateTime`.
- Сначала проверяются optional day/date ограничения.
- Затем проверяется `From..To` текущего дня.
- Если `lastDismissedAt` существует и относится к тому же подходящему дню/окну, следующий due = `lastDismissedAt + everyMinutes`.
- Если `lastDismissedAt` отсутствует, относится к другому дню или вне текущего окна, используется дневная сетка от `From`.
- Если следующий confirmation due выходит за `From..To`, не показывать; на следующем подходящем дне снова использовать сетку от `From`.
- Snooze не обновляет `lastDismissedAt`.

## 11. Audio contracts

Звук должен быть строго неблокирующим. Во время playback UI должен оставаться отзывчивым: popup, dismiss, snooze, settings и exit должны работать без зависаний.

Запрещено:

- `QThread::sleep` для playback;
- busy loop;
- ручной `while playing`;
- вложенный `QEventLoop::exec()` внутри playback;
- генерация бесконечного PCM buffer для `playCount = 0`.

### 11.1 Audio modules

- `SoundPresetRegistry`: id, display name, pattern string.
- `SoundPatternParser`: pure parser pattern string.
- `ToneGenerator`: генерирует PCM buffer для одного pattern.
- `LoopingAudioSource : QIODevice`: читает один pattern buffer циклически.
- `AudioPlayer`: обертка над `QAudioSink`.
- `AudioQueue`: последовательная очередь реальных notification sounds.
- `PreviewPlayer`: отдельный playback для preview.

### 11.2 LoopingAudioSource

`LoopingAudioSource`:

- получает PCM buffer одного pattern;
- для `playCount > 0` отдает buffer `playCount` раз, затем сигнализирует completion;
- для `playCount = 0` зацикливает buffer до `stop`;
- `readData` не блокирует, только копирует доступные bytes;
- finite completion должен переводить `AudioQueue` к следующей задаче.

Preview использует тот же механизм с `playCount = 1`.

### 11.3 Queue rules

- Popup при показе ставит свою звуковую задачу в `AudioQueue`, если `volume > 0`.
- Если `volume == 0`, audio task не создавать и очередь не занимать.
- Звуки разных notification не накладываются, а играют последовательно.
- Если `playCount = 0`, задача занимает очередь до stop.
- Если `playCount > 0`, pattern проигрывается указанное число раз, затем очередь идет дальше, даже если popup еще открыт.
- Закрытие popup любым способом останавливает звук этой notification и удаляет ожидающие задачи этой notification.

### 11.4 Pattern parser limits

Ограничения custom pattern:

- numeric frequency: `20..20000 Hz`;
- note names: `A0..C8`, `#`/`b`, letter case-insensitive;
- duration per segment: `1..10000 ms`;
- max segments: `128`;
- max total pattern duration before repeats: `60000 ms`;
- waveform: `sine`, `square`, `triangle`, `sawtooth`;
- waveform по умолчанию: `sine`;
- пробелы вокруг частей trim-ить;
- пустой segment после лишней запятой считать ошибкой.

### 11.5 Wave generation

Для MVP:

- sample rate: `44100 Hz`;
- mono PCM, формат выбрать тот, который проще и стабильнее подать в `QAudioSink` в Qt 6.8;
- `sine`: `sin(phase)`;
- `square`: знак sine;
- `triangle`: простая normalized triangle wave;
- `sawtooth`: linear phase `[-1, 1]`;
- pause segment генерирует тишину;
- volume `0..100` масштабирует амплитуду линейно;
- на tone segments добавить короткий fade-in/fade-out около `5 ms`, чтобы уменьшить clicks.

## 12. UI implementation contracts

UI создавать вручную в C++, без `.ui`.

Сложные окна не должны иметь огромные конструкторы. Разделять на методы:

- `createTopBar()`
- `createTable()`
- `createScheduleTabs()`
- `createSoundSection()`
- `connectSignals()`
- `loadFromModel()`
- `applyValidationErrors()`

Не создавать универсальный form framework. Использовать обычные Qt widgets и небольшие кастомные компоненты.

### 12.1 Main window

- Close event скрывает окно в tray: `event->ignore(); hide();`.
- При app exit controller ставит флаг `isExiting`, тогда close event принимает закрытие.
- Если окно скрыто, double click по tray показывает его.
- Если окно свернуто, восстановить, затем `raise()` и `activateWindow()`.

### 12.2 Table

Использовать:

- `QTableView`
- `NotificationTableModel : QAbstractTableModel`
- delegates

Не использовать `setIndexWidget` для action buttons.

Колонка `enabled`:

- `Qt::CheckStateRole`;
- `setData` вызывает controller mutation;
- при save failure вернуть прежнее состояние.

Колонка `color`:

- swatch через role/delegate.

Колонка `Actions`:

- `QStyledItemDelegate`;
- рисует две icon-only кнопки;
- mouse events эмитят `editRequested(row)` / `deleteRequested(row)`.

Сортировку и фильтры не реализовывать в MVP. Порядок таблицы равен порядку JSON/model. Новая notification добавляется в конец.

### 12.3 Editor dialog

Диалог редактирования работает на копии `Notification`.

- `Save/OK` применяет изменения только после успешной validation и успешного save.
- `Cancel` и закрытие окна ничего не меняют.
- При save failure диалог остается открытым.
- Все поля и правила из `PLAN.md` реализовать в первом проходе.

Обязательные кастомные виджеты:

- `TimeEdit`
- `DateEdit`
- `DayOfWeekSelector`
- `ColorPaletteWidget`

### 12.4 TimeEdit

`TimeEdit` = `QLineEdit` + clear button `X`.

- Состояние: `std::optional<QTime>`.
- Пустое значение допустимо на уровне widget.
- Required/optional проверяет validator формы.
- Нормализация вынесена в pure `TimeInputNormalizer`.
- На blur нормализовать в `HH:mm`.
- Clear оставляет поле пустым.

### 12.5 DateEdit

`DateEdit` = read-only text field + calendar button + clear button `X`.

- Состояние: `std::optional<QDate>`.
- Показывает `yyyy-MM-dd` или пусто.
- Ручной ввод запрещен.
- Клик по полю/календарю открывает `QCalendarWidget`.
- Если дата пустая, календарь открывается на сегодняшней системной дате.
- Required/optional проверяет validator формы.

### 12.6 Custom sound UI

- В sound combo последним пунктом всегда `Custom pattern...`.
- Если выбран custom, показать pattern input и кнопку `?`.
- `Play` валидирует custom pattern перед playback.
- `Save` валидирует custom pattern обязательно.
- Если пользователь переключился с custom на preset, custom pattern не записывается в `Notification` и JSON.
- Внутри того же открытого диалога можно сохранить набранный custom text в UI-памяти, чтобы пользователь не потерял ввод при переключении туда-сюда.

### 12.7 PopupManager

`PopupManager` хранит активные popup по `QUuid` и порядок показа.

- Для одного `notificationId` максимум один открытый popup.
- При trigger для уже открытого popup новое окно и новый звук не создаются.
- При добавлении/закрытии любого popup вызвать `repositionAll()`.
- Геометрия: `QGuiApplication::primaryScreen()->availableGeometry()`.
- При смене global notification position пересчитать уже открытые popup.

Позиционирование:

- `top_left` / `top_right`: первое окно у верхнего края, следующие ниже.
- `bottom_left` / `bottom_right`: первое окно у нижнего края, следующие выше.
- `center`: считать общую высоту группы и центрировать вертикальную стопку.

### 12.8 Accessibility and keyboard

MVP должен быть не только mouse-only:

- все стандартные поля доступны через Tab;
- `Save/OK` и `Cancel` используют стандартные default/escape semantics;
- clear buttons в `TimeEdit`/`DateEdit` доступны через Tab;
- day buttons toggle через Space/Enter;
- color swatches доступны через Tab и Space/Enter;
- `SlideToDismiss` имеет keyboard fallback: Enter/Space на слайдере выполняет dismiss.

Не добавлять видимый текст-инструкцию про shortcuts в UI.

## 13. Styling contract

Внешний вид должен быть похож на `C:\_\cpp\TextHelper`, но без использования `.ui` файлов.

Использовать только стиль, палитру и компактные параметры.

### 13.1 Global style

В `main` применить Fusion:

```cpp
QStyle *fusionStyle = QStyleFactory::create("Fusion");
if (fusionStyle) {
    app.setStyle(fusionStyle);
}
```

Применить светлую Windows-like палитру:

- `QPalette::Window`: `#f0f0f0`
- `QPalette::WindowText`: `#000000`
- `QPalette::Base`: `#ffffff`
- `QPalette::AlternateBase`: `#f5f5f5`
- `QPalette::ToolTipBase`: `#ffffe1`
- `QPalette::ToolTipText`: `#000000`
- `QPalette::Text`: `#000000`
- `QPalette::Button`: `#f0f0f0`
- `QPalette::ButtonText`: `#000000`
- `QPalette::BrightText`: `#ff0000`
- `QPalette::Highlight`: `#0078d7`
- `QPalette::HighlightedText`: `#ffffff`

Global stylesheet оставить минимальным:

```css
QToolTip { color: #000000; background-color: #ffffe1; border: 1px solid #b5b5b5; }
QMenu::item:selected { background-color: #0078d7; color: #ffffff; }
```

### 13.2 Layout style

- Интерфейс компактный, утилитарный, Windows-like.
- Margins в формах и панелях: обычно `4..6px`.
- Spacing: обычно `4..6px`.
- Не делать landing-page, hero layout, decorative cards.
- Не использовать крупные скругления.
- Border radius: `0..4px`, только там, где это уместно.
- Стандартные Qt widgets оставлять похожими на Fusion/native.
- Точечный QSS использовать только для:
  - validation error border;
  - selected day buttons;
  - color swatches;
  - popup border;
  - slide-to-dismiss;
  - custom floating popup/menu elements.

### 13.3 Popup style

Popup notification должен сохранить требования `PLAN.md`, но визуально может использовать подход floating menu из `TextHelper`:

- white container;
- compact margins;
- optional shadow;
- no rounded card-heavy design;
- blinking color border remains the main visual signal.

## 14. Icons

Для MVP не добавлять внешние icon packs и бинарные ассеты только ради иконок.

Использовать:

- `QStyle::standardIcon(...)`, где подходит;
- custom tray icon, нарисованный через `QPixmap`/`QPainter`;
- disabled tray state = та же иконка с красной диагональной линией.

Позже иконки можно заменить на SVG/resources, но MVP должен быть самодостаточным.

## 15. Startup and shutdown

Startup order:

1. Создать `QApplication`.
2. Установить metadata `SmartAlarm`.
3. Применить Fusion style и palette.
4. Установить `setQuitOnLastWindowClosed(false)`.
5. Проверить single-instance через `QLockFile`.
6. Проверить `QSystemTrayIcon::isSystemTrayAvailable()`.
7. Загрузить config через `ConfigStore`.
8. Создать controller/model/audio/scheduler/ui/tray.
9. Показать tray icon, главное окно не показывать.
10. Запустить scheduler с немедленной проверкой текущей минуты.
11. `app.exec()`.

Если second instance или tray недоступен: показать `QMessageBox` и выйти до создания основного графа объектов.

Shutdown через tray `Exit`:

1. Остановить `MinuteScheduler`.
2. Запретить новые popup/sound triggers.
3. Закрыть editor/settings dialogs.
4. Закрыть все notification popups.
5. Остановить preview player.
6. Очистить `AudioQueue` и остановить `QAudioSink`.
7. Скрыть tray icon.
8. Вызвать `QCoreApplication::quit()`.

Дополнительное сохранение на выходе не делать. Несохраненные изменения в открытых dialog теряются как при Cancel.

## 16. Error handling

- Для штатных ошибок не использовать exceptions.
- Использовать result types.
- UI показывает `QMessageBox` только для пользовательски значимых ошибок:
  - second instance;
  - tray unavailable;
  - save failure;
  - explicit delete confirmation;
  - import/export отсутствуют в MVP, поэтому таких ошибок нет.
- JSON repair не показывает UI-сообщения.
- Диагностика через `qWarning()` / `qInfo()` на английском.

Qt callbacks должны быть exception-safe; не допускать выброс исключений через signal/slot boundary.

## 17. Tests

Добавить test target на `Qt::Test`.

Покрыть в первую очередь logic без UI:

- JSON load/save/repair:
  - missing file;
  - corrupted JSON backup;
  - wrong root backup;
  - invalid settings field repair;
  - broken notification removed;
  - unknown fields ignored;
- `TimeInputNormalizer`;
- `ScheduleEvaluator`:
  - Once;
  - Weekly;
  - NthWeek off-by-one cases;
  - Interval trigger grid;
  - Interval confirmation;
  - date ranges;
  - From > To no-trigger behavior;
- snooze/runtime rules:
  - normal trigger wins over snooze;
  - ceil-to-next-minute;
  - enabled=false cleanup;
  - global toggle blocking;
- `SoundPatternParser`;
- `ToneGenerator` smoke tests without audio device.

UI, tray и popup не обязаны иметь автотесты в MVP. `NotificationTableModel` можно покрыть model tests, если это не раздувает первый проход.

## 18. MVP exclusions

В MVP не реализовывать:

- autostart;
- installer-specific code;
- import/export settings;
- sorting/filtering/search in notification table;
- native/system notifications;
- QML;
- `.ui` files;
- сторонние библиотеки;
- сложную обработку timezone/DST;
- передачу команды первой копии при запуске второй;
- platform-specific branches без явной необходимости.

Не добавлять UI-пункты для будущих возможностей.

## 19. Implementation checklist for agent

Перед началом реализации:

- прочитать `PLAN.md` и `TECH.md`;
- создать CMake/source layout;
- реализовать domain value types;
- реализовать validation и serialization до UI;
- покрыть core/storage/audio parser тестами;
- после этого собирать UI и controller wiring.

Во время реализации:

- не смешивать виджеты и schedule logic;
- все persistent changes проводить через `AppController`;
- проверять save result до runtime cleanup;
- не блокировать GUI thread audio playback;
- держать UI strings на английском;
- не писать лишние комментарии;
- не добавлять неиспользуемый код.

Готовность MVP:

- приложение собирается CMake;
- тесты проходят;
- запускается в tray;
- создает/редактирует/удаляет notifications;
- сохраняет JSON атомарно;
- корректно repair-ит поврежденный config;
- scheduler показывает popup;
- audio queue играет неблокирующе;
- snooze/dismiss/delete/disable выполняют согласованные runtime правила.
