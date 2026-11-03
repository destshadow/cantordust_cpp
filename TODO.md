# CantorDust++ — obiettivi e appunti

## Obiettivo

Replicare il comportamento del CantorDust originale mostrato in
`supermario-resized-gif.gif` (e `cantordustlaunch-png.png`), con tutto
interattivo. Il progetto deve restare **semplice**: olcPixelGameEngine +
stb, nessuna dipendenza nuova, codice leggibile.

Layout di riferimento (dalla GIF):

```
┌──────┬──────┬──────────────────────────────┬─────┐
│ bar1 │ bar2 │                              │ [1] │
│ file │ zoom │        vista principale      │ [2] │
│intero│ di   │        (grande, quadrata)    │ [3] │
│      │ bar1 │                              │ [4] │
│ ▲▼   │ ▲▼   │                              │ [5] │
└──────┴──────┴──────────────────────────────┴─────┘
  start - end (hex)     [<][>]───●─── [<][>]─●────
                          larghezza     offset
```

- **bar1**: tutti i byte del file, in sequenza riga per riga (verde =
  valore del byte), con una selezione trascinabile (maniglie ▲/▼).
- **bar2**: mostra solo il range selezionato in bar1; ha a sua volta una
  selezione, che decide cosa va nella vista centrale.
- **miniature a destra**: cliccabili, cambiano vista.
- **slider in basso**: larghezza (stride) e offset fine per i raw pixel.
  Il "momento Mario": a 24bpp con la larghezza giusta compare l'immagine.

## Fase 1 — bug (fatto)

Parte nei commit di settembre (306371c), parte nella revisione di novembre.

- [x] Parser PE/ELF: overflow sugli offset letti dal file (lettura fuori
      buffer con `peOff` ostile), big-endian, sezioni NOBITS o fuori file
- [x] `BinaryReader`: file illeggibile/directory non distrugge più lo
      stato precedente (anche il reader ora accetta solo file regolari)
- [x] Navigator: finestra più grande del file con file < 256 byte;
      `selectRange()` per selezionare una sezione
- [x] `[ ]` e `C`/`V` ricalcolano sulla finestra, non sul file intero
- [x] Calcolo async solo per la vista attiva, niente copia del file intero
- [x] Overlay sezioni solo sulle viste con un asse offset (DotPlot,
      Entropy, RawPixels), relativo alla finestra; niente più linee
      senza senso sul digraph
- [x] Zona di drag della overview bar attiva ma invisibile sopra il
      canvas: rimossa (torna con le barre laterali, fase 2.2)
- [x] In "Apri file" i tasti (frecce, C, V, WASD...) non agiscono più
      sulla vista; aggiunto il tasto `/`
- [x] Scroll delle tab passato alla UI; sprite/decal liberati all'uscita
- [x] MetricMap: file piccoli non ripetono l'ultimo byte; info byte
      assoluta; entropia locale normalizzata correttamente
- [x] Repo: binari e `:Zone.Identifier` non più tracciati; Makefile
      incrementale (`build/*.o`); `make test` con test di regressione

- [x] Tenendo premuto sulla barra di navigazione non si ricalcola più a
      ogni frame se la finestra non cambia
- [x] (verificato: non era una race — il thread UI legge solo `m_ready`,
      che `classifyFile` non tocca)

## Fase 2 — cosa manca (in ordine)

1. [x] **Raw pixel con larghezza e offset variabili**: sprite di
       dimensione variabile (fino a 2048×2048, righe campionate oltre),
       adattato all'area; slider WIDTH/OFFSET con `<` `>` (Shift = ±10),
       tasti `,` `.` (Ctrl = offset). Le viste 256×256 ora riempiono
       l'area. Verificato: `gatto.jpeg` in RGB grezzo nascosto nel rumore
       compare con W=317 e l'offset giusto.
2. [ ] **Le due barre a sinistra** (bar1 → bar2 → vista), con i byte veri
       e selezione trascinabile a due livelli. `OverviewBar` attuale è da
       rifare (oggi disegna un istogramma per riga e non è collegata).
3. [ ] **Nuovo layout**: miniature cliccabili a destra al posto delle tab,
       etichetta `start - end` in basso.
4. [ ] Ritocchi grafici (palette digraph come l'originale, ecc.).

## Fase 3 — viste e modalità nuove (in ordine)

Restare leggeri: una vista nuova = un file piccolo con `compute()` /
`toRGB()` come le altre; dove possibile meglio un'opzione su una vista
esistente che una vista in più.

1. [x] **Autocorrelazione / stride finder** — fatto con la fase 2.1
       (`autocorr.cpp`, pannello sotto i raw pixel, `A` = applica,
       click sul grafico = usa quella larghezza; test in `make test`).
       Precisione: esatta o ±1 pixel; sul rumore puro non suggerisce nulla. Per ogni distanza k (1..~4096) conta quante volte
       `byte[i] == byte[i+k]` e la disegna come grafico: i picchi sono la
       dimensione dei record o la larghezza di un'immagine. Click su un
       picco → imposta lo slider larghezza dei raw pixel.
2. [ ] **Hex view della selezione** — pannello offset | hex | ASCII dei
       byte sotto il mouse o della zona selezionata (`DrawString`).
       Collega i pattern visivi ai byte veri.
3. [ ] **Colorazione per classe di byte** sulla MetricMap (nuova
       `ColorMode`, stile binvis.io): `0x00` nero, `0xFF` bianco, ASCII
       stampabile blu, controllo verde, alti rosso. Funziona subito,
       senza template del classifier.
4. [ ] **Bit-plane** come opzione dei raw pixel: mostra un solo bit
       (0..7) di ogni byte in bianco/nero, con un tasto per scegliere il
       bit. Utile per steganografia e struttura.
5. [ ] **Delta** come opzione dei raw pixel: `byte[i] - byte[i-1]`.
       Audio/immagini non compressi diventano quasi piatti, i dati
       compressi o cifrati restano rumore.
6. [ ] *(bassa priorità)* **Forma d'onda**: byte come campioni audio con
       segno (8/16 bit) in un grafico a linea, per riconoscere il PCM.

Scartate per ora (appesantirebbero il progetto): digraph a strati in 3D
(la vista 3D copre quasi tutto), diff tra due file, disassembler o
parsing dei formati.

## Da decidere più avanti

- `ClassifierModel` è collegato (cartella `templates/`, un `.bin` per
  classe) ma i template non esistono ancora: senza, la modalità
  Classifier viene saltata.
- Cartelle `python/` e `dataset/` vuote (rete neurale WIP).
- Le viste sono tutte 256×256: va bene per digraph/istogramma, i raw
  pixel invece avranno dimensione variabile (fase 2.1).
