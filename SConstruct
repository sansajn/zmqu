# --release-build option
AddOption(
	'--release-build', 
	action='store_true', 
	dest='release_build', 
	help='create optimized build ment for release', 
	default=False
)

cxxflags = ['-std=c++14', '-Wall']

if GetOption('release_build'):
	cxxflags.extend(['-O2'])
else:
	cxxflags.extend(['-g', '-O0'])

env = Environment(
	CXXFLAGS=cxxflags,
	CPPPATH=['.'],
	LIBS=['pthread', 'boost_system', 'boost_thread'],
	CPPDEFINES=['BOOST_SPIRIT_THREADSAFE'],
)

env.ParseConfig('pkg-config --cflags --libs libzmq')

zmqu_objs = env.Object(Glob('zmqu/*.cpp'))

# tests
test_env = env.Clone()

test_env.Program('utest', [
	'test/main.cpp',
	Glob('test/test_*.cpp'),
	zmqu_objs
])

# samples
env.Program(['test/client_sock_events.cpp', zmqu_objs])
env.Program(['test/server_sock_events.cpp', zmqu_objs])
env.Program(['test/monitor.cpp', zmqu_objs])
env.Program(['test/recv_vector.cpp', zmqu_objs])

# legacy
#env.Program(['legacy/rrserv.cpp', zmqu_objs])
#env.Program(['legacy/rrclient.cpp', zmqu_objs])
#env.Program(['legacy/ccclient.cpp', zmqu_objs])
#env.Program(['legacy/ccserv.cpp', zmqu_objs])
#env.Program(['legacy/varargs_recv_client.cpp', zmqu_objs])
