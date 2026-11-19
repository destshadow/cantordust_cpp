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

**Meta (decisa il 2026-11-18): esploratore universale "spendibile e che fa la
sua figura"**, non una ricostruzione fedele dell'originale. Le funzioni si
scelgono per la domanda a cui rispondono su un file sconosciuto (cosa c'e',
dove cambia, cosa si ripete, che periodo ha, com'e' fatto un record); le viste
dell'originale entrano solo dove coincidono. La versione Windows nativa non e'
una priorita'. In futuro l'utente potrebbe provare a venderlo: licenze, dati
di terzi e nome contano (vedi "Se un giorno si vende").

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
| G | Spettrogramma: periodi lungo il file (menu' "Altro") | `fft.*`, `spectrogram*.cpp`, `visualizer_spectrogram.cpp` |
| K | Profilo record: com'e' fatto un record (menu' "Altro") | `record_profile.*`, `record_field.cpp`, `visualizer_record.cpp` |

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
2. [x] **Spettrogramma** (tasto `G`, menu' "Altro"; `fft.*`, `spectrogram*.cpp`,
       `visualizer_spectrogram.cpp`), 2026-11-18: X = posizione, Y = periodo
       (log, da 2 byte a meta' della FFT), luce = potenza rispetto alla media
       del tratto. FFT nostra (radice 2), Hann, 256 FFT per finestra: 8 ms su
       100 MB (81 ms con 8192 byte). Periodo DI BASE con la somma dei
       sottoarmonici di Hermes (1988), poi l'intero esatto contando i byte
       uguali a distanza p. Click = cursore e Raw largo quel periodo.
       Provato: record da 24 e 40 byte, WAV (Alarm01: le note e le righe di
       2 e 4 byte del 16 bit stereo), il file dimostrativo.
       - [ ] picchi: i periodi piu' forti del file, cliccabili
       - [ ] click su un periodo: dove compare, con i segni sulle barre
             (l'"immagine modale" della tesi di Davis, cap. 3)
3. [x] **Profilo record** (tasto `K`, menu' "Altro"; `record_profile.*`,
       `record_field.cpp`, `visualizer_record.cpp`), 2026-11-19: X = byte nel
       record, Y = valore, luce = quante volte, colore = tipo di campo
       (fisso, contatore, testo, pochi valori, vario, casuale; soglie in
       `record_field.cpp`). Oltre 65536 record si campiona. Lo Spettrogramma
       ci porta: il click seleziona come ZONA il tratto dove il periodo dura
       e passa il periodo di base anche se si clicca un'armonica.
       - [ ] inizio del record automatico: oggi il disegno puo' uscire
             "ruotato" (si sistema con `[ ]`). Idea: dopo la corsa piu'
             lunga di colonne fisse uguali (il riempimento), o prima di un
             contatore. Da provare su formati veri prima di sceglierla.
       - [ ] campi su piu' byte (un contatore a 16/32 bit oggi e' un
             contatore + colonne "pochi valori")
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
- Trovato dal test: nella Metric Map il colore **Spectrum era identico a
  Wavelength** (stesse immagini). Sostituito (2026-11-18) da **Frequenza**:
  ogni byte colorato per quanto e' comune nella zona, su scala logaritmica
  dal piu' comune (blu) al piu' raro (rosso).

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

### Se un giorno si vende

Deciso dall'utente il 2026-11-18, se ne prende lui la responsabilita': il
repository resta privato e non sara' mai pubblico; il nome si sceglie dopo;
i campioni del Classifier li faremo noi. Restano come promemoria:

- olcPixelGameEngine: licenza OLC-3 (tipo BSD), uso commerciale permesso
  tenendo l'avviso di copyright. stb: pubblico dominio o MIT.
- Il nostro codice e' MIT: chiunque potrebbe rivenderlo. Se si vuole
  venderlo, decidere la licenza prima di pubblicare altro.
- Template del Classifier: quelli di Battelle sono Apache 2.0 (serve
  l'avviso), ma contengono byte di programmi di terzi; quelli PHP vengono da
  PHPMailer (LGPL) e Slim (MIT). Oggi non sono nel repository: non
  distribuirli senza aver controllato; meglio campioni fatti da noi.
- Il nome "CantorDust" e' di Domas e Battelle: per un prodotto serve un nome
  nostro.
- Idee prese da Battelle (Apache 2.0) e da articoli: citarle in un NOTICE.

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
  senza intestazione. Non incluse perche' troppo corte: zlib, MP3. Gli script
  si trovano solo dal marcatore (`#!/...`, `<?php`, `<script`, `<html`);
  senza marcatore li cerca il Classifier.
- Classifier (misurato il 2026-11-18, finestre di 64 byte mai viste):
  x86/x64/MIPS/SPARC/PowerPC/testo 92-100%; JavaScript 92%, Python 85%,
  HTML 82%, PHP 97%; ARM a 32 bit (arm7) 47%, bytecode Java 74%.
  Compresso, PNG e immagine si confondono fra loro (sono tutti quasi
  casuali). Su cio' che non somiglia a nessun campione l'etichetta e' la
  meno lontana, non "sconosciuto". 108 MB di memoria (2^20 secchi x 27).
  I cinque campioni a 64 bit di Battelle sono solo modelli: non si puo'
  tenere da parte un pezzo per misurarli, quindi non sono misurati.
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
  - Moire: non e' una vista, e' un font (vedi sotto, "Cosa c'e' davvero")
- `hexcymatix` di Domas (2026) e l'avatar GitHub: note in `CLAUDE.md`.

**Cosa c'e' davvero nella demo del 2012 (letto nell'eseguibile, 2026-11-18).**
Metodo: stringhe .NET in ordine (il codice generato da Visual Studio scrive
per ogni controllo immagine, nome, testo), tabella delle risorse .NET, le 12
icone da 30x30 dei pulsanti. Il codice e' offuscato (Dotfuscator): niente
formule. La demo implementa solo il 3-tuple; gli altri pulsanti dicono
"Come see the complete set".
- **Moire NON e' una vista: e' un font** (Moire, Microsoft, di Jim Ford). La
  stringa e' nella finestra About: `"Moire"` (font) → `labelLoading` (nome)
  → `"..cantor.dust.."` (testo) → `label1` → `"v0.0.00001a"`.
- La About dice: *"with research from greg conti and aldo cortesi"*. Conti e
  Dean, Black Hat 2008 ("Visual Forensic Analysis", VizSEC 2008): Bitplot,
  BytePlot, RGBPlot, BytePresence, ByteFrequency, Digram, Dotplot, Dynamic
  DotPlot (solo una finestra di 500x500 lungo la diagonale). Cortesi: curve
  di Hilbert e colore per entropia (binvis).
- Pulsante (nome interno → scritta → com'e' l'icona → ipotesi):
  - Presence → **Locality Translation** → verde con zone nere a triangolo →
    combacia con il **Byte Presence** di Conti: righe = tratti del file,
    colonne = 256 valori, acceso se il valore c'e' nel tratto. Il nome
    "Locality Translation" resta strano: forse scritta e pulsante scambiati.
  - Bit → **Bit Map** → trattini su una griglia → **Bit Plot** di Conti, un
    pixel per bit
  - BytePlot → **Metric Map** → rumore azzurro → byteplot lungo la curva
    (nell'eseguibile ci sono tabelle `hilbert_2d_1d`)
  - Frequency → **Histogram** → istogramma rosso → c'e'
  - SlidingHistogram → **Sliding Histogram** → grigio a grana, piu' chiaro in
    alto → istogramma per tratto in luminosita' (come One Tuple)
  - ContactMap → **Residue Contact Map** → curve e anelli verdi su nero →
    ipotesi: recurrence plot (|b_i - b_j| < soglia) su una finestra, che su
    segnali lisci fa proprio curve; il Dot Plot esatto fa solo diagonali
  - AdjacencyGraph → **Adjacency Graph** → nodi luminosi blu uniti da linee
    dritte → ipotesi: punti = coppie di byte, una linea da ogni coppia alla
    successiva; i nodi luminosi sono gli stati frequenti
  - AdjacencyBin → **Adjacency Bin** → fasci curvi grigi → ipotesi: le stesse
    linee accumulate in una griglia (densita'), non disegnate una per una
  - 2TupleSystem, 3TupleSystem, Cloud, Hex: ci sono gia'
- Altro: modi di colore del 3D `Time`/`Space`, effetti di animazione dei punti
  (Wiggle, Wind, Shatter, Tornado, Explode, RiseExplode, Bounce,
  ParametricBoredom), `GenerateExcludedFrequency` (frequenza escludendo
  certi valori, es. lo zero), "Open as Raw".

**Ricerca sulle viste ignote (2026-11-18).** Nessuna fonte usa i nomi della
demo per i binari; queste sono le tecniche piu' vicine.
- *Moire come tecnica*: due schemi periodici sovrapposti danno frange di
  periodo ingrandito, p1*p2/|p1-p2|; lo spettro della sovrapposizione mostra
  i periodi (Amidror e Hersch, EPFL). Nei nostri dati nasce da solo quando
  si campiona (Metric Map su file grandi: un byte ogni `step`) o con la
  larghezza sbagliata nel Raw: e' aliasing. Una vista "moire" voluta:
  sovrapporre il file a una copia spostata di k byte (o disegnata con
  larghezza W e W+1) e far vedere dove le frange si fermano = dove il
  periodo coincide. E' parente di `hexcymatix` (diff con copia spostata) e
  dell'autocorrelazione del Raw. Nella demo pero' "Moire" e' solo il font.
- *Residue Contact Map*: in biologia, matrice simmetrica 1/0 delle coppie
  di residui piu' vicine di una soglia (6-12 A). Per una sequenza e' il
  recurrence plot di Eckmann, Kamphorst e Ruelle (1987):
  R(i,j) = 1 se |x_i - x_j| <= eps. Con eps = 0 e' il nostro Dot Plot dei
  valori; con eps > 0 vede le ripetizioni "quasi uguali".
- *Byte Presence*: solo nominato da Conti e Dean (slide BH 2008: immagine
  verde con buchi neri). Parenti documentati: "unique bytes in a sliding
  window" (R-bloggers 2013: HTML ~15% di valori distinti per 256 byte, tgz
  molto di piu'), la "Layered Distribution" di ImHex (una riga di densita'
  per pezzo di file).
- *Sliding Histogram*: nessuna fonte con questo nome; la Layered
  Distribution di ImHex e' la stessa idea (istogramma impilato lungo il
  file), cioe' il nostro One Tuple.
- *Adjacency Graph*: nessuna fonte con byte come nodi; il piu' vicino e' il
  "Markov byte plot" (Donahue, Paturi, Mukkamala 2009) per il malware
  impacchettato: matrice delle transizioni, come il Digraph.
Fonti: Conti e Dean BH 2008; Amidror-Hersch (lspwww.epfl.ch/publications/
moire); en.wikipedia.org/wiki/Recurrence_plot; r-bloggers.com "Unique bytes
in a sliding window"; docs.werwolv.net/imhex/views/data-information.

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
- (Superata: Moire e' un font, vedi sopra.) Moire, ipotesi: e' l'interferenza tra due periodi, con
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

15. [x] **Classifier funzionante (2026-11-18)**: campioni di Battelle
       (`tools/get_templates.sh`) letti nei loro tre formati
       (`template_file.*`: prima si allenava anche sul modello serializzato
       in coda ai file), piu' 5 classi di script dai file della macchina
       (`tools/make_templates.sh`). Modello riscritto come quello di Battelle
       (secchi fissi, logaritmi, LAMBDA fisso), contesto di 64 byte, un
       calcolo per pixel invece che per blocco, annullabile, caricato in un
       thread. Le classi "rumore" (compresso, PNG) vincono solo sopra 5 bit
       di entropia su 64 byte. Riga SOMIGLIA nell'Inspector. Firme per gli
       script. Al posto di Spectrum (identico a Wavelength) c'e' Frequenza.

Dal CantorDust originale sono venuti: barre `macro`/`micro` con maniglie ed
entropia, luminosita' col 90esimo percentile (`density.h`), click ripetuto per
l'occorrenza dopo, picking del 3D, colore Entropy della Metric Map.
