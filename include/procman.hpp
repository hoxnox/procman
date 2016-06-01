/**@author hoxnox <hoxnox@gmail.com>
 * @date 20160531 16:13:29 */

#pragma once

#include <functional>
#include <string>
#include <memory>
#include <cstdint>

namespace procman {

class proc_builder;

/**@brief Can be used to stop or retrieve some options.*/
class process
{
public:
	~process();
	enum stop_reason_t
	{
		STOP_NORMAL   = 1,
		STOP_SIGNAL   = 2, 
		STOP_CRITICAL = 3
	};
	/**Stop current process.*/
	void stop(stop_reason_t reason = STOP_NORMAL);
	/**Get current process instance.*/
	static std::shared_ptr<process> get() { return impl_; }
	void emit(int signal);

private:
	enum Options : int16_t {
		OPT_NO_STD_FD    =  1 << 1,
		OPT_CLOSE_ALL_FD =  1 << 2,
		OPT_SET_PID      =  1 << 3,
		OPT_SET_WORKDIR  =  1 << 4,
		OPT_SET_UMASK    =  1 << 5,
		OPT_SET_SIGNALS  =  1 << 6,
		OPT_NO_CNTL_TTY  =  1 << 7
	};
	process() {}
	process(const process&) = delete;
	process& operator=(const process&) = delete;
	std::string              name_;
	std::string              pidfile_;
	int                      pidfd_;
	std::string              workdir_;
	int16_t                  opts_{0};
	int                      umask_{0};
	std::function<void(stop_reason_t, std::shared_ptr<process>)>
	                                              on_stop_ {nullptr};
	std::function<void(std::shared_ptr<process>)> on_start_{nullptr};
	std::function<void(std::shared_ptr<process>)> on_hup_  {nullptr};
	std::function<void(std::shared_ptr<process>)> on_usr1_ {nullptr};
	std::function<void(std::shared_ptr<process>)> on_usr2_ {nullptr};

	static std::shared_ptr<process> impl_;

friend class proc_builder;
};

/**@brief Configure application's main process and replace current with
 * the configured when Start() will be called. You can get option values
 * with help of `process` singleton.*/
class proc_builder
{
public:
	/**@note Setup some options first.
	 * 
	 * @return nullptr on error - use `strerror()` to retrieve error
	 * message.*/
	std::shared_ptr<process> start();
	std::string strerror() { return last_error_; };

	proc_builder(std::string name);

	/**@brief Set 0,1 and 2 file descriptors to /dev/null*/
	proc_builder& set_no_std_fd();
	/**@brief Close all open file descriptors.*/
	proc_builder& set_close_all_fd();
	/**@brief Set PID file name. If name is empty /var/run/<name>.pid is
	 * used. If not called - no pid file would be created.*/
	proc_builder& set_pid_file(std::string filename = "");
	/**@brief Change the current working directory. With no argument -
	 * root directory is used. If not called - doesn't changes.*/
	proc_builder& set_work_dir(std::string filename = "/");
	/**@brief Change file mode mask. Default is 0.*/
	proc_builder& set_umask(int umask = 0);
	/**@brief Setup signals.*/
	proc_builder& set_signals();
	/**@brief Lose controlling TTY*/
	proc_builder& set_no_control_tty();
	/**@brief Sugar for all `set` calls with default arguments.*/
	proc_builder& daemonize();

	/**@brief Callback on starting.*/
	proc_builder& on_start(
		std::function<void(std::shared_ptr<process>)> callback);
	proc_builder& on_start(std::function<void()> callback);

	/**@brief Callback on stopping.*/
	proc_builder& on_stop(
		std::function<void(process::stop_reason_t,
		                   std::shared_ptr<process>)> callback);
	proc_builder& on_stop(
		std::function<void(process::stop_reason_t)> callback);
	proc_builder& on_stop(
		std::function<void(std::shared_ptr<process>)> callback);
	proc_builder& on_stop(std::function<void()> callback);

	/**@brief Callback on SIGHUP receiving.*/
	proc_builder& on_hup(
		std::function<void(std::shared_ptr<process>)> callback);
	proc_builder& on_hup(std::function<void()> callback);

	/**@brief Callback on SIGUSR1 receiving.*/
	proc_builder& on_usr1(
		std::function<void(std::shared_ptr<process>)> callback);
	proc_builder& on_usr1(std::function<void()> callback);

