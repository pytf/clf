
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _lf_FILES_H_INCLUDED_
#define _lf_FILES_H_INCLUDED_


#include <lf_config.h>
#include <lf_core.h>


typedef int                      lf_fd_t;
typedef struct stat              lf_file_info_t;
typedef ino_t                    lf_file_uniq_t;


typedef struct {
    u_char                      *name;
    size_t                       size;
    void                        *addr;
    lf_fd_t                     fd;
    lf_log_t                   *log;
} lf_file_mapping_t;


typedef struct {
    DIR                         *dir;
    struct dirent               *de;
    struct stat                  info;

    unsigned                     type:8;
    unsigned                     valid_info:1;
} lf_dir_t;


typedef struct {
    size_t                       n;
    glob_t                       pglob;
    u_char                      *pattern;
    lf_log_t                   *log;
    lf_uint_t                   test;
} lf_glob_t;


#define lf_INVALID_FILE         -1
#define lf_FILE_ERROR           -1



#ifdef __CYGWIN__

#ifndef lf_HAVE_CASELESS_FILESYSTEM
#define lf_HAVE_CASELESS_FILESYSTEM  1
#endif

#define lf_open_file(name, mode, create, access)                            \
    open((const char *) name, mode|create|O_BINARY, access)

#else

#define lf_open_file(name, mode, create, access)                            \
    open((const char *) name, mode|create, access)

#endif

#define lf_open_file_n          "open()"

#define lf_FILE_RDONLY          O_RDONLY
#define lf_FILE_WRONLY          O_WRONLY
#define lf_FILE_RDWR            O_RDWR
#define lf_FILE_CREATE_OR_OPEN  O_CREAT
#define lf_FILE_OPEN            0
#define lf_FILE_TRUNCATE        (O_CREAT|O_TRUNC)
#define lf_FILE_APPEND          (O_WRONLY|O_APPEND)
#define lf_FILE_NONBLOCK        O_NONBLOCK

#if (lf_HAVE_OPENAT)
#define lf_FILE_NOFOLLOW        O_NOFOLLOW

#if defined(O_DIRECTORY)
#define lf_FILE_DIRECTORY       O_DIRECTORY
#else
#define lf_FILE_DIRECTORY       0
#endif

#if defined(O_SEARCH)
#define lf_FILE_SEARCH          (O_SEARCH|lf_FILE_DIRECTORY)

#elif defined(O_EXEC)
#define lf_FILE_SEARCH          (O_EXEC|lf_FILE_DIRECTORY)

#elif (lf_HAVE_O_PATH)
#define lf_FILE_SEARCH          (O_PATH|O_RDONLY|lf_FILE_DIRECTORY)

#else
#define lf_FILE_SEARCH          (O_RDONLY|lf_FILE_DIRECTORY)
#endif

#endif /* lf_HAVE_OPENAT */

#define lf_FILE_DEFAULT_ACCESS  0644
#define lf_FILE_OWNER_ACCESS    0600


#define lf_close_file           close
#define lf_close_file_n         "close()"


#define lf_delete_file(name)    unlink((const char *) name)
#define lf_delete_file_n        "unlink()"


lf_fd_t lf_open_tempfile(u_char *name, lf_uint_t persistent,
    lf_uint_t access);
#define lf_open_tempfile_n      "open()"


ssize_t lf_read_file(lf_file_t *file, u_char *buf, size_t size, off_t offset);
#if (lf_HAVE_PREAD)
#define lf_read_file_n          "pread()"
#else
#define lf_read_file_n          "read()"
#endif

ssize_t lf_write_file(lf_file_t *file, u_char *buf, size_t size,
    off_t offset);

ssize_t lf_write_chain_to_file(lf_file_t *file, lf_chain_t *ce,
    off_t offset, lf_pool_t *pool);


#define lf_read_fd              read
#define lf_read_fd_n            "read()"

/*
 * we use inlined function instead of simple #define
 * because glibc 2.3 sets warn_unused_result attribute for write()
 * and in this case gcc 4.3 ignores (void) cast
 */
static lf_inline ssize_t
lf_write_fd(lf_fd_t fd, void *buf, size_t n)
{
    return write(fd, buf, n);
}

#define lf_write_fd_n           "write()"


#define lf_write_console        lf_write_fd


#define lf_linefeed(p)          *p++ = LF;
#define lf_LINEFEED_SIZE        1
#define lf_LINEFEED             "\x0a"


