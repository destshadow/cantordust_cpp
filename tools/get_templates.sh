#!/bin/sh
# Scarica i template del Classifier dal CantorDust originale (Battelle,
# Apache 2.0) nella cartella templates/, che non va su git.
#
#   sh tools/get_templates.sh
#
# Circa 87 MB. Un file gia' scaricato non si riscarica.
# I template sono campioni di byte per ogni classe (x86, ARM, testo...):
# il Classifier li legge come byte grezzi, l'intestazione .NET in testa a
# ogni file e' qualche centinaio di byte su milioni e non pesa.
set -e
cd "$(dirname "$0")/.."
mkdir -p templates
BASE=https://raw.githubusercontent.com/Battelle/cantordust/master/resources/templates
for c in arm4 arm7 ascii_english compressed java mips msil ones png powerpc \
         sparc_32 utf_16_english x64 x86 x86_padding zeros embedded_image \
         arm64 riscv64 s390x ppc64el mips64el; do
    f="templates/$c.template"
    if [ -s "$f" ]; then echo "gia' presente: $c"; continue; fi
    echo "scarico: $c"
    curl -fsSL "$BASE/$c.template" -o "$f.part" && mv "$f.part" "$f"
done
echo "fatto: $(ls templates/*.template | wc -l) template in templates/"
