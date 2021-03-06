#include<stdio.h>
#include<stdlib.h>

/**
 * Queue struct with 5 properties. The Queue works only with packets currently.
 * @param capacity The number of elements the Queue can contain
 * @param size The current size of the Queue
 * @param front The index of the first element in the Queue (where to dequeue)
 * @param rear The index of the last element in the Queue (where to enqueue)
 * @param elements A pointer to the elements in the Queue
 */
typedef struct Queue {
	int capacity;
	int size;
	int front;
	int rear;
	packet_plus *elements;
} Queue;

/**
 * Returns true if the Queue is empty, false otherwise
 * @return true if the Queue is empty, false otherwise
 */
bool isEmpty(Queue *Q) {
	if (Q->size == 0) {
		return true;
	}
	
	return false;
}

/**
 * Returns the size of the Queue.
 * @param Q A pointer to the Queue
 * @return (int) The size of the Queue
 */
int size(Queue *Q) {
	return Q->size;
}

/**
 * Returns the capacity of the Queue.
 * @param Q A pointer to the Queue
 * @return (int) The capacity of the Queue
 */
int capacity(Queue *Q) {
	return Q->capacity;
}

/**
 * Creates a Queue with the max number of allowed elements as the parameter.
 * Returns a pointer to the Queue.
 * @param maxElements The max number of elements in the Queue
 * @return A pointer to the Queue
 */
Queue * newQueue(int capacity) {
	// Create the Queue
	Queue *Q;
	Q = (Queue *) malloc( sizeof(Queue) );
	
	// Initialize the properties of the Queue
	Q->elements = (packet_plus *) malloc( sizeof(packet_plus) * capacity);
	Q->size = 0;
	Q->capacity = capacity;
	Q->front = 0;
	Q->rear = -1;
	
	
	return Q;
}


/**
 * Removes the front element from the Queue.  If the Queue is empty,
 * prints out that it is empty and returns.
 * @param Q The pointer to the Queue you want to dequeue
 */
int dequeue(Queue *Q) {
	// Can't pop an empty Queue
	if (Q->size==0) {
		//printf("Queue is empty\n");
		return -1;
	}
	
	// Dequeueing === front++
	else {
		Q->size--;
		Q->front++;
		
		/* As we fill elements in circular fashion */
		if (Q->front == Q->capacity) {
			Q->front = 0;
		}
	}
	return 0;
}

/**
 * Returns the element at the front of the Queue.
 * @param Q The pointer to the Queue
 * @return The packet at the front of the queue
 */
packet_plus first(Queue *Q) {
	if(Q->size == 0) {
		printf("Queue is empty. Exiting.\n");
		exit(-1);
	}
	
	return Q->elements[Q->front];
}

/**
 * Adds an element to the back of the Queue
 * @param A pointer to the Queue to add to
 * @param element A packet struct to add to the Queue
 * @return 0 if successful, -1 if Queue is full
 */
int enqueue(Queue *Q, packet_plus element) {
	// Can't push onto full Queue
	if(Q->size == Q->capacity) {
		//printf("Queue is full\n");
		return -1;
	}
	else {
		Q->size++;
		Q->rear = Q->rear + 1;
		/* As we fill the queue in circular fashion */
		if(Q->rear == Q->capacity) {
			Q->rear = 0;
		}
		
		// Insert to the back of the Queue
		Q->elements[Q->rear] = element;
	}
	return 0;
}