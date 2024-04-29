#!/bin/bash

cale_fisier=$1
director="Izolated_space_dir"
nr_linii=$(wc -l < "$cale_fisier")
nr_cuvinte=$(wc -w < "$cale_fisier")
nr_caractere=$(wc -c < "$cale_fisier")

if [ ! -f "$cale_fisier" ];
    then echo "Fisierul nu a fost gasit: $cale_fisier"
    exit 1
fi

cuvinte_cheie=("corrupted" "dangerous" "risk" "attack" "malware" "malicious")
ok=0
suspect=0

nr_linii=$(wc -l < "$cale_fisier")
if [ "$nr_linii" -lt 3 ];
    then if [ "$nr_cuvinte" -gt 1000 ];
             then if [ "$nr_caractere" -gt 2000 ];
                      then suspect=1
                  fi
         fi
fi

if [ $suspect -eq 1 ]
   then for cuvant_cheie in "${cuvinte_cheie[@]}";
            do
                if grep -q "$cuvant_cheie" "$cale_fisier";
                   then echo "S-a gasit cuvantul '$cuvant_cheie' in fisierul: $cale_fisier"
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
   if [ $ok -eq 1 ];
       then mv "$cale_fisier" "$director"
       echo "$cale_fisier"
   fi
fi

if [ $suspect -eq 0 ]
    then echo "SAFE"
fi
