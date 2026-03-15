# CantorDust++

Binary visualization tool ispirato al lavoro di [Christopher Domas](https://github.com/Battelle/cantordust).

Visualizza file binari come immagini per identificare pattern, sezioni e anomalie a colpo d'occhio.

![CantorDust++](https://raw.githubusercontent.com/destshadow/cantordust_cpp/test_digrapg.png)

---

## Visualizzazioni

| Tasto | Vista | Descrizione |
|-------|-------|-------------|
| `1` | Digraph | Coppie di byte consecutive su piano 2D |
| `2` | Dot Plot | Offset vs valore byte |
| `3` | Entropy | Entropia di Shannon per sezione |
| `4` | Histogram | Frequenza di ogni byte value 0-255 |
| `5` | 3D View | Triple di byte nello spazio 3D |

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

# Compila
make

# Avvia
./cantordust /path/to/binary
```

---

## Controlli

### Generale
| Tasto | Azione |
|-------|--------|
| `1-5` | Cambia vista |
| `O` | Apri file |
| `S` | Salva PNG |
| `←→` | Naviga nel file |
| `Home` | Reset (file intero) |

### Vista 2D (1-4)
| Azione | Effetto |
|--------|---------|
| Rotella mouse | Zoom in/out |
| Drag sinistro | Pan (con zoom > 1x) |

### Vista 3D (5)
| Azione | Effetto |
|--------|---------|
| Drag sinistro | Ruota |
| Drag destro | Pan camera |
| `WASD` | Muovi camera |
| `Q/E` | Su/Giù |
| Rotella | Zoom |
| `R` | Reset |

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
│   ├── binary_format.{h,cpp}      # Parser ELF/PE
│   ├── digraph.{h,cpp}            # Visualizzazione digraph
│   ├── dotplot.{h,cpp}            # Dot plot
│   ├── entropy.{h,cpp}            # Entropia di Shannon
│   ├── histogram.{h,cpp}          # Istogramma byte
│   ├── trigraph.{h,cpp}           # Nuvola punti 3D
│   ├── renderer3d.{h,cpp}         # Renderer 3D software
│   ├── file_navigator.{h,cpp}     # Navigazione nel file
│   ├── screenshot.{h,cpp}         # Export PNG (stb)
│   ├── input_handler.{h,cpp}      # Gestione input
│   ├── ui_renderer.{h,cpp}        # Rendering UI
│   ├── constants.h                # Layout e costanti
│   ├── visualizer.{h,cpp}         # Orchestrazione principale
│   ├── visualizer_2d.cpp          # Canvas 2D + zoom
│   ├── visualizer_3d.cpp          # Input 3D
│   ├── visualizer_nav.cpp         # Navigazione + threading
│   └── visualizer_sections.cpp    # Overlay sezioni ELF/PE
├── python/                  # Neural network (WIP)
├── setup.sh                 # Setup automatico
├── Makefile
└── requirements.txt
```

---

## Dipendenze

### C++ (single-header, no install)
- [olcPixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine) — rendering
- [stb_image_write](https://github.com/nothings/stb) — export PNG

### Python (neural network — WIP)
```
numpy
matplotlib
pillow
```

---

## Crediti

- Christopher Domas — [CantorDust](https://github.com/Battelle/cantordust) (ispirazione)
- javidx9 / One Lone Coder — [olcPixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine)
- Sean Barrett — [stb libraries](https://github.com/nothings/stb)

---

## License

MIT
