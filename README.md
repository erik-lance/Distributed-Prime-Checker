# Distributed-Prime-Checker
A prime number checker that is distributed among machines. Edit the `Structures.h` file to edit the number of machines and the address of the machines. The program will then distribute the numbers to be checked among the machines and then collect the results.

# How to setup
1. Edit the `Structures.h` file to include the number of machines and the address of the machines.
2. Start the Master server on one of the machines by running the program then type `master` in the console.
3. Start the Slave server on the other machines by running the program then type `slave` in the console.
	1. Type the slave id (start from 0). Must be unique.
	2. Note: The slave server will not start if the master server is not running.
4. Start the client on the same machine as the master.
5. Type the range on the client as you wish.