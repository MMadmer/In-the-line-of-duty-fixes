## RU

## Тема

Свои настройки экрана и радио, и NPC, которые больше не застревают

## Изменения

* NPC, застрявшие на квестовой задаче, теперь распознаются и возвращаются к делу сами. Тот, кто прошёл пару шагов и встал в проходе, или дошёл до точки, уселся и больше не двигается, раньше стоял так до перезагрузки сохранения — теперь этого ждать не нужно. Задания, которые ждут сигнала от игрока или инфопоршня, не затрагиваются.
* Во вкладке «Звук» появилась громкость радио: она приглушает только мировые радио и музыку, применяется сразу, а на нуле даёт полную тишину.
* Во вкладке «Видео» вместо галочки полноэкранного режима теперь выбор режима экрана: полноэкранный, полноэкранный в окне и оконный. В оконных режимах игра больше не держит экран монопольно — работают Alt+Tab, клавиша Windows и скриншоты, а у оконного есть обычная рамка окна.
* Раньше одного несовпавшего файла движка хватало, чтобы отключить фикс-пак целиком: у части игроков из-за этого не пропадал поток красных ошибок в консоли и оставалось устаревшее предупреждение у ножей. Теперь каждая правка отвечает только за себя и работает на любой сборке 1.0006.
* Каждый запуск пишет `.ild-fixes\runtime\loader-report.txt` — что проверено и что применилось, а вмешательства в застрявших NPC попадают в `.ild-fixes\runtime\npc-watchdog.txt`. Эти файлы можно приложить к сообщению о проблеме.

## Установка

Распакуйте архив в корень игры. Ни один оригинальный файл игры или мода не заменяется, сохранения остаются рабочими. Аддон можно удалить в любой момент по списку `.ild-fixes/managed-files.txt`, и мод продолжит работать как прежде.

---

## EN

## Theme

Screen and radio settings of its own, and NPCs that no longer get stuck

## Changes

* NPCs stalled on a quest task are now detected and put back to work on their own. The one that takes a few steps and stops in a doorway, or reaches its point, sits down and never moves again, used to stay that way until the save was reloaded. Sections that are meant to wait for the player or for an info portion are left alone.
* The Sound tab has a radio volume: it scales world radio and music only, applies as soon as it is picked, and gives complete silence at zero.
* The Video tab now offers a screen mode instead of the fullscreen checkbox: fullscreen, borderless window and window. A windowed mode no longer holds the display exclusively — Alt+Tab, the Windows key and screenshots all work, and the windowed mode has an ordinary window frame.
* One unfamiliar engine file used to disable the whole fix pack, which is why some players still saw the flood of red console errors and the obsolete knife warning. Each repair now stands on its own and works on any 1.0006 build.
* Every launch writes `.ild-fixes\runtime\loader-report.txt` — what was checked and what applied — and any intervention on a stalled NPC goes to `.ild-fixes\runtime\npc-watchdog.txt`. Attach them when reporting a problem.

## Installation

Extract the archive into the game root. No original game or mod file is replaced and existing saves keep working. The addon can be removed at any time using `.ild-fixes/managed-files.txt`, and the mod keeps running as before.
