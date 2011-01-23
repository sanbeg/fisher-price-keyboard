/*******************************
 Hidraw test
 (c) Alan Ott
 May be used for any purpose.
*******************************/

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
  int i, res, fd;
  char filename[32];
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

  

int main(int argc, char **argv)
{
  int fd=0, res=0;
  unsigned char buf[256];
  
  /* for now, just search for the device */
  fd=open_kb();
  
  if (fd == 0) 
    {
      puts ("failed to find device");
      return 1;
    }
	
  /* Get a report from the device */
  int prev_sum=0;
  int numpad=0;
	
  while ((res = read(fd, buf, 16)) > 0) 
    {
      /* fields are "20", sequence, keys x 7, "01", ? x 6 (may include press/release) */
      int sum=0, i;
      
      if ((buf[2] +((buf[7]>>6)<<8)) != numpad) 
	{
	  numpad=buf[2] +((buf[7]>>6)<<8);
	  if (numpad) 
	    {
	      printf ("found keys %hx: ", numpad);
	      for (i=0; i<10; ++i)
		if (numpad & (1<<i))
		  printf ("%d ", i+1);
	      puts("");
	      
	    }
	}
      
      
      
      if (buf[9] == 1) 
	{
	  for (i=2; i<9; ++i)
	    if (i==7) 
	      sum += buf[i]&(0xff>>2);
	    else
	      sum+=buf[i];
	  
	  
	  if ((sum) == 0) {
	    prev_sum=sum;
	    continue;
	  }
	  prev_sum=sum;
	  
	  for (i = 1; i < res; i++) {
	    printf("%.2hhx ", buf[i]);
	  }
	}
      else 
	{
	  
	  for (i = 10; i < res; i++) {
	    printf("%d ", (unsigned char)buf[i]);
	  }
	}
      
      puts("");
      //if (buf[10]) puts ("Key event");
      
      //sleep(1);
      
    }
  
  close(fd);
  
  return 0;
	
}

