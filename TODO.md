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

Ancora aperto (minore):
- [ ] Tenendo premuto sulla barra di navigazione si ricalcola a ogni
      frame anche se la finestra non cambia
- [ ] `m_classifier` viene modificato dal thread di calcolo mentre il
      thread UI legge `isReady()` (race innocua ma da sistemare)

## Fase 2 — cosa manca (in ordine)

1. [ ] **Raw pixel con larghezza e offset variabili**: sprite di
       dimensione variabile, scalato nella vista centrale; slider
       larghezza/offset con pulsanti `<` `>`. Test: file con rumore +
       `gatto.jpeg` convertito in RGB grezzo in mezzo.
2. [ ] **Le due barre a sinistra** (bar1 → bar2 → vista), con i byte veri
       e selezione trascinabile a due livelli. `OverviewBar` attuale è da
       rifare (oggi disegna un istogramma per riga e non è collegata).
3. [ ] **Nuovo layout**: miniature cliccabili a destra al posto delle tab,
       etichetta `start - end` in basso.
4. [ ] Ritocchi grafici (palette digraph come l'originale, ecc.).

## Da decidere più avanti

- `ClassifierModel` è collegato (cartella `templates/`, un `.bin` per
  classe) ma i template non esistono ancora: senza, la modalità
  Classifier viene saltata.
- Cartelle `python/` e `dataset/` vuote (rete neurale WIP).
- Le viste sono tutte 256×256: va bene per digraph/istogramma, i raw
  pixel invece avranno dimensione variabile (fase 2.1).
