# CantorDust++

Implementazione autonoma in C++ ispirata al lavoro originale di [Christopher Domas](https://github.com/Battelle/cantordust).

Visualizza file binari come immagini per identificare pattern, sezioni e anomalie a colpo d'occhio.

![CantorDust++](test_digraph.png)

Obiettivo e stato dei lavori: vedi [TODO.md](TODO.md).
Tutti i tasti e le aree della finestra: vedi [COMANDI.txt](COMANDI.txt).

---

## Visualizzazioni

| Tasto | Vista | Descrizione |
|-------|-------|-------------|
| `1` | Digraph | Coppie di byte consecutive su piano 2D |
| `2` | Dot Plot | Offset vs valore byte |
| `3` | Entropy | Entropia di Shannon per sezione |
| `4` | Histogram | Frequenza di ciascun valore byte (0–255) |
| `5` | 3D View | Triple di byte nello spazio 3D |
| `6` | RawPixels | Interpretazione dei byte come pixel, BPP con `[` e `]` |
| `7` | MetricMap | Curve Hilbert, Z-order e lineare (`V`); colori con `C` |
| `8` | ByteCloud | Frequenze dei byte in una griglia esadecimale |
| `9` | OneTuple | Distribuzione dei byte per gruppi consecutivi |

---

## Requisiti

### Linux / WSL2
```bash
# Ubuntu / Debian
sudo apt install build-essential g++ libglu1-mesa-dev libpng-dev libx11-dev python3 python3-pip python3-venv wget
```

### WSL2 su Windows 11
WSLg è necessario per il rendering grafico (incluso in Windows 11).

---

## Installazione rapida
```bash
git clone git@github.com:destshadow/cantordust_cpp.git
cd cantordust_cpp
./setup.sh
```

---

## Build manuale
```bash
# Scarica le librerie
cd libs
wget https://raw.githubusercontent.com/OneLoneCoder/olcPixelGameEngine/master/olcPixelGameEngine.h
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
cd ..

# Compila (incrementale, oggetti in build/)
make -j

# Avvia
./cantordust /path/to/binary
```

---

## Interfaccia

- **Sinistra**: sezioni e barre di navigazione (maniglie, trascinamento,
  rotella): decidono quale parte del file finisce nella vista.
- **Centro**: la vista, con una riga di legenda che spiega gli assi.
- **Destra — Inspector**: cosa c'è sotto il mouse (offset, sezione, byte
  in hex/ASCII, composizione, entropia, ipotesi sul contenuto); per le
  viste "per valore" quante volte compare e dove la prima volta. Senza
  mouse sulla vista: riassunto del file e della finestra.

## Controlli

### Generale
| Tasto | Azione |
|-------|--------|
| `1-9` | Cambia vista |
| `O` | Apri file |
| `S` | Salva PNG |
| `B` | Barre: valore dei byte / entropia |
| `Home` | Reset (file intero) |

### Viste 2D (tutte tranne 5)
| Azione | Effetto |
|--------|---------|
| Rotella mouse | Zoom in/out |
| Drag sinistro | Pan (con zoom > 1x) |

### Raw Pixels (6)
| Azione | Effetto |
|--------|---------|
| `[` `]` | Cambia bpp (8/16/24/32/64) |
| `,` `.` | Larghezza −1 / +1 (Shift = ±10) |
| `Ctrl` + `,` `.` | Offset −1 / +1 |
| `A` | Applica la larghezza suggerita dall'autocorrelazione |
| Slider WIDTH / OFFSET | Click, trascina, pulsanti `<` `>` |
| Click sul grafico AUTOCORR | Usa quella larghezza |

### Vista 3D (5)
| Azione | Effetto |
|--------|---------|
| Drag sinistro | Ruota |
| Drag destro | Pan camera |
| `WASD` | Muovi camera |
| `Q/E` | Su/Giù |
| Rotella | Zoom |
| `R` | Reset camera |
| `C` | Colore: sezione, tipo di byte, frequenza |
| Mouse su un punto | Tripla, conteggio, dove compare |
| Click su un punto | Scorre le occorrenze |

---

