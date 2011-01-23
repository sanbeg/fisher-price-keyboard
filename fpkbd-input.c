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
  struct uinput_user_dev uidev;
  
  fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if(fd < 0)
    die("/dev/uinput");
  
  if(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
    die("/dev/uinput");

  for (i=0; positions[i]; ++i) 
    if (positions[i] > 0)
      ioctl(fd, UI_SET_KEYBIT, positions[i]);

  //copy all this from hidraw?
  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "fisher price keyboard");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor  = 0x0813;
  uidev.id.product = 0x1007;
  uidev.id.version = 1;
  
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
      struct input_event event={0};

      event.type=EV_KEY;
      event.code=key;
      event.value=value;
      write(fd, &event, sizeof(event));
    }
  
}

int main(int argc, char **argv)
{
  int rfd=0, wfd=0, res=0;
  unsigned char buf[32];
  typedef unsigned long long keystate;
  
  wfd=open_uinput();
  
  rfd=open_kb();
  
  if (rfd <= 0) 
    {
      puts ("failed to find keyboard");
      return 1;
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
	      int i=0;
	      keystate b=1;
	      for (i=0; positions[i]!=0; ++i) 
		{
		  if (cur_old & b)
		    press_key(wfd,positions[i], 0);
		  else if (cur_new & b)
		    press_key(wfd,positions[i], 1);
		  b<<=1;
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