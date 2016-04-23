# TODO: separate debug setting

env = Environment(
	CCFLAGS=['-std=c++11', '-Wall', '-g', '-O0']
)


env.ParseConfig('pkg-config --cflags --libs libzmq')

zmqu_objs = env.Object(Glob('zmqu/*.cpp'))

env.Program(['rrserv.cpp', zmqu_objs])
env.Program(['rrclient.cpp', zmqu_objs])

env.Program(['variable_recv_client.cpp', zmqu_objs])
env.Program(['variable_recv_serv.cpp', zmqu_objs])