## Struttura progetto
```
cantordust_cpp/
├── libs/                    # Single-header libraries
│   ├── olcPixelGameEngine.h
│   ├── stb_image_write.h
│   └── stb_image.h
├── src/
│   ├── main.cpp
│   ├── binary_reader.{h,cpp}      # Lettura file binario
│   ├── binary_format{,_elf,_pe}.cpp  # Parser ELF/PE
│   ├── digraph.{h,cpp}            # Visualizzazione digraph
│   ├── dotplot.{h,cpp}            # Dot plot
│   ├── entropy.{h,cpp}            # Entropia di Shannon
│   ├── histogram.{h,cpp}          # Istogramma byte
│   ├── trigraph.{h,cpp}           # Nuvola punti 3D
│   ├── renderer3d.{h,cpp}         # Renderer 3D software
│   ├── renderer3d_cube.cpp        # Cubo, assi e segni nel 3D
│   ├── camera3d.h, line_clip.h    # Proiezione e ritaglio delle linee
│   ├── cloud_frame.{h,cpp}        # Fotogramma: densità e picking
│   ├── cloud_color.{h,cpp}        # Colori della nuvola (tasto C)
│   ├── file_navigator.{h,cpp}     # Navigazione nel file
│   ├── screenshot.{h,cpp}         # Export PNG (stb)
│   ├── input_handler.{h,cpp}      # Gestione input
│   ├── ui_renderer.{h,cpp}        # Rendering UI
│   ├── constants.h                # Layout e costanti
│   ├── visualizer.{h,cpp}         # Orchestrazione principale
│   ├── visualizer_2d.cpp          # Area della vista, zoom, scorrimento
│   ├── visualizer_canvas.cpp      # Vista nello sprite, salvataggio PNG
│   ├── visualizer_status.cpp      # Spinner, tab, barre di stato
│   ├── visualizer_options.cpp     # Riga di opzioni, viste 6 e 7
│   ├── visualizer_3d.cpp          # Input 3D
│   ├── visualizer_3d_draw.cpp     # Disegno e legenda 3D
│   ├── visualizer_nav.cpp         # Navigazione + threading
│   ├── visualizer_raw.cpp         # Pannello raw pixel: slider + autocorrelazione
│   ├── autocorr.{h,cpp}           # Autocorrelazione → larghezza immagini
│   ├── visualizer_inspector*.cpp  # Inspector: scelta del pannello e pannelli
│   ├── inspector_format.h         # Formattazione dei pannelli
│   ├── visualizer_legend.cpp      # Legenda delle viste
│   ├── byte_info.h                # Classi di byte, entropia, ipotesi contenuto
│   ├── nav_bars*.{h,cpp}          # Barre: logica, disegno, geometria
│   ├── occurrences.{h,cpp}        # Dove compare un valore
│   ├── view_results.{h,cpp}       # Calcolo delle viste (thread separato)
│   ├── visualizer_highlight.cpp   # Sezione -> vista
│   ├── visualizer_links.cpp       # Vista -> file
│   ├── range_sel.h                # Logica della selezione
│   ├── density.h                  # Luminosità di digraph e dot plot
│   └── ui_widgets.h               # Slider, pannelli
├── python/                  # Neural network (WIP)
├── setup.sh                 # Setup automatico
├── Makefile
└── requirements.txt
```

---

## Classificatore N-gram e verifiche

La modalità Classifier della MetricMap usa campioni binari in
`templates/<classe>.bin`, relativi alla directory da cui si avvia il programma.
I nomi delle classi sono elencati in `src/classifier_model.h` (per esempio
`ascii.bin`, `x86.bin`, `x64.bin`). I campioni devono contenere almeno 4 byte.
Le classi prive di campioni non partecipano al confronto. Senza template validi
la modalità viene saltata con un messaggio; non vengono generate classificazioni
fittizie. I blocchi troppo corti per un N-gram sono mostrati in grigio.
Questa è una classificazione statistica, non una rete neurale.
Le cartelle Python e dataset sono predisposizioni attualmente senza codice.

Eseguire `make test` per i test di regressione senza interfaccia grafica.
I sorgenti sono in `tests/`, gli eseguibili generati in `build/`.

La selezione di una sezione mostra il suo intervallo esatto; cambiando colori,
BPP o vista l'intervallo viene mantenuto. Ogni ricalcolo elabora solo la vista
attiva e copia solo la finestra selezionata. Il file viene comunque caricato
interamente in memoria; il rendering 3D di file grandi può essere oneroso.
Il parser supporta ELF32/64 little-endian e PE; ELF big-endian viene rifiutato,
e le sezioni ELF senza byte nel file (`SHT_NOBITS`, come `.bss`) sono escluse.

---

## Dipendenze

### C++ (single-header, no install)
- [olcPixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine) — rendering
- [stb_image_write](https://github.com/nothings/stb) — export PNG

### Python (area sperimentale — WIP)

La cartella Python è una predisposizione e non fa parte delle funzionalità attuali.

---

## Crediti

- Christopher Domas — [CantorDust](https://github.com/Battelle/cantordust) (ispirazione)
- javidx9 / One Lone Coder — [olcPixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine)
- Sean Barrett — [stb libraries](https://github.com/nothings/stb)

---

## Licenza

MIT