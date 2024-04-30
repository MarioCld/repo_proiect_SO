#!/bin/bash

cale_fisier=$1
nume_fisier=$2
director="Izolated_space_dir"
nr_linii=$(wc -l < "$cale_fisier")
nr_cuvinte=$(wc -w < "$cale_fisier")
nr_caractere=$(wc -c < "$cale_fisier")

if [ ! -f "$cale_fisier" ];
    then echo -e "\nFisierul nu a fost gasit: $nume_fisier\n"
    exit 1
fi

cuvinte_cheie=("corrupted" "dangerous" "risk" "attack" "malware" "malicious")
ok=0
suspect=0

nr_linii=$(wc -l < "$cale_fisier")
if [ "$nr_linii" -lt 3 ];
    then if [ "$nr_cuvinte" -gt 10 ];
             then if [ "$nr_caractere" -gt 20 ];
                      then suspect=1
                  fi
         fi
fi

if [ $suspect -eq 1 ]
   then for cuvant_cheie in "${cuvinte_cheie[@]}";
            do
                if grep -q "$cuvant_cheie" "$cale_fisier";
                   then ok=1
                fi
            done 
   if grep -q -P '[^\x00-\x7F]' "$cale_fisier";
       then ok=1
   fi
   if [ $ok -eq 0 ];
       then echo -e "\nSAFE\n"
   fi
   if [ $ok -eq 1 ];
       then mv "$cale_fisier" "$director"
       echo -e "\n$nume_fisier\n"
   fi
fi

if [ $suspect -eq 0 ]
    then echo -e "\nSAFE\n"
fi
