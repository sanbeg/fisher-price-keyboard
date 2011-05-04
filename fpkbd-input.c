#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

#include <linux/uinput.h>

// 2-9, 16-23, 30-37, 44-50, ...

int positions[]= 
  {
    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, 
    KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, 
    KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, 
    KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, -1, 
    -1, KEY_ENTER, -1, -1, KEY_SPACE, KEY_RIGHTSHIFT, KEY_L, -1, 
    -1, -1, -1, -1, KEY_P, KEY_O, KEY_9, KEY_0, 
    0
  };

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

void die(const char * msg) 
{
  perror(msg);
  exit(EXIT_FAILURE);
}

int open_uinput (void) 
{
  int fd, i;
  struct uinput_user_dev uidev = {
    .name = "Fisher-Price keyboard",
    .id = {
      .bustype = BUS_USB,
      .vendor  = 0x0813,
      .product = 0x1007,
      .version = 1
    }
  };
  
  if ((fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) < 0)
    die("/dev/uinput");
  
  if(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
    die("/dev/uinput");

  for (i=0; positions[i]; ++i) 
    if (positions[i] > 0)
      ioctl(fd, UI_SET_KEYBIT, positions[i]);

  if(write(fd, &uidev, sizeof(uidev)) < 0)
    die("error: write");
  
  if(ioctl(fd, UI_DEV_CREATE) < 0)
    die("error: ioctl");
  
  return fd;
}

void press_key(int fd, int key, int value) 
{
  if (key>0) 
    {
      static struct input_event event={
	.type = EV_KEY
      };
      const static struct input_event syn_event={
	.type = EV_SYN,
	.code = SYN_REPORT,
	.value = 0
      };
      
      event.code=key;
      event.value=value;

      write(fd, &event, sizeof(event));
      write(fd, &syn_event, sizeof(event));
    }
}

int main(int argc, char **argv)
{
  int rfd=0, wfd=0, res=0;
  unsigned char buf[16];
  typedef unsigned long long keystate;
  
  int opt;
  unsigned char do_fork=1;
  
  while ((opt = getopt(argc, argv, "f")) != -1) 
    {
      switch (opt) 
	{
	case 'f':
	  do_fork = 0;
	  break;
	default:
	  fprintf (stderr, "Invalid option: -%c\n", opt);
	  return 1;
	}
    }

  if (argc > optind+1) 
    {
      fprintf (stderr, "Usage: %s [device]\n", *argv);
      exit(1);
    }
  if (argc == optind+1) 
    {
      if ((rfd = open (argv[optind], O_RDONLY)) <= 0) 
	{
	  perror (argv[optind]);
	  return 1;
	}
    }
  else 
    {
      rfd=open_kb();
    }
  
  wfd=open_uinput();
  
  if (rfd <= 0) 
    {
      puts ("failed to find keyboard");
      return 1;
    }
  
  if (do_fork) 
    {
      switch(fork())
	{
	case -1:
	  perror ("fork");
	  return 1;
	case 0:
	  break;
	default:
	  return 0;
	}
    }

  keystate prev=0, cur=0;
  while ((res = read(rfd, buf, 16)) > 0) 
    {
      cur=*((keystate*)(buf+2));
      
      if (cur && (cur != prev)) 
	{
	  keystate cur_new = cur & (cur ^ prev);
	  keystate cur_old = prev & (cur ^ prev);

	  if (cur_new || cur_old) 
	    {
	      int i;
	      keystate b;
	      for (i=0, b=1; positions[i]!=0; ++i, b<<=1) 
		{
		  if (cur_old & b)
		    press_key(wfd,positions[i], 0);
		  else if (cur_new & b)
		    press_key(wfd,positions[i], 1);
		}
	      prev=cur;
	    }
	}
    }

  close(rfd);
  ioctl(wfd, UI_DEV_DESTROY);
  close(wfd);

  return 0;
}
