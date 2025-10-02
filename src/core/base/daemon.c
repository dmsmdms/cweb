#include <core/base/daemon.h>
#include <core/base/file.h>
#include <core/base/log.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

static const char *pid_file_path = NULL;

daemon_err_t daemon_init(const char *pid_path)
{
    if(access(pid_path, F_OK) == 0) {
        log_error("pid file %s exists", pid_path);
        return DAEMON_ERR_PID_FILE;
    }

    pid_t pid = fork();
    if(pid < 0) {
        log_error("fork failed - %s", strerror(errno));
        return DAEMON_ERR_FORK;
    } else if(pid > 0) {
        if(file_write(pid_path, &pid, sizeof(pid_t)) != FILE_ERR_OK) {
            return DAEMON_ERR_PID_FILE;
        }
        return DAEMON_ERR_PARENT;
    }

    if(setsid() < 0) {
        log_error("setsid failed - %s", strerror(errno));
        return DAEMON_ERR_CHILD;
    }
    signal(SIGHUP, SIG_IGN);
    pid_file_path = pid_path;

    return DAEMON_ERR_OK;
}

daemon_err_t daemon_stop(const char *pid_path)
{
    pid_t pid;
    if(file_read(pid_path, &pid, sizeof(pid_t)) != FILE_ERR_OK) {
        return DAEMON_ERR_PID_FILE;
    }
    if(kill(pid, SIGTERM) < 0) {
        log_error("send SIGTERM to pid %d - %s", pid, strerror(errno));
        return DAEMON_ERR_KILL;
    }
    return DAEMON_ERR_OK;
}

void daemon_cleanup(void)
{
    if(pid_file_path) {
        unlink(pid_file_path);
        pid_file_path = NULL;
    }
}
