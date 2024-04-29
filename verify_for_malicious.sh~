#!/bin/bash

cale_fisier=$1

if [ ! -f "$cale_fisier" ];
    then echo "Fisierul nu a fost gasit: $cale_fisier"
    exit 1
fi

cuvinte_cheie=("corrupted" "dangerous" "risk" "attack" "malware" "malicious")
ok=0

for cuvinte_cheie in "${cuvinte_cheie[@]}";
    do
        if grep -q "$cuvinte_cheie" "$cale_fisier";
	    then echo "S-a gasit cuvantul '$cuvinte_cheie' in fisierul: $cale_fisier"
                ok=1
        fi
done

if grep -q -P '[^\x00-\x7F]' "$cale_fisier";
    then echo "Fisierul contine caractere non-ASCII: $cale_fisier"
    ok=1
fi

if [ $ok -eq 0 ];
    then echo "Fisierul este sigur: $cale_fisier"
fi
