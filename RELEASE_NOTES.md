## RU

## Тема

Вылеты на входе в локацию и сцены, которые не доигрывались до конца

## Изменения

* Возвращение на Кордон больше не вылетает. Причин было две: окно дома с призраком, у которого после появления зомби не оставалось ни одной подходящей ветки логики, и зоны-триггеры, следившие за персонажами, которых мод к этому моменту уже удалил, — дезертиры со Свалки, БТР, вертушка, бандит в клетке, Тихоныч на АТП.
* Смерть NPC, которого скрипт уже успел убрать из мира, больше не роняет игру. Так вылетал отстрел снайпера на Агропроме после разговора с игрушечником.
* Ввод кода на двери в Х18 больше не вылетает: терминал с подсказкой «ввести пароль» уходил в несуществующую секцию.
* Тайник с «золотой рыбкой» на Кордоне открывается: он просил звук по неверному пути, а отсутствующий звук для движка — фатальная ошибка.
* Квест Тихоныча «Подключение» больше не встаёт после сцены на АТП. Если сцена оборвалась, она доигрывается сама, и финальный диалог у Тихона появляется.
* Задание Рэбы «УБИЙЦА МЕСЯЦА» больше не проваливается в тот самый момент, когда вы отчитываетесь об убитых шпионах. Награда, выкуп артефактов и тайник в яме остаются на месте.
* Разговор с Карабином о националистах на Агропроме теперь доступен: досье, снятое с террориста, игроку не выдавалось никогда.
* У пси-установки Тихоныча больше не копится по новому экранному эффекту на каждый кадр во время затемнения, а шпион у антенн не объявляет войну игроку по десять раз в секунду.
* Вылет по вине скрипта показывает настоящую причину вместо «bad argument #2 to 'format'». Теперь по сообщению об ошибке видно, какой объект и какая секция её вызвали.

## Установка

Распакуйте архив в корень игры. Ни один оригинальный файл игры или мода не заменяется, сохранения остаются рабочими. Аддон можно удалить в любой момент по списку `.ild-fixes/managed-files.txt`, и мод продолжит работать как прежде. Если опубликован и патч-архив, то это альтернатива полному: встроенный обновлятор выбирает патч сам.

---

## EN

## Theme

Crashes on the way into a level, and scenes that never played out

## Changes

* Coming back to Cordon no longer crashes. There were two causes: the ghost house's window, which had no branch of its logic left to run once the scripted zombie had appeared, and trigger zones watching characters the mod had already removed — the Garbage deserters, the BTR, the helicopter, the caged bandit, Tikhon at the ATP.
* The death of an NPC a script had already removed from the world no longer takes the game down. That is what crashed when shooting the sniper at Agroprom after the toy-maker's conversation.
* Entering the door code in X18 no longer crashes: the terminal with the "enter the password" tip switched to a section that does not exist.
* The gold-fish stash on Cordon can be opened: it asked for a sound by the wrong path, and a missing sound is fatal to the engine.
* Tikhon's "Connection" quest no longer stalls after the ATP scene. An interrupted scene now plays itself out, and his closing dialog appears.
* Reba's "Killer of the Month" job no longer fails at the very moment you report both spies dead. The reward, the artefact ransom and the stash in the pit all stay as they were.
* The conversation with Karabin about the nationalists at Agroprom is now available: the dossier taken from the terrorist was never handed to the player.
* Tikhon's psi installation no longer piles up a fresh screen effect every frame during the blackout, and the spy by the antennas no longer declares war on the player ten times a second.
* A crash caused by a script now shows the real reason instead of "bad argument #2 to 'format'", so the error message names the object and the section that caused it.

## Installation

Extract the archive into the game root. No original game or mod file is replaced and existing saves keep working. The addon can be removed at any time using `.ild-fixes/managed-files.txt`, and the mod keeps running as before. When a patch archive is published as well, the two are alternatives: the in-game updater picks the patch by itself.
