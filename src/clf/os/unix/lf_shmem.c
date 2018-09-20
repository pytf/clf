
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <lf_config.h>
#include <lf_core.h>


#if (lf_HAVE_MAP_ANON)

lf_int_t
lf_shm_alloc(lf_shm_t *shm)
{
    shm->addr = (u_char *) mmap(NULL, shm->size,
                                PROT_READ|PROT_WRITE,
                                MAP_ANON|MAP_SHARED, -1, 0);

    if (shm->addr == MAP_FAILED) {
        lf_log_error(lf_LOG_ALERT, shm->log, lf_errno,
                      "mmap(MAP_ANON|MAP_SHARED, %uz) failed", shm->size);
        return lf_ERROR;
    }

    return lf_OK;
}


void
lf_shm_free(lf_shm_t *shm)
{
    if (munmap((void *) shm->addr, shm->size) == -1) {
        lf_log_error(lf_LOG_ALERT, shm->log, lf_errno,
                      "munmap(%p, %uz) failed", shm->addr, shm->size);
    }
}

#elif (lf_HAVE_MAP_DEVZERO)

lf_int_t
lf_shm_alloc(lf_shm_t *shm)
{
    lf_fd_t  fd;

    fd = open("/dev/zero", O_RDWR);

    if (fd == -1) {
        lf_log_error(lf_LOG_ALERT, shm->log, lf_errno,
                      "open(\"/dev/zero\") failed");
        return lf_ERROR;
    }

    shm->addr = (u_char *) mmap(NULL, shm->size, PROT_READ|PROT_WRITE,
                                MAP_SHARED, fd, 0);

    if (shm->addr == MAP_FAILED) {
        lf_log_error(lf_LOG_ALERT, shm->log, lf_errno,
                      "mmap(/dev/zero, MAP_SHARED, %uz) failed", shm->size);
    }

    if (close(fd) == -1) {
        lf_log_error(lf_LOG_ALERT, shm->log, lf_errno,
                      "close(\"/dev/zero\") failed");
    }

    return (shm->addr == MAP_FAILED) ? lf_ERROR : lf_OK;
}


void
lf_shm_free(lf_shm_t *shm)
{
    if (munmap((void *) shm->addr, shm->size) == -1) {
        lf_log_error(lf_LOG_ALERT, shm->log, lf_errno,
                      "munmap(%p, %uz) failed", shm->addr, shm->size);
    }
}

#elif (lf_HAVE_SYSVSHM)

#include <sys/ipc.h>
#include <sys/shm.h>


lf_int_t
lf_shm_alloc(lf_shm_t *shm)
{
    int  id;

    id = shmget(IPC_PRIVATE, shm->size, (SHM_R|SHM_W|IPC_CREAT));

    if (id == -1) {
        lf_log_error(lf_LOG_ALERT, shm->log, lf_errno,
                      "shmget(%uz) failed", shm->size);
        return lf_ERROR;
    }

    lf_log_debug1(lf_LOG_DEBUG_CORE, shm->log, 0, "shmget id: %d", id);

    shm->addr = shmat(id, NULL, 0);

    if (shm->addr == (void *) -1) {
        lf_log_error(lf_LOG_ALERT, shm->log, lf_errno, "shmat() failed");
    }

    if (shmctl(id, IPC_RMID, NULL) == -1) {
        lf_log_error(lf_LOG_ALERT, shm->log, lf_errno,
                      "shmctl(IPC_RMID) failed");
    }

    return (shm->addr == (void *) -1) ? lf_ERROR : lf_OK;
}


void
lf_shm_free(lf_shm_t *shm)
{
    if (shmdt(shm->addr) == -1) {
        lf_log_error(lf_LOG_ALERT, shm->log, lf_errno,
                      "shmdt(%p) failed", shm->addr);
    }
}

#endif
