# Copyright Micu Florian-Luis 331CA 2022 - Assignment 3 APD

## Generate topology
The coordinators read data from their associated files to create their 
cluster's topology. Then, they send the newly created topology to the
other coordinators so that every single coordinator will have the 
complete topology. After the coordinators print the complete topology,
they send their rank to the associated workers, to establish the leader,
and they also send the full topology to them.

## Create result array
Coordinator 0 creates the original array and sends it to the others,
thus everyone will have the full array at its disposal. However,
every cluster will have its own start index, and every worker will
receive its own start index (computed by its leader) along with the
number of elements to be computed. Therefore, rank 0 sends the array
to its workers, then it sends it to the other leaders. The other
leaders compute the current index and forward the array to the workers.
If the number of tasks cannot be equally divided, then the last worker
in cluster 2 will compute extra elements. After each worker does its
job, it sends the full array (with the corresponding elements altered)
to the leader. The leader takes only the altered elements and updates
its array, then it sends the altered array to the leader 0. After
leader 0 receives all of the updates, it prints the final array.

## Bonus - cut connection
Task 0 cannot communicate with task 1, therefore the following changes
were made:

### Generate topology
Task 0 will send its topology to task 2 and it will be forwarded to task
1. Likewise, task 1 will send its topology to task 2 to be forwarded to
task 0. Thus, task 2 will ask as a mediator between the other leaders.
The rest of the algorithm remains the same as it does create a communication
between the task 0 and 1.

### Create result array
Task 0 will send the original array to task 2, task 2 will send the array
to its workers and it will forward the array received from 0 to task 1.
Task 2 will send its updated array to task 0, then it will receive the
updated array from task 1 and it will forward it to task 0. Task 0 will
have two receives from task 2, since task 2 forwards messages from task 1,
however since the order of receives will always be the same (recv from task 2
first and then from task 1 through task 2), the array will be updated 
correctly.

From the two tasks, a conclusion can be drawn that the only communication
that is affected is when data is sent/received from task 0 to task 1. Thus,
not many changes are to be made.

## Note
A barrier has been added before the algorithm prints the array since the 
STDOUT buffer might become full when a lot of data is printed without flushing
(specifically test 2), therefore the program first prints the topology and
the messages M(sender, receiver) and only then, the final array is printed so
that no previous message will merge with the printing of the final array (while
the array was printing, some messages of type M(i, j) will show which 
invalidates the checker). The barrier will not be passed before the other tasks
are made. Another solution to this problem would have been to store the message
to a string, however I prefer the barrier since it is easier to implement and I
do not have to declare a char* buffer of fixed size.

For more details, please check the added comments in my code.
