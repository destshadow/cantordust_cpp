#!/bin/bash
# check_ui.sh: le schermate dell'app compilata ora contro quelle dell'ultimo
# commit, pixel per pixel. Stampa una riga di esito ed esce con 0 se sono
# uguali. Si lancia con:  make check-ui
#
# Sono attese differenze solo nelle viste toccate di proposito. La finestra
# "Apri file" (open.png) non conta: ha un cursore che lampeggia.
#
# Lavora in una cartella temporanea nuova a ogni lancio e non cancella nulla:
# le vecchie restano in /tmp (pochi MB l'una).
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=$(mktemp -d /tmp/cantordust_check_ui.XXXXXX) || { echo "check-ui: mktemp fallito"; exit 2; }
mkdir -p "$WORK/base"
export XSEND="$WORK/xsend"
gcc "$ROOT/tools/xsend.c" -o "$XSEND" -lX11 -lXtst || { echo "check-ui: manca Xlib/XTest"; exit 2; }

# La versione dell'ultimo commit, con lo script di cattura di adesso
git -C "$ROOT" archive HEAD | tar -x -C "$WORK/base"
cp "$ROOT/tools/shots.sh" "$WORK/base/tools/shots.sh"
# I campioni del Classifier non sono su git: senza, l'ultimo commit
# mostrerebbe la Metric Map e l'Inspector senza Classifier (differenze finte)
[ -d "$ROOT/templates" ] && ln -s "$ROOT/templates" "$WORK/base/templates"
make -C "$WORK/base" -j8 >/dev/null 2>&1 || { echo "check-ui: l'ultimo commit non compila"; exit 2; }

(cd "$WORK/base" && bash tools/shots.sh ./cantordust "$WORK/ref" >/dev/null 2>&1)
sleep 1
(cd "$ROOT" && bash tools/shots.sh ./cantordust "$WORK/new" >/dev/null 2>&1)

# I tasti sono arrivati? Se la finestra non aveva il focus tutte le schermate
# restano sulla prima vista e il confronto non vale nulla.
for d in ref new; do
    same=$(compare -metric AE "$WORK/$d/v1.png" "$WORK/$d/v3.png" null: 2>&1); same=${same%% *}
    if [ ! -s "$WORK/$d/v3.png" ] || [ "$same" = "0" ]; then
        echo "check-ui: CATTURA NON VALIDA ($d): i tasti non sono arrivati alla finestra. Riprova."
        exit 2
    fi
done

out=$(bash "$ROOT/tools/cmp.sh" "$WORK/ref" "$WORK/new" | grep -v '^  open\.png')
count=$(ls "$WORK/new" | wc -l)
changed=$(echo "$out" | grep -c '^  ')
if [ "$changed" = "0" ]; then
    echo "check-ui: OK, $((count - 1)) schermate identiche all'ultimo commit"
    exit 0
fi
echo "check-ui: $changed schermate su $((count - 1)) diverse dall'ultimo commit"
echo "$out" | grep '^  '
echo "schermate in $WORK/ref e $WORK/new"
exit 1
