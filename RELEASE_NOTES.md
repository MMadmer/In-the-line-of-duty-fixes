## RU

## Тема

Вылеты на входе в локацию и ветки, которые обрывались на полпути

## Изменения

* Возвращение на Кордон больше не вылетает. Причин было две: окно дома с призраком, у которого после появления зомби не оставалось ни одной подходящей ветки логики, и зоны-триггеры, следившие за персонажами, которых мод к этому моменту уже удалил, — дезертиры со Свалки, БТР, вертушка, бандит в клетке, Тихоныч на АТП.
* Смерть NPC, которого скрипт уже успел убрать из мира, больше не роняет игру. Так вылетал отстрел снайпера на Агропроме после разговора с игрушечником.
* Ввод кода на двери в Х-18 больше не вылетает: терминал с подсказкой «ввести пароль» уходил в несуществующую секцию.
* Тайник с «золотой рыбкой» на Кордоне открывается: он просил звук по неверному пути, а отсутствующий звук для движка — фатальная ошибка.
* Квест Тихоныча «Подключение» больше не встаёт после сцены на АТП. Если сцена оборвалась, она доигрывается сама, и финальный диалог у Тихона появляется.
* Ветка военных на Агропроме доигрывается до конца. Прапорщик и два сержанта, которых по диалогу убивает контролёр в подземке, больше не остаются вечно в позе «подконтрольных»: их смерть наконец происходит, а вместе с ней просыпается и гарнизон наверху, который её ждал. После зачистки базы приходит подсказка, куда идти дальше, — раньше квесты были закрыты, метки не было, а единственный путь вёл к человеку в неотмеченном углу Свалки.
* Построение в Баре больше не может остановить всю игру. Речь командира ждала звука без всякого таймаута, а от её окончания зависит вообще вся дальнейшая сюжетка.
* Вход в лабораторию Х-18 наконец засчитывается. Дверь в моде открыта с самого начала, поэтому задание «найти вход» не закрывалось никогда, Бармен не принимал документы, задание по Х-16 не выдавалось, а его ассортимент не обновлялся.
* Решётка в Х-16 открывается после отключения излучателя, как и должна. В моде она была наглухо закрыта и ждала условия, которого в игре нет.
* Псевдогигант в Х-18 больше не запирает финал, если умрёт в яме: сцена завершается, задание закрывается, обе запертые двери открываются.
* Прощание с Летягиным больше не может оставить без управления и HUD. Раньше их возвращала единственная проверка расстояния, а игрока в это время держал телепорт.
* Концовка защищена от единственной точки отказа: после шести генераторов путь в последнюю комнату открывается, даже если сигнал тревоги не отыграется. Дверь в генераторный зал открывается, даже если смерть одного из пятерых охранников не была засчитана. Затемнение на Радаре возвращает управление, даже если сон или звук не доиграют.
* Задания «Уничтожить кровососов в деревне» и «Убить кровососов» на Складах закрываются: самих кровососов из мода вырезали, а задания остались. Угрюмый и командир блокпоста снова разговаривают. Приказ Лукаша против группы Долга, цели которого в мод не попали, честно отмечается проваленным вместо вечной записи в КПК.
* Задание Рэбы «УБИЙЦА МЕСЯЦА» больше не проваливается в тот самый момент, когда вы отчитываетесь об убитых шпионах. Награда, выкуп артефактов и тайник в яме остаются на месте.
* Разговор с Карабином о националистах на Агропроме и последний разговор с Мазаем в Тёмной долине снова доступны — оба ждали того, чего в игре нет.
* Таймер выброса на ЧАЭС больше не висит на экране до конца уровня с нулями и служебной строкой вместо подписи. Немецкие снайперы в Мёртвом городе возвращаются на позиции утром. Сирена налёта играет один раз, а не каждый кадр. У пси-установки Тихоныча больше не копится по новому экранному эффекту на каждый кадр во время затемнения, а шпион у антенн не объявляет войну игроку по десять раз в секунду.
* Вылет по вине скрипта показывает настоящую причину вместо «bad argument #2 to 'format'». Теперь по сообщению об ошибке видно, какой объект и какая секция её вызвали.
* Число обнаруженных NPC встало по центру своего кружка у мини-карты. Мод перерисовал HUD круглым, а счётчик остался размечен по старой панели и печатался ниже и левее своего места. Сама мини-карта осталась какой была.
* Правки мелких файлов конфигурации наконец доходят до игры. Файлы меньше определённого размера движок читает мимо того механизма, на котором эти правки держались, поэтому они молча не срабатывали: кавычки в описании навыка у станка, ночной лагерь на Кордоне, звук у пси-установки, описание ремонта машины и текст задания про пропуск.

