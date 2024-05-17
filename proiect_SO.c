#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>


#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec


void scriere_in_snapshot(int snapshot, const void *buffer, size_t nr)
{
  if(write(snapshot,buffer,nr)==-1) //scriem informatiile in snapshot
    {
      perror("\nNu s-a putut scrie in snapshot!\n\n");
      exit(7);
    }
}


char *tip_fisier(struct stat statbuf, char *case_stat)
{
  switch(statbuf.st_mode & S_IFMT) //se verifica ce tip de fisier a fost citit din director
    {
    case S_IFBLK:     strcpy(case_stat,"block device");         break;
    case S_IFCHR:     strcpy(case_stat,"character device");     break;
    case S_IFDIR:     strcpy(case_stat,"directory");            break;
    case S_IFIFO:     strcpy(case_stat,"FIFO/pipe");            break;
    case S_IFLNK:     strcpy(case_stat,"symlink");              break;
    case S_IFREG:     strcpy(case_stat,"regular file");         break;
    case S_IFSOCK:    strcpy(case_stat,"socket");               break;
    default:          strcpy(case_stat,"unknown?");             break;
    }
  return case_stat;
}


void drepturi_fisier(char *cale_fisier, struct dirent *dir, struct stat statbuf, int *nr_fisiere_periculoase)
{
  if(!(statbuf.st_mode & S_IRWXU) && !(statbuf.st_mode & S_IRWXG) && !(statbuf.st_mode & S_IRWXO)) //se verifica daca fisierul nu are niciun drept
    {
      pid_t npid1,npid2;
      int status;
      int pfd[2];
      char buffer_pipe[256];
      if(pipe(pfd)<0) //se creaza un pipe
	{
	  perror("\nNu s-a putut crea un pipe!\n\n");
	  exit(10);
	}
      if((npid1=fork())<0) //se creeaza un proces nou
	{
	  perror("\nProcesul fiu nu a putut fi creat!\n\n");
	  exit(8);
	}
      if(npid1==0)
	{
	  close(pfd[0]); //inchidem capatul de citire al pipe-ului
	  if(dup2(pfd[1],1)==-1) //redirectam iesirea standard spre pipe (vom scrie in pipe)
	    {
	      perror("\nNu s-a putut redirectiona iesirea standard!\n\n");
	      exit(12);
	    }
	  char *comanda_verificare[] = {"./verify_for_malicious.sh",cale_fisier,dir->d_name,NULL}; //vector de argumente
	  if(execvp(comanda_verificare[0],comanda_verificare)==-1) //se executa scriptul care verifica fisierele
	    {
	      perror("\nEroare la executarea scriptului!\n\n");
	      exit(9);
	    }
	}
      close(pfd[1]); //inchidem capatul de scriere al pipe-ului
      if(dup2(pfd[0],0)==-1) //redirectam intrarea standard spre pipe (vom citi din pipe)
	{
	  perror("\nNu s-a putut redirectiona intrarea standard!\n\n");
	  exit(12);
	}
      waitpid(npid1,&status,0); //se asteapta incheierea primului proces "nepot"
      if(read(0,buffer_pipe,sizeof(buffer_pipe))==-1) //citim rezultatul scriptului din pipe
	{
	  perror("\nNu s-a putut citi din pipe!\n\n");
	  exit(13);
	}
      else
	{
	  if(!strcmp(buffer_pipe,"SAFE")==0) //daca fisierul nu e sigur
	    {
	      (*nr_fisiere_periculoase)++; //crestem numarul fisierelor periculoase din acest Fisier_mare
	      if((npid2=fork())<0) //se creeaza un proces nou
		{
		  perror("\nProcesul fiu nu a putut fi creat!\n\n");
		  exit(8);
		}
	      if(npid2==0)
		{
		  char *comanda_mutare[] = {"mv",cale_fisier,"Izolated_space_dir",NULL}; //vector de argumente
		  if(execvp(comanda_mutare[0],comanda_mutare)==-1) //izolam fisierul periculos
		    {
		      perror("\nEroare la executarea scriptului!\n\n");
		      exit(9);
		    }
		}
	    }
	}
       waitpid(npid2,&status,0); //se asteapta incheierea celui de al doilea proces "nepot"
       if(!WIFEXITED(status)) //procesul copil preia statusul procesului copil
	 {
	   printf("\nProcesul nepot pentru verificarea scriptului si mutarea fisierelor s-a incheiat anormal!\n\n");
	 }
    }
}


