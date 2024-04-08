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


#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
#define MAX_CASE 25
#define CHUNK 512


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


void parcurgere_director(const char *director, int snapshot)
{
  DIR *d;
  struct dirent *dir;
  struct stat statbuf;
  char case_stat[MAX_CASE];
  d=opendir(director);
  if(!d)
    {
      perror("Nu s-a putut deschide directorul!\n");
      exit(2);
    }
  while((dir=readdir(d))!=NULL)
    {
      char cale_fisier[strlen(director)+strlen(dir->d_name)+2];
      snprintf(cale_fisier,sizeof(cale_fisier),"%s/%s",director,dir->d_name);
      if (strcmp(dir->d_name,".")==0 || strcmp(dir->d_name,"..")==0)
	{
	  continue;
	}
      if(stat(cale_fisier,&statbuf)==-1)
	{
	  perror("Eroare stat");
	  exit(4);
	}
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
      else strcpy(case_stat,"unknown?");
      char i_node[MAX_CASE];
      sprintf(i_node,"%ld\n",statbuf.st_ino);
      write(snapshot,"Numele fisierului:          ",strlen("Numele fisierului:          "));
      if((write(snapshot,dir->d_name,strlen(dir->d_name)))==-1)
	{
	  perror("Nu s-a putut scrie in snapshot!\n");
	  exit(7);
	}
      write(snapshot,"\n",strlen("\n"));
      write(snapshot,"Tipul fisierului:           ",strlen("Tipul fisierului:           "));
      if((write(snapshot,case_stat,strlen(case_stat)))==-1)
	{
	  perror("Nu s-a putut scrie in snapshot!\n");
	  exit(7);
	}
      write(snapshot,"\n",strlen("\n"));
      write(snapshot,"I-node number:              ",strlen("I-node number:              "));
      if((write(snapshot,i_node,strlen(i_node)))==-1)
	{
	  perror("Nu s-a putut scrie in snapshot!\n");
	  exit(7);
	}
      write(snapshot,"Last status changed:        ",strlen("Last status changed:        "));
      if((write(snapshot,ctime(&statbuf.st_ctime),strlen(ctime(&statbuf.st_ctime))))==-1)
	{
	  perror("Nu s-a putut scrie in snapshot!\n");
	  exit(7);
	}
      write(snapshot,"Last file access:           ",strlen("Last file access:           "));
      if((write(snapshot,ctime(&statbuf.st_atime),strlen(ctime(&statbuf.st_atime))))==-1)
	{
	  perror("Nu s-a putut scrie in snapshot!\n");
	  exit(7);
	}
      write(snapshot,"Last file modification:     ",strlen("Last file modification:     "));
      if((write(snapshot,ctime(&statbuf.st_mtime),strlen(ctime(&statbuf.st_mtime))))==-1)
	{
	  perror("Nu s-a putut scrie in snapshot!\n");
	  exit(7);
	}
      write(snapshot,"\n",strlen("\n"));
    }
  if(closedir(d)==-1)
    {
      perror("Nu s-a putut inchide directorul!\n");
      exit(3);
    }
}
 

int main(int argc, char *argv[])
{
  if(argc<2 || argc>11)
    {
      perror("Numar incorect de argumente!\n");
      exit(1);
    }
  int snapshot;
  snapshot=open("Snapshot.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  if(snapshot==-1)
    {
      perror("Nu s-a putut deschide snapshot-ul!\n");
      exit(5);
    }
  for(int i=1;i<argc;i++)
    {
      char *director=argv[i];
      parcurgere_director(director,snapshot);
    }
  if(close(snapshot)==-1)
    {
      perror("Nu s-a putut inchide snapshot-ul!\n");
      exit(6);
    }
  printf("\nSnapshot-ul a fost modificat cu succes!\n");
  return 0;
}
