# --release-build option
AddOption(
	'--release-build', 
	action='store_true', 
	dest='release_build', 
	help='create optimized build ment for release', 
	default=False
)

cxxflags = ['-std=c++11', '-Wall']

if GetOption('release_build'):
	cxxflags.extend(['-O2'])
else:
	cxxflags.extend(['-g', '-O0'])

env = Environment(
	CXXFLAGS=cxxflags,
	CPPPATH=['.'],
	LIBS=['gtest', 'pthread', 'boost_system', 'boost_thread'],
	CPPDEFINES=['BOOST_SPIRIT_THREADSAFE'],
)

env.ParseConfig('pkg-config --cflags --libs libzmq')

zmqu_objs = env.Object(Glob('zmqu/*.cpp'))

# tests
env.Program([
	'tests.cpp',
	'test_clone_client.cpp',
	'test_varargs_recv_send.cpp',
	'test_cclient_cserv.cpp',
	'test_clone_server.cpp',
	zmqu_objs
])

env.Program(['monit.cpp', zmqu_objs])

# legacy
env.Program(['legacy/rrserv.cpp', zmqu_objs])
env.Program(['legacy/rrclient.cpp', zmqu_objs])
env.Program(['legacy/ccclient.cpp', zmqu_objs])
env.Program(['legacy/ccserv.cpp', zmqu_objs])
env.Program(['legacy/varargs_recv_client.cpp', zmqu_objs])
