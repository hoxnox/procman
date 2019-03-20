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

