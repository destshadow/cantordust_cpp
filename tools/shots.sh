#!/bin/bash
# shots.sh <eseguibile> <cartella> [file]: le stesse schermate, con il mouse in
# punti fissi, per confrontare due versioni pixel per pixel (vedi cmp.sh).
# Serve tools/xsend compilato:  gcc tools/xsend.c -o /tmp/xsend -lX11 -lXtst
XSEND=${XSEND:-/tmp/xsend}; BIN=$1; OUT=$2; FILE=${3:-/bin/ls}
rm -rf "$OUT"; mkdir -p "$OUT"; pkill -x cantordust; sleep 0.4
(timeout 60 "$BIN" "$FILE" >/dev/null 2>&1 &); sleep 2.5
W=$(xwininfo -root -tree | grep -i cantordust | head -1 | awk '{print $1}')
X="$XSEND $W"; shot(){ sleep 0.45; import -window "$W" "$OUT/$1.png"; }
$X click 700 300 sleep 500
for k in 1 2 3 4 6 7 8 9 0; do $X key $k sleep 900 move 600 350; shot v$k; $X move 1100 700; shot s$k; done
$X key 5 sleep 1000 move 1100 700; shot v5; $X move 620 330 sleep 300; shot h5
$X key Home sleep 900 move 40 300 sleep 400; shot sec; $X click 40 300 sleep 1000 move 1100 700; shot zone
$X key o sleep 300; shot open; $X key Escape sleep 200
pkill -x cantordust
