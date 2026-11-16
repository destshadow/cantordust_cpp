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
- **Annullamento** (`cancel.h`): `m_cancel` si alza quando cambia vista
  durante un calcolo o si apre un file; `computeViews` restituisce `nullptr`.
  Un ciclo lungo nuovo sui byte deve controllare la bandierina, altrimenti
  quella vista torna a bloccare: `Cancel::every(i)` se l'indice avanza di 1,
  `Cancel::Poll` se salta (con `every` un ciclo a salti non si ferma quasi
  mai: errore gia' fatto una volta in Stringhe e Somiglianza). Un risultato
  calcolato per una vista diversa da quella mostrata viene scartato
  (`ViewResults::mode`). Solo il 3D supera il secondo su
  100 MB; le altre stanno sotto 0,5 s.
- **Viste**: 1 Digraph, 2 Dot Plot, 3 Entropia, 4 Istogramma, 5 3D, 6 Raw,
  7 Metric Map, 8 Byte Cloud, 9 One Tuple, 0 Stringhe, H Hex, P Panoramica. Le tab sono in
  `view_tabs.h`, larghe quanto il testo, ognuna con il suo tasto.
- **3D**: i punti si dispongono a cubo, cilindro o sfera (`coord_system.h`,
  tasto `F`); cambia solo la posizione, non i dati.
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

Ogni vista e' descritta in un posto solo: la tabella in
`visualizer_views.cpp` (campi in `view_def.h`). Il resto del programma chiede
alla vista cosa sa fare (`view().draw`, `view().legend`...), non "che vista
e'?". Non aggiungere `if (m_mode == ...)` nei file comuni: si aggiunge un
campo alla tabella.

1. Classe di calcolo senza grafica (`nome.h/.cpp`), con test in `tests/`.
2. `ViewMode` e `VIEW_MODE_COUNT` in `view_mode.h`; tab in `view_tabs.h`.
3. Campo in `ViewResults`, `case` in `computeViews`, spostamento in
   `applyResults` (`visualizer_nav.cpp`).
4. Una voce nella tabella: `rgb` se e' un'immagine nel canvas (zoom,
   scorrimento e cursore sono gia' pronti), altrimenti `canvas = false` e
   `draw`; poi `legend`, e se servono `input`, `options`, `keys`, `inspect`.
   La tabella si controlla da sola all'avvio (`assert`).
5. I file della vista (`visualizer_nome.cpp`) e le dichiarazioni in
   `visualizer.h`; `Makefile`.
6. `COMANDI.txt`, `TODO.md`; una riga in `tools/shots.sh`.

Restano fuori dalla tabella, per ora, i punti dove conta il TIPO di vista
(un pixel e' un punto, un tratto o un valore): `visualizer_highlight.cpp`,
`visualizer_links.cpp`, `visualizer_cursor.cpp`, `visualizer_inspector.cpp`.

Le cifre sono finite: le tab nuove usano una lettera (Hex = `H`,
Panoramica = `P`). Le tab arrivano a x=1136 su 1210: **non ce ne sta
un'altra**. La prossima vista richiede la voce "Altro ▾" decisa con
l'utente (vedi TODO), oppure va messa come modo di una vista esistente
(come la Somiglianza nel Dot Plot).

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
- **Tasti**: occupati O S B H N P Home Esc, 1-9 0, e per vista A F C V L T
  `[ ] , .` (T anche nel Dot Plot), nel 3D W A S D Q E R C F. I tasti globali valgono in ogni vista:
  una lettera usata dal 3D (W A S D Q E R C F) non puo' diventare globale.
- **`view_tabs.h` non deve includere olcPGE**: lo usano i test, che non
  collegano il motore. Per questo il tasto di una tab e' un `char`.

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
- **Attenzione ai test con la finestra**: se i tasti simulati non arrivano
  (la finestra non ha il focus) le schermate restano sulla vista 1 e il
  confronto pixel da' differenze enormi e false. Prima di credere a una
  regressione, guardare una schermata.
- **Tasti e click simulati**: `tools/xsend.c` (XTest; anche `down x y` e `up`
  per i trascinamenti). Compilarlo fuori dal
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

Tutto committato fino alla Panoramica. In corso il lavoro per menu' e
sottomenu' (vedi TODO, "Impianto per i menu'"): fatta la tabella delle
viste, da committare se `git status` la mostra. I test
ora durano circa 20 secondi: quelli sull'annullamento lavorano su 64 MB.
`Cancel::polls` conta i controlli della bandierina: serve ai test per
verificare senza cronometro che ogni calcolo lungo la guardi.

Controllare con `git status` cosa e' ancora da committare.

Nella radice c'e' `_cantor.dust_.7z.zip`: la demo originale del 2012
(Windows, .NET/XNA, solo vista 3D, senza sorgenti). **Non va su git**; serve
come spunto. Sul desktop Windows dell'utente c'e' il collegamento
"CantorDust++ (WSL)" a questa cartella: la copia e' una sola, dentro WSL.

Ricerca sulle viste dell'originale che conosciamo solo di nome: non esiste
documentazione pubblica (dettagli e ipotesi in `TODO.md`). Non inventarle
spacciandole per fedeli all'originale.

## Prossimi passi

In ordine (dettagli in `TODO.md`, "Da fare, in ordine"):

1. **Spettrogramma** — FFT scritta a mano; provarla su un WAV e su record noti.
2. **Profilo record**.
3. **Versione nativa per Windows**.

Proposte ancora da confermare con l'utente (vedi TODO, "L'ipotesi sono tutte
operazioni su immagini"): filtro Raw "differenza con spostamento".

## Ricerca sul profilo di Christopher Domas (2026-11-15)

Fonti primarie controllate:

- <https://github.com/xoreaxeaxeax?tab=repositories>
- <https://github.com/xoreaxeaxeax/..cantor.dust..>
- <https://github.com/xoreaxeaxeax/hexcymatix>
- <https://github.com/xoreaxeaxeax/hexcymatix/blob/main/hexcymatix.py>

Fatti verificati:

- Il repository storico `xoreaxeaxeax/..cantor.dust..` esiste, ma GitHub lo
  dichiara vuoto: non contiene il sorgente originale.
- Nel 2026 Domas ha pubblicato `hexcymatix`, ricerca esplorativa per trovare
  struttura e relazioni attraverso sottostringhe di byte ripetute, senza
  conoscere il formato.
- `hexcymatix` confronta il file con copie traslate di se stesso. Per ogni
  distanza positiva `shift` calcola, concettualmente,
  `diff[j] = data[j] - data[j + shift]`: le corse di zeri sono frammenti
  identici in due offset. Esamina quindi le diagonali di una matrice di
  autosomiglianza senza costruire la meta' simmetrica.
- Conserva ripetizioni superiori a 8 byte, fonde risultati sovrapposti e
  collega le coppie di intervalli con corde e archi, in layout lineare o
  circolare.
- L'algoritmo pubblicato e' quadratico. Lo stesso autore dice che il caso
  d'uso si e' rivelato stretto e non e' diventato uno strumento realmente
  usabile: e' una fonte concettuale, non codice da trasporre alla lettera.
- L'avatar GitHub e' una nuvola Cantor.Dust in coordinate sferiche. I punti
  occupano il volume interno, con strutture radiali e guide circolari: e'
  coerente con `byte1/byte2 = angoli`, `byte3 = raggio`, non con punti tutti
  sulla superficie. Non fornisce formule e non chiarisce Moire/Adjacency.

Conseguenza proposta, ancora da discutere con l'utente:

- La futura vista **Somiglianza** ha ora un precedente diretto dello stesso
  autore. Potrebbe avere due rappresentazioni complementari:
  1. matrice posizione x posizione campionata, per il quadro generale; le
     diagonali parallele mostrano ripetizioni;
  2. esploratore di frammenti esatti, limitato alla finestra selezionata, con
     archi fra le occorrenze.
- Per CantorDust++ non usare la scansione O(n^2) ingenua su file grandi. Una
  possibile versione scalabile usa n-gram da 8/16 byte come semi, hash degli
  offset, esclusione dei semi troppo comuni, estensione dei match, fusione e
  un tetto ai risultati migliori.
- Non e' provato che `hexcymatix` corrisponda alla vecchia **Residue Contact
  Map**. Rafforza l'idea di autosomiglianza, ma non va presentato come
  ricostruzione fedele.
- Nessuna nuova evidenza per le altre viste misteriose. Restano inferenze:
  Adjacency Graph = grafo pesato delle transizioni; Adjacency Bin = matrice
  delle transizioni; Sliding Histogram = molto vicino a One Tuple; Moire =
  ignoto.

## Punti deboli noti e come affrontarli

| Problema | Strada proposta |
|---|---|
| Tutto il file in memoria (`BinaryReader`), file da GB | `mmap` in sola lettura dietro la stessa interfaccia; poi campionare nelle viste che oggi leggono tutta la finestra |
| ELF senza tabella delle sezioni = nessuna sezione | leggere i program header in `binary_format_elf.cpp`; con test su file troncati e fuzzing |
| Mai provato su Windows nativo e macOS | olcPGE li supporta; serve un Makefile per MinGW/MSVC e una prova della tastiera |
| "Apri file" richiede di scrivere il percorso | argomento da riga di comando gia' c'e'; valutare il trascinamento del file |
| `S` non salva il 3D ne' Stringhe | per il 3D leggere il fotogramma di `cloud_frame` |
| Stringhe: tante false su dati casuali | filtro per entropia del tratto o lunghezza minima piu' alta di default |
| Firme: nessuna sui pezzi senza intestazione | e' il limite del metodo: i confini li mostrano Somiglianza e Panoramica (riga "salti") |
