# CantorDust++ — consegne per chi continua il lavoro

Questo file lo legge l'agente che lavora sul progetto. Il piano e lo storico
delle decisioni stanno in `TODO.md`, i comandi in `COMANDI.txt`: qui c'e' cio'
che non si ricava da quei file o dal codice. Ultimo aggiornamento: 2026-11-15.

## L'utente e come lavorare con lui

- Parla italiano: risposte, commenti nel codice e documenti in italiano. I
  messaggi di commit sono in inglese (`feat:`, `fix:`, `refactor:`, `style:`).
- **Commit e push solo quando lo chiede.** Di solito prova con `make run`, poi
  dice "commit e push". Si lavora su `main`, remoto `origin` (GitHub).
- Vuole ragionare prima delle scelte grosse (viste nuove, interfaccia): proporre
  con pro e contro, aspettare il "vai".
- Apprezza che gli si dica cosa NON e' stato verificato e quando una previsione
  si e' rivelata sbagliata.
- Durante i test con tasti simulati sente un suono in cuffia: lo sa, non e' un
  problema, si continua a provare sullo schermo `:0`.

## Cosa deve restare vero

- **Esploratore universale di file**, non strumento di sicurezza. Una funzione
  entra se aiuta a scoprire struttura o contenuto in qualunque sequenza di
  byte; conoscere un formato puo' arricchire il risultato, mai essere
  necessario. Fuori: import, hardening, regole tipo YARA, disassembler.
- Solo olcPixelGameEngine + stb (in `libs/`). **Nessuna libreria nuova**, niente
  port web, una sola finestra 1280x800 che non si allarga.
- **File piccoli, una responsabilita' per file** (il piu' lungo e' sotto le 200
  righe, escluso `visualizer.h`). Logica separata dalla grafica, cosi' si testa.
- **Schermo sobrio**: niente pannelli in piu'. Le opzioni vanno nella riga in
  basso (`option_bar.h`), i dettagli nell'Inspector.
- Il 3D resta com'e' (WASD, riquadro dei comandi).
- Non cancellare: le frecce commentate in `visualizer.cpp`, i file vecchi nella
  radice (`test_digraph*`, `test_reader`, `test_screenshot`,
  `digraph_export.png`), le immagini di riferimento.
- Il "passo 5" del TODO (miniature al posto delle tab) e' rimandato
  dall'utente: non riproporlo.

## Come e' fatto

`Visualizer` (classe unica, divisa in tanti `visualizer_*.cpp`) possiede tutto.
Un frame, in `visualizer.cpp`: `applyResults` → `applyPendingOption` → input →
`recomputeWindow` se la finestra e' cambiata → `drawFrame` → `drawChrome` →
`drawOptions`.

- **Finestra**: le barre a sinistra (`nav_bars.*`, `range_sel.h`) scelgono un
  tratto del file; le viste mostrano solo quello. `m_computedStart/End` dicono
  su quale tratto sono calcolate le viste correnti.
- **Calcolo**: `computeViews` (`view_results.cpp`) gira in un thread
  (`std::async`) e riempie un `ViewResults`; `applyResults` lo sposta nelle
  viste all'inizio del frame. Il thread non tocca mai cio' che si disegna. Si
  calcola solo la vista attiva.
- **Viste**: 1 Digraph, 2 Dot Plot, 3 Entropia, 4 Istogramma, 5 3D, 6 Raw,
  7 Metric Map, 8 Byte Cloud, 9 One Tuple, 0 Stringhe. Le tab sono in
  `view_tabs.h`, larghe quanto il testo.
- **Inspector** (`visualizer_inspector*.cpp`): spiega cio' che sta sotto il
  mouse; senza nulla sotto mostra il riassunto, con l'elenco delle firme.
- **Collegamenti**: sezione → vista (`visualizer_highlight.cpp`), valore → file
  (`visualizer_links.cpp`, `occurrences.*`), cursore
  (`visualizer_cursor.cpp`: `goToOffset`, riquadro in Raw e Metric Map).
