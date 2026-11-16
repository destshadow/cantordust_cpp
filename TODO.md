# CantorDust++ — obiettivi e appunti

## Obiettivo

Uno strumento per **esplorare** file binari e trovare strutture dati
nascoste o inattese (immagini, tabelle, record, testo, codice) — che
funzioni su qualunque file, non solo su formati noti. Riferimento di
partenza: il CantorDust originale in `supermario-resized-gif.gif`.

Il progetto resta **semplice e "batterie incluse"**: solo
olcPixelGameEngine + stb (single header), nessuna dipendenza nuova.
Quello che manca ce lo scriviamo noi (kit di widget in `ui_widgets.h`).

**Direzione (decisa il 2026-11-07): resta un esploratore universale di file.**
La sicurezza e' un caso d'uso, non l'obiettivo. Regola per ogni funzione nuova:
aiuta a scoprire struttura o contenuto in qualunque sequenza di byte; la
conoscenza di un formato puo' arricchire il risultato, mai essere necessaria.

## Principi di design (decisi il 2026-11-03)

1. **Ogni vista dice cosa mostra**: una riga di legenda in alto
   (es. "X = byte i, Y = byte i+1, luminosità = frequenza").
2. **Hover = Inspector**: un pannello fisso a destra spiega cosa c'è
   sotto il mouse. Parte generica per qualunque file (offset, hex+ASCII,
   tipo di byte, entropia locale); la sezione ELF/PE solo se nota.
   Cosa mostra dipende dalla vista:
   - RAW, MetricMap, Hex: un pixel = un punto del file → offset, byte, sezione
   - DotPlot, Entropy, OneTuple: un pixel = un tratto del file → range, sezione
   - Somiglianza: un pixel = due tratti → cosa hanno in comune
   - Digraph, 3D, Histogram, ByteCloud: un pixel = un valore →
     quante volte compare e **dove** compare nel file
   - Stringhe: una riga = un testo trovato → offset, sezione, byte attorno
3. **Evidenziazione collegata** (brushing & linking):
   - *selezione → vista*: selezionando un range (sezione o tratto sulle
     barre) ogni vista mostra il contributo di quel range e attenua il
     resto (nel 3D/digraph: punti del range colorati, gli altri grigi)
   - *vista → file*: hover su un valore (coppia, tripla) → sulla barra
     del file compaiono i segni dove quel valore compare
   - *cursore*: un punto scelto (stringa, firma, occorrenza, byte) resta
     segnato sulle barre e in ogni vista che ha una posizione nel file
4. **Sezioni = annotazioni sulle barre**, come bande colorate con il nome;
   niente più linee che tagliano la vista (al massimo attivabili con un
   tasto).
