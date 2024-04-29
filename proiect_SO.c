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


void drepturi_fisier(char *cale_fisier, struct dirent *dir, struct stat statbuf)
{
  if(!(statbuf.st_mode & S_IRWXU) && !(statbuf.st_mode & S_IRWXG) && !(statbuf.st_mode & S_IRWXO))
    {
      pid_t npid;
      if((npid=fork())<0)
	{
	  perror("\nProcesul fiu nu a putut fi creat!\n\n");
	  exit(8);
	}
      if(npid==0)
	{
	  char *executa[] = {"./verify_for_malicious.sh",cale_fisier,NULL};
	  execvp(executa[0],executa);
	  perror("\nEroare la executarea scriptului!\n\n");
	  exit(9);
	}
    }
}


void parcurgere_director(const char *director, int snapshot, int check_dir)
{
  DIR *d;
  struct dirent *dir;
  struct stat statbuf;
  char case_stat[strlen("character device")+1];
  d=opendir(director);
  if(!d)
    {
      perror("\nNu s-a putut deschide directorul!\n\n");
      exit(2);
    }
  if(check_dir==0)
    {
      write(snapshot,"***** Numele directorului curent: ",strlen("***** Numele directorului curent: "));
      scriere_in_snapshot(snapshot,director,strlen(director));
      write(snapshot," *****\n",strlen(" *****\n"));
    }
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
      strcpy(case_stat,tip_fisier(statbuf,case_stat));
      if(strcmp(case_stat,"directory")==0)
	{
	  check_dir=1;
	  char cale_director[strlen(director)+strlen(dir->d_name)+2];
	  snprintf(cale_director,sizeof(cale_director),"%s/%s",director,dir->d_name);
	  parcurgere_director(cale_director,snapshot,check_dir);
	}
      if(strcmp(case_stat,"regular file")==0)
	{
	  drepturi_fisier(cale_fisier,dir,statbuf);
	}
      check_dir=0;
      char i_node[strlen("4300000000")];
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
    }
  if(closedir(d)==-1)
    {
      perror("\nNu s-a putut inchide directorul!\n\n");
      exit(3);
    }
}


void creare_snapshot(const char *director, int i)
{
  int snapshot;
  int check_dir=0;
  char fisier_snapshot[strlen(director)+strlen("/Snapshot[]_.txt")+2];
  snprintf(fisier_snapshot,sizeof(fisier_snapshot),"%s/Snapshot[%d]_.txt",director,i);
  snapshot=open(fisier_snapshot, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  if(snapshot==-1)
    {
      perror("\nNu s-a putut deschide snapshot-ul!\n\n");
      exit(5);
    }
  parcurgere_director(director,snapshot,check_dir);
   if(close(snapshot)==-1)
     {
       perror("\nNu s-a putut inchide snapshot-ul!\n\n");
       exit(6);
     }
   printf("\nSnapshot-ul pentru %s a fost creat cu succes!\n",director);
}


void creare_director_snapshot(const char *director, const char *director_snapshot, int i, char *argv[])
{
  int snapshot;
  int j=0;
  int check_dir=0;
  char fisier_snapshot[strlen(director_snapshot)+strlen("/Director_snapshot[]_.txt")+2];
  if(strcmp(argv[3],"-s")==0)
    {
      j=i-4;
    }
  else
    {
      j=i-2;
    }
  snprintf(fisier_snapshot,sizeof(fisier_snapshot),"%s/Director_snapshot[%d]_.txt",director_snapshot,j);
  snapshot=open(fisier_snapshot, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  if(snapshot==-1)
    {
      perror("\nNu s-a putut deschide snapshot-ul!\n\n");
      exit(5);
    }
  parcurgere_director(director,snapshot,check_dir);
  if(close(snapshot)==-1)
    {
      perror("\nNu s-a putut inchide snapshot-ul!\n\n");
      exit(6);
    }
  printf("\nSnapshot-ul pentru %s a fost creat cu succes!\n",director);
}


void creare_proces(pid_t pid, int status, int argc, char *argv[], int i, int j, int vector_pid[])
{
  if(i!=argc-1)
    {
      if((pid=fork())<0)
	{
	  perror("\nProcesul fiu nu a putut fi creat!\n\n");
	  exit(8);
	}
      if(pid==0)
	{
	  i++;
	  j++;
	  vector_pid[i-3]=getpid();
	  creare_proces(pid,status,argc,argv,i,j,vector_pid);
	  printf("\nProcesul copil %d s-a terminat cu PID %d si exit code 0.\n\n",j,vector_pid[j-1]);
	  exit(0);
	}
    }
  char *director_snapshot=argv[2];
  char *director=argv[i];
  creare_director_snapshot(director,director_snapshot,i,argv);
  if(i!=argc-1)
    {
      wait(&status);
      if(!WIFEXITED(status))
	{
	  printf("\nProcesul copil cu id-ul %d din iteratia i=%d s-a incheiat anormal!\n\n",vector_pid[j-1],j);
	}
    }
}


void lansare_procese(int argc, char *argv[], int i, int vector_pid[])
{
  pid_t pid;
  int status;
  int j=0;
  if(strcmp(argv[3],"-s")==0)
    {
      j=i-4;
    }
  else
    {
      j=i-2;
    }
  if((pid=fork())<0)
    {
      perror("\nProcesul fiu nu a putut fi creat!\n\n");
      exit(8);
    }
  if(pid==0)
    {
      vector_pid[i-3]=getpid();
      creare_proces(pid,status,argc,argv,i,j,vector_pid);
      printf("\nProcesul copil %d s-a terminat cu PID %d si exit code 0.\n\n",j,vector_pid[j-1]);
      exit(0);
    }
  wait(&status);
  if(!WIFEXITED(status))
    {
      printf("\nProcesul copil cu id-ul %d din iteratia i=%d s-a incheiat anormal!\n\n",vector_pid[j-1],j);
    }
}


int main(int argc, char *argv[])
{
  if(strcmp(argv[3],"-s")==0)
    {
      if(argc<6 || argc>15)
	{
	  perror("\nNumarul de argumente trebuie sa fie intre 6 si 15!\n\n");
	  exit(1);
	}
      int i=5;
      int vector_pid[argc-i];
      lansare_procese(argc,argv,i,vector_pid);
    }
  else if(strcmp(argv[1],"-o")==0)
    {
      if(argc<4 || argc>13)
	{
	  perror("\nNumarul de argumente trebuie sa fie intre 4 si 13!\n\n");
	  exit(1);
	}
      int i=3;
      int vector_pid[argc-i];
      lansare_procese(argc,argv,i,vector_pid);
    }
  else
    {
      if(argc<2 || argc>11)
	{
	  perror("\nNumarul de argumente trebuie sa fie intre 2 si 11!\n\n");
	  exit(1);
	}
      for(int i=1;i<argc;i++)
	{
	  char *director=argv[i];
	  creare_snapshot(director,i);
	}
      printf("\n");
    }
  return 0;
}
