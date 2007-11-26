#!/bin/sh

usage()
{
    cat <<EOF 
Usage: $0 all
	to run all the available tests

	$0 standard
	to run all the standard tests
	or
	you can run the tests for optional Posix
	functionality areas as specified by the
	letter tag in the posix spec
	$0 [AIO|MEM|MSG|SEM|SIG|THR|TMR|TPS]
	
EOF
}


targets()
{
	$1
}


standard_tests()
{
	echo "difftime()"
	conformance/interfaces/difftime/difftime_1-1
	echo ""
	echo "fork()"
#	conformance/interfaces/fork/fork_3-1
	echo "fork_3-1: FIXME : test sometimes fails, see bug #1639"
	conformance/interfaces/fork/fork_4-1
	conformance/interfaces/fork/fork_6-1
	conformance/interfaces/fork/fork_8-1
	conformance/interfaces/fork/fork_9-1
	conformance/interfaces/fork/fork_12-1
}


asynchronous_input_output_tests()
{
	echo "asynchronous_input_output_tests: Not yet implemented."
	#aio_* lio_listio
}


threads_tests()
{
	echo "pthread_getspecific()"
	conformance/interfaces/pthread_getspecific/pthread_getspecific_1-1
	conformance/interfaces/pthread_getspecific/pthread_getspecific_3-1
	echo ""
	echo "pthread_key_create()"
	conformance/interfaces/pthread_key_create/pthread_key_create_1-1
#	conformance/interfaces/pthread_key_create/pthread_key_create_1-2
	echo "pthread_key_create_1-2: FIXME: test fails, see bug #1644"
#	conformance/interfaces/pthread_key_create/pthread_key_create_2-1
	echo "pthread_key_create_2-1: FIXME: test invokes the debugger, see bug #1646"
	conformance/interfaces/pthread_key_create/pthread_key_create_3-1
	echo ""
	echo "pthread_key_delete()"
	conformance/interfaces/pthread_key_delete/pthread_key_delete_1-1
	conformance/interfaces/pthread_key_delete/pthread_key_delete_1-2
#	conformance/interfaces/pthread_key_delete/pthread_key_delete_2-1
	echo "pthread_key_delete_2-1: FIXME: test blocks, see bug #1642"
	echo ""
	echo "pthread_setspecific()"
	conformance/interfaces/pthread_setspecific/pthread_setspecific_1-1
	conformance/interfaces/pthread_setspecific/pthread_setspecific_1-2
}


signals_tests()
{
	echo "kill()"
	conformance/interfaces/kill/kill_2-1
	echo ""
	echo "sighold()"
	conformance/interfaces/sighold/sighold_1-1
	conformance/interfaces/sighold/sighold_2-1
	conformance/interfaces/sighold/sighold_3-core-buildonly 1
	conformance/interfaces/sighold/sighold_3-core-buildonly 2
	conformance/interfaces/sighold/sighold_3-core-buildonly 3
	conformance/interfaces/sighold/sighold_3-core-buildonly 4
	echo ""
	echo "sigignore()"
	conformance/interfaces/sigignore/sigignore_1-1
	conformance/interfaces/sigignore/sigignore_4-1
	conformance/interfaces/sigignore/sigignore_5-core-buildonly 1
	conformance/interfaces/sigignore/sigignore_5-core-buildonly 2
	conformance/interfaces/sigignore/sigignore_5-core-buildonly 3
	conformance/interfaces/sigignore/sigignore_5-core-buildonly 4
	conformance/interfaces/sigignore/sigignore_6-1
	conformance/interfaces/sigignore/sigignore_6-2
	echo ""
	echo "sigprocmask()"
	conformance/interfaces/sigprocmask/sigprocmask_8-1
	conformance/interfaces/sigprocmask/sigprocmask_8-2
	conformance/interfaces/sigprocmask/sigprocmask_8-3
	conformance/interfaces/sigprocmask/sigprocmask_12-1
	echo ""
	echo "sigrelse()"
	conformance/interfaces/sigrelse/sigrelse_1-1
	conformance/interfaces/sigrelse/sigrelse_2-1
	conformance/interfaces/sigrelse/sigrelse_3-core-buildonly 1
	conformance/interfaces/sigrelse/sigrelse_3-core-buildonly 2
	conformance/interfaces/sigrelse/sigrelse_3-core-buildonly 3
	conformance/interfaces/sigrelse/sigrelse_3-core-buildonly 4
	echo ""
	echo "signal()"
	conformance/interfaces/signal/signal_1-1
	conformance/interfaces/signal/signal_2-1
	conformance/interfaces/signal/signal_3-1
	conformance/interfaces/signal/signal_5-1
	conformance/interfaces/signal/signal_6-1
	conformance/interfaces/signal/signal_7-1
	echo ""
	echo "sigset()"
	conformance/interfaces/sigset/sigset_1-1
	conformance/interfaces/sigset/sigset_2-1
	conformance/interfaces/sigset/sigset_3-1
	conformance/interfaces/sigset/sigset_4-1
	conformance/interfaces/sigset/sigset_5-1
	conformance/interfaces/sigset/sigset_6-1
	conformance/interfaces/sigset/sigset_7-1
	conformance/interfaces/sigset/sigset_8-1
	conformance/interfaces/sigset/sigset_9-1
	conformance/interfaces/sigset/sigset_10-1
	echo ""
	echo "sigsuspend()"
	echo "FIXME: haiku' sigsuspend can not 'wake up' yet."
}

all_tests()
{
	standard_tests
	asynchronous_input_output_tests
	signals_tests
	threads_tests
}


echo "posixtestsuit-1.5.2"
sleep 1

	case $1 in
  all) echo "Executing all tests:"
  	all_tests
  	;;
  standard) echo "Executing standard tests:"
  	standard_tests
  	;;
  AIO) echo "Executing asynchronous I/O tests"
	asynchronous_input_output_tests
	;;
  SIG) echo "Executing signals tests"
	signals_tests
	;;
  SEM) echo "Executing semaphores tests"
	#TODO sem*
	;;
  THR) echo "Executing threads tests"
	threads_tests
	;;
  TMR) echo "Executing timers and clocks tests"
	#TODO time* *time clock* nanosleep
	;;
  MSG) echo "Executing message queues tests"
	#TODO mq_*
	;;
  TPS) echo "Executing process and thread scheduling tests"
	#TODO *sched*
	;;
  MEM) echo "Executing mapped, process and shared memory tests"
	#TODO m*lock* m*map shm_*
	;;
  *)	usage
	exit 1
	;;
esac


echo "**** Tests Completed ****"
