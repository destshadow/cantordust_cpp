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
   - RAW, MetricMap: un pixel = un punto del file → offset, byte, sezione
   - DotPlot, Entropy: un pixel = un tratto del file → range, sezione
   - Digraph, 3D, Histogram, ByteCloud: un pixel = un valore →
     quante volte compare e **dove** compare nel file
3. **Evidenziazione collegata** (brushing & linking):
   - *selezione → vista*: selezionando un range (sezione o tratto sulle
     barre) ogni vista mostra il contributo di quel range e attenua il
     resto (nel 3D/digraph: punti del range colorati, gli altri grigi)
   - *vista → file*: hover su un valore (coppia, tripla) → sulla barra
     del file compaiono i segni dove quel valore compare
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

Layout obiettivo:

```
┌─bar1─┬─bar2─┬────────── vista ──────────┬── INSPECTOR ──────┐
│▓.text│      │ X=byte i  Y=byte i+1      │ OFF  0x0004D373   │
│▓     │ ▲    │ luminosità=frequenza      │ SEZ  .text        │
│░.roda│ sel  │                           │ 48 89 E5 41 57 41 │
│░     │ ▼    │                           │ H..AWA            │
│▒.data│      │                           │ tipo: codice      │
│      │      │                           │ entropia: 6.1/8   │
│      │      │                           │──── sezioni ──────│
│      │      │                           │ ■ .text   45%     │
├──────┴──────┴───────────────────────────┤ ■ .rodata 20%     │
│ controlli della vista (slider, bpp...)  │──── viste ────────│
│ 0x121A5C - 0x16E238                     │ [1][2][3][4][5].. │
└─────────────────────────────────────────┴───────────────────┘
```

- **bar1**: tutti i byte del file in sequenza (verde = valore), con le
  bande delle sezioni e una selezione trascinabile (maniglie ▲/▼).
- **bar2**: solo il range selezionato in bar1, con una sua selezione che
  decide cosa va nella vista. Sostituisce la navigazione con le frecce.

## Fatto

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

