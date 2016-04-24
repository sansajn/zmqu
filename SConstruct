# TODO: separate debug setting

env = Environment(
	CCFLAGS=['-std=c++11', '-Wall', '-g', '-O0'],
	CPPPATH=['.'],
	LIBS=['gtest', 'pthread'],
)

env.ParseConfig('pkg-config --cflags --libs libzmq')

zmqu_objs = env.Object(Glob('zmqu/*.cpp'))

env.Program(['rrserv.cpp', zmqu_objs])
env.Program(['rrclient.cpp', zmqu_objs])

env.Program(['ccclient.cpp', zmqu_objs])
env.Program(['ccserv.cpp', zmqu_objs])

env.Program(['test_varargs_recv_send.cpp', zmqu_objs])
env.Program(['varargs_recv_client.cpp', zmqu_objs])