## Установка

Распакуйте архив в корень игры. Ни один оригинальный файл игры или мода не заменяется, сохранения остаются рабочими. Аддон можно удалить в любой момент по списку `.ild-fixes/managed-files.txt`, и мод продолжит работать как прежде. Если опубликован и патч-архив, то это альтернатива полному: встроенный обновлятор выбирает патч сам.

---

## EN

## Theme

Crashes on the way into a level, and branches that stopped halfway

## Changes

* Coming back to Cordon no longer crashes. There were two causes: the ghost house's window, which had no branch of its logic left to run once the scripted zombie had appeared, and trigger zones watching characters the mod had already removed — the Garbage deserters, the BTR, the helicopter, the caged bandit, Tikhon at the ATP.
* The death of an NPC a script had already removed from the world no longer takes the game down. That is what crashed when shooting the sniper at Agroprom after the toy-maker's conversation.
* Entering the door code in X-18 no longer crashes: the terminal with the "enter the password" tip switched to a section that does not exist.
* The gold-fish stash on Cordon can be opened: it asked for a sound by the wrong path, and a missing sound is fatal to the engine.
* Tikhon's "Connection" quest no longer stalls after the ATP scene. An interrupted scene now plays itself out, and his closing dialog appears.
* The Agroprom military branch plays out to its end. The warrant officer and the two sergeants the dialog says the controller killed no longer hold the mind-controlled pose for ever: they finally die, and with them the garrison above, which was waiting for exactly that, wakes up. Once the base is cleared a hint says where to go next — the quests were already closed, there was no marker, and the only way on led to a man in an unmarked corner of the Garbage.
* The Bar formation can no longer stop the whole game. The commander's speech waited on a sound with no timeout, and every quest that follows hangs on that speech ending.
* Entering the X-18 laboratory finally counts. The mod leaves the door open from the start, so the "find the entrance" task never completed, the Barman never took the documents, the X-16 task was never given and his stock never improved.
* The X-16 grate opens once the emitter is shut down, as it should. In the mod it was sealed and waiting for a condition the game does not have.
* The X-18 pseudogiant no longer locks the finale by dying in the pit: the scene ends, the task closes and both locked doors open.
* Letyagin's farewell can no longer leave you without controls or a HUD. They used to come back through a single distance check, while a teleport held the player in place.
* The ending is protected against its single point of failure: after six generators the way into the last room opens even if the alarm never sounds. The door to the generator hall opens even if one of the five guards' deaths went unregistered. The Radar blackout gives the controls back even if the dream or the sound never finishes.
* The Warehouse tasks "Kill the bloodsuckers in the village" and "Kill the bloodsuckers" can be closed: the bloodsuckers themselves were cut from the mod and the tasks were left behind. Ugrumy and the blockpost commander can be talked to again. Lukash's order against a Duty group whose targets are not in the mod is reported failed instead of sitting in the PDA for ever.
* Reba's "Killer of the Month" no longer fails at the very moment you report both spies dead. The reward, the artefact ransom and the stash in the pit all stay as they were.
* The Karabin conversation about the nationalists at Agroprom and Mazai's last talk in Dark Valley are available again — both waited for something the game does not contain.
* The CNPP surge timer no longer hangs on screen at zero for the rest of the level with its own string id as the caption. The German snipers in Dead City return to their posts at dawn. The air-raid siren plays once instead of every frame. Tikhon's psi installation no longer piles up a fresh screen effect every frame during the blackout, and the spy by the antennas no longer declares war on the player ten times a second.
* A crash caused by a script now shows the real reason instead of "bad argument #2 to 'format'", so the error message names the object and the section that caused it.
* The number of detected NPCs now sits in the middle of its own disc on the minimap. The mod repainted the HUD round while the counter stayed laid out for the old panel, so it printed below and to the left of where it belongs. The minimap itself is untouched.
* Repairs to small configuration files finally reach the game. The engine reads anything below a certain size past the mechanism those repairs rode on, so they had been failing silently: the quotation marks in a workbench skill line, the night camp on Cordon, the sound at the psi installation, the car repair description and the wording of the pass task.

## Installation

Extract the archive into the game root. No original game or mod file is replaced and existing saves keep working. The addon can be removed at any time using `.ild-fixes/managed-files.txt`, and the mod keeps running as before. When a patch archive is published as well, the two are alternatives: the in-game updater picks the patch by itself.
