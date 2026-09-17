## RU

## Тема

Микросхема для детектора всегда на месте, вылет по F после 1.0.10 и панель X18, которая слушается своих правил

## Изменения

* Стоит 1.0.5, 1.0.6 или 1.0.7? Из игры это обновление не встанет (ошибка с кодом 21 или 2): один раз распакуйте полный архив Setup_Manual с GitHub в корень игры, дальше обновления снова идут сами.
* Вылет «Can't find variable on_use in [ph_idle@enable]» (и любой другой секции без on_use) при нажатии F на дверях и ящиках — регрессия 1.0.10 — исправлен. Хотфикс ild_mod_repairs.script с форума больше не нужен: обновлятор заменит его сам, а не откажет кодом 27.
* Задание Броневика «Ремонт научного детектора»: микросхема EVA-1400 теперь появляется всегда, в момент выдачи записи — внутри детского тайника на Агропроме, если он уже есть (дневник на вышке прочитан), иначе прямо на месте, где этот тайник потом появится; ни тайник, ни микросхема друг друга не затирают. Метка задания стоит на тайнике или микросхеме с самого начала, шаги записи связаны с меткой, поэтому в ПДА есть стрелка; с микросхемой в рюкзаке метка переходит на Броневика. В 1.0.8–1.0.10 микросхема не появлялась вообще: тайник без [logic] не получает биндер, и ожидавшийся хук никогда не срабатывал.
* Первый шпион Ребы (Лёха): его смерть вдали от места, где он сидел, больше не оставляет игрока без HUD и управления. Через восемь секунд игрока переносят в дом рядом с ящиком, куда сцена всё равно ведёт, и цепочка автора доигрывается сама; если через минуту порции так и нет, управление возвращается.
* Панель из шести кнопок в X18 подчиняется своему правилу троек: нажатие переключает ровно двух соседей по кольцу, а не 0, 1 или 2 раза наугад. Сохранения, где панель уже сбилась в нерешаемое положение, возвращаются к исходному (горят 1, 2, 4, 6); из него панель гасят кнопки 1 и 6 либо 2 и 4.
* Сейф в тоннеле со смертью: автор спрятал кодовый кейс внутри модели сейфа. Нажатие F на сейф без введённого кода открывает клавиатуру кода; после кода сейф открывается как написано.
* Мирная ветка с Игрушечником (красноречие) больше не запирает навсегда комнату с ключом от вышки и кассетой №2: мирный исход засчитывается как конец истории с полтергейстом, дверь открывается по своей логике. Сохранения 1.0.8/1.0.9, где сдавшегося стрелка у котельной застрелили до конца разговора, получают раненого убийцу у лестницы.
* Нэмо, бессмертный стрелок, которого пак добавил в 1.0.8 в лагерь нацистов, убран: снова появляется второй смертный статист, как в моде, и разговор с Карабином открывается. В старых сохранениях Нэмо удаляется один раз.
* ОЦ-33 «Пернач» больше не роняет игру при выстреле: партикл гильз weapons\arsenal_shells1, которого в игре нет, заменён на обычный.
* «Крысиный король» теперь действительно не съедается клавишей аптечки: класс объекта перехватывается там, где движок его читает (r_clsid в XR_3DA.exe и xrGame.dll); ремонт 1.0.10 не срабатывал.
* Замки, открытые ключом или отмычкой, помнятся по уровню и имени объекта: открытый тайник на Кордоне больше не роняет игру у одноимённых ящиков на Свалке, Агропроме и в Баре. Кодовые замки, чей код ведёт в nil (тоннель, радиотайник Свалки, X18, база «Последнего дня»), при повторе больше не переключаются — клавиатура остаётся, как в самой сессии.
* Волк: два необязательных задания («ПРОПАЛИ!» и дежурство с Толиком) закрываются, когда сюжет убирает Волка с Кордона: отбытое дежурство засчитывается, остальное проваливается. В старых сохранениях они закрываются при загрузке.
* КПК Закорпата: Волк берёт его только после того, как Юра его прочитал; если КПК уже у Волка, разговор Юры «тайник пуст» открывается и без него.
* Юра, вернувшись из оффлайна, продолжает тот этап, который называют его инфопорции, а не уходит заново к тоннелю.
* Круг «Тайник Лёхи Говорящего» снимается с карты Кордона, когда ящик опустошён.
* Сцены: построение в Баре ждёт конца речи командира (54 с), командир эсэсовцев — своей (32 с); прежние «полы» пака обрывали речь и открывали следующий разговор посреди фразы.
* Поручение Сахарова на костюм «Монолита» больше не роняет игру: порции monolit_have и monolit_done, которые мод нигде не объявлял, объявлены.
* Прибор в депо Свалки: второй прыжок возвращает на землю до удара (таймер 2 с, как у первого).
* Кузьма: сохранения, где он заспавнен до 1.0.9, тоже не теряют разговор с братом; порция, выданная его таймером до крика, снимается.
* Пси-кейс у Толика в старых сохранениях больше не роняет игру по F, а его голоса на ЖД-мосту замолкают после того, как Толика увели.
* Капитан базы Агропрома: если после подземки он убит или исчез, двери на третий этаж открываются за него, и Карабин появляется на Свалке.
* Тайник Искателя «Товар сталкера» на Кордоне выдаётся в ящик, для которого он написан (автор убрал его story id), записи без ящика не съедают выпадение, а в два тайника Искателя на Свалке добавлено по одной фаре: три гарантированные фары на ЗАЗ, ЗИЛ и «копейку».
* Нива на автопарке Бара: после ремонта кнопка гаснет и детали не списываются второй раз.
* Удаление «норм» на Кордоне больше не сносит посторонние объекты с «norm» в имени по всей Зоне (дверь кровососа на Складах и другие).
* 82 скриптовых NPC из файлов с UTF-8 BOM (Бар, Тёмная долина, Склады, финал) больше не забираются лагерями: их none = true читается.
* Люк вышки Агропрома: если ключ был потрачен до 1.0.10 и люк закрылся снова, он открывается.
* Подсказки депо и тюрьмы срабатывают только на своём уровне, депо — ещё и рядом с логовом карлика.
* Тексты: подпись пояса «Belt» → «Пояс»; описания переделанных стволов (АК-47 под 7,62x54, MP5 под .45, ПМ и Форт под 9x19, HPSA под 9x18, обрез ТОЗ) и немецкого Karabin-98 (7,92) больше не обещают чужой калибр; механик говорит про родной 7.62x25 у ППС-43; реплика напильника Эконома подписана «Напильник.».
* Обновлятор: при отказе GitHub API (60 запросов в час на адрес) проверка повторяется с паузой, которую называет сам API, и в крайнем случае берёт список релизов из резервного файла репозитория; ответ API кэшируется по ETag. В главном меню внизу слева — версия фикспака и итог проверки. loader-report.txt получил секции [installation] и [update], помощник пишет update-last.txt. Патч, распакованный без своей базы, больше не роняет игру: пак говорит, какого файла нет, и работает без Lua-починок.
* Обновление больше не отказывает кодом 26 из-за файлов пака, оставшихся после ручного отката, и кодом 27 из-за изменённого вручную ild_*.script; остаток ild_fixes_text.xml от 1.0.5–1.0.7 удаляется сам.

