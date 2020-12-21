#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

ssize_t write_all(int fd, const void *vptr, size_t n)
{
        size_t remaining;
        ssize_t cc;
        const uint8_t *ptr;

        ptr = vptr;
        remaining = n;
        while (remaining > 0) {
                if ((cc = write(fd, ptr, remaining)) <= 0) {
                        if (cc < 0 && errno == EINTR)
                                cc = 0;
                        else
                                return -1;
                }

                remaining -= cc;
                ptr += cc;
        }
        return n;
}

ssize_t read_all(int fd, void *vptr, size_t n)
{
	size_t remaining;
	ssize_t cc;
        uint8_t *ptr;

        ptr = vptr;
        remaining = n;
        while (remaining > 0) {
                if ((cc = read(fd, ptr, remaining)) < 0) {
                        if (errno == EINTR)
                                cc = 0;
                        else
                                return -1;
                } else if (cc == 0) /* EOF */
                        break;

                remaining -= cc;
                ptr += cc;
        }
        return (n - remaining);
}
