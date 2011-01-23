/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int open_kb(void) 
{
  int i, res;
  char filename[32];
  int fd=0;
  
  for (i=0; i<32; ++i) 
    {
      snprintf (filename, 32, "/dev/hidraw%d", i);
      fd = open (filename, O_RDONLY);
      if (fd > 0) 
	{
	  char buf[256];
	  struct hidraw_report_descriptor rpt_desc;
	  struct hidraw_devinfo info;
	  
	  /* Get Raw Info */
	  res = ioctl(fd, HIDIOCGRAWINFO, &info);
	  if (res < 0)
	    perror("HIDIOCGRAWINFO");
	  else if ((info.vendor == 0x0813) && (info.product == 0x1007)) 
	    {
	      printf ("Found it in %s\n", filename);
	      break;
	    }
	}
      close(fd);
      fd=0;
      
    }
  return fd;
}

int positions[] = 
  {
    '1', '2', '3', '4', '5', '6', '7', '8', 
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 
    'z', 'x', 'c', 'v', 'b', 'n', 'm', '?', 
    -1, '\n',  -5,  -6, ' ', '^', 'l', '?', 
    -2,   -3,  -4, '?', 'p', 'o', '9', '0', 
    -10, -11, -12, -13, -14,  -7, -8,  -9,
    0
  };
char *pos2[]=
  {
    "world", /* -1 */
    "draw",
    "play",
    "search",
    "print",
    "home",
    "write", /* -7 */
    "caclulator",
    "power",
    "m1", /* -10 */
    "m2",
    "m3",
    "m4",
    "m5"
  }
  ;


int main(int argc, char **argv)
{
  int fd=0, res=0;
  unsigned char buf[32];
  const int SHIFT_OFF=37;
  const unsigned long long SHIFT_BIT=(unsigned long long)1<<SHIFT_OFF;
  
  /* for now, just search for the device */
  fd=open_kb();
  
  if (fd == 0) 
    {
      puts ("failed to find device");
      return 1;
    }

  unsigned long long prev=0, cur=0;
  union 
  {
    unsigned long long number;
    unsigned char buffer[8];
  } curu;
  
  setvbuf(stdout, 0, _IONBF, 0);
  
  
  while ((res = read(fd, buf, 16)) > 0) 
    {
      if (buf[9] != 1) continue;
      
      memcpy(&cur, buf+2, 8);
      if (cur && (cur != prev)) 
	{
	  

	  unsigned long long cur_new = cur & (cur ^ prev);
	  int i=0;
	  unsigned long long b=1;

	  //printf ("%llx %llx\n", cur, cur_new);

	  for (i=0; positions[i]!=0; ++i) 
	    {
	      if ((i!=SHIFT_OFF) && (cur_new & b)) 
		{
		  
		  char c = positions[i];
		  if (c<0) 
		    {
		      printf ("[%s]", pos2[-c-1]);
		    }
		  else 
		    {
		      if (cur & SHIFT_BIT)
			c=c-'a'+'A';
		      putchar(c);
		    }
		  
		}
	      
	      b<<=1;
	      
	    }
	  prev=cur;
	  
	}
    }
}