- **Firme** (`signatures.*`, `signature_checks.*`, `visualizer_signatures.cpp`):
  cercate su tutto il file al caricamento, tacche azzurre sulle barre, `N`.
- **Widget**: `ui_widgets.h` (Slider, PanelWriter), `option_bar.h`,
  `list_state.h` + `list_view.h` (lista scorrevole, pronta per altre viste).

### Per aggiungere una vista

1. Classe di calcolo senza grafica (`nome.h/.cpp`), con test in `tests/`.
2. `ViewMode` in `view_mode.h`, tab in `view_tabs.h` (e `VIEW_TAB_COUNT`).
3. Campo in `ViewResults`, `case` in `computeViews`, spostamento in
   `applyResults` (`visualizer_nav.cpp`).
4. Disegno: se e' un'immagine 256x256, `getCurrentRGB` in
   `visualizer_canvas.cpp`; se no, come Stringhe (`hasCanvas()` falso,
   `visualizer_strings.cpp`).
5. Legenda in `visualizer_legend.cpp`, pannello dell'Inspector, eventuali
   opzioni in `visualizer_options.cpp`.
6. `Makefile`: il `.cpp` in `SRC` e, se serve ai test, in `TEST_SRC`.
7. `COMANDI.txt`, `TODO.md`.

I tasti 1-9 e 0 sono finiti: la prossima tab non avra' un numero. Oltre le 12
tab circa serve la voce "Altro ▾" decisa con l'utente (vedi TODO).

## Trappole gia' incontrate

- **Decal sopra i pixel**: in olcPGE i decal coprono tutto cio' che e'
  disegnato con `Draw*`/`FillRect`. Sopra una vista si disegna con `*Decal`;
  mentre e' aperta "Apri file" la vista non si disegna.
- **Mai distruggere un decal a frame gia' composto**: un click sulla riga di
  opzioni che cambia la dimensione di un'immagine va rimandato
  (`m_pendingOption`, applicato a inizio frame). Altrimenti crash.
- **Tastiera**: su X11 il tasto arriva gia' con Shift applicato e `mapKeys`
  conosce pochi simboli. Per i caratteri usare `GetKeyPressCache()` (vedi
  `input_handler.cpp`). Provato su X11 con tastiera italiana e americana; mai
  su macOS o Windows nativo.
- **Focus**: se la finestra perde il focus il rilascio di un tasto si perde
  (`IsFocused()` nel 3D).
- **Offset relativi**: i risultati di una vista sono relativi alla finestra su
  cui sono stati calcolati. `ViewResults::windowStart` li accompagna; non
  usare `m_navigator.getStart()` per interpretarli, puo' essere gia' cambiato.
- **Tasti liberi**: occupati O S B Home Esc N, 1-9 0, e per vista A F C V L T
  `[ ] , .`, nel 3D W A S D Q E R C. Controllare prima di assegnarne uno.

## Come si verifica (da fare prima di dire "funziona")

```
make test                      # test di regressione, senza finestra
make -j8 && make run FILE=...  # l'app
```

- **Warning**: zero con `-Wall -Wextra -Wshadow` (controllato il 2026-11-15):
  `for f in src/*.cpp; do g++ -std=c++17 -Wall -Wextra -Wshadow -isystem libs/
  -I src -fsyntax-only $f; done`. Tenerlo a zero.
- **Memoria**: compilare tutto con `-g -O1 -fsanitize=address,undefined`
  (link: `-lGL -lX11 -lpng -lpthread -ldl -lstdc++fs`) e usare l'app su
  `/bin/ls`, un file vuoto, uno da 2 byte, casuale da qualche MB. Avviare con
  `ASAN_OPTIONS=detect_leaks=0`.
- **Thread**: `-fsanitize=thread`, avviato con `setarch x86_64 -R ./programma`
  (su questo kernel WSL altrimenti non parte).
