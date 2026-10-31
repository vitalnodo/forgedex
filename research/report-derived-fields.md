# Похідні поля при серіалізації з декларативних описів — звіт

Дата: 30.10.2026. Джерела: deep research (22 джерела, 25 тверджень × 3 голоси: 24 підтверджено, 1 спростовано) + власні прогони й пошук.

## 1. Як це розв'язують (перевірено)

| підхід | хто | що виводить | де ламається |
|---|---|---|---|
| Монадні біпарсери: поле довжини анотоване проєкцією з даних (`int `upon` length`) | Xia, Orchard, Wang, ESOP 2019 | префіксні довжини | лише вперед-залежності; зміщень і циклів немає |
| Invertible syntax (часткові ізоморфізми, незалежний добуток) | Rendel & Ostermann 2010; K7/cassette (Boespflug & Spiwack 2025) | нічого залежного | довжина не може керувати наступним полем |
| `Rebuild(subcon, func)` — перерахунок із цілого об'єкта | Construct | довжини, лічильники | однопрохідно; циклів не розв'язує |
| Відкладене обчислення (suspensions) + буферизація + дописування | Daffodil / DFDL `outputValueCalc` | прямі посилання вперед | справжні цикли → SuspensionDeadlockException (DFDL-1598, DAFFODIL-2361/1517) |
| Нічого не виводить; `_check()` лише перевіряє | Kaitai Struct v0.11 `--read-write` (Java, Python; NLnet 2022–23) | — | свідоме рішення: `_io`-вирази неоднозначні, довільні KSY-вирази не обертаються |

Спростовано: «Daffodil уникає циклів, бо використовує `dfdl:valueLength`» (1–2): навпаки, `valueLength` сам буває причиною дедлоку.

## 2. Чого в перевіреному наборі немає

EverParse/3D, Daedalus, Spicy, FormatFuzzer/010, Nail, PADS, Boomerang — твердження або не потрапили в топ-25 для перевірки, або не знайдені. Окремий пошук: Daedalus і Spicy у документації — лише розбір; Nail (з витягнутих, але не перевірених тверджень) — відкладений запис + трансформації, напряму виводить лише лічильник `n_of`.
Ітерація розкладки до нерухомої точки для форматів даних — не знайдено жодного джерела.

