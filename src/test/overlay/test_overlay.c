#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static int test_open_overlay_file(void) {
    // /tmp/overlay_test.txt should resolve to the overlay dir's copy
    int fd = open("/tmp/overlay_test.txt", O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "FAIL: open(/tmp/overlay_test.txt): %s\n", strerror(errno));
        return 1;
    }
    char buf[256] = {0};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) {
        fprintf(stderr, "FAIL: read returned %zd\n", n);
        return 1;
    }
    if (strncmp(buf, "overlay_file_content", 20) != 0) {
        fprintf(stderr, "FAIL: unexpected content: '%s'\n", buf);
        return 1;
    }
    fprintf(stderr, "PASS: open overlay file\n");
    return 0;
}

static int test_stat_overlay_file(void) {
    struct stat st;
    if (stat("/tmp/overlay_test.txt", &st) != 0) {
        fprintf(stderr, "FAIL: stat(/tmp/overlay_test.txt): %s\n", strerror(errno));
        return 1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "FAIL: not a regular file\n");
        return 1;
    }
    fprintf(stderr, "PASS: stat overlay file\n");
    return 0;
}

static int test_access_overlay_file(void) {
    if (access("/tmp/overlay_test.txt", R_OK) != 0) {
        fprintf(stderr, "FAIL: access(/tmp/overlay_test.txt, R_OK): %s\n", strerror(errno));
        return 1;
    }
    fprintf(stderr, "PASS: access overlay file\n");
    return 0;
}

static int test_lstat_overlay_file(void) {
    struct stat st;
    if (lstat("/etc/overlay_config.txt", &st) != 0) {
        fprintf(stderr, "FAIL: lstat(/etc/overlay_config.txt): %s\n", strerror(errno));
        return 1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "FAIL: not a regular file\n");
        return 1;
    }
    fprintf(stderr, "PASS: lstat overlay file\n");
    return 0;
}

static int test_real_fs_fallback(void) {
    // /dev/null should still be accessible (not in overlay)
    if (access("/dev/null", R_OK) != 0) {
        fprintf(stderr, "FAIL: access(/dev/null, R_OK): %s\n", strerror(errno));
        return 1;
    }
    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "FAIL: open(/dev/null): %s\n", strerror(errno));
        return 1;
    }
    close(fd);
    fprintf(stderr, "PASS: real FS fallback\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_open_overlay_file();
    failures += test_stat_overlay_file();
    failures += test_access_overlay_file();
    failures += test_lstat_overlay_file();
    failures += test_real_fs_fallback();

    if (failures > 0) {
        fprintf(stderr, "%d test(s) FAILED\n", failures);
        return 1;
    }
    fprintf(stderr, "All overlay tests passed\n");
    return 0;
}
