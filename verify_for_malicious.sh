#!/bin/bash

chmod 400 $1 #se modifica temporar drepturile fisierului pe read-only pentru a putea fi citite informatiile din el

cale_fisier=$1 #calea fisierului analizat
nume_fisier=$2 #numele fisierului analizat
director="Izolated_space_dir" #directorul in care fisierul va fi mutat daca este periculos

nr_linii=$(wc -l < "$cale_fisier") #preia numarul de linii din fisier
nr_cuvinte=$(wc -w < "$cale_fisier") #preia numarul de cuvinte din fisier
nr_caractere=$(wc -c < "$cale_fisier") #preia numarul de caractere din fisier

if [ ! -f "$cale_fisier" ]; #verifica daca fisierul exista
    then echo "Fisierul nu a fost gasit: $nume_fisier"
    exit 1
fi

cuvinte_cheie=("corrupted" "dangerous" "risk" "attack" "malware" "malicious")
ok=0 #verifica daca fisierul este periculos sau nu
suspect=0 #verifica daca fisierul este suspect sau nu

if [ "$nr_linii" -lt 3 ]; #se compara numarul de linii
    then if [ "$nr_cuvinte" -gt 10 ]; #se compara numarul de cuvinte
             then if [ "$nr_caractere" -gt 20 ]; #se compara numarul de caractere
                      then suspect=1
                  fi
         fi
fi

if [ $suspect -eq 1 ]
   then for cuvant_cheie in "${cuvinte_cheie[@]}"; #se parcurge fisierul cuvant cu cuvant
            do
                if grep -q "$cuvant_cheie" "$cale_fisier"; #se cauta cuvant cheie in fisier
                   then ok=1
                fi
            done 
   if grep -q -P '[^\x00-\x7F]' "$cale_fisier"; #se cauta caractere NON-ASCII in fisier
       then ok=1
   fi
   if [ $ok -eq 0 ]; #daca fisierul nu e periculos
       then echo "SAFE"
   fi
   if [ $ok -eq 1 ]; #daca fisierul e periculos
       then echo "$nume_fisier"
   fi
fi

if [ $suspect -eq 0 ] #daca fisierul nu e suspect
    then echo "SAFE"
fi

chmod 000 $1 #se modifica drepturile fisierului inapoi pe niciun drept
