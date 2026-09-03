## RU

## Тема

Фикс-пак теперь работает на любых бинарниках движка

## Изменения

* Раньше одного несовпавшего файла движка хватало, чтобы отключить фикс-пак целиком. Теперь каждая правка отвечает только за себя: исправления скриптов, конфигов, диалогов и все квестовые правки работают на любой сборке 1.0006, а нативные мелочи, которые пишут по фиксированным адресам, пропускают только себя.
* Из-за этого у части игроков не пропадал поток красных ошибок в консоли и оставалось устаревшее предупреждение у ножей — теперь это чинится и у них.
* Каждый запуск пишет `.ild-fixes\runtime\loader-report.txt`: какие файлы проверены, их ожидаемые и настоящие хеши и что именно применилось. Этот файл можно приложить к сообщению о проблеме.
* Добавлены две служебные консольные команды, по умолчанию выключенные: `ild_update diag_spam_on` печатает строку в лог примерно дважды в секунду, чтобы проверить набор текста в консоли под нагрузкой, и `ild_update diag_knife` выдаёт нож.

## Установка

Распакуйте архив в корень игры. Ни один оригинальный файл игры или мода не заменяется, сохранения остаются рабочими. Аддон можно удалить в любой момент по списку `.ild-fixes/managed-files.txt`, и мод продолжит работать как прежде.

---

## EN

## Theme

The fix pack now works on any build of the engine

## Changes

* One unfamiliar engine file used to disable the whole fix pack. Each repair now stands on its own: script, config, dialog and quest repairs work on any 1.0006 build, and the native tweaks that write to fixed addresses skip only themselves.
* That is why some players still saw the flood of red console errors and the obsolete knife warning; both are now repaired for them too.
* Every launch writes `.ild-fixes\runtime\loader-report.txt`: which files were checked, their expected and actual hashes, and what applied. Attach it when reporting a problem.
* Two support console commands were added, both off by default: `ild_update diag_spam_on` prints a log line about twice a second so console typing can be tested under load, and `ild_update diag_knife` gives a knife.

## Installation

Extract the archive into the game root. No original game or mod file is replaced and existing saves keep working. The addon can be removed at any time using `.ild-fixes/managed-files.txt`, and the mod keeps running as before.
