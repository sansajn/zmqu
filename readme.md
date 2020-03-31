# zmqu

ZeroMQ C++ clone pattern client/server implementation. Server part is implemented by `zmqu::clone_server` and client part by `zmqu::clone_client` class.

The simplest possible server implementation can look like this

```c++
class simple_server : public zmqu::clone_server 
{
public:
	string on_question(string const & q) override {
		cout << "question: " << q << endl;
		if (q == "who is there?")
			return "ZMQ clone pattern server implementation!";
		else
			return string{};  // no answer
	}

	void on_notify(string const & s) override {
		cout << "notify: " << s << endl;
	}
};

void main() 
{
	// create, bind and start server
	async<simple_server> serv;
	serv.bind(5556, 5557, 5558);
	serv.run();
	wait_event([&serv]{return serv.ready();}, seconds{1});  // wait for server thread
	
	serv.publish("welcome all ...");

	serv.join();
}
```

and the simplest client implementation can looks like this

```c++
struct simple_client : public zmqu::clone_client
{
	void on_news(std::string const & s) override {
		cout << "news: " << s << endl;
	}

	void on_answer(std::string const & answer) override {
		cout << "answer: " << answer << endl;
	}
};

void main()
{
	async<simple_client> cli;
	cli.connect("localhost", 5556, 5557, 5558);
	cli.run();
	wait_event([&cli]{return cli.ready();}, seconds{1});
	
	cli.notify("client ready");
	cli.ask("who is there?");

	cli.join();
}
```

See `test/` directory for more samples.


## how to build

install dependences: 

	libzmq3-dev (4.1.4-7), 
	libboost-all-dev (1.58),
	catch (1.2.0.-1)

and just run

	$ scons -j8

command from `zmqu/` directory.