void parcurgere_director(const char *director, int snapshot, int check_dir, int *nr_fisiere_periculoase)
{
  DIR *d;
  struct dirent *dir; //variabila referinta catre aceasta structura va identifica in mod unic directorul curent
  struct stat statbuf; //variabila structura ce va contine atributele unui fisier
  char case_stat[20]; //posibilul tip de fisier
  d=opendir(director); //se deschide directorul pentru parcurgere
  if(!d)
    {
      perror("\nNu s-a putut deschide directorul!\n\n");
      exit(2);
    }
  if(check_dir==0) //luam in evidenta orice Fisier_mare, deoarece nu pot fi in alte subdirectoare
    {
      write(snapshot,"***** Numele directorului curent: ",strlen("***** Numele directorului curent: "));
      scriere_in_snapshot(snapshot,director,strlen(director)); //scriem in snapshot directorul curent (functia de scriere in snapshot se afla la linia 20 in cod)
      write(snapshot," *****\n",strlen(" *****\n"));
    }
  while((dir=readdir(d))!=NULL) //citim din director toate fisierele
    {
      char cale_fisier[strlen(director)+strlen(dir->d_name)+2]; //se creeaza calea fiecarui fisier din director
      snprintf(cale_fisier,sizeof(cale_fisier),"%s/%s",director,dir->d_name);
      if(strcmp(dir->d_name,".")==0 || strcmp(dir->d_name,"..")==0) //se ignora directorul curent si parinte
	{
	  continue;
	}
      if(stat(cale_fisier,&statbuf)==-1) //aflam atributele unui fisier
	{
	  perror("\nEroare stat\n\n");
	  exit(4);
	}
      strcpy(case_stat,tip_fisier(statbuf,case_stat)); //preluam tipul fisierului citit (functia de preluare a tipului fisierului se afla la linia 30 in cod)
      if(strcmp(case_stat,"directory")==0) //verificam daca avem subdirector in director
	{
	  check_dir=1; //avem subdirector in director
	  char cale_director[strlen(director)+strlen(dir->d_name)+2]; //se creeaza noua cale a subdirectorului
	  snprintf(cale_director,sizeof(cale_director),"%s/%s",director,dir->d_name);
	  parcurgere_director(cale_director,snapshot,check_dir,nr_fisiere_periculoase); //se reapeleaza functia
	}
      if((strcmp(case_stat,"regular file")==0) && *nr_fisiere_periculoase!=-1) //verificam daca suntem pe cazul "-s Izolated_space_dir"
	{
	  drepturi_fisier(cale_fisier,dir,statbuf,nr_fisiere_periculoase); //daca suntem, atunci se verifica drepturile fisierului citit (functia de verificare a drepturilor se afla la linia 47 in cod)
	}
      check_dir=0; //nu mai avem subdirector in director
      char i_node[strlen("4300000000")];
      sprintf(i_node,"%ld\n",statbuf.st_ino); //preluam i-node-ul fisierului citit
      write(snapshot,"\nNumele fisierului:          ",strlen("Numele fisierului:          \n"));
      scriere_in_snapshot(snapshot,dir->d_name,strlen(dir->d_name)); //scriem in snapshot numele fisierului citit (functia de scriere in snapshot se afla la linia 20 in cod)
      write(snapshot,"\nTipul fisierului:           ",strlen("Tipul fisierului:           \n"));
      scriere_in_snapshot(snapshot,case_stat,strlen(case_stat)); //sciem in snapshot tipul fisierului citit (functia de scriere in snapshot se afla la linia 20 in cod)
      write(snapshot,"\nI-node number:              ",strlen("I-node number:              \n"));
      scriere_in_snapshot(snapshot,i_node,strlen(i_node)); //scriem in snapshot i-node-ul fisierului citit (functia de scriere in snapshot se afla la linia 20 in cod)
      write(snapshot,"Last status changed:        ",strlen("Last status changed:        "));
      scriere_in_snapshot(snapshot,ctime(&statbuf.st_ctime),strlen(ctime(&statbuf.st_ctime))); //scriem in snapshot ultima data de status schimbat a fisierului citit (functia de scriere in snapshot se afla la linia 20 in cod)
      write(snapshot,"Last file access:           ",strlen("Last file access:           "));
      scriere_in_snapshot(snapshot,ctime(&statbuf.st_atime),strlen(ctime(&statbuf.st_atime))); //scriem in snapshot ultima data de accesare a fisierului citit (functia de scriere in snapshot se afla la linia 20 in cod)
      write(snapshot,"Last file modification:     ",strlen("Last file modification:     "));
      scriere_in_snapshot(snapshot,ctime(&statbuf.st_mtime),strlen(ctime(&statbuf.st_mtime))); //scriem in snapshot ultima data de modificare a fisierului citit (functia de scriere in snapshot se afla la linia 20 in cod)
      write(snapshot,"\n",strlen("\n"));
    }
  if(closedir(d)==-1) //se inchide directorul
    {
      perror("\nNu s-a putut inchide directorul!\n\n");
      exit(3);
    }
}