**Nail — за текстом статті** (Bangert & Zeldovich, «Nail: A practical tool for parsing and generating data formats»; people.csail.mit.edu/nickolai/papers/bangert-nail.pdf; площадка — OSDI 2014 за пам'яттю, в тексті не підтверджено; є також версія на IEEE S&P Workshops 2014):
- «When generating output, Nail visits a dependent field twice. First … reserves space for the dependent field in the output. Once the dependent field goes out of scope, the generator writes the dependent field's value to this space.» — класичний backpatching.
- Єдиний вбудований комбінатор із залежним полем — `n_of`; зміщення, розміри, контрольні суми — через трансформації (`offset_u32_generate`: «*off = position of out_current; append in_fragment»). ZIP: 78 рядків трансформацій проти 1600 рукописного парсера.
- Наслідок (висновок, не цитата): резервування місця вимагає **фіксованої ширини** поля (`offset_u32`). Поле змінної ширини (ULEB/VLQ), значення якого відоме лише після запису, так не дописати — це саме той випадок, де потрібна ітерація. Nail цей клас не покриває.

## 3. Що додали власні прогони (закривають 2 з 4 відкритих питань звіту)

**Корпус** (`kaitai_struct_formats` @7d17566, 191 .ksy, 1704 вирази):
нетривіальних 1039 — поле 72%, афінна 9%, лінійна 2%, кусково-лінійна 2%, добуток 4%, `_io` 3%, інше 9%.
Повністю закриваються ітерацією розкладки 62% форматів (68% з `_io.size` як змінною, 75% якщо добутки лише перевіряти).

**Справжній монотонний цикл у dex:** `class_data` іде перед `code`, `encoded_method.code_off` — ULEB128. Ширина ← значення ← позиція коду ← розмір class_data ← ширини. fasmg (forgedex) доходить до нерухомої точки за 2–3 проходи; 11/11 classes.dex відтворюються побайтово.

**Взаємна перевірка forgedex ↔ dex.ksy:** баг у `dex.ksy` — `string_data_item` читає `utf16_size` *байтів* як ASCII; має бути `terminator: 0` + UTF-8 (MUTF-8). З виправленням 11/11 файлів розбираються без помилок. Урок: семантика похідного поля ≠ «розмір у байтах».

## 4. Теоретичний якір

Szymanski, *Assembling code for machines with span-dependent instructions*, CACM 21(4), 1978, doi:10.1145/359460.359474: для операндів-міток і виразів обмеженої форми — ефективний оптимальний алгоритм; для довільних виразів мінімізація довжини NP-повна. Branch relaxation (LLVM MC, Cranelift) — ітерація до збіжності з монотонним зростанням.
=> Корпусна карта фактично вимірює, яка частка .ksy лежить у трактабельному класі Szymanski (~81% нетривіальних виразів — поле/афінна).

## 5. Висновок

1. Прогалина реальна й підтверджена першоджерелами: жоден перевірений інструмент не розв'язує цикли «розмір ↔ зміщення»; Kaitai свідомо не виводить нічого.
2. Атлас: ациклічні частини графа залежностей — один прохід (Construct) або дописування (Nail, Daffodil); цикли — монотонна ітерація (асемблери). Daffodil дедлокає саме на циклах.
3. Для Kaitai: розпізнавання простих форм (поле/афінна, ~81%) + ітерація розкладки з монотонним зростанням ширин + хуки для контрольних сум і `process` (як трансформації Nail).
4. Академічно: воркшоп (LangSec, SLE/GPCE, BAR) або магістерська. Новизна — не сама ітерація (Szymanski), а перенесення на DDL + корпусний вимір + dex як бенчмарк з реальним циклом і незалежним генератором.

## 6. Відкриті питання

- Чи гарантує монотонна ітерація *мінімальну* розкладку для класу «поле/афінна» (аналог оптимального алгоритму Szymanski)? — потрібен повний текст статті.
- Граф залежностей sqlite3.ksy та решти VLQ-форматів.
- Корпус зразкових файлів для round-trip.
- Перевірити Nail/EverParse/Daedalus по першоджерелах (у deep research не перевірені).
- Kaitai 0.12-SNAPSHOT: чи щось змінилося в серіалізації.

## Джерела
- Xia, Orchard, Wang, ESOP 2019 — https://arxiv.org/pdf/1902.06950
- Rendel & Ostermann 2010 — https://www.informatik.uni-marburg.de/%7Erendel/unparse/rendel10invertible.pdf
- Boespflug & Spiwack 2025 — https://arxiv.org/pdf/2508.09856v1
- Construct Rebuild — https://construct.readthedocs.org/en/latest/api/misc.html
- Kaitai serialization — https://doc.kaitai.io/serialization.html
- Kaitai v0.11 — https://kaitai.io/news/2025/09/07/kaitai-struct-v0.11-released.html
- generalmimon notes — https://gist.github.com/generalmimon/fc22e97faf1fe4b4edc8279b0caa152d
- Pučil, ČVUT 2024 — https://dspace.cvut.cz/entities/publication/3d31f788-d100-4dfd-a3aa-86ab674ab561
- NLnet — https://nlnet.nl/project/Kaitai-Serialization/
- DFDL-1598 — https://opensource.ncsa.illinois.edu/jira/browse/DFDL-1598
- DAFFODIL-1517 — https://issues.apache.org/jira/browse/DAFFODIL-1517
- Daffodil wiki — https://cwiki-test.apache.org/confluence/x/e7NzB
- Szymanski 1978 — https://cacmb4.acm.org/magazines/1978/4/11235-assembling-code-for-machines-with-span-dependent-instructions
- LLVM BranchRelaxation — https://llvm.org/doxygen/BranchRelaxation_8cpp_source.html
- Cranelift relaxation — https://github.com/bytecodealliance/cranelift/issues/73


## 7. Додано 31.10.2026

**EverParse / LowParse — за вихідним кодом** (project-everest/everparse @43b28b1, `lib/everparse/lowparse/LowParse.Spec.VLData.fsti`, `serialize_bounded_vldata_strong_aux`):
серіалізує вміст (`let pl = s x`), вимірює (`Seq.length pl`) і пише префікс **фіксованої ширини** `l`, обраної статично: `l >= log256' max /\ l <= 4` (`LowParse.Spec.BoundedInt.fsti`). Тобто «зсередини назовні», без циклів: ширина не залежить від значення, зміщень вперед немає (TLV-формати TLS/QUIC).

**Другий баг у бібліотеці Kaitai — `database/sqlite3.ksy`:**
`cell_payload.column_serials` має `size: len_header_and_len.value - 1`, тобто вважає, що varint заголовка завжди 1 байт. За форматом SQLite розмір заголовка **включає сам varint**. Перевірено на реальних базах (Python sqlite3, page_size 65536, одна таблиця з N TEXT-колонками):

| колонок | заголовок | ширина varint | оригінал | виправлено |
|---|---|---|---|---|
| 10 | 11 | 1 | OK | OK |
| 60 | 61 | 1 | OK | OK |
| 127 | 129 | 2 | EndOfStreamError | OK, 127/127 |
| 128 | 130 | 2 | EndOfStreamError | OK |
| 200 | 202 | 2 | EndOfStreamError | OK |

Виправлення: `size: len_header_and_len.value - len_header_and_len.groups.size` (`_sizeof` компілятор відкидає для типу динамічного розміру; інстансу `len` у `vlq_base128_be` немає).

**Таксономія за тим, що пише себе (ключове уточнення до атласу):**
1. Префікс довжини, що **не включає себе** → «зсередини назовні»: спершу вміст, потім префікс. Циклу немає навіть із varint. (EverParse, Construct `Prefixed`.)
2. Довжина, що **включає себе**, змінної ширини → рівняння з однією змінною h = w(h) + S; монотонне, 1–2 ітерації. (sqlite3 record header.)
3. **Зсув уперед** змінної ширини → система: ширини ← значення ← позиції ← ширини. (dex `code_off`.)
4. Зсув уперед фіксованої ширини → резервування + дописування. (Nail `offset_u32`, Daffodil.)

Класи 2 і 3 — саме ті місця, де люди помиляються: обидва знайдені баги (dex.ksy, sqlite3.ksy) у форматах із varint, і обидва — неправильно закодована ширина/семантика самоописового поля. Серед 7 VLQ-форматів бібліотеки шаблон «.value ± константа» лише в sqlite3.

Джерела: https://github.com/project-everest/everparse ; https://www.sqlite.org/fileformat2.html#record_format
