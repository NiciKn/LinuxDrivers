#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define GPIOCTRL_IOC_MAGIC 'g'
#define GPIOCTRL_IOC_SET   _IOW(GPIOCTRL_IOC_MAGIC, 1, int)
#define GPIOCTRL_IOC_CLR   _IOW(GPIOCTRL_IOC_MAGIC, 2, int)

int main(int argc, char *argv[])
{
	int fd;
	int gpio;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <gpio-number>\n", argv[0]);
		return 1;
	}

	gpio = atoi(argv[1]);
	fd = open("/dev/gpioctrl", O_RDWR);
	if (fd < 0)
	{
		perror("open");
		return 1;
	}

	printf("Toggling GPIO %d every 2 seconds....\n", gpio);

	while(1)
	{
		ioctl(fd, GPIOCTRL_IOC_SET, &gpio);
		sleep(2);
		ioctl(fd, GPIOCTRL_IOC_CLR, &gpio);
		sleep(2);
	}

	close(fd);
	return 0;
}