#define lf_rename_file(o, n)    rename((const char *) o, (const char *) n)
#define lf_rename_file_n        "rename()"


#define lf_change_file_access(n, a) chmod((const char *) n, a)
#define lf_change_file_access_n "chmod()"


lf_int_t lf_set_file_time(u_char *name, lf_fd_t fd, time_t s);
#define lf_set_file_time_n      "utimes()"


#define lf_file_info(file, sb)  stat((const char *) file, sb)
#define lf_file_info_n          "stat()"

#define lf_fd_info(fd, sb)      fstat(fd, sb)
#define lf_fd_info_n            "fstat()"

#define lf_link_info(file, sb)  lstat((const char *) file, sb)
#define lf_link_info_n          "lstat()"

#define lf_is_dir(sb)           (S_ISDIR((sb)->st_mode))
#define lf_is_file(sb)          (S_ISREG((sb)->st_mode))
#define lf_is_link(sb)          (S_ISLNK((sb)->st_mode))
#define lf_is_exec(sb)          (((sb)->st_mode & S_IXUSR) == S_IXUSR)
#define lf_file_access(sb)      ((sb)->st_mode & 0777)
#define lf_file_size(sb)        (sb)->st_size
#define lf_file_fs_size(sb)     lf_max((sb)->st_size, (sb)->st_blocks * 512)
#define lf_file_mtime(sb)       (sb)->st_mtime
#define lf_file_uniq(sb)        (sb)->st_ino


lf_int_t lf_create_file_mapping(lf_file_mapping_t *fm);
void lf_close_file_mapping(lf_file_mapping_t *fm);


#define lf_realpath(p, r)       (u_char *) realpath((char *) p, (char *) r)
#define lf_realpath_n           "realpath()"
#define lf_getcwd(buf, size)    (getcwd((char *) buf, size) != NULL)
#define lf_getcwd_n             "getcwd()"
#define lf_path_separator(c)    ((c) == '/')


#if defined(PATH_MAX)

#define lf_HAVE_MAX_PATH        1
#define lf_MAX_PATH             PATH_MAX

#else

#define lf_MAX_PATH             4096

#endif


#define lf_DIR_MASK_LEN         0


lf_int_t lf_open_dir(lf_str_t *name, lf_dir_t *dir);
#define lf_open_dir_n           "opendir()"


#define lf_close_dir(d)         closedir((d)->dir)
#define lf_close_dir_n          "closedir()"


lf_int_t lf_read_dir(lf_dir_t *dir);
#define lf_read_dir_n           "readdir()"


#define lf_create_dir(name, access) mkdir((const char *) name, access)
#define lf_create_dir_n         "mkdir()"


#define lf_delete_dir(name)     rmdir((const char *) name)
#define lf_delete_dir_n         "rmdir()"


#define lf_dir_access(a)        (a | (a & 0444) >> 2)


#define lf_de_name(dir)         ((u_char *) (dir)->de->d_name)
#if (lf_HAVE_D_NAMLEN)
#define lf_de_namelen(dir)      (dir)->de->d_namlen
#else
#define lf_de_namelen(dir)      lf_strlen((dir)->de->d_name)
#endif

static lf_inline lf_int_t
lf_de_info(u_char *name, lf_dir_t *dir)
{
    dir->type = 0;
    return stat((const char *) name, &dir->info);
}

#define lf_de_info_n            "stat()"
#define lf_de_link_info(name, dir)  lstat((const char *) name, &(dir)->info)
#define lf_de_link_info_n       "lstat()"

#if (lf_HAVE_D_TYPE)

/*
 * some file systems (e.g. XFS on Linux and CD9660 on FreeBSD)
 * do not set dirent.d_type
 */

#define lf_de_is_dir(dir)                                                   \
    (((dir)->type) ? ((dir)->type == DT_DIR) : (S_ISDIR((dir)->info.st_mode)))
#define lf_de_is_file(dir)                                                  \
    (((dir)->type) ? ((dir)->type == DT_REG) : (S_ISREG((dir)->info.st_mode)))
#define lf_de_is_link(dir)                                                  \
    (((dir)->type) ? ((dir)->type == DT_LNK) : (S_ISLNK((dir)->info.st_mode)))

#else

