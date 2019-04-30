# PA-P3-actividad_3
Programación Avanzada - Parcial 3: actividad 3

# Instructions

Tunnel Synchronization:

There is a long windy tunnel through a remote mountain that needs to be repaired, necessitating the closure of all but a single lane, which must now be shared by vehicles traveling in both directions. If two vehicles going in opposite directions meet inside the tunnel, they will crash causing the whole tunnel to collapse. Furthermore, given the lack of good ventilation, the tunnel is safe to use as long as there are no more than N vehicles traveling inside the tunnel.

Design a synchronization protocol with the following properties:

* Once a vehicle enters the tunnel, it is guaranteed to get to the other side without crashing into a vehicle going the other way.
* There are never more than N=4 vehicles in the tunnel.
* A continuing stream of vehicles traveling in one direction should not starve vehicles going in the other direction.

Implement your protocol using threads representing vehicles going in the two directions. 

Explain the purpose from any semaphore/variables in your solution and its initialization.

Show that your code works by having a set of threads (10 or 20) that repeat the following forever:

1. pick a direction for travel,
2. wait until it is safe to enter tunnel,
3. travel inside tunnel for some amount of time,
4. get out of the tunnel on the other end.

To test your protocol, you may want to experiment with various parameters (make one direction much more popular than the other to make sure that starvation does not happen, change N, etc.) 


# Building and running

On a UNIX-like system, run:

```
gcc -pthread -o tunnel.exe tunnel.c
```

Or, if you have GNU make:

```
make
```

To execute the program, run:

```
./tunnel.exe
```