void creare_snapshot(const char *director, int i)
{
  int snapshot;
  int check_dir=0; //va fi folosit pentru verificare de subdirectoare in directoare
  int nr_fisiere_periculoase=-1; //va ramane mereu -1, deoarece nu suntem pe cazul cu "-s Izolated_space_dir"
  char fisier_snapshot[strlen(director)+strlen("/Snapshot[]_.txt")+2]; //se creeaza calea snapshot-ului
  snprintf(fisier_snapshot,sizeof(fisier_snapshot),"%s/Snapshot[%d]_.txt",director,i);
  snapshot=open(fisier_snapshot, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU); //se deschide un snapshot pentru scriere
  if(snapshot==-1)
    {
      perror("\nNu s-a putut deschide snapshot-ul!\n\n");
      exit(5);
    }
  parcurgere_director(director,snapshot,check_dir,&nr_fisiere_periculoase); //parcurgem Fisierul_mare curent (functia de parcurgere a unui director se afla la linia 122 in cod)
  if(close(snapshot)==-1) //se inchide snapshot-ul
     {
       perror("\nNu s-a putut inchide snapshot-ul!\n\n");
       exit(6);
     }
   printf("\nSnapshot-ul pentru %s a fost creat cu succes!\n",director);
}


void creare_director_snapshot(const char *director, const char *director_snapshot, int i, char *argv[], int *nr_fisiere_periculoase)
{
  int snapshot;
  int j=0; //contor al numarului de snapshot-uri
  int check_dir=0; //va fi folosit pentru verificare de subdirectoare in directoare
  char fisier_snapshot[strlen(director_snapshot)+strlen("/Director_snapshot[]_.txt")+2]; //se creeaza iar calea snapshot-ului
  if(strcmp(argv[3],"-s")==0)
    {
      j=i-4; //pe cazul "-s Izolated_space_dir"
    }
  else
    {
      j=i-2; //pe cazul "-o Fisier_rezultat"
      *nr_fisiere_periculoase=-1; //va ramane mereu -1, deoarece nu suntem pe cazul cu "-s Izolated_space_dir"
    }
  snprintf(fisier_snapshot,sizeof(fisier_snapshot),"%s/Director_snapshot[%d]_.txt",director_snapshot,j);
  snapshot=open(fisier_snapshot, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU); //se deschide un snapshot pentru scriere
  if(snapshot==-1)
    {
      perror("\nNu s-a putut deschide snapshot-ul!\n\n");
      exit(5);
    }
  parcurgere_director(director,snapshot,check_dir,nr_fisiere_periculoase); //parcurgem Fisierul_mare curent (functia de parcurgere a unui director se afla la linia 122 in cod)
  if(close(snapshot)==-1) //se inchide snapshot-ul
    {
      perror("\nNu s-a putut inchide snapshot-ul!\n\n");
      exit(6);
    }
  if(!strcmp(argv[3],"-s")==0) //pentru cazul fara "-s Izolated_space_dir"
    {
      printf("\nSnapshot-ul pentru %s a fost creat cu succes!\n",director);
    }
}


void creare_proces(pid_t pid, int status, int argc, char *argv[], int i, int j, int vector_pid[], int *nr_fisiere_periculoase)
{
  if(i!=argc-1) //verificam daca am ajuns la ultimul Fisier_mare
    {
      if((pid=fork())<0) //daca nu am ajuns, se creeaza alt proces copil
	{
	  perror("\nProcesul fiu nu a putut fi creat!\n\n");
	  exit(8);
	}
      if(pid==0)
	{
	  i++; //urmatorul Fisier_mare
	  j++; //urmatorul proces
	  vector_pid[j-1]=getpid(); //aflam id-ul procesului copil
	  creare_proces(pid,status,argc,argv,i,j,vector_pid,nr_fisiere_periculoase); //se creeaza recursiv cate un proces copil pentru fiecare Fisier_mare (functia de creare de procese se afla la linia 248 in cod)
	  if(strcmp(argv[3],"-s")==0) //pe cazul "-s Izolated_space_dir"
	    {
	      printf("\nProcesul copil %d s-a terminat cu PID %d si cu %d fisiere cu potential periculos.\n\n",j,vector_pid[j-1],*nr_fisiere_periculoase);
	    }
	  else //pe cazul "-o Fisier_rezultat"
	    {
	      printf("\nProcesul copil %d s-a terminat cu PID %d si exit code 0.\n\n",j,vector_pid[j-1]);
	    }
	  exit(0);
	}
    }
  char *director_snapshot=argv[2]; //pentru cazul "-o Fisier_rezultat"
  char *director=argv[i]; //ultimul Fisier_mare
  creare_director_snapshot(director,director_snapshot,i,argv,nr_fisiere_periculoase); //se creeaza de aceasta data in directorul "Fisier_rezultat" cate un snapshot pentru fiecare Fisier_mare (functia de creare de director snapshot se afla la linia 213 in cod)
  if(i!=argc-1) //verificam daca am ajuns la ultimul Fisier_mare, deoarece nu are dupa ce proces astepta
    {
      wait(&status); //se asteapta incheierea procesului copil
      if(!WIFEXITED(status)) //procesul parinte preia statusul procesului copil
	{
	  printf("\nProcesul copil cu id-ul %d din iteratia i=%d s-a incheiat anormal!\n\n",vector_pid[j-1],j);
	}
    }
}


