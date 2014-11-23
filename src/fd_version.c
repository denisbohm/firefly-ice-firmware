#include "fd_version.h"

bool fd_version_is_equal(fd_version_t a, fd_version_t b) {
    return (a.major == b.major) && (a.minor == b.minor) && (a.patch == b.patch);
}