#!/bin/bash
# shots.sh <eseguibile> <cartella> [file]: le stesse schermate, con il mouse in
# punti fissi, per confrontare due versioni pixel per pixel (vedi cmp.sh).
# Serve tools/xsend compilato:  gcc tools/xsend.c -o /tmp/xsend -lX11 -lXtst
XSEND=${XSEND:-/tmp/xsend}; BIN=$1; OUT=$2; FILE=${3:-/bin/ls}
rm -rf "$OUT"; mkdir -p "$OUT"; pkill -x cantordust; sleep 0.4
(timeout 120 "$BIN" "$FILE" >/dev/null 2>&1 &); sleep 2.5
W=$(xwininfo -root -tree | grep -i cantordust | head -1 | awk '{print $1}')
X="$XSEND $W"; shot(){ sleep 0.45; import -window "$W" "$OUT/$1.png"; }
$X click 700 300 sleep 500
for k in 1 2 3 4 6 7 8 9 0 h p; do $X key $k sleep 900 move 600 350; shot v$k; $X move 1100 700; shot s$k; done
$X key 5 sleep 1000 move 1100 700; shot v5; $X move 620 330 sleep 300; shot h5
# Il menu' "Altro": aperto, poi una voce scelta col mouse
$X key 1 sleep 700 click 950 12 sleep 400 move 960 52 sleep 300; shot menu; $X click 960 52 sleep 900 move 1100 700; shot menu_scelta
# Le opzioni di ogni vista, da tastiera e col mouse sulla riga in basso
$X key 2 sleep 900 key t sleep 1500 move 500 300; shot o2t; $X click 110 783 sleep 1200 move 1100 700; shot o2v
$X key 7 sleep 900 key c sleep 900 key c sleep 900 key c sleep 900 key c sleep 900 key v sleep 900 move 1100 700; shot o7
$X key 6 sleep 900 key bracketright sleep 700 key bracketright sleep 700 key f sleep 700 key period sleep 500 key a sleep 900 move 500 300; shot o6
$X key 0 sleep 900 key l sleep 400 key t sleep 400 click 600 59 sleep 400 move 1100 700; shot o0; $X key h sleep 700; shot oh; $X key p sleep 900; shot op
$X key 5 sleep 1000 key f sleep 900 key c sleep 700 move 1100 700; shot o5; $X key f key f sleep 900
$X key Home sleep 900 move 40 300 sleep 400; shot sec; $X click 40 300 sleep 1000 move 1100 700; shot zone
$X key o sleep 300; shot open; $X key Escape sleep 200
pkill -x cantordust
