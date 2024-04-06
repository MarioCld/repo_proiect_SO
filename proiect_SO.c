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


void parcurgere_director(const char *director, FILE *snapshot)
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
      if (strcmp(dir->d_name,".")==0 || strcmp(dir->d_name,"..")==0)
	{
	  continue;
	}
      if(stat(director,&statbuf)==-1)
	{
	  perror("Eroare stat");
	  exit(4);
	}
      switch(statbuf.st_mode & S_IFMT)
	{
	case S_IFBLK:  strcpy(case_stat,"block device");         break;
	case S_IFCHR:  strcpy(case_stat,"character device");     break;
	case S_IFDIR:  strcpy(case_stat,"directory");            break;
	case S_IFIFO:  strcpy(case_stat,"FIFO/pipe");            break;
	case S_IFLNK:  strcpy(case_stat,"symlink");              break;
	case S_IFREG:  strcpy(case_stat,"regular file");         break;
	case S_IFSOCK: strcpy(case_stat,"socket");               break;
	default:       strcpy(case_stat,"unknown?");             break;
	}
      if((fprintf(snapshot,"\nNumele fisierului:          %s\n""Tipul fisierului:           %s\n"
	                     "I-node number:              %ju\n""Last status change:         %s"
                             "Last file access:           %s""Last file modification:     %s",
                  dir->d_name,case_stat,
	          statbuf.st_ino,ctime(&statbuf.st_ctime),
		  ctime(&statbuf.st_atime),ctime(&statbuf.st_mtime)))<0)
	{
	  perror("Nu s-a putut scrie in snapshot!\n");
	  exit(7);
	}
    }
  if(closedir(d)==-1)
    {
      perror("Nu s-a putut inchide directorul!\n");
      exit(3);
    }
}
 

int main(int argc, char *argv[])
{
  if(argc<2 || argc>10)
    {
      perror("Numar incorect de argumente!\n");
      exit(1);
    }
  FILE *snapshot;
  snapshot=fopen("Snapshot.txt","w+");
  if(!snapshot)
    {
      perror("Nu s-a putut deschide snapshot-ul!\n");
      exit(5);
    }
  for(int i=1;i<argc;i++)
    {
      char *director=argv[i];
      parcurgere_director(director,snapshot);
    }
  if(fclose(snapshot)!=0)
    {
      perror("Nu s-a putut inchide snapshot-ul!\n");
      exit(6);
    }
  printf("\nSnapshot-ul a fost modificat cu succes!\n");
  return 0;
}