- **Tasti e click simulati**: `tools/xsend.c` (XTest). Compilarlo fuori dal
  repo: `gcc tools/xsend.c -o /tmp/xsend -lX11 -lXtst`. Uso:
  `xsend <id finestra> key 6 sleep 900 click 600 300 move 1100 700 wheel 4`.
  L'id si prende con `xwininfo -root -tree | grep -i cantordust`; la
  schermata con `import -window <id> file.png`. Una sola istanza per volta,
  chiuderla con `pkill -x cantordust`.
- **Confronto pixel**: `tools/shots.sh` su due eseguibili (quello nuovo e uno
  compilato da `git archive HEAD`), poi `tools/cmp.sh`. Le immagini sono
  deterministiche: una vista non toccata deve dare 0 pixel diversi.
- **File di prova**: `dataset/` (ignorata da git) contiene
  `visual_re.example`, il file dimostrativo del cantor.dust originale: pezzi
  di file diversi cuciti insieme. Coordinate utili: riga opzioni y≈783, tab
  y≈12, barre x≈110 (FILE) e x≈177 (ZONA), sezioni x≈40.

Non verificabile in automatico: la scelta di un punto nel 3D (dipende dalla
finestra). Va riprovata a mano quando si tocca quella parte.

## Stato al 2026-11-15

Ultimo commit su `origin/main`: `f8d384c` (vista Stringhe). **Non committato**,
provato (test, ASan/UBSan su 6 file, prove a mano) ma non ancora visto
dall'utente:

- Firme: `signatures.*`, `signature_checks.*`, `visualizer_signatures.cpp`,
  `tests/test_signatures.cpp`, tacche in `nav_bars_draw.cpp`.
- Cursore visibile in Raw e Metric Map: `visualizer_cursor.cpp`,
  `RawPixels::pixelOf`, `MetricMap::pixelOf`; un'occorrenza scelta resta come
  cursore quando si cambia vista.
- `tools/` (xsend, shots, cmp), questo file, `.gitignore` (zip e 7z),
  `TODO.md`, `COMANDI.txt`.

Nella radice c'e' `_cantor.dust_.7z.zip`: la demo originale del 2012
(Windows, .NET/XNA, solo vista 3D, senza sorgenti). **Non va su git**; serve
come spunto. Sul desktop Windows dell'utente c'e' il collegamento
"CantorDust++ (WSL)" a questa cartella: la copia e' una sola, dentro WSL.

## Prossimi passi concordati

In ordine (dettagli in `TODO.md`, "Piano del 2026-11-07"):

1. **Hex View** — nuova, dall'originale; riusa `list_view.h`.
2. **3D in coordinate sferiche e cilindriche** — dall'originale (tasto `F`
   la'; da noi `F` e' libero nel 3D).
3. **Spettrogramma** — FFT scritta a mano; provarla su un WAV e su record noti.
4. **Panoramica** — righe di indicatori lungo il file; usa entropia, testo,
   zeri, periodicita', firme.
5. **Profilo record**.

## Punti deboli noti e come affrontarli

| Problema | Strada proposta |
|---|---|
| Tutto il file in memoria (`BinaryReader`), file da GB | `mmap` in sola lettura dietro la stessa interfaccia; poi campionare nelle viste che oggi leggono tutta la finestra |
| Calcolo non interrompibile | un `std::atomic<bool>` di annullamento passato a `computeViews` e controllato nei cicli lunghi |
| ELF senza tabella delle sezioni = nessuna sezione | leggere i program header in `binary_format_elf.cpp`; con test su file troncati e fuzzing |
| Mai provato su Windows nativo e macOS | olcPGE li supporta; serve un Makefile per MinGW/MSVC e una prova della tastiera |
| "Apri file" richiede di scrivere il percorso | argomento da riga di comando gia' c'e'; valutare il trascinamento del file |
| `S` non salva il 3D ne' Stringhe | per il 3D leggere il fotogramma di `cloud_frame` |
| Stringhe: tante false su dati casuali | filtro per entropia del tratto o lunghezza minima piu' alta di default |
| Firme: nessuna sui pezzi senza intestazione | e' il limite del metodo: i confini li daranno Panoramica e Spettrogramma |