5. **Una sola finestra, 1280×800**, con pannello informazioni a destra
   (olcPGE gestisce una finestra per processo; due finestre
   vorrebbe dire un'altra libreria).
6. **Niente port su HTML** per ora: i problemi erano di design, non del
   motore.
7. **File piccoli, divide et impera**: un file, una responsabilità. Quando
   un file cresce si divide; la logica sta separata dalla grafica, così si
   può testare da sola (es. `range_sel.h` e `nav_bars.cpp`).

Layout (com'e' oggi):

```
┌ 1 Digraph  2 Dot Plot  3 Entropia ... 0 Stringhe  H Hex ──── fps ┐
├─sezioni─┬─FILE─┬─ZONA─┬─ legenda della vista ──┬── INSPECTOR ────┤
│▓ .text  │      │ ▲    │                        │ OFFSET 0x0004D3 │
│▓        │ ▲    │ sel  │        vista           │ SEZIONE .text   │
│░ .rodata│ sel  │ ▼    │                        │ 48 89 E5 41 57  │
│▒ .data  │ ▼    │      │                        │ ...             │
├─────────┴──────┴──────┴────────────────────────┴─────────────────┤
│ file e messaggi                                                  │
│ opzioni della vista (cliccabili)            O apri  S salva  ... │
└──────────────────────────────────────────────────────────────────┘
```

- **FILE**: tutti i byte del file in sequenza (verde = valore), con le bande
  delle sezioni e una selezione trascinabile (maniglie ▲/▼).
- **ZONA**: solo il tratto selezionato in FILE, con una sua selezione che
  decide cosa va nella vista.

## Le viste oggi

| Tasto | Vista | File |
|---|---|---|
| 1 | Digraph: coppie di byte | `digraph.*` |
| 2 | Dot Plot: posizione × valore; `T` = **Somiglianza** (il file con se stesso) | `dotplot.*`, `similarity.*` |
| 3 | Entropia lungo il file | `entropy.*` |
| 4 | Istogramma dei valori | `histogram.*` |
| 5 | 3D: triple di byte; `F` = cubo, cilindro, sfera; `C` = colore | `trigraph.*`, `renderer3d*`, `coord_system.h` |
| 6 | Raw: byte come pixel; bpp, larghezza, offset, filtri; `A` = automatico | `raw_pixels.*`, `autocorr*`, `pixel_guess.*` |
| 7 | Metric Map: curva di Hilbert; `C` colore, `V` curva | `metric_map*` |
| 8 | Byte Cloud: griglia dei 256 valori | `byte_cloud.*` |
| 9 | One Tuple: valori usati da ogni tratto | `one_tuple.*` |
| 0 | Stringhe: ASCII e UTF-16, tag, filtri | `string_scan.*`, `string_tag.cpp` |
| H | Hex: 16 byte per riga | `hex_layout.h`, `visualizer_hex.cpp` |
| P | Panoramica: indicatori lungo il file | `overview.*`, `visualizer_overview.cpp` |

In ogni vista: **Firme** (`N`, tacche azzurre sulle barre; `signatures.*`) e
**cursore** (`visualizer_cursor.cpp`).

## Da fare, in ordine

1. [x] **Panoramica** (tasto `P`; `overview.*`, `chi_square.h`,
       `visualizer_overview.cpp`): X = posizione, una riga per indicatore
       (entropia, salti, testo, zeri, 0xFF, periodo, bit bassi, firme, fuori
       dalle sezioni). Click = cursore. 100 MB in 0,1 s.
       - "Bit bassi": test chi-quadro di Westfeld e Pfitzmann (2000) su almeno
         64 KB. Provato: netto su codice e testo (0 contro 1), ma **in foto e
         audio segnala sempre**, perche' li' i bit bassi sono gia' rumore.
         Per questo l'etichetta e' "casuali", non "dati nascosti".
       - "Periodo": quota di byte uguali a quello p posizioni dopo, oltre il
         caso, per p da 1 a 64, sui primi 4 KB del tratto. Periodi piu' lunghi
         di 64 non li vede: per quelli c'e' `A` nel Raw.
       - [ ] idea: trascinare per selezionare un tratto come finestra
2. [ ] **Spettrogramma** (tab): X = posizione nel file, Y = periodo in byte,
       luce = forza del periodo. Mostra dove cambia il periodo. FFT scritta
       da noi, in un file piccolo. Provarla su un WAV e su record noti.
       - [ ] picchi: i periodi piu' forti del file, cliccabili
       - [ ] click su un periodo: dove compare, con i segni sulle barre
             (l'"immagine modale" della tesi di Davis, cap. 3)
       - [ ] togliere lo spettro medio, per far risaltare le zone diverse
3. [ ] **Profilo record** (tab): dato un periodo, mostra colonna per colonna i
       campi fissi, i contatori e quelli casuali. Conteggio di valori per
       posizione nel record.
4. [ ] **Versione nativa per Windows** (l'utente lavora da Windows, oggi passa
       da WSL): Makefile per MinGW o MSVC, prova della tastiera.

Le tab sono 12 e arrivano a x=1136 su 1210: non ce ne sta un'altra.
La prossima vista richiede la voce **"Altro ▾"** in fondo, con le viste usate meno; le altre
restano sempre visibili. Niente barra scorrevole: nasconde le viste.

### Impianto per i menu' (deciso il 2026-11-16)

Per poter aggiungere menu', sottomenu' e viste senza rifare il codice. Tre
passi mirati, non un sistema generale di componenti:
1. [x] **Tabella delle viste** (`view_def.h`, `visualizer_views.cpp`): ogni
       vista in un posto solo. Spostamento puro: 34 schermate su 35 identiche
       (la 35esima e' il cursore che lampeggia). I controlli "che vista e'"
       scendono da 102 a 60.
2. [x] **Un menu' aperto prende tutto** (`visualizer.cpp`, `OnUserUpdate`):
       mentre e' aperto nessun'altra parte riceve mouse o tasti, nemmeno il
       click che lo chiude. E' la stessa porta gia' usata da "Apri file".
       Provato: un click su una sezione a menu' aperto non la seleziona.
3. [x] **Strato sopra tutto**: il menu' si disegna con i decal, per ultimo.
4. [x] **Voce "Altro"** nelle tab (`popup_menu.h`, `menu_state.h`): contiene
       Byte Cloud e One Tuple, scelte da me come "usate meno" (si cambia con
       `inMenu` in `view_tabs.h`). I tasti 8 e 9 funzionano come prima.
       Le tab ora finiscono a x=1044: c'e' posto per una vista nuova con la
       sua tab, e quante se ne vuole nel menu'.
- [ ] Sottomenu': `PopupMenu` e' un elenco piatto. Per i sottomenu' serve una
      voce che ne apre un altro; la porta che blocca il resto c'e' gia'.
- [ ] Le opzioni lunghe (es. i filtri del Raw) potrebbero diventare un menu'.

**Controllo delle immagini senza farlo a mano** (2026-11-16):
- `make test` confronta 37 immagini delle viste con impronte salvate
  (`tests/test_golden.cpp`); se un'immagine cambia apposta: `make golden`.
- `make check-ui` confronta le schermate dell'app con l'ultimo commit e
  stampa una riga (circa 2 minuti e mezzo, serve lo schermo).
- Trovato dal test: nella Metric Map il colore **Spectrum e' identico a
  Wavelength** (stesse immagini). Da decidere: toglierlo o dargli un senso.

### Robustezza

- [ ] File da diversi GB: oggi il file va tutto in memoria (`BinaryReader`).
      Strada: `mmap` in sola lettura dietro la stessa interfaccia. Il test
      piu' grande finora e' da 100 MB.
- [x] Calcolo interrompibile (`cancel.h`): cambiando vista o aprendo un altro
      file, il calcolo in corso smette entro 64 KB di byte. (Le opzioni di una
      vista si possono cambiare solo a calcolo finito, come prima.)
      Su 100 MB, dal 3D a un'altra vista: da 3,5 s a 0,5 s. Se cambia solo la
      finestra il calcolo finisce: cosi' trascinando le barre la vista si
      aggiorna man mano.
- [ ] Parser ELF: leggere anche i program header. Oggi un ELF senza tabella
      delle sezioni risulta senza sezioni. Con test su file troncati, offset
      ostili e fuzzing, come ogni estensione del parser.
- [ ] Overlay nell'Inspector: dati dopo la fine dichiarata di un ELF o PE.

### Comodita'

- [ ] Trascinare un file sulla finestra: il codice c'e' (`GetDroppedFiles` in
      `visualizer.cpp`), ma il motore lo riceve **solo su Windows**. Mai
      provato: serve la versione nativa per Windows. Su Linux andrebbe scritto
      il protocollo XDND dentro il motore; sotto WSL trascinare da Esplora
      file a una finestra Linux probabilmente non arriva comunque.
- [ ] `S` non salva il 3D (li' e' il tasto per indietreggiare), ne' Stringhe
      e Hex.
- [ ] Stringhe: molte false sui dati casuali (164 mila su 100 MB con minimo
      6). Idea: filtro per entropia del tratto.

### Proposte da valutare

- [ ] Filtro Raw "differenza con spostamento k": generalizza delta (k=1) e
      delta riga (k=larghezza). Aggiunge poco.
- [ ] **Impronta spettrale**: lo spettro di una zona, per trovare i confini tra
      tipi di dati (tesi di Davis, cap. 5). Dopo lo Spettrogramma.
- [ ] Nel Raw, isolare un solo periodo e nascondere il resto ("Seeing Sound").
- [ ] *(bassa priorita')* **Forma d'onda**: byte come campioni audio con segno
      (8/16 bit), per riconoscere il PCM.
- [ ] Un tipo comune per stringhe e firme, solo se serve davvero. Vale per i
      risultati discreti: entropia e periodicita' sono curve, non elenchi.
- `ClassifierModel` e' collegato (cartella `templates/`, un `.bin` per classe)
  ma i template non ci sono: senza, il colore Classifier viene saltato.
  L'originale ne ha 22 (87 MB, Apache 2.0): da non mettere nel repo;
  eventualmente uno script che li scarica.

### Fuori dal progetto

- Import e capacita', protezioni (hardening), regole e IOC, tabella delle
  sezioni sospette: spostano l'idea da "guarda cosa c'e' nei byte" a
  "interpreta questo eseguibile"; esistono gia' capa, checksec, Detect It
  Easy e YARA.
- Digraph a strati in 3D, diff tra due file, disassembler, port su HTML,
  seconda finestra, librerie nuove.
- Miniature delle viste al posto delle tab: rimandato dall'utente.
- Dal Visual Microphone: piramide steerable, rolling shutter, motion
  magnification, Interactive Dynamic Video. Servono per i video ripresi.

## Limiti noti

- 3D su dati casuali da 100 MB: 16,7 milioni di punti, 267 MB. Calcolo 1,4 s
  senza sezioni, 3,9 s se le sezioni coprono tutto il file. Fotogramma 30 ms
  in movimento, 97 ms completo.
- 3D: la sezione "prevalente" e' esatta solo se una sezione ha piu' di meta'
  delle occorrenze; le triple a cavallo di due sezioni non vengono contate.
- La scelta di un punto nel 3D non ha un test automatico: dipende dalla
  finestra, va provata nell'app.
- Somiglianza: un file fatto di centinaia di copie mostra solo una fascia
  attorno alla diagonale (ogni sequenza si confronta con le 32 occorrenze
  dopo). Stringendo la finestra tornano le diagonali.
- `A` nel Raw: a 3 byte per pixel non si puo' sapere da quale byte inizia il
  pixel, quindi i colori possono restare scambiati (si corregge con OFFSET).
- Firme: solo l'inizio di un formato, non la fine. Nessuna firma sui pezzi
  senza intestazione. Non incluse perche' troppo corte: zlib, MP3, script.
- Tag delle stringhe: indizi (`1.2.3.4` puo' essere una versione).
- Tastiera provata solo su X11 (italiana e americana).
- `FileNavigator::moveForward/moveBackward` restano solo per le frecce
  commentate e per i test.

## Ricerca e riferimenti

**CantorDust originale.**
- Plugin Ghidra di Battelle: <https://github.com/Battelle/cantordust> (Java,
  Apache 2.0), letto il 2026-11-05. Idee da reimplementare; se copiamo dati o
  codice serve l'attribuzione richiesta dalla licenza.
- Demo del 2012: `_cantor.dust_.7z.zip` nella radice (non su git; Windows,
  .NET/XNA, solo vista 3D, senza sorgenti). Il suo file di prova e' in
  `dataset/visual_re.example`: pezzi di file diversi cuciti insieme.
- Slide di Domas, REcon 2013: confermano "Tuple systems / Coordinate
  Systems" e "Metric map", senza formule.
- Viste dell'originale note solo di nome (Adjacency Graph, Adjacency Bin,
  Moire, Sliding Histogram, Residue Contact Map, Locality Translation):
  **nessuna descrizione pubblica** (ricerca del 2026-11-15). Ipotesi dal
  nome, da non presentare come fedeli all'originale:
  - Locality Translation = la curva di Hilbert (c'e': Metric Map)
  - Sliding Histogram = istogramma per tratto (c'e': One Tuple)
  - Residue Contact Map = somiglianza del file con se stesso (c'e':
    Somiglianza, ma non e' provato che coincidano)
  - Adjacency Graph = grafo "byte seguito da byte"; Adjacency Bin = la sua
    matrice (sarebbe il Digraph)
  - Moire: ignoto; vedi sotto
- `hexcymatix` di Domas (2026) e l'avatar GitHub: note in `CLAUDE.md`.

**Formule usate.**
- 3D a cilindro e sfera: Veles, `trigram/coords.glsl` (codilime/veles).
- Somiglianza: Church e Helfman, "Dotplot: A Program for Exploring
  Self-Similarity in Millions of Lines of Text and Code" (1993). Gettoni di 4
  byte, peso 1/frequenza, cella(i) = i*256/N. Nostre differenze: gettoni
  campionati per contenuto (hash) e, per quelli comuni, confronto solo con le
  32 occorrenze successive invece di scartarli.
- Byte per pixel (`pixel_guess.*`): euristica nostra, nessuna letteratura
  trovata. punteggio(p) = 1 - media(mad[p,2p,3p]) / media(mad alle altre
  distanze), dove mad e' la differenza media dell'autocorrelazione.

**L'ipotesi "sono tutte operazioni su immagini" (2026-11-15).**
Dell'utente: Moire e le altre viste ignote sarebbero operazioni da immagini
applicate al file. Ricerca fatta, esito:
- Vero in parte. Tecniche da analisi forense di immagini che valgono su
  qualunque sequenza di byte: piani di bit (fatto), residuo di rumore = filtro
  passa-alto (i nostri delta), matrice di co-occorrenza (e' il Digraph),
  spettro (Spettrogramma, in piano), test chi-quadro sui bit bassi (Westfeld e
  Pfitzmann 2000; non fatto). Nataraj (2011) classifica malware trattando il
  file come immagine in scala di grigi.
- Falso per i nomi: "Residue Contact Map" viene dalla biologia (matrice 1 se
  due elementi distano meno di una soglia: e' un recurrence plot), "Adjacency
  Graph" dai grafi, "Sliding Histogram" dalla statistica. L'originale pescava
  da piu' campi: il filo comune e' la sequenza, non l'immagine.
- Limite: un file diventa immagine solo scelta la larghezza. Le operazioni che
  guardano i vicini in 2D (bordi, sfocature) hanno senso solo nel Raw con la
  larghezza giusta.
- Moire, ipotesi non verificata: e' l'interferenza tra due periodi, con
  frange di periodo p1*p2/|p1-p2|. Nel Raw si vede gia': con la larghezza
  sbagliata le righe diventano bande oblique. Sovrapporre il file a una sua
  copia spostata e' cio' che fa `hexcymatix` (differenza con spostamento).

**Visual Microphone** e tesi di Abe Davis, *Visual Vibration Analysis*: non la
tecnica sui video, ma il modo di leggere un segnale (spettrogramma, picchi,
immagine modale). <https://people.csail.mit.edu/mrub/VisualMic/>,
<https://abedavis.com/files/papers/thesis.pdf>

**File video: cosa aspettarsi.**
Previsioni dai formati, ancora da provare su file veri:
- non compresso (`.yuv`, `.y4m`, certi AVI): fotogrammi in fila, con periodo
  pari alla dimensione del fotogramma; si vedono in Raw e nello Spettrogramma
- MJPEG: tanti JPEG in fila, utile per le Firme
- MPEG-TS (`.ts`): pacchetti da 188 byte che iniziano con `0x47`, periodo netto
  anche se il contenuto e' compresso
- MP4 normale: quasi tutto rumore, tabelle di indici leggibili all'inizio o
  alla fine; i fotogrammi hanno dimensioni diverse, quindi nessun periodo

## Storico: cosa e' stato fatto

**Bug (fase 1)** — parte nei commit di settembre (306371c), parte a novembre:

- [x] Parser PE/ELF: overflow sugli offset letti dal file (lettura fuori
      buffer con `peOff` ostile), big-endian, sezioni NOBITS o fuori file
- [x] `BinaryReader`: file illeggibile/directory non distrugge più lo
      stato precedente; accetta solo file regolari
- [x] Navigator: finestra più grande del file con file < 256 byte;
      `selectRange()` per selezionare una sezione
- [x] `[ ]` e `C`/`V` ricalcolano sulla finestra, non sul file intero
- [x] Calcolo async solo per la vista attiva, niente copia del file intero
- [x] Overlay sezioni non più sul digraph; zona di drag invisibile della
      overview bar rimossa
- [x] In "Apri file" i tasti non agiscono più sulla vista; tasto `/`
- [x] Scroll delle tab, sprite/decal liberati, MetricMap con file piccoli,
      barra di stato con percorsi lunghi, nessun ricalcolo a vuoto
      tenendo premuto sulla barra di navigazione
- [x] Repo: binari non tracciati; Makefile incrementale; `make test`

**Raw pixel + autocorrelazione (fase 2.1)**:

- [x] Larghezza 1..2048 e offset variabili, altezza dalla finestra
      (righe campionate oltre 2048), viste 2D adattate all'area
- [x] Slider WIDTH/OFFSET con `<` `>`, tasti `,` `.` (Ctrl = offset,
      Shift = ×10), `A` = larghezza suggerita
- [x] Autocorrelazione (`autocorr.cpp`): grafico cliccabile, larghezza
      esatta o ±1 px sulle immagini di prova, nessun falso positivo sul
      rumore. Verificato: `gatto.jpeg` grezzo nascosto nel rumore compare.

**Interfaccia e viste (novembre 2026)**:

1. [x] **Layout 1280×800 + Inspector + legenda per vista**
       - [x] Inspector diviso: `visualizer_inspector.cpp` sceglie il pannello,
             i pannelli stanno in `_file`, `_value` e `_summary`
       - [x] layout in `constants.h` (NAV | legenda+vista | INSPECTOR)
       - [x] kit widget: `drawPanel`, `PanelWriter` (righe, campi,
             separatori, barre %) accanto allo Slider in `ui_widgets.h`
       - [x] Inspector (`visualizer_inspector.cpp`): PUNTO (RAW, MetricMap),
             TRATTO (DotPlot, Entropy), VALORE (Digraph, Histogram,
             ByteCloud, OneTuple) con conteggio e prima occorrenza;
             senza hover: riassunto file + finestra + sezioni in %
       - [x] `byte_info.h`: classe di byte, composizione, entropia,
             levigatezza e ipotesi ("testo", "zeri", "alta entropia",
             "valori lisci: immagine/audio raw?" — sul gatto 90-100%
             delle zone, su ls/python3/libc ~0%)
       - [x] legenda di una riga per ogni vista
       - [x] barra a sinistra: byte del file + finestra (cornice), inizio/
             fine/%, click = vai; tolta la barra di navigazione in basso
       - [x] fix: WASD del 3D (W andava indietro), vista 9 OneTuple che
             riempiva solo poche righe (ora 256 righe = tutta la finestra)
       - [x] Inspector per il 3D (passo 4)
2. [x] **Barre di navigazione con le bande delle sezioni**
       - [x] `nav_bars.cpp`: sezioni | file | zona; byte veri o entropia
             (`B` / tasto destro); maniglie, trascinamento, rotella
       - [x] logica in `range_sel.h`, testata in `make test`
       - [x] hover su una sezione: la vista attenua il resto (RAW, Metric
             Map, Dot Plot, Entropy), l'Inspector la descrive; click = la
             mostra. Mouse su una barra: Inspector su quel byte
       - [x] linee delle sezioni e `OverviewBar` rimosse (restano nella
             cronologia di git)
       - [x] frecce disattivate: codice commentato in `visualizer.cpp`,
             `moveForward`/`moveBackward` ancora in `FileNavigator`
       - [x] fix: i decal coprono i pixel normali, quindi la finestra
             "Apri file" e lo spinner restavano sotto la vista
3. [x] **Evidenziazione collegata**
       - [x] sezione → vista, in tutte le viste (`visualizer_highlight.cpp`):
             per posizione si attenua il resto; digraph e 3D: punti della
             sezione accesi, gli altri grigi; istogramma: quota della barra;
             byte cloud: valori soprattutto nella sezione
       - [x] vista → file (`visualizer_links.cpp`, `occurrences.cpp`): segni
             viola sulle barre, click = scorre le occorrenze ("3 di 120")
       - [x] dall'occorrenza scelta alla vista: e' il cursore (vedi sotto)
4. [x] **Esplorazione 3D**
       - [x] cubo e assi etichettati (`renderer3d_cube.cpp`): byte 1/2/3,
             00 e FF; camera in `camera3d.h`, ritaglio in `line_clip.h`
       - [x] fotogramma (`cloud_frame.cpp`): punti sommati, colore = media,
             luce = densita'; ogni pixel ricorda il punto piu' pesante
       - [x] picking: tripla, conteggio, sezione prevalente, occorrenze;
             click = scorre le occorrenze, segni sulle barre
       - [x] colore `C` (`cloud_color.cpp`): sezione, tipo di byte, frequenza,
             con legenda; sezione prevalente col voto di maggioranza
       - [x] costo: fotogramma ricalcolato solo se cambia qualcosa; oltre
             1,5 milioni di punti, in movimento se ne usa un campione
5. *(rimandato dall'utente, non riproporre)* Miniature delle viste a destra
       al posto delle tab.

6. [x] **Pulizia visiva**: palette con ruoli in `constants.h` (fondo neutro,
       testo, testo secondario, un solo accento per cio' che e' attivo; i
       colori vivaci solo sui dati), tab con i nomi interi, tolta la
       scrollbar delle tab
7. [x] **Riga di opzioni** (`option_bar.h`, `visualizer_options.cpp`) al
       posto della barra di stato, nelle viste 6 e 7:
       tutte le scelte visibili e cliccabili, quella attiva evidenziata
8. [x] **Viste e modi nuovi**: colore per tipo di byte nella Metric Map,
       filtri del Raw (piani di bit, delta, delta riga), `A` che trova anche
       la dimensione dei record (`autocorr_period.cpp`; negli ELF spesso 24)
9. [x] **Tab larghe quanto il testo**, ognuna con il suo tasto (`view_tabs.h`)
10. [x] **Stringhe**, con la lista scorrevole (`list_state.h`, `list_view.h`)
11. [x] **Firme**: 32 marcatori; quelli corti passano solo con l'intestazione
       plausibile. Su 100 MB casuali: 1 falso positivo, 0,4 s.
12. [x] **Cursore**: `goToOffset()`; riquadro in Raw e Metric Map, linea in
       Dot Plot, Entropia e One Tuple, byte evidenziato in Hex. Un click nella
       Somiglianza lo porta al tratto Y.
13. [x] **Hex**, **3D a cubo/cilindro/sfera**, **Somiglianza**, **`A` con i
       byte per pixel**
14. [x] **Barre, casi limite (2026-11-15)**: trascinando FILE la selezione di
       ZONA la segue (prima si accorciava fino a 16 byte); rotella col mouse
       fuori dalla selezione non la fa piu' saltare; `goToOffset` tiene la
       dimensione della finestra; le scrollbar della vista si trascinano
       anche uscendo dalla barra. Test a caso su 80.000 mosse.

Dal CantorDust originale sono venuti: barre `macro`/`micro` con maniglie ed
entropia, luminosita' col 90esimo percentile (`density.h`), click ripetuto per
l'occorrenza dopo, picking del 3D, colore Entropy della Metric Map.
