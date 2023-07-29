# Interprocess Management Project
This project demonstrates how to coordinate communication and synchronization between instances or processes using shared memory and mutexes. It provides a mechanism to control the execution flow and ensure proper handling of resources in multi-instance environments.

# Dependencies
Boost C++ Library (interprocess module) [Boost Interprocess - Synchronization Mechanisms](https://www.boost.org/doc/libs/1_47_0/doc/html/interprocess/synchronization_mechanisms.html#interprocess.synchronization_mechanisms.conditions)

# Functionality
The project includes the following components:

## SharedData Class
The SharedData class represents the shared data structure between instances. It contains the necessary structures and variables for communication and synchronization using shared memory. The class includes the following members:

--> mutex: Mutex to protect access to shared resources. <br>
-->condition: Condition variable to wait when the queue is empty. <br>
-->items: Array to store the items to be filled. <br>
-->message_in: Flag indicating whether there is a message present. <br>

## coordinateInstanceCommunication Function <br>
The coordinateInstanceCommunication function facilitates communication and synchronization between instances or processes. It receives two functions as parameters: mainInstance and secondaryInstance, which are executed depending on whether the current instance is the main instance or a secondary instance.

The function performs the following steps:

-->Creates or opens the shared memory object. <br>
-->Sets the size of the shared memory region. <br>
-->Maps the shared memory region. <br>
-->Checks if another instance of the application is already running and tries to lock the mutex. <br>
-->Executes the appropriate logic based on the instance type (main or secondary). <br>
-->Releases the mutex and updates the shared memory object after completing the processing. <br>