	/**@brief Callback on SIGUSR2 receiving.*/
	proc_builder& on_usr2(
		std::function<void(std::shared_ptr<process>)> callback);
	proc_builder& on_usr2(std::function<void()> callback);

private:
	std::shared_ptr<process> process_;
	std::string last_error_;
};


////////////////////////////////////////////////////////////////////////
// inline

inline proc_builder::proc_builder(std::string name)
	: process_(new process())
{
	process_->name_ = name;
	process_->impl_ = process_;
}

inline proc_builder&
proc_builder::set_no_control_tty()
{
	process_->opts_ |= process::OPT_NO_CNTL_TTY;
	return *this;
}

inline proc_builder&
proc_builder::set_signals()
{
	process_->opts_ |= process::OPT_SET_SIGNALS;
	return *this;
}

inline proc_builder&
proc_builder::set_umask(int umask)
{
	process_->opts_ |= process::OPT_SET_UMASK;
	process_->umask_ = umask;
	return *this;
}

inline proc_builder&
proc_builder::set_no_std_fd()
{
	process_->opts_ |= process::OPT_NO_STD_FD;
	return *this;
}

inline proc_builder&
proc_builder::set_close_all_fd()
{
	process_->opts_ |= process::OPT_CLOSE_ALL_FD;
	return *this;
}

inline proc_builder&
proc_builder::set_pid_file(std::string filename)
{
	process_->opts_ |= process::OPT_SET_PID;
	process_->pidfile_ = filename;
	return *this;
}

inline proc_builder&
proc_builder::set_work_dir(std::string filename)
{
	process_->opts_ |= process::OPT_SET_WORKDIR;
	process_->workdir_ = filename;
	return *this;
}

inline proc_builder&
proc_builder::daemonize()
{
	process_->opts_ = process::OPT_NO_STD_FD
	                | process::OPT_CLOSE_ALL_FD
	                | process::OPT_SET_PID
	                | process::OPT_SET_WORKDIR
	                | process::OPT_SET_UMASK
	                | process::OPT_SET_SIGNALS
	                | process::OPT_NO_CNTL_TTY;
	process_->pidfile_ = std::string("/var/run/") + process_->name_ + ".pid";
	process_->workdir_ = "/";
	process_->umask_ = 0;
	return *this;
}

inline proc_builder&
proc_builder::on_start(std::function<void(std::shared_ptr<process>)> callback)
{
	process_->on_start_ = callback;
	return *this;
}

inline proc_builder&
proc_builder::on_start(std::function<void()> callback)
{
	process_->on_start_ = [callback](std::shared_ptr<process>){ callback(); };
	return *this;
}

inline proc_builder&
proc_builder::on_stop(std::function<void(process::stop_reason_t,
	                  std::shared_ptr<process>)> callback)
{
	process_->on_stop_ = callback;
	return *this;
}

inline proc_builder&
proc_builder::on_stop(std::function<void(process::stop_reason_t)> callback)
{
	process_->on_stop_ = [callback](process::stop_reason_t reason,
	                                std::shared_ptr<process> )
	                               { callback(reason); };
	return *this;
}

inline proc_builder&
proc_builder::on_stop(std::function<void(std::shared_ptr<process>)> callback)
{
	process_->on_stop_ = [callback](process::stop_reason_t,
	                                std::shared_ptr<process> p)
	                               { callback(p); };
	return *this;
}

inline proc_builder&
proc_builder::on_stop(std::function<void()> callback)
{
	process_->on_stop_ = [callback](process::stop_reason_t,
	                                std::shared_ptr<process>)
	                               { callback(); };
	return *this;
}

inline proc_builder&
proc_builder::on_hup(std::function<void(std::shared_ptr<process>)> callback)
{
	set_signals();
	process_->on_hup_ = callback;
	return *this;
}

inline proc_builder&
proc_builder::on_hup(std::function<void()> callback)
{
	set_signals();
	process_->on_hup_ = [callback](std::shared_ptr<process>){callback();};
	return *this;
}

inline proc_builder&
proc_builder::on_usr1(std::function<void(std::shared_ptr<process>)> callback)
{
	set_signals();
	process_->on_usr1_ = callback;
	return *this;
}

inline proc_builder&
proc_builder::on_usr1(std::function<void()> callback)
{
	set_signals();
	process_->on_usr1_ = [callback](std::shared_ptr<process>){ callback(); };
	return *this;
}

inline proc_builder&
proc_builder::on_usr2(std::function<void(std::shared_ptr<process>)> callback)
{
	set_signals();
	process_->on_usr2_ = callback;
	return *this;
}

inline proc_builder&
proc_builder::on_usr2(std::function<void()> callback)
{
	set_signals();
	process_->on_usr2_ = [callback](std::shared_ptr<process>){ callback(); };
	return *this;
}

inline void
process::stop(stop_reason_t reason)
{
	if (on_stop_)
		on_stop_(reason, impl_);
}

} // namespace

