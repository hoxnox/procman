Process Manager
===============

Process Manager (procman) helps to create daemons and applications that
is sensible to process properties. When the application starts in UNIX
environment, it inherits a lot of process properties that are
inappropriate for daemons and may be undesirable for some utilities.

Below are problems, that procman helps to solve:

- `set_no_std_fd()` redirects `stdin`, `stdout` and `stderr` to
  `/dev/null`. It is mandatory for security reasons. Suppose you forked
  daemon from your terminal and switched off the session. Now if the
  other user will login on the same terminal device, she will receive
  all daemon's output.
- `set_work_dir(std::string)` changes current working directory to
  given, or to "/" by default. Helps avoid hanging when some daemon
  blocks unmount the filesystem because it's working directory is on it.
- `set_umask(int)` sets file mode creation mask to a known value. Daemon
  shouldn't rely on the parent's one - it could be `rwxrwxrwx`.
- `set_close_all_fd()` closes all file descriptors to prevent unneded
  holdings.
- `set_pid_file(std::string)` cause the application to create `PID` file
  and lock it. It helps to run only one copy of the application.
- `set_signals()` call `on_stop` callback on `SIGTERM`, `SIGQUIT`
  `SIGINT` receiving instead of unexpected process termination.
- `set_no_control_tty()` disassociate the process from the calling terminal,
  make it leader of the new session and process group.
- You can initiate normal stop process from any thread of the program
  with help of `process` singleton.
- You can easy register some useful operations on receiving `SIGHUP`,
  `SIGUSR1` or `SIGUSR2`.
- and others ...

You don't have to do all the listed above. You can configure exactly
what you need for your particular case - see "Example: fine tuning".

Building
--------

The library is very small - just one `cpp` file. You can build it as
usual library with help of cmake:

```sh
cd ~/procman
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/procman ..
make install 
g++ -std=c++11 -I /path/to/procman/include -L /path/to/procman/lib test.cpp -l procman
```

or by hand:

```sh
g++ -std=c++11 -c -o procman.o ~/procman/src/procman.cpp
g++ -std=c++11 -I ~/procman/include test.cpp ptocman.o
```


Examples
--------

Watch your messages:

```sh
sudo tail -f /var/log/messages
```

Launch the example and play with it - try to launch several copies, send
signals with help of kill:

```sh
kill -s SIGHUP <PID>
```

### daemonize

The code below is fully functional daemon with `PID` at `/tmp/my_daemon.pid`.

> Default daemonize set PID at `/var/run/<name>.pid`. But I decided to
> change this location to `/tmp/my_daemon.pid` - in that case you don't
> need root privileges to start the daemon.

```c++
#include <syslog.h>
#include <unistd.h>

#include <procman.hpp>

using namespace procman;

int
main(int argc, char* argv[])
{
	bool stop = false;
	openlog("my_daemon", 0, LOG_USER);
	auto pm = proc_builder("my_daemon").daemonize()
		.set_pid_file("/tmp/my_daemon.pid")
		.on_stop([&stop](){ syslog(LOG_INFO, "stopped"); stop = true; })
		.on_hup([](){ syslog(LOG_INFO, "config update"); })
		.on_start([&stop]()
			{
				syslog(LOG_INFO, "started");
				while (!stop)
					usleep(100);
			});
	if (!pm.start())
		syslog(LOG_ERR, "Process error: %s", pm.strerror().c_str());
	return 0;
}
```

### fine tuning


Try to start several instances and send them `SIGHUP` and `SIGTERM`.

```c++
#include <syslog.h>
#include <unistd.h>
#include <iostream>

#include <procman.hpp>

using namespace procman;

int
main(int argc, char* argv[])
{
	bool stop = false;
	openlog("my_daemon", 0, LOG_USER);
	auto pm = proc_builder("my_daemon")
		.set_work_dir("/tmp")
		.set_no_control_tty()
		.on_stop([&stop](process::stop_reason_t reason)
			{
				if (reason == process::STOP_SIGNAL)
					syslog(LOG_INFO, "stopped signal");
				syslog(LOG_INFO, "stopped normal");
				stop = true;
			})
		.on_hup([](std::shared_ptr<process> p) // set_signals() automatically
			{
				syslog(LOG_ERR, "imagine error, normal stop");
				p->stop();
			})
		.on_start([&stop]()
			{
				syslog(LOG_INFO, "started");
				std::cout << getpid() << std::endl;
				while (!stop)
					usleep(100);
			});
	if (!pm.start())
		syslog(LOG_ERR, "Process error: %s", pm.strerror().c_str());
	return 0;
}
```