void lansare_procese(int argc, char *argv[], int i, int vector_pid[])
{
  pid_t pid;
  int status;
  int j=0; //contor al numarului de procese
  if(strcmp(argv[3],"-s")==0)
    {
      j=i-4; //pe cazul "-s Izolated_space_dir"
    }
  else
    {
      j=i-2; //pe cazul "-o Fisier_rezultat"
    }
  if((pid=fork())<0) //se creaza un proces nou
    {
      perror("\nProcesul fiu nu a putut fi creat!\n\n");
      exit(8);
    }
  if(pid==0)
    {
      int nr_fisiere_periculoase=0; //pentru cazul "-s Izolated_space_dir"
      vector_pid[j-1]=getpid(); //aflam id-ul procesului copil
      creare_proces(pid,status,argc,argv,i,j,vector_pid,&nr_fisiere_periculoase); //vom crea recursiv cate un proces copil pentru fiecare Fisier_mare (functia de creare de procese se afla la linia 248 in cod)
      if(strcmp(argv[3],"-s")==0) //pe cazul "-s Izolated_space_dir"
	{
	  printf("\nProcesul copil %d s-a terminat cu PID %d si cu %d fisiere cu potential periculos.\n\n",j,vector_pid[j-1],nr_fisiere_periculoase);
	}
      else //pe cazul "-o Fisier_rezultat"
	{
	  printf("\nProcesul copil %d s-a terminat cu PID %d si exit code 0.\n\n",j,vector_pid[j-1]);
	}
      exit(0);
    }
  wait(&status); //se asteapta incheierea procesului copil
  if(!WIFEXITED(status)) //procesul parinte preia statusul procesului copil
    {
      printf("\nProcesul copil cu id-ul %d din iteratia i=%d s-a incheiat anormal!\n\n",vector_pid[j-1],j);
    }
}


int main(int argc, char *argv[]) //argumente date in linia de comanda
{
  if(strcmp(argv[3],"-s")==0) //se verifica daca suntem pe cazul cu "-s Izolated_space_dir"
    {
      if(argc<6 || argc>15) //se verifica numarul de argumente (cel putin 6: executabilul, -o, Fisierul_rezultat,-s, Izolated_space_dir si oricare Fisier_mare
	{
	  perror("\nNumarul de argumente trebuie sa fie intre 6 si 15!\n\n");
	  exit(1);
	}
      int i=5; //oricare Fisier_mare va fi mereu cel putin argv[5]
      int vector_pid[argc-i]; //vector de numarul de procese pe care il vom avea in functie de argc
      lansare_procese(argc,argv,i,vector_pid); //lanseaza procese pentru fiecare Fisier_mare + pipe-uri (functia de lansare a proceselor se afla la linia 288 in cod)
    }
  else if(strcmp(argv[1],"-o")==0) //se verifica daca suntem pe cazul cu "-o Fisier_rezultat"
    {
      if(argc<4 || argc>13) //se verifica numarul de argumente (cel putin 4: executabilul, -o, Fisierul_rezultat si oricare Fisier_mare
	{
	  perror("\nNumarul de argumente trebuie sa fie intre 4 si 13!\n\n");
	  exit(1);
	}
      int i=3; //oricare Fisier_mare va fi mereu cel putin argv[3]
      int vector_pid[argc-i]; //vector de numarul de procese pe care il vom avea in functie de argc
      lansare_procese(argc,argv,i,vector_pid); //lanseaza procese pentru fiecare Fisier_mare
    }
  else
    {
      if(argc<2 || argc>11) //se verifica numarul de argumente (cel putin 2: executabilul si oricare Fisier_mare
	{
	  perror("\nNumarul de argumente trebuie sa fie intre 2 si 11!\n\n");
	  exit(1);
	}
      for(int i=1;i<argc;i++) //incepe de la 1, deoarece argv[0] va fi mereu executabilul
	{
	  char *director=argv[i]; //luam pe rand fiecare Fisier_mare dat ca parametru in linia de comanda
	  creare_snapshot(director,i); //se creeaza un snapshot pentru fiecare fisier din Fisier_mare
	}
      printf("\n");
    }
  return 0;
}