## Prossimi passi (in ordine)


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
       - [ ] manca ancora: Dot Plot come valore (Y)
       - [ ] idea: dall'occorrenza scelta, portare la vista in quel punto
             (oggi si vede nell'Inspector e come riga bianca sulle barre)
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
5. [ ] **Miniature delle viste** a destra al posto delle tab,
       **riassunto del file** (formato, architettura, entry point,
       sezioni in %, entropia media), ritocchi grafici

6. [x] **Pulizia visiva**: palette con ruoli in `constants.h` (fondo neutro,
       testo, testo secondario, un solo accento per cio' che e' attivo; i
       colori vivaci solo sui dati), tab con i nomi interi, tolta la
       scrollbar delle tab
7. [x] **Riga di opzioni** (`option_bar.h`, `visualizer_options.cpp`) al
       posto della barra di stato, nelle viste 6 e 7:
       tutte le scelte visibili e cliccabili, quella attiva evidenziata

## Limiti noti (controllo del 2026-11-05)

- Un calcolo in corso non si può interrompere: su file grandi un cambio
  vista parte solo quando finisce quello prima.
- 3D su dati casuali da 100 MB: 16,7 milioni di punti, 267 MB. Calcolo 1,4 s
  senza sezioni, 3,9 s se le sezioni coprono tutto il file (voto di
  maggioranza). Fotogramma 30 ms in movimento, 97 ms completo.
- 3D: la sezione "prevalente" e' esatta solo se una sezione ha piu' di meta'
  delle occorrenze; le triple a cavallo di due sezioni non vengono contate.
- `S` non salva la vista 3D (nel 3D e' il tasto per indietreggiare).
- La scelta di un punto nel 3D (primo click, punto giusto con molti punti) non
  ha un test automatico: dipende dalla finestra, va provata nell'app.
- `FileNavigator::moveForward/moveBackward` restano solo per le frecce
  commentate e per i test.
- Per ThreadSanitizer su questo kernel: `setarch x86_64 -R ./programma`.

## Spunti dal CantorDust originale

Sorgente letto il 2026-11-05: <https://github.com/Battelle/cantordust>
(Java, Apache 2.0). Sono idee da reimplementare; se copiamo dati o codice
serve l'attribuzione richiesta dalla licenza.

- [x] **Barre**: `macro`/`micro`, maniglie, entropia nella striscia, e
  selezione che resta sugli stessi byte quando cambia la zona.
- [x] **Luminosità**: `density.h`, percentile p90 + curva logaritmica al
  posto del massimo, per digraph e dot plot; digraph verde come la GIF.
- [x] **Click**: ripetendo il click si passa all'occorrenza dopo.
- [x] **Passo 4, 3D**: buffer grande come la vista con il punto più pesante
  di ogni pixel (picking immediato), punti sommati per vedere la densità.
- [x] **Metric Map**: colore Entropy come default (è il viola/magenta
  della GIF) ed entropia locale su 32 byte come l'originale.

## Viste e modalità nuove (dopo)

1. [x] **Colorazione per tipo di byte** sulla Metric Map (`C`)
2. [x] **Bit-plane** come filtro dei raw pixel (`F`, `raw_filter.h`)
3. [x] **Delta** come filtro dei raw pixel: dal pixel prima e dalla riga
       sopra (quest'ultimo per le tabelle di record)
       - [x] `A` trova anche la dimensione dei record, non solo la riga di
             un'immagine (`autocorr_period.cpp`); negli ELF trova spesso 24,
             la dimensione delle tabelle di simboli e rilocazioni
4. [ ] *(bassa priorità)* **Forma d'onda**: byte come campioni audio con
       segno (8/16 bit), per riconoscere il PCM.

### Piano del 2026-11-07

Spunti dal Visual Microphone e dalla tesi di Abe Davis, *Visual Vibration
Analysis*: non la tecnica sui video, ma il modo di leggere un segnale.
<https://people.csail.mit.edu/mrub/VisualMic/>,
<https://abedavis.com/files/papers/thesis.pdf>

In ordine:
1. [x] **Tab larghe quanto il testo** (`view_tabs.h`); la decima vista usa il
       tasto `0`. Niente barra scorrevole: nasconde le viste.
2. [x] **Stringhe** (tab): ASCII e UTF-16, lunghezza minima, pochi tag (URL, IP,
       percorsi), con click per arrivarci. Costo medio.
       - [x] lista scorrevole (`list_state.h`, `list_view.h`)
       - [x] `goToOffset()` generico, senza passare da `Occur::Walker`: con un
             offset fuori dalla finestra la sposta (`RangeSel::reveal`)
       - [x] offset e lunghezza, non una `std::string` per stringa
       - [ ] portare anche la vista scelta su quel punto (oggi solo cursore
             sulle barre e dettagli nell'Inspector)
3. [ ] **Firme** (segni sulle barre, non una tab): cerca marcatori noti (PNG,
       JPEG, ZIP, PDF, MP4...) e trova i file dentro altri file. Riconosce solo
       l'inizio di un formato ("inizio rilevato"), non lo legge.
4. [ ] **Spettrogramma** (tab): X = posizione nel file, Y = periodo in byte,
       luce = forza del periodo. Mostra dove cambia la struttura. FFT scritta
       da noi, in un file piccolo. Provarla su un WAV e su record noti.
       - [ ] picchi: i periodi piu' forti del file, cliccabili
       - [ ] click su un periodo: dove compare, con i segni sulle barre
             (l'"immagine modale" della tesi, cap. 3)
       - [ ] togliere lo spettro medio, per far risaltare le zone diverse
5. [ ] **Panoramica** (tab; "indicatori", non "triage"): X = posizione nel file,
       ogni riga un indicatore (entropia, salti di entropia, testo, zeri,
       periodicita', firme, dati fuori dalle sezioni). Click: seleziona il
       tratto sulle barre.
6. [ ] **Profilo record** (tab): dato un periodo, mostra colonna per colonna i
       campi fissi, i contatori e quelli casuali. Conteggio di valori per
       posizione nel record.

Quando le tab non ci stanno piu' (circa 12): voce **"Altro ▾"** in fondo, con le
viste usate meno. Le altre restano sempre visibili.

Annotazioni, a parte dall'ordine:
- [ ] Overlay nell'Inspector: dati dopo la fine dichiarata di un ELF o PE.
- [ ] Parser ELF: leggere anche i program header. Oggi un ELF senza tabella
      delle sezioni risulta senza sezioni. Con test su file troncati, offset
      ostili e fuzzing, come ogni estensione del parser.

Da valutare dopo:
- [ ] Un tipo comune per stringhe e firme, solo quando ci sono entrambe e si
      vede cosa hanno in comune. Vale per i risultati discreti: entropia e
      periodicita' sono curve, non elenchi.
- [ ] **Impronta spettrale**: lo spettro di una zona, per trovare i confini tra
      tipi di dati senza conoscere il formato (tesi, cap. 5).
- [ ] Nel Raw, isolare un solo periodo e nascondere il resto ("Seeing Sound").

Fuori dal progetto: import e capacita', protezioni (hardening), regole e IOC,
tabella delle sezioni sospette. Spostano l'idea da "guarda cosa c'e' nei byte"
a "interpreta questo eseguibile"; esistono gia' capa, checksec, Detect It Easy
e YARA.

Scartato dai lavori del Visual Microphone: piramide steerable, rolling shutter,
motion magnification e Interactive Dynamic Video (cap. 6) servono per i video
ripresi, non per i byte.

### File video: cosa aspettarsi

Previsioni dai formati, ancora da provare su file veri:
- non compresso (`.yuv`, `.y4m`, certi AVI): fotogrammi in fila, con periodo
  pari alla dimensione del fotogramma; si vedono in Raw e nello Spettrogramma
- MJPEG: tanti JPEG in fila, utile per le Firme
- MPEG-TS (`.ts`): pacchetti da 188 byte che iniziano con `0x47`, periodo netto
  anche se il contenuto e' compresso
- MP4 normale: quasi tutto rumore, tabelle di indici leggibili all'inizio o
  alla fine; i fotogrammi hanno dimensioni diverse, quindi nessun periodo

Limite: il file viene caricato tutto in memoria, quindi un video da diversi GB
e' un problema. Il test piu' grande finora e' da 100 MB.

Scartate per ora (appesantirebbero il progetto): digraph a strati in 3D,
diff tra due file, disassembler o parsing dei formati, port su HTML,
seconda finestra.

## Da decidere più avanti

- `ClassifierModel` è collegato (cartella `templates/`, un `.bin` per
  classe) ma i template non ci sono: senza, la modalità Classifier viene
  saltata. L'originale ne ha 22 (87 MB, Apache 2.0): rimandato, da non
  mettere nel repo; eventualmente uno script che li scarica. Con quelli
  l'Inspector potrebbe dire "probabile codice x86 / ARM / testo".
- Cartelle `python/` e `dataset/` vuote (rete neurale WIP).
