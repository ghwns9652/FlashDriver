make clean
make simulator_memory_check
if [ "$1" = "n" ] ;then
	echo "no show test"
#<<<<<<< HEAD
	#valgrind ./simulator_memory_check --leak-check=full 2>log 1> /dev/null &
	valgrind --leak-check=full ./simulator_memory_check 2>log 1> /dev/null &
elif [ "$1" = "b" ]; then
	echo "nohup no show test"
	#nohup valgrind ./simulator_memory_check --leak-check=full 2>log 1> /dev/null &
	nohup valgrind --leak-check=full ./simulator_memory_check 2>log 1> /dev/null &
else
	echo "show test"
	#valgrind ./simulator_memory_check --leak-check=full 2>log
	valgrind --leak-check=full ./simulator_memory_check 2>log
=======
	valgrind --leak-check=full ./simulator_memory_check  2>log 1> /dev/null &
elif [ "$1" = "b" ]; then
	echo "nohup no show test"
	nohup valgrind --leak-check=full ./simulator_memory_check 2>log 1> ttt &
else
	echo "show test"
	valgrind --leak-check=full ./simulator_memory_check  2>log 
>>>>>>> 15f3b6e8d6807de47d50b7c5ae4c8fdb2fabbb99
fi