## Установка

Распакуйте полный архив In-the-line-of-duty-fixes-1.0.11-Setup_Manual.zip в корень игры. Ни один оригинальный файл игры или мода не заменяется, сохранения остаются рабочими. Аддон можно удалить в любой момент по списку `.ild-fixes/managed-files.txt`. Update_Patch содержит только изменения относительно 1.0.10 и годится только поверх установленной 1.0.10 — его ставит встроенный обновлятор, распаковывать его вручную не нужно. Если сейчас стоит 1.0.5, 1.0.6 или 1.0.7, обновиться из игры не выйдет — распакуйте полный архив руками один раз, дальше обновления снова ставятся сами.

---

## EN

## Theme

The detector chip is always there, the F crash after 1.0.10, and an X18 panel that obeys its own rules

## Changes

* Running 1.0.5, 1.0.6 or 1.0.7? This update cannot install from inside the game (error code 21 or 2): unpack the full Setup_Manual archive from GitHub into the game root once, and updates install themselves again from then on.
* The crash "Can't find variable on_use in [ph_idle@enable]" (or any other section without on_use) on pressing F at doors and boxes - a 1.0.10 regression - is fixed. The forum hotfix of ild_mod_repairs.script is no longer needed: the updater replaces it instead of refusing with code 27.
* Bronevik's job "Ремонт научного детектора": the EVA-1400 chip now always exists, from the moment the entry is given - inside the children's cache on Agroprom when the cache is already there (the diary on the tower read), otherwise right at the spot where that cache will appear; neither the cache nor the chip ever overwrites the other. The job's spot is on the cache or the chip from the start and the entry's steps are keyed to it, so the PDA draws its arrow; with the chip in the rucksack the spot moves to Bronevik. In 1.0.8-1.0.10 the chip never appeared at all: a box without [logic] gets no binder, and the hook that waited for it never fired.
* Reba's first spy (Lyokha): his death far from where he sat no longer leaves the player without HUD and input. Eight seconds later the player is put in the house beside the box the scene leads to anyway, and the author's chain finishes on its own; if the portion is still missing a minute later, the input is given back.
* The six-button panel in X18 obeys its own rule of threes: a press switches exactly its two ring neighbours instead of 0, 1 or 2 times at random. Saves whose panel had already been knocked into an unsolvable position return to the shipped one (1, 2, 4 and 6 lit); from there buttons 1 and 6, or 2 and 4, put it out.
* The safe in the death tunnel: the author hid the keypad case inside the safe's own model. Pressing F on the safe without the code opens the keypad; with the code entered the safe opens as written.
* The peaceful branch with the Toymaker (eloquence) no longer locks the room with the tower key and cassette 2 for good: the peaceful outcome counts as the end of the poltergeist story and the door opens by its own logic. 1.0.8/1.0.9 saves in which the surrendered boiler-house shooter was shot before the talk ended get the wounded killer at the stairs.
* Nemo, the immortal shooter the pack added to the nationalist camp in 1.0.8, is withdrawn: the mod's second mortal extra is back and the talk with Karabin opens. Old saves lose Nemo once.
* The OC-33 "Pernach" no longer crashes the game on firing: its shell particle weapons\arsenal_shells1, which the game does not have, is replaced by the ordinary one.
* The Rat King is now really kept off the medkit key: the object class is intercepted where the engine reads it (r_clsid in XR_3DA.exe and xrGame.dll); the 1.0.10 repair never took effect.
* Locks opened with a key or a lockpick are remembered by level and object name: an opened Cordon stash no longer crashes the game at namesake boxes on the Garbage, Agroprom and in the Bar. Code locks whose code leads to nil (the tunnel, the Garbage radio stash, X18, the Last Day base) are no longer switched on replay - the keypad stays, as within a session.
* Volk: his two optional jobs ("ПРОПАЛИ!" and the duty with Tolik) close when the story removes Volk from the Cordon: a served duty counts as completed, the rest fails. Old saves close them on load.
* Zakorpat's PDA: Volk takes it only once Yura has read it; where Volk already has it, Yura's "the stash is empty" talk opens without it.
* Yura back from offline resumes the stage his portions name instead of walking off to the tunnel again.
* The "Lyokha the Talker's stash" circle comes off the Cordon map once the box is emptied.
* Scenes: the Bar formation waits for the end of the commander's speech (54 s), the SS commander for his (32 s); the pack's earlier floors cut the speech and opened the next talk mid-sentence.
* Sakharov's Monolith-suit errand no longer crashes the game: the portions monolit_have and monolit_done, declared nowhere in the mod, are declared.
* The depot device on the Garbage: the second throw brings the actor back before the impact (a 2 s timer, like the first).
* Kuzma: saves that spawned him before 1.0.9 keep the brother's talk too; a portion his timer granted before the scream is withdrawn.
* The psi case beside Tolik in old saves no longer crashes the game on F, and its voices at the railway bridge fall silent once Tolik is taken away.
* The captain of the Agroprom base: if he is dead or gone after the underground, the third-floor doors are opened for him and Karabin appears on the Garbage.
* The Seeker's Cordon stash "Товар сталкера" is granted into the box it was written for (the author removed its story id), records without a box no longer eat the draw, and two Seeker stashes on the Garbage carry one headlight each: three guaranteed headlights for the ZAZ, the ZIL and the Kopeika.
* The Niva on the Bar autopark: after the repair the button goes out and the parts are not taken a second time.
* The "norm" removals on the Cordon no longer delete unrelated objects with "norm" in their name across the Zone (the bloodsucker door at the Warehouses and others).
* 82 scripted NPCs from files with a UTF-8 BOM (Bar, Dark Valley, Warehouses, finale) are no longer hired away by camps: their none = true is read.
* The Agroprom tower hatch: if the key was spent before 1.0.10 and the hatch closed again, it opens.
* The depot and prison hints fire only on their own level, the depot's also only near the karlik's lair.
* Texts: the belt caption "Belt" becomes "Пояс"; descriptions of the re-chambered guns (AK-47 in 7.62x54, MP5 in .45, PM and Fort in 9x19, HPSA in 9x18, the sawn-off TOZ) and of the German Karabin-98 (7.92) no longer promise another calibre; the mechanic names the PPS-43's own 7.62x25; the Economist's file line is captioned "Напильник.".
* Updater: when the GitHub API refuses (60 requests an hour per address) the check is retried after the pause the API names and, failing that, reads the release list from a fallback file in the repository; the API answer is cached by ETag. The main menu shows the pack version and the check's verdict bottom left. loader-report.txt gains [installation] and [update] sections and the helper writes update-last.txt. A patch unpacked without its base no longer crashes the game: the pack names the missing file and runs without its Lua repairs.
* An update is no longer refused with code 26 over pack files left behind by a manual rollback, nor with code 27 over a hand-modified ild_*.script; the ild_fixes_text.xml left by 1.0.5-1.0.7 is removed by itself.

## Installation

Extract the full archive In-the-line-of-duty-fixes-1.0.11-Setup_Manual.zip into the game root. No original game or mod file is replaced and existing saves keep working. The addon can be removed at any time using `.ild-fixes/managed-files.txt`. Update_Patch carries only the changes since 1.0.10 and fits only over an installed 1.0.10 - the in-game updater applies it, there is no need to unpack it by hand. If 1.0.5, 1.0.6 or 1.0.7 is installed, the in-game update cannot reach you - unpack the full archive by hand once and updates install themselves again afterwards.
