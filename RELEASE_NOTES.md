## RU

## Тема

Перевес с костюмом, обещание Кочевника и обновления через любое число версий

## Изменения

* Перевес считается от предела, который показывает инвентарь с надетым костюмом: в костюме на 85 кг выносливость при ходьбе больше не тает как у перегруженного уже после 60 кг. Без костюма всё как было.
* Костюм «Курьер» у Кочевника на чёрном рынке держит обещанные 90 кг, и в его описании теперь тоже «под 90».
* Сенсор ПП-4а называется по-русски и показывает свою иконку, МП-40 и МП-41 носят свои имена: в 1.0.8 эти правки до игры не доходили.
* Игра больше не вылетает изредка при запуске с «bad node in heap».
* Снайперы на постах, спящие и часовые, которые стоят на месте по сценарию, больше не вскакивают раз в минуту: сторож зависших NPC перестал принимать их за застрявших — это была ошибка самого фикспака.
* Тюрьма на мельнице: когда охрана уводит двоих на работу и всё замирает, игрок получает одну подсказку, что побег дальше идёт через разговор с Закорпатом — мод об этом молчал, а сам перед этим велел ждать полуночи.
* Фонарь, ПДА, детектор, бинокль и болт можно ставить в слот вручную из инвентаря без вылета: движок обращался к UI-списку слота, которого у этих слотов нет. Фонарь, ПДА и детектор при этом не пытаются стать «оружием в руках».
* Кузьма: ветка про брата больше не теряется — портия с трупа приходит, даже если труп заспавнился вне зоны видимости, а таймер у аномалии не закрывает диалог заранее.
* Гжегож: когда Макаров заново сажает пленника, старая копия у ворот и новичок больше не остаются в лагере.
* Повторный заход в тоннель смерти больше не оставляет без управления навсегда: оно возвращается вместе с «зовом тоннеля».
* Кодовые замки помнят, что код уже введён: замок, открытый и выгруженный вместе с уровнем, не запирается снова.
* Артефакт «Грави» весит 0,5 кг, а не ноль; монтировка в рюкзаке весит столько же, сколько в руке.
* Встроенный обновлятор теперь ставит обновление правилами той версии, на которую обновляет, и сам меняет неподходящий патч на полный архив ещё до выхода из игры — следующие обновления дойдут через сколько угодно пропущенных версий.

## Установка

Распакуйте архив в корень игры. Ни один оригинальный файл игры или мода не заменяется, сохранения остаются рабочими. Аддон можно удалить в любой момент по списку `.ild-fixes/managed-files.txt`. Полный архив и патч — альтернативы, встроенный обновлятор выбирает патч сам. Если сейчас стоит 1.0.5, 1.0.6 или 1.0.7, обновиться из игры не выйдет — распакуйте полный архив руками один раз, дальше обновления снова ставятся сами.

---

## EN

## Theme

Overweight in a suit, Nomad's promise and updates across any number of versions

## Changes

* Overweight is measured against the limit the inventory shows with the suit on: in a suit rated for 85 kg, walking no longer drains stamina like an overloaded stalker's from the 60th kilogram. Without a suit nothing changes.
* The "Courier" suit Nomad sells at the black market carries the 90 kg he promises, and its description now says so too.
* The PP-4a sensor reads its proper name and shows its own icon, and the MP-40 and MP-41 carry their own names: in 1.0.8 these fixes never reached the game.
* The game no longer crashes now and then on start with "bad node in heap".
* Snipers at their posts, sleepers and guards who stand still by design are no longer stood up once a minute: the stalled-NPC watchdog stopped taking them for stuck - a fault of the fix pack itself.
* The prison at the mill: when the guards take two prisoners out to work and everything stops, one hint tells the player that the escape goes on through a talk with Zakorpat - the mod said nothing, having just told the player to wait for midnight.
* The torch, PDA, detector, binoculars and bolt can be put into their slots by hand from the inventory without a crash: the engine asked for a slot list the window does not have. The torch, PDA and detector no longer try to become the item in the hands.
* Kuzma: the story of his brother is no longer lost - the corpse's portion arrives even when the corpse spawned out of sight, and the timer at the anomaly no longer closes the dialog ahead of time.
* Jegoj: once Makarov puts the prisoner back, the old copy at the gate and the novice no longer linger in the camp.
* Walking back into the death tunnel no longer leaves the player without controls for good: they return with the tunnel's call.
* Code locks remember an entered code: a lock opened and then unloaded with the level does not lock again.
* The Gravi artefact weighs 0.5 kg instead of nothing, and the crowbar weighs the same in the rucksack as in the hand.
* The in-game updater now applies an update with the rules of the version it installs and swaps an unfitting patch for the full archive before the game closes, so later updates arrive across any number of skipped versions.

## Installation

Extract the archive into the game root. No original game or mod file is replaced and existing saves keep working. The addon can be removed at any time using `.ild-fixes/managed-files.txt`. The full archive and the patch are alternatives; the in-game updater picks the patch by itself. If 1.0.5, 1.0.6 or 1.0.7 is installed, the in-game update cannot reach you - unpack the full archive by hand once and updates install themselves again afterwards.
