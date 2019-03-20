/**@author hoxnox <hoxnox@gmail.com>
 * @date 20160531 16:13:29*/

#include <sstream>
#include <cstring>

#include <procman.hpp>

#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#ifndef _
#define _(x) (x)
#endif

namespace procman {

std::shared_ptr<process> process::impl_{nullptr};

class Result
{
public:
	Result() {};
	Result(const Result& copy) { operator=(copy); }
	Result& operator=(const Result& copy) {err_.str(copy.err_.str()); return *this; }
	operator bool()
	{
		return err_.str().empty();
	}
	operator const char*()
	{
		return err_.str().c_str();
	}
	Result& operator<<(std::string str)
	{
		err_ << str;
		return *this;
	}
	static Result OK;
private:
	std::stringstream err_;
};

Result Result::OK;

Result
do_no_std_fd()
{
	if (freopen("/dev/null", "r", stdin) == nullptr)
	{
		return Result() << _("Error setting stdin to /dev/null.")
		                << _("Message: ") << strerror(errno);
	}
	if (freopen("/dev/null", "w", stdout) == nullptr)
	{
		return Result() << _("Error setting stdout to /dev/null.")
		                << _("Message: ") << strerror(errno);
	}
	if (freopen("/dev/null", "w", stderr) == nullptr)
	{
		return Result() << _("Error setting stderr to /dev/null.")
		                << _("Message: ") << strerror(errno);
	}
	return Result::OK;
}

Result
do_close_all_fd()
{
	struct rlimit limit;
	if (getrlimit(RLIMIT_NOFILE, &limit) < 0)
	{
		return Result() << _("Error retrieving soft resource limits.")
		                << _(" Message: ") << strerror(errno);
	}
	if (limit.rlim_max == RLIM_INFINITY)
		limit.rlim_max = 1024;
	for (size_t i = 0; i < limit.rlim_max; ++i)
		close(i);
	return Result::OK;
}

int
lockfile(int fd)
{
	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 10;
	return fcntl(fd, F_SETLK, &fl);
}

Result
do_pid_file(std::string filename, int& fd)
{
	fd = open(filename.c_str(),
			O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (fd < 0)
	{
		return Result() << _("Cannot open pidfile.")
		                << _(" Filename: \"") << filename << "\""
		                << _(" Message: ") << strerror(errno);
	}
	if (lockfile(fd) < 0)
	{
		if (errno == EACCES || errno == EAGAIN)
		{
			return Result() << _("The process already running.")
			                << _(" Pidfile: \"") << filename.c_str() << "\"";
		}
		return Result() << _("The process already running.")
		                << _(" Pidfile: \"") << filename.c_str() << "\""
		                << _(" Message: ") << strerror(errno);
	}
	auto _ = ftruncate(fd, 0);
	std::stringstream ss;
	ss << getpid();
	_ = write(fd, ss.str().c_str(), ss.str().length());
	return Result::OK;
}

process::~process()
{
	if (opts_ & OPT_SET_PID)
		if (!pidfile_.empty())
			unlink(pidfile_.c_str());
}

Result
do_work_dir(std::string filename = "/")
{
	if (chdir("/") < 0)
	{
		return Result() << _("Error changing working directory.")
		                << _(" Message: ") << strerror(errno);
	}
	return Result::OK;
}

Result
do_umask(mode_t  umsk = 0)
{
	umask(umsk);
	return Result::OK;
}

Result
do_no_cntl_tty()
{
	pid_t pid = 0;
	if ((pid = fork()) < 0)
	{
		return Result() << _("Error in attempt to fork process.")
		                << _(" Message: ") << strerror(errno);
	}
	else if (pid != 0) // parent
		exit(0);
	if (setsid() < 0)
	{
		return Result() << _("Error running in new session in attempt"
		                     " to loose controling tty.")
		                << _(" Message: ") << strerror(errno);
	}

	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
	{
		return Result() << _("Error in attempt to ignore SIGHUP.")
		                << _(" Message: ") << strerror(errno);
	}
	if ((pid = fork()) < 0)
	{
		return Result() << _("Error in attempt to fork process.")
		                << _(" Message: ") << strerror(errno);
	}
	else if (pid != 0) // parent
		exit(0);
	return Result::OK;
}

void
process::emit(int signal)
{
	switch(signal)
	{
		case SIGQUIT:
		case SIGINT:
		case SIGTERM:
			if (!on_stop_)
				return;
			on_stop_(process::STOP_SIGNAL, impl_);
			break;
		case SIGUSR1:
			if (!on_usr1_)
				return;
			on_usr1_(impl_);
			break;
		case SIGUSR2:
			if (!on_usr2_)
				return;
			on_usr2_(impl_);
			break;
		case SIGHUP:
			if (!on_hup_)
				return;
			on_hup_(impl_);
			break;
	}
}

void
signal_handler(int sig, siginfo_t *si, void* ptr)
{
	auto proc = process::get();
	if (!proc)
		return;
	proc->emit(sig);
}

Result
do_signals(std::shared_ptr<process> proc)
{
	struct sigaction sigact;
	sigact.sa_flags = SA_SIGINFO;
	sigact.sa_sigaction = signal_handler;
	sigemptyset(&sigact.sa_mask);
	if (sigaction(SIGQUIT, &sigact, 0) < 0
	 || sigaction(SIGINT , &sigact, 0) < 0
	 || sigaction(SIGTERM, &sigact, 0) < 0
	 || sigaction(SIGUSR1, &sigact, 0) < 0
	 || sigaction(SIGUSR2, &sigact, 0) < 0
	 || sigaction(SIGHUP , &sigact, 0) < 0)
	{
		return Result() << _("Error registering signal handler.")
		                << _(" Message: ") << strerror(errno);
	}
	return Result::OK;
}

std::shared_ptr<process>
proc_builder::start()
{
	Result rs;
	if (!process_)
		return nullptr;
	if (rs && (process_->opts_ & process::OPT_SET_UMASK))
		rs = do_umask(static_cast<mode_t>(process_->umask_));
	if (rs && (process_->opts_ & process::OPT_CLOSE_ALL_FD))
		rs = do_close_all_fd();
	if (rs && (process_->opts_ & process::OPT_NO_CNTL_TTY))
		rs = do_no_cntl_tty();
	if (rs && (process_->opts_ & process::OPT_SET_SIGNALS))
		rs = do_signals(process_);
	if (rs && (process_->opts_ & process::OPT_SET_WORKDIR))
		rs = do_work_dir(process_->workdir_);
	if (rs && (process_->opts_ & process::OPT_NO_STD_FD))
		rs = do_no_std_fd();
	if (rs && (process_->opts_ & process::OPT_SET_PID))
	{
		rs = do_pid_file(process_->pidfile_, process_->pidfd_);
		if (!rs)
			process_->pidfile_.clear();
	}
	if (!rs)
	{
		last_error_.assign((const char*)rs);
		return nullptr;
	}
	if (process_->on_start_)
		process_->on_start_(process_);
	return process_;
}

} // namespace

