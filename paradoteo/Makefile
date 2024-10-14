all : jobCommander jobExecutorServer sleep_test_1 progDelay

sleep_test_1: 
	gcc -o sleep_test_1 sleep_test_1.c -I.

progDelay:
	gcc -o progDelay progDelay.c 

jobCommander:
	gcc -o jobCommander jobCommander.c -I.

jobExecutorServer:
	gcc -o jobExecutorServer jobExecutorServer.c queue.c list.c -I.

clean:
	rm -f queue list jobExecutorServer jobCommander jobCommander.o jobExecutorServer.txt commander_write_fifo* executor_write_fifo* sleep_test_1 progDelay aFile* bFile* ren_aFile* ren_bFile*