#define lf_de_is_dir(dir)       (S_ISDIR((dir)->info.st_mode))
#define lf_de_is_file(dir)      (S_ISREG((dir)->info.st_mode))
#define lf_de_is_link(dir)      (S_ISLNK((dir)->info.st_mode))

#endif

#define lf_de_access(dir)       (((dir)->info.st_mode) & 0777)
#define lf_de_size(dir)         (dir)->info.st_size
#define lf_de_fs_size(dir)                                                  \
    lf_max((dir)->info.st_size, (dir)->info.st_blocks * 512)
#define lf_de_mtime(dir)        (dir)->info.st_mtime


lf_int_t lf_open_glob(lf_glob_t *gl);
#define lf_open_glob_n          "glob()"
lf_int_t lf_read_glob(lf_glob_t *gl, lf_str_t *name);
void lf_close_glob(lf_glob_t *gl);


lf_err_t lf_trylock_fd(lf_fd_t fd);
lf_err_t lf_lock_fd(lf_fd_t fd);
lf_err_t lf_unlock_fd(lf_fd_t fd);

#define lf_trylock_fd_n         "fcntl(F_SETLK, F_WRLCK)"
#define lf_lock_fd_n            "fcntl(F_SETLKW, F_WRLCK)"
#define lf_unlock_fd_n          "fcntl(F_SETLK, F_UNLCK)"


#if (lf_HAVE_F_READAHEAD)

#define lf_HAVE_READ_AHEAD      1

#define lf_read_ahead(fd, n)    fcntl(fd, F_READAHEAD, (int) n)
#define lf_read_ahead_n         "fcntl(fd, F_READAHEAD)"

#elif (lf_HAVE_POSIX_FADVISE)

#define lf_HAVE_READ_AHEAD      1

lf_int_t lf_read_ahead(lf_fd_t fd, size_t n);
#define lf_read_ahead_n         "posix_fadvise(POSIX_FADV_SEQUENTIAL)"

#else

#define lf_read_ahead(fd, n)    0
#define lf_read_ahead_n         "lf_read_ahead_n"

#endif


#if (lf_HAVE_O_DIRECT)

lf_int_t lf_directio_on(lf_fd_t fd);
#define lf_directio_on_n        "fcntl(O_DIRECT)"

lf_int_t lf_directio_off(lf_fd_t fd);
#define lf_directio_off_n       "fcntl(!O_DIRECT)"

#elif (lf_HAVE_F_NOCACHE)

#define lf_directio_on(fd)      fcntl(fd, F_NOCACHE, 1)
#define lf_directio_on_n        "fcntl(F_NOCACHE, 1)"

#elif (lf_HAVE_DIRECTIO)

#define lf_directio_on(fd)      directio(fd, DIRECTIO_ON)
#define lf_directio_on_n        "directio(DIRECTIO_ON)"

#else

#define lf_directio_on(fd)      0
#define lf_directio_on_n        "lf_directio_on_n"

#endif

size_t lf_fs_bsize(u_char *name);


#if (lf_HAVE_OPENAT)

#define lf_openat_file(fd, name, mode, create, access)                      \
    openat(fd, (const char *) name, mode|create, access)

#define lf_openat_file_n        "openat()"

#define lf_file_at_info(fd, name, sb, flag)                                 \
    fstatat(fd, (const char *) name, sb, flag)

#define lf_file_at_info_n       "fstatat()"

#define lf_AT_FDCWD             (lf_fd_t) AT_FDCWD

#endif


#define lf_stdout               STDOUT_FILENO
#define lf_stderr               STDERR_FILENO
#define lf_set_stderr(fd)       dup2(fd, STDERR_FILENO)
#define lf_set_stderr_n         "dup2(STDERR_FILENO)"


#if (lf_HAVE_FILE_AIO)

lf_int_t lf_file_aio_init(lf_file_t *file, lf_pool_t *pool);
ssize_t lf_file_aio_read(lf_file_t *file, u_char *buf, size_t size,
    off_t offset, lf_pool_t *pool);

extern lf_uint_t  lf_file_aio;

#endif

#if (lf_THREADS)
ssize_t lf_thread_read(lf_file_t *file, u_char *buf, size_t size,
    off_t offset, lf_pool_t *pool);
ssize_t lf_thread_write_chain_to_file(lf_file_t *file, lf_chain_t *cl,
    off_t offset, lf_pool_t *pool);
#endif


#endif /* _lf_FILES_H_INCLUDED_ */
