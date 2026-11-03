# CantorDust++ — obiettivi e appunti

## Obiettivo

Uno strumento per **esplorare** file binari e trovare strutture dati
nascoste o inattese (immagini, tabelle, record, testo, codice) — che
funzioni su qualunque file, non solo su formati noti. Riferimento di
partenza: il CantorDust originale in `supermario-resized-gif.gif`.

Il progetto resta **semplice e "batterie incluse"**: solo
olcPixelGameEngine + stb (single header), nessuna dipendenza nuova.
Quello che manca ce lo scriviamo noi (kit di widget in `ui_widgets.h`).

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

1. [ ] **Layout 1280×800 + Inspector + legenda per vista**
       - kit widget minimo in `ui_widgets.h` (pannello, etichetta,
         tabella, pulsante; lo Slider c'è già)
       - Inspector generico + specifico per tipo di vista (vedi principi)
       - assorbe la vecchia "hex view"
2. [ ] **Barre di navigazione bar1/bar2 con le bande delle sezioni**
       - byte veri, selezione con maniglie, rotella sulla barra
       - hover/click su una sezione → evidenziazione nella vista
       - sostituisce frecce e linee delle sezioni
       - `OverviewBar` attuale da rifare
3. [ ] **Evidenziazione collegata**
       - selezione → vista: range attivo acceso, resto attenuato
       - vista → file: hover su coppia/tripla → segni sulla barra
4. [ ] **Esplorazione 3D**
       - cubo e assi etichettati (byte 1/2/3, angoli 0x00/0xFF)
       - picking del punto sotto il mouse → tripla, conteggio, dove
         compare, sezione; click → vai nel file
       - colore per sezione o per selezione (con legenda) al posto
         del colore attuale, che non comunica nulla
5. [ ] **Miniature delle viste** a destra al posto delle tab,
       **riassunto del file** (formato, architettura, entry point,
       sezioni in %, entropia media), ritocchi grafici

## Viste e modalità nuove (dopo)

1. [ ] **Colorazione per classe di byte** sulla MetricMap (nuova
       `ColorMode`, stile binvis.io): `0x00` nero, `0xFF` bianco, ASCII
       stampabile blu, controllo verde, alti rosso. Utile anche
       all'Inspector ("tipo di byte").
2. [ ] **Bit-plane** come opzione dei raw pixel: un solo bit (0..7) per
       byte in bianco/nero. Utile per steganografia e struttura.
3. [ ] **Delta** come opzione dei raw pixel: `byte[i] - byte[i-1]`.
       Audio/immagini non compressi diventano quasi piatti.
4. [ ] *(bassa priorità)* **Forma d'onda**: byte come campioni audio con
       segno (8/16 bit), per riconoscere il PCM.

Scartate per ora (appesantirebbero il progetto): digraph a strati in 3D,
diff tra due file, disassembler o parsing dei formati, port su HTML,
seconda finestra.

## Da decidere più avanti

- `ClassifierModel` è collegato (cartella `templates/`, un `.bin` per
  classe) ma i template non esistono ancora: senza, la modalità
  Classifier viene saltata. Se ci fossero, l'Inspector potrebbe dire
  "probabile codice x86 / ARM / testo / compresso".
- Cartelle `python/` e `dataset/` vuote (rete neurale WIP).
