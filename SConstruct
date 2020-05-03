# dependencies:
#   libzmq3-dev (4.1.4-7, ubuntu 16.04)
#   libboost-all-dev (1.58.0, ubuntu 16.04)
#   catch (1.2.0-1, ubuntu 16.04)

# --release-build option
AddOption(
	'--release-build', 
	action='store_true', 
	dest='release_build', 
	help='create optimized build ment for release', 
	default=False
)

# pkg-config based dependencies
dependencies = ['libzmq >= 4.3.2']


def main():
	zmqu_lib = build_zmqu()
	build_tests(zmqu_lib)


def build_zmqu():
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

	check_dependencies(env)

	env.ParseConfig('pkg-config --cflags --libs libzmq')

	zmqu_objs = env.Object(Glob('zmqu/*.cpp'))

	# static libzmqu library
	zmqu_lib = env.StaticLibrary('zmqu', [zmqu_objs])

	return zmqu_lib

def build_tests(zmqu_lib):

	test_env = Environment(
		CCFLAGS=['-std=c++17', '-O0', '-g', '-Wall'],
		LIBS=['pthread', 'boost_thread', 'boost_system', 'boost_fiber',
			'boost_context', 'boost_log'],
		CPPDEFINES=['BOOST_SPIRIT_THREADSAFE', 'BOOST_LOG_DYN_LINK'],
		CPPPATH=['.'])

	test_env.ParseConfig('pkg-config --cflags --libs libzmq')

	test_env.Program('utest', [
		'test/main.cpp',
		Glob('test/test_*.cpp'),
		zmqu_lib
	])

	# samples
	test_env.Program(['test/client_sock_events.cpp', zmqu_lib])
	test_env.Program(['test/server_sock_events.cpp', zmqu_lib])
	test_env.Program(['test/monitor.cpp', zmqu_lib])
	test_env.Program(['test/recv_vector.cpp', zmqu_lib])
	test_env.Program(['test/subscriber_events.cpp', zmqu_lib])
	test_env.Program(['test/clone_monitor.cpp', zmqu_lib])
	test_env.Program(['test/pong_classics.cpp', zmqu_lib])
	test_env.Program(['test/pong_fiber.cpp', zmqu_lib])
	test_env.Program(['test/sync_send_hang.cpp', zmqu_lib])


def check_dependencies(env):
	conf = Configure(env, custom_tests = {
		'check_pkg_config': check_pkg_config,
		'check_pkg': check_pkg})

	if not conf.check_pkg_config('0.15'):
		print('pkg-config >= 0.15 not found')
		Exit(1)

	for dep in dependencies:
		if not conf.check_pkg(dep):
			print('%s not found' % dep)
			Exit(1)

def check_pkg_config(context, version):
	context.Message('checking for pkg-config...')
	ret = context.TryAction('pkg-config --atleast-pkgconfig-version=%s' % version)[0]
	context.Result(ret)
	return ret

def check_pkg(context, name):
	context.Message('checking for %s...' % name)
	ret = context.TryAction("pkg-config --exists '%s'" % name)[0]
	context.Result(ret)
	return ret

main()
