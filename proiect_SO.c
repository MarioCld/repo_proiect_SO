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


/*struct dirent {
   ino_t          d_ino; // Inode number
   off_t          d_off; // Not an offset; see below
   unsigned short d_reclen; // Length of this record
   unsigned char  d_type; // Type of file; not supported by all filesystem types
   char           d_name[256]; // Null-terminated filename
};

struct stat {
       dev_t     st_dev;      // ID of device containing file
       ino_t     st_ino;      // Inode number
       mode_t    st_mode;     // File type and mode
       nlink_t   st_nlink;    // Number of hard links
       uid_t     st_uid;      // User ID of owner
       gid_t     st_gid;      // Group ID of owner
       dev_t     st_rde;      // Device ID (if special file)
       off_t     st_size;     // Total size, in bytes
       blksize_t st_blksize;  // Block size for filesystem I/O
       blkcnt_t  st_blocks;   // Number of 512B blocks allocated
       struct timespec st_atim;  // Time of last access
       struct timespec st_mtim;  // Time of last modification 
       struct timespec st_ctim;  // Time of last status change
};*/

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
  if(S_ISBLK(statbuf.st_mode))
    {
      strcpy(case_stat,"block device");
    }
  else if(S_ISCHR(statbuf.st_mode))
    {
      strcpy(case_stat,"character device");
    }
  else if(S_ISDIR(statbuf.st_mode))
    {
      strcpy(case_stat,"directory");
    }
  else if(S_ISFIFO(statbuf.st_mode))
    {
      strcpy(case_stat,"FIFO/pipe");
    }
  else if(S_ISLNK(statbuf.st_mode))
    {
      strcpy(case_stat,"symlink");
    }
  else if(S_ISREG(statbuf.st_mode))
    {
      strcpy(case_stat,"regular file");
    }
  else if(S_ISSOCK(statbuf.st_mode))
    {
      strcpy(case_stat,"socket");
    }
  else
    {
      strcpy(case_stat,"unknown?");
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
 

int main(int argc, char *argv[])
{
  if(argc<2 || argc>11)
    {
      perror("\nNumarul de argumente trebuie sa fie intre 2 si 10!\n\n");
      exit(1);
    }
  int snapshot;
  bool ok=false;
  snapshot=open("Snapshot.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  if(snapshot==-1)
    {
      perror("\nNu s-a putut deschide snapshot-ul!\n\n");
      exit(5);
    }
  for(int i=1;i<argc;i++)
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
      printf("\nSnapshot-ul a fost modificat cu succes!\n\n");
    }
  else if(!ok)
    {
      printf("\nEroare la modificarea snapshot-ului!\n\n");
    }
  return 0;
}
