#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

//***********| Constants | **********************
#define SIMULATION_DEFAULT_RIGHT_CARS 10;
#define SIMULATION_DEFAULT_LEFT_CARS 10;

#define DEFAULT_CAR_SPEED 1
#define DEFAULT_TUNNEL_LENGTH 4
#define DEFAULT_CONCURRENT_CARS 4
#define DEFAULT_TUNNEL_INIT_ORIENTATION CLOSED

//********** | Definition of entities | ****************

typedef enum {
	TO_LEFT,
	TO_RIGHT,
	CLOSED,
} FlowOrientation;

// The tunnel
typedef struct{
	// Length of the tunnel. The speed of each car determines
	// how long it takes it to cross
	size_t length;

	// Current flow orientation. To avoid starving.
	FlowOrientation current_orientation;

	 // Maximum amount of cars that can enter the tunnel at any given time.
	size_t max_concurrent_cars;

	// Keep track of how many cars have passed in the current orientation
	size_t current_orientation_passed;

	// Mutex for synchronization
	pthread_mutex_t mutex;
} Tunnel;

Tunnel *new_Tunnel(size_t length, size_t max_concurrent_cars, FlowOrientation init_orientation) {
	Tunnel *tunnel = (Tunnel *)malloc(sizeof(Tunnel));
	if(tunnel == NULL) {
		return NULL;
	}
	
	tunnel->length = length;
	tunnel->max_concurrent_cars = max_concurrent_cars;
	tunnel->current_orientation_passed = 0;
	tunnel->current_orientation = init_orientation;

	// Initialize mutex
	if( pthread_mutex_init(&tunnel->mutex, NULL) != 0 ) {
		fprintf(stderr, "error creating mutex\n");
		return NULL;
	}

	return tunnel;
}

void destroy_tunnel(Tunnel *tunnel) {
	pthread_mutex_destroy(&tunnel->mutex);
	free(tunnel);
}



// Definition of a single car
typedef struct{
	// ID Number of the car
	int id;

	// Current orientation of the car
	FlowOrientation current_orientation;

	// Speed is how much length the car passes in a unit of time.
	// Tunnel legth is divided by this to know how many literal
	// seconds it takes to pass it.
	size_t speed;
} Car;

Car *new_Car(int id, FlowOrientation init_orientation, size_t speed) {
	Car *car = (Car *)malloc(sizeof(Car));
	if(car == NULL) {
		return NULL;
	}

	car->id = id;
	car->current_orientation = init_orientation;
	car->speed = speed;

	return car;
}

void destroy_car(Car *car) {
	free(car);
}


//************** | Helper functions | *****************

// Simply return the opposite orientation to the one passed
FlowOrientation switch_orientation(FlowOrientation current_flow) {
	if(current_flow == TO_LEFT) {
		return TO_RIGHT;
	}
	else {
		return TO_LEFT;
	}
}

// Describe with a string the orientation of the flow
char *orientation_description(FlowOrientation orientation) {
	if(orientation == TO_LEFT) {
		return "from right to left <----";
	}
	else {
		return "from left to right ---->";
	}
}

// Check if the tunnel quota for this flow has been reached.
// Returns true for "cars still can pass" and false for
// "time to let the other side pass"
int check_flow_quota(Tunnel *tunnel) {
	return tunnel->current_orientation_passed != tunnel->max_concurrent_cars;
}

// Calculate how much a car wastes in the tunnel, i.e.,
// how much time should the thread be asleep
size_t time_asleep(Car *car, Tunnel *tunnel) {
	return tunnel->length/car->speed;
}

//************** | The threads themselves and stuff needed for them | ****************

// Struct for the args
typedef struct {
	// The car whith which the thread will act
	Car *car;
	
	// The tunnel that will be used by the car
	Tunnel *tunnel;
} CarThreadArgs;

