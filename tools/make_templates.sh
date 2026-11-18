#!/bin/sh
# Fa i campioni del Classifier con i file di questa macchina, in templates/
# (che non va su git). Si aggiungono a quelli di Battelle (get_templates.sh):
# stessa classe = si usano entrambi.
#
#   sh tools/make_templates.sh
#
# Classi nuove, per trovare script nascosti in un file:
#   javascript, python, shell, html  dai file di /usr e /usr/share/doc
#   php        da due progetti PHP pubblici su GitHub (qui di PHP non ce n'e')
# Classi allargate:
#   ascii_english   README e .txt di /usr/share/doc
#   utf_16_english  il campione di Battelle e' solo prosa inglese: qui testo
#                   vario (licenze, codice, cifre) convertito in UTF-16
#
# Ogni campione e' al massimo 2 MB, preso da tanti file diversi (uno ogni N
# in ordine alfabetico: lo stesso risultato a ogni esecuzione).
set -e
cd "$(dirname "$0")/.."
mkdir -p templates
MAX=2097152
TMP=$(mktemp -d /tmp/cantordust_templates.XXXXXX)

# $1 = classe, poi `find` sui file: ne prende fino a 400 sparsi su tutti
sample() {
    name=$1; shift
    find "$@" -type f -size -300k 2>/dev/null | sort > "$TMP/list"
    n=$(wc -l < "$TMP/list")
    if [ "$n" -eq 0 ]; then echo "nessun file per: $name"; return; fi
    step=$(( n / 400 + 1 ))
    awk -v s="$step" 'NR % s == 0' "$TMP/list" | while read -r f; do cat "$f"; done \
        | head -c $MAX > "templates/$name.bin"
    echo "$name: $(wc -c < "templates/$name.bin") byte da $n file"
}

sample javascript /usr -name '*.js' ! -name '*.min.js'
sample python     /usr/lib -name '*.py'
sample html       /usr/share/doc -name '*.html'
# Testo inglese: il campione di Battelle e' piccolo (190 KB), e senza questo
# un testo qualunque somiglia di piu' ai commenti degli script di shell.
# Niente licenze (copyright): servono per provare il risultato.
sample ascii_english /usr/share/doc \( -name 'README*' -o -name '*.txt' -o -name 'NEWS' \) \
       ! -name '*.gz' ! -name 'copyright'

# Script di shell: i file di /usr/bin che iniziano con #! e finiscono in sh
for f in /usr/bin/* /usr/sbin/*; do
    head -c 32 "$f" 2>/dev/null | head -n 1 | grep -q '^#!.*sh' && echo "$f"
done | sort > "$TMP/shell"
while read -r f; do cat "$f"; done < "$TMP/shell" | head -c $MAX > templates/shell.bin
echo "shell: $(wc -c < templates/shell.bin) byte da $(wc -l < "$TMP/shell") file"

# PHP: due progetti pubblici (ramo principale), solo i .php
for repo in PHPMailer/PHPMailer slimphp/Slim; do
    curl -fsSL "https://api.github.com/repos/$repo/tarball" -o "$TMP/php.tgz" &&
        mkdir -p "$TMP/php" && tar -xzf "$TMP/php.tgz" -C "$TMP/php" ||
        echo "non scaricato: $repo"
done
if [ -d "$TMP/php" ]; then
    sample php "$TMP/php" -name '*.php'
fi

# UTF-16: testo vario, in little endian (Windows) e un po' in big endian
{ cat /usr/share/common-licenses/* 2>/dev/null
  find /usr/lib -name '*.py' -size -50k 2>/dev/null | sort | awk 'NR % 50 == 0' | head -100 \
      | while read -r f; do cat "$f"; done
} | head -c 900000 > "$TMP/text"
{ iconv -f UTF-8 -t UTF-16LE -c "$TMP/text"
  head -c 100000 "$TMP/text" | iconv -f UTF-8 -t UTF-16BE -c
} | head -c $MAX > templates/utf_16_english.bin
echo "utf_16_english: $(wc -c < templates/utf_16_english.bin) byte"

echo "fatto. I file temporanei sono in $TMP (si possono cancellare)"
