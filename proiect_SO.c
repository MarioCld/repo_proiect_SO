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
#define MAX_CASE 25


void scriere_in_snapshot(int snapshot, const void *buffer, size_t nr)
{
  if(write(snapshot,buffer,nr)==-1)
    {
      perror("\nNu s-a putut scrie in snapshot!\n\n");
      exit(7);
    }
}


char *tip_fisier(struct stat statbuf, char *case_stat)
{
  switch(statbuf.st_mode & S_IFMT)
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


void parcurgere_director(const char *director, int snapshot)
{
  DIR *d;
  struct dirent *dir;
  struct stat statbuf;
  char *case_stat=malloc(MAX_CASE*sizeof(char*));
  d=opendir(director);
  if(!d)
    {
      perror("\nNu s-a putut deschide directorul!\n\n");
      exit(2);
    }
  write(snapshot,"***** Numele directorului curent: ",strlen("***** Numele directorului curent: "));
  scriere_in_snapshot(snapshot,director,strlen(director));
  write(snapshot," *****\n",strlen(" *****\n"));
  while((dir=readdir(d))!=NULL)
    {
      char cale_fisier[strlen(director)+strlen(dir->d_name)+2];
      snprintf(cale_fisier,sizeof(cale_fisier),"%s/%s",director,dir->d_name);
      if(strcmp(dir->d_name,".")==0 || strcmp(dir->d_name,"..")==0)
	{
	  continue;
	}
      if(stat(cale_fisier,&statbuf)==-1)
	{
	  perror("\nEroare stat\n\n");
	  exit(4);
	}
      case_stat=tip_fisier(statbuf,case_stat);
      char *i_node=malloc(MAX_CASE*sizeof(char*));
      sprintf(i_node,"%ld\n",statbuf.st_ino);
      write(snapshot,"\nNumele fisierului:          ",strlen("Numele fisierului:          \n"));
      scriere_in_snapshot(snapshot,dir->d_name,strlen(dir->d_name));
      write(snapshot,"\nTipul fisierului:           ",strlen("Tipul fisierului:           \n"));
      scriere_in_snapshot(snapshot,case_stat,strlen(case_stat));
      write(snapshot,"\nI-node number:              ",strlen("I-node number:              \n"));
      scriere_in_snapshot(snapshot,i_node,strlen(i_node));
      write(snapshot,"Last status changed:        ",strlen("Last status changed:        "));
      scriere_in_snapshot(snapshot,ctime(&statbuf.st_ctime),strlen(ctime(&statbuf.st_ctime)));
      write(snapshot,"Last file access:           ",strlen("Last file access:           "));
      scriere_in_snapshot(snapshot,ctime(&statbuf.st_atime),strlen(ctime(&statbuf.st_atime)));
      write(snapshot,"Last file modification:     ",strlen("Last file modification:     "));
      scriere_in_snapshot(snapshot,ctime(&statbuf.st_mtime),strlen(ctime(&statbuf.st_mtime)));
      write(snapshot,"\n",strlen("\n"));
      free(i_node);
    }
  if(closedir(d)==-1)
    {
      perror("\nNu s-a putut inchide directorul!\n\n");
      exit(3);
    }
  free(case_stat);
}


void creare_snapshot(const char *director, int i)
{
  int snapshot;
  char fisier_snapshot[MAX];
  snprintf(fisier_snapshot,sizeof(fisier_snapshot),"%s/Snapshot[%d]_.txt",director,i);
  snapshot=open(fisier_snapshot, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  if(snapshot==-1)
    {
      perror("\nNu s-a putut deschide snapshot-ul!\n\n");
      exit(5);
    }
   parcurgere_director(director,snapshot);
   if(close(snapshot)==-1)
     {
       perror("\nNu s-a putut inchide snapshot-ul!\n\n");
       exit(6);
     }
   printf("Snapshot-ul pentru %s a fost creat cu succes!\n",director);
}
 

int main(int argc, char *argv[])
{
  bool ok=false;
  if(strcmp(argv[1],"-o")==0)
    {
      if(argc<4 || argc>13)
	{
	  perror("\nNumarul de argumente trebuie sa fie intre 2 si 10!\n\n");
	  exit(1);
	}
      ok=true;
    }
  if(ok)
    {
      int snapshot;
      bool ok=false;
      char *director_snapshot=argv[2];
      char fisier_snapshot[strlen(director_snapshot)+strlen("/Director_snapshot_.txt")+2];
      snprintf(fisier_snapshot,sizeof(fisier_snapshot),"%s/Director_snapshot_.txt",director_snapshot);
      snapshot=open(fisier_snapshot, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
      if(snapshot==-1)
	{
	  perror("\nNu s-a putut deschide snapshot-ul!\n\n");
	  exit(5);
	}
      for(int i=3;i<argc;i++)
	{
	  char *director=argv[i];
	  parcurgere_director(director,snapshot);
	  if(i==argc-1)
	    {
	      ok=true;
	    }
	}
      if(close(snapshot)==-1)
	{
	  perror("\nNu s-a putut inchide snapshot-ul!\n\n");
	  exit(6);
	}
      if(ok)
	{
	  printf("\nDirectorul cu snapshot-uri a fost modificat cu succes!\n\n");
	}
      else if(!ok)
	{
	  printf("\nEroare la modificarea directorului cu snapshot-uri!\n\n");
	}
    }
  else
    {
      for(int i=1;i<argc;i++)
	{
	  char *director=argv[i];
	  creare_snapshot(director,i);
	}
    }
  return 0;
}
