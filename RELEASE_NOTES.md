## RU

## Тема

Детектор аномалий, вылет в диалоге, бесконечные тушканы и квест с Порчей

## Изменения

* Разговор с механиком в баре больше не вылетает. Ветка про исполнителя желаний после ответа механика вела обратно на одноразовую реплику «вот тебе водка», которая к тому моменту уже недоступна — и ветка оставалась без единого продолжения прямо в открытом окне диалога. Теперь она возвращается в общий круг тем, как все остальные ветки этого разговора.
* Засада в депо на Свалке больше не выглядит сломанным квестом. Тушканов бесконечно подгоняет карлик, и пока он жив, спавн не кончится — про это игра не говорила ничего. Через двадцать секунд после начала боя ГГ отмечает про себя, что мутантами кто-то управляет и стоит поискать вокруг. Один раз, и только если карлик ещё жив. Сам бой, карлик и награда не тронуты.
* Квест с починкой детектора аномалий доведён до конца. Раньше Броневик брал пять тысяч за осмотр, называл нужную деталь — микросхему EVA-1400 — и на этом всё кончалось: микросхема в файлах была, но нигде не лежала, вернуться к нему было не с чем и не за чем. Теперь микросхема лежит там, где её и задумывали спрятать — в тайнике детей на Агропроме, — а Броневик собирает из неё рабочий элитный детектор. Карта ведёт по этапам: пока микросхема не найдена, отмечен тайник; как только она в рюкзаке, отметка переходит на Броневика; после починки обе снимаются. Тем, кто уже отдал пять тысяч, ничего перепроходить не нужно — квест подхватывается с того места, где стоит, а микросхема появится в тайнике, даже если его уже обыскали. Цена осмотра и содержимое тайника не тронуты.
* Две метки на карте Кордона больше не могут уронить игру. Мод читал у объекта-ориентира идентификатор, не проверив, что объект вообще есть, — а делается это внутри обработчика инфопоршней, где ошибка фатальна. Теперь метка просто не ставится, если ставить её не на что.
* Побочный квест с Порчей и Душой на Свалке больше нельзя загубить. У могилы призрака лежат две одинаковые кучи земли, и раскопка не той уничтожала Душу, оставляла задачу «В поисках души» в КПК навсегда и меньше чем через минуту возвращала Порчу. Теперь закапывание срабатывает на любой из них: задача закрывается, проклятие снимается.
* Свечи на той же могиле больше не зажигаются раньше времени. Один из двух подсвечников не ждал разговора с призраком, и со спичками в кармане квест проходился в обход Порчи — то есть в обход того, ради чего он есть.
* Порча больше не раздваивается, если убрать её в ящик или тайник: артефакт возвращается на пояс, а не появляется вторым.
* Замок схрона с артефактами на Свалке отвечает на неудачное вскрытие, как все остальные замки мода. Раньше отмычка молча пропадала, и ящик выглядел сломанным. Шансы вскрытия не тронуты.

## Установка

Распакуйте архив в корень игры. Ни один оригинальный файл игры или мода не заменяется, сохранения остаются рабочими. Аддон можно удалить в любой момент по списку `.ild-fixes/managed-files.txt`, и мод продолжит работать как прежде. Если опубликован и патч-архив, то это альтернатива полному: встроенный обновлятор выбирает патч сам.

---

## EN

## Theme

The detector job, a dialogue crash, the endless tushkans and the curse quest

## Changes

* The drinking talk with the Bar mechanic no longer crashes. After his answer about the wish-granter the branch pointed back at the one-shot "here, take the vodka" reply, which is gone by then, leaving the branch with no reachable continuation while the talk window was still open on it. It now returns to the hub every other branch of that conversation returns to.
* The depot ambush on the Garbage no longer reads as a broken quest. The endless tushkans are the karlik's doing and they stop when he does, and nothing in the game said so. Twenty seconds into the fight the player notes to himself that something is driving the mutants and it is worth looking around. Once, and only while the karlik is alive. The fight, the karlik and the reward are untouched.
* The anomaly detector repair job now finishes. Bronevik used to take five thousand for the inspection, name the part he needed - an EVA-1400 microchip - and that was the end of it: the chip existed in the files but lay nowhere, and there was nothing to bring back and no one to bring it to. The chip is now in the children's cache on Agroprom where it was meant to be hidden, and bringing it to Bronevik gets a working elite detector built out of it. The map leads the way one stage at a time: the cache is marked until the chip is found, then Bronevik is, and both marks come down once the detector is rebuilt. A save that already paid the five thousand carries on from where it stands, and the chip appears in the cache even if it has already been looted. The price and the cache's other loot are untouched.
* Two Cordon map spots can no longer bring the game down. The mod read an id off the landmark they mark without checking the landmark is there, inside the info-portion callback where a raise is fatal. The spot is now simply not placed when there is nothing to place it on.
* The Garbage side quest about Порча and Душа can no longer be ruined. Two identical mounds of earth lie at the ghost's grave, and digging the wrong one destroyed the Душа, left the "В поисках души" task in the PDA for good and handed the curse back in under a minute. Digging either mound now closes the task and lifts the curse.
* The candles at that grave can no longer be lit too early. One of the two candle boxes did not wait for the conversation with the ghost, so a player carrying matches finished the quest without ever receiving the curse the quest is about.
* Порча no longer splits in two when it is put into a box or a stash: the artefact returns to the belt instead of a second one appearing.
* The lock on the artefact stash on the Garbage answers a failed pick the way every other lock in the mod does. A lockpick used to disappear in silence and the box looked broken. The chances of picking it are untouched.

## Installation

Extract the archive into the game root. No original game or mod file is replaced and existing saves keep working. The addon can be removed at any time using `.ild-fixes/managed-files.txt`, and the mod keeps running as before. When a patch archive is published as well, the two are alternatives: the in-game updater picks the patch by itself.
