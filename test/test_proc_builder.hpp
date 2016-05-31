#include <procman.hpp>
#include <sstream>
#include <thread>
#include <chrono>
#include <future>
#include <fstream>

using namespace procman;

#ifdef test_stop_normal
TEST(test_proc_builder, stop_normal)
{
	std::stringstream ss;
	bool stop_flag = false;
	auto pb = proc_builder("test_procman");
	std::promise<void> start_notifier;
	std::future<void> start_waiter = start_notifier.get_future();
	pb.set_work_dir().set_umask().set_signals()
		.on_start([&ss, &stop_flag, &start_notifier](std::shared_ptr<process> proc)
			{
				ss << "hello";
				start_notifier.set_value();
				// loop while separate thread will not stop
				while(!stop_flag)
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
			})
		.on_stop([&ss, &stop_flag](process::stop_reason_t reason,
		                           std::shared_ptr<process> proc)
			{
				ss << "goodbye";
				stop_flag = true;
			});

	// stop the process in separate thread by stop()
	std::thread t([&ss, &start_waiter]()
		{
			start_waiter.wait();
			auto p = process::Get();
			ss << " and ";
			if (p)
				p->stop();
		});

	auto p = pb.start(); // will be stopped in separate thread `t`
	ASSERT_FALSE(!p) << pb.strerror();
	t.join();
	ASSERT_EQ("hello and goodbye", ss.str()) << ss.str();
}
#endif

#ifdef test_stop_signal
TEST(test_proc_builder, stop_signal)
{
	std::stringstream ss;
	bool stop_flag = false;
	auto pb = proc_builder("test_procman");
	std::promise<void> start_notifier;
	std::future<void> start_waiter = start_notifier.get_future();
	pb.set_work_dir().set_umask().set_signals()
		.on_start([&ss, &stop_flag, &start_notifier](std::shared_ptr<process> proc)
			{
				ss << "hello";
				start_notifier.set_value();
				// loop while separate thread will not stop
				while(!stop_flag)
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
			})
		.on_hup([&ss](std::shared_ptr<process> proc)
			{
				ss << " and ";
			})
		.on_stop([&ss, &stop_flag](process::stop_reason_t reason,
		                           std::shared_ptr<process> proc)
			{
				ss << "goodbye";
				stop_flag = true;
			});

	// stop the process in separate thread by SIGTERM
	std::thread t([&ss, &start_waiter]()
		{
			start_waiter.wait();
			auto p = process::Get();
			kill(getpid(), SIGHUP);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			kill(getpid(), SIGTERM);
		});

	auto p = pb.start(); // will be stopped in separate thread `t`
	ASSERT_FALSE(!p) << pb.strerror();
	t.join();
	ASSERT_EQ("hello and goodbye", ss.str()) << ss.str();
}
#endif

#ifdef test_pid_file
TEST(test_proc_builder, pid_file)
{
	std::string pidfname = "/tmp/promon_test_pidfile";
	pid_t pid = fork();
	if (pid == 0) // child
	{
		std::stringstream ss;
		bool stop_flag = false;
		auto pb = proc_builder("test_procman");
		pb.set_work_dir().set_umask().set_pid_file(pidfname).set_signals()
			.on_start([&ss, &stop_flag](std::shared_ptr<process> proc)
				{
					ss << "hello";
					// loop while separate thread will not stop
					while(!stop_flag)
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
				})
			.on_stop([&ss, &stop_flag](process::stop_reason_t reason,
			                           std::shared_ptr<process> proc)
				{
					ss << " and goodbye";
					stop_flag = true;
				});
		auto p = pb.start();
		ASSERT_FALSE(!p) << pb.strerror();
		ASSERT_EQ("hello and goodbye", ss.str()) << ss.str();
	}
	else
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		auto pb = proc_builder("test_procman");
		pb.set_work_dir().set_umask().set_pid_file(pidfname).set_signals()
			.on_start([](std::shared_ptr<process> proc) { void(0); });
		auto p = pb.start();
		kill(pid, SIGTERM);
		ASSERT_TRUE(!p);
		ASSERT_NE(std::string::npos, pb.strerror().find("already running"))
			<< pb.strerror();
	}
}
#endif