CarThreadArgs *new_CarThreadArgs(Car *car, Tunnel *tunnel) {
	CarThreadArgs *args = (CarThreadArgs *)malloc(sizeof(CarThreadArgs));
	if(args == NULL) {
		return NULL;
	}

	args->car = car;
	args->tunnel = tunnel;

	return args;
}

void destroy_CarThreadArgs(CarThreadArgs *args) {
	free(args);
}

void *car_thread(void *vargp) {
	CarThreadArgs *args = (CarThreadArgs *)vargp;

	printf("Car %02d ready\n", args->car->id);

	while(1) {
		// Get general lock
		pthread_mutex_lock(&args->tunnel->mutex);

		// If the tunnel is still not receiving our direction,
		// let others pass and try again later
		if(args->tunnel->current_orientation != args->car->current_orientation) {
			pthread_mutex_unlock(&args->tunnel->mutex);
			continue;
		}

		// Signal that a car is passing
		args->tunnel->current_orientation_passed++;

		printf("Car %02d passing %s (total in tunnel: %d)\n",
				args->car->id,
				orientation_description(args->car->current_orientation),
				args->tunnel->current_orientation_passed);	

		args->car->current_orientation = switch_orientation(args->car->current_orientation);

		// If we still can let others pass, immediately let them
		if( check_flow_quota(args->tunnel) ) {
			pthread_mutex_unlock(&args->tunnel->mutex);
			sleep(time_asleep(args->car, args->tunnel));
			continue;
		}
		// Otherwise, we are the last ones to enter. Prepare everything for
		// the other side
		else {
			args->tunnel->current_orientation = switch_orientation(args->tunnel->current_orientation);
			args->tunnel->current_orientation_passed = 0;

			// This car has to exit before others can enter,
			// so sleep is first and unlock second
			sleep(time_asleep(args->car, args->tunnel));
			pthread_mutex_unlock(&args->tunnel->mutex);
		}
	}

	destroy_car(args->car);
	destroy_CarThreadArgs(args);
	

	return NULL;
}


// ********************* | Main function | ************************************

int main(int argc, char *argv[]) {
	Tunnel *tunnel = new_Tunnel(DEFAULT_TUNNEL_LENGTH, DEFAULT_CONCURRENT_CARS, DEFAULT_TUNNEL_INIT_ORIENTATION);
	if(tunnel == NULL) {
		fprintf(stderr, "error creating tunnel\n");
		exit(1);
	}

	size_t rights = SIMULATION_DEFAULT_RIGHT_CARS;
	size_t lefts = SIMULATION_DEFAULT_LEFT_CARS;
	size_t total_cars = rights + lefts;

	int i;
	Car *car;
	CarThreadArgs *args;

	printf("Total cars: %d\n", total_cars);
	printf("lefts: %d\n", lefts);
	pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t)*total_cars);

	for(i = 0; i < rights; i++) {
		car = new_Car(i+1, TO_RIGHT, DEFAULT_CAR_SPEED);
		if(car == NULL) {
			fprintf(stderr, "error creating car %d\n", i+1);
			exit(1);
		}

		args = new_CarThreadArgs(car, tunnel);
		if(args == NULL) {
			fprintf(stderr, "Error creating args for thread %d\n", i+1);
			exit(1);
		}

		pthread_create(&threads[i], NULL, car_thread, (void *)args);
	}


	for(i = rights; i < total_cars; i++) {
		car = new_Car(i+1, TO_LEFT, DEFAULT_CAR_SPEED);
		if(car == NULL) {
			fprintf(stderr, "error creating car %d\n", i+1);
			exit(1);
		}

		args = new_CarThreadArgs(car, tunnel);
		if(args == NULL) {
			fprintf(stderr, "Error creating args for thread %d\n", i+1);
			exit(1);
		}

		pthread_create(&threads[i], NULL, car_thread, (void *)args);
	}

	// Start the simulation
	tunnel->current_orientation = TO_RIGHT;

	for(i = 0; i < total_cars; i++) {
		pthread_join(threads[i], NULL);
	}

	destroy_tunnel(tunnel);

	return 0;
}
