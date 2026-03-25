/*
 * The Shadow Simulator
 * See LICENSE for licensing information
 */

#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "test/test_glib_helpers.h"

// Save an fd to the real root and chroot back to it after each test.
// This is the standard technique for escaping a chroot.
static void _restore_root(int real_root_fd) {
    assert_nonneg_errno(fchdir(real_root_fd));
    assert_nonneg_errno(chroot("."));
}

static void _test_chroot_basic() {
    int real_root_fd = open("/", O_RDONLY | O_DIRECTORY);
    assert_nonneg_errno(real_root_fd);

    char tmpl[] = "/tmp/chroot-test-XXXXXX";
    assert_nonnull_errno(mkdtemp(tmpl));

    // Create a marker file inside the future chroot so we can verify it worked.
    char marker_path[PATH_MAX];
    snprintf(marker_path, sizeof(marker_path), "%s/.chroot_marker", tmpl);
    int marker_fd = open(marker_path, O_CREAT | O_WRONLY, 0644);
    assert_nonneg_errno(marker_fd);
    close(marker_fd);

    // chroot into the temp dir
    assert_nonneg_errno(chroot(tmpl));
    assert_nonneg_errno(chdir("/"));

    // Verify we're in the chroot: the marker file should be visible at /.chroot_marker
    struct stat st;
    assert_nonneg_errno(stat("/.chroot_marker", &st));

    // A file that existed at the real root should NOT be visible (e.g. /tmp).
    // After chroot, /tmp refers to <chroot>/tmp which doesn't exist.
    g_assert_cmpint(stat("/tmp", &st), ==, -1);
    assert_errno_is(ENOENT);

    // Restore real root and clean up
    _restore_root(real_root_fd);
    close(real_root_fd);
    unlink(marker_path);
    assert_nonneg_errno(rmdir(tmpl));
}

static void _test_chroot_enoent() {
    int rv = chroot("/nonexistent/path/that/does/not/exist");
    g_assert_cmpint(rv, ==, -1);
    assert_errno_is(ENOENT);
}

static void _test_chroot_enotdir() {
    char tmpl[] = "/tmp/chroot-notdir-XXXXXX";
    int fd = mkstemp(tmpl);
    assert_nonneg_errno(fd);
    close(fd);

    int rv = chroot(tmpl);
    g_assert_cmpint(rv, ==, -1);
    assert_errno_is(ENOTDIR);

    unlink(tmpl);
}

static void _test_chroot_chdir_interaction() {
    int real_root_fd = open("/", O_RDONLY | O_DIRECTORY);
    assert_nonneg_errno(real_root_fd);

    char tmpl[] = "/tmp/chroot-chdir-XXXXXX";
    assert_nonnull_errno(mkdtemp(tmpl));

    // Create a subdirectory inside the future chroot
    char subdir[PATH_MAX];
    snprintf(subdir, sizeof(subdir), "%s/subdir", tmpl);
    assert_nonneg_errno(mkdir(subdir, 0755));

    // chroot into the temp dir, then chdir into the subdirectory
    assert_nonneg_errno(chroot(tmpl));
    assert_nonneg_errno(chdir("/subdir"));

    char cwd[PATH_MAX];
    assert_nonnull_errno(getcwd(cwd, sizeof(cwd)));
    g_assert_cmpstr(cwd, ==, "/subdir");

    // Restore real root and clean up
    _restore_root(real_root_fd);
    close(real_root_fd);
    assert_nonneg_errno(rmdir(subdir));
    assert_nonneg_errno(rmdir(tmpl));
}

int main(int argc, char* argv[]) {
    g_test_init(&argc, &argv, NULL);

    // chroot requires CAP_SYS_CHROOT; skip if unprivileged.
    if (geteuid() != 0) {
        // Try a harmless chroot to check capability
        if (chroot("/") != 0 && errno == EPERM) {
            g_test_skip("chroot requires CAP_SYS_CHROOT (run as root)");
            return 0;
        }
    }

    g_test_add_func("/chroot/basic", _test_chroot_basic);
    g_test_add_func("/chroot/enoent", _test_chroot_enoent);
    g_test_add_func("/chroot/enotdir", _test_chroot_enotdir);
    g_test_add_func("/chroot/chdir_interaction", _test_chroot_chdir_interaction);

    return g_test_run();
}
