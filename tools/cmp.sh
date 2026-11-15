#!/bin/bash
# cmp.sh <cartella a> <cartella b>: pixel diversi per ogni schermata di
# shots.sh. La barra in alto e' coperta: contiene gli fps, che cambiano sempre.
T=$(mktemp -d); tot=0
for f in "$1"/*.png; do b=$(basename "$f")
  convert "$1/$b" -fill black -draw "rectangle 0,0 1280,25" "$T/a.png"
  convert "$2/$b" -fill black -draw "rectangle 0,0 1280,25" "$T/b.png"
  d=$(compare -metric AE "$T/a.png" "$T/b.png" null: 2>&1); d=${d%% *}
  [ "$d" != "0" ] && echo "  $b: $d"; tot=$((tot+${d%.*}))
done; echo "totale pixel diversi: $tot su $(ls "$1"/*.png | wc -l) schermate"; rm -rf "$T"
