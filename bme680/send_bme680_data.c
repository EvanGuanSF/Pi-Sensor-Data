#include <errno.h>
#include <mongoc/mongoc.h>
#include <semaphore.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Interrupt flag.
volatile sig_atomic_t exitFlag = 0;
// MMap constants.
const char SHARED_PATH[] = "/bme680_data";
const int32_t NUM_DATA_POINTS = 4;
const int32_t SHARED_BYTES = NUM_DATA_POINTS * sizeof(double);
const u_int32_t PERMISSIONS = 400;
const long NANO_SLEEP_DURATION = 10000000;
const char *FIELD_NAMES[] = {"temperature_c", "relative_humidity",
                             "pressure_hpa", "gas_resistance_ohms"};

void handleSigInt(int sig) {
  printf("\nSignal %d received, exiting...\n", sig);
  exitFlag = 1;
}

void cleanup(void **sharedMemory, sem_t **sharedSemaphore,
             int64_t *fileDescriptor) {
  // Remove handles to memory, file descriptor, and semaphore.
  printf("Cleaning up sM: %p, sS: %p, fd: %d\n", *sharedMemory, *sharedSemaphore,
         *fileDescriptor);
  sleep(1);
  munmap(*sharedMemory, (size_t)SHARED_BYTES);
  close(*fileDescriptor);
  sem_close(*sharedSemaphore);

  exit(0);
}

int send_sensor_data(char *db_credentials[], bson_t *insert) {
  printf("Attempting to write sensor data to DB...\n");

  int32_t max_uri_length = 250;
  char *uri_string = malloc(max_uri_length * sizeof(char));
  snprintf(uri_string, max_uri_length, "mongodb://%s:%s@%s", db_credentials[0],
           db_credentials[1], db_credentials[2]);

  puts(uri_string);

  mongoc_uri_t *uri;
  mongoc_client_t *client;
  mongoc_database_t *database;
  mongoc_collection_t *collection;
  bson_error_t error;
  bool retval;

  // Required to initialize libmongoc's internals
  mongoc_init();

  // Safely create a MongoDB URI object from the given string
  uri = mongoc_uri_new_with_error(uri_string, &error);
  if (!uri) {
    fprintf(stderr,
            "failed to parse URI: %s\n"
            "error message:       %s\n",
            uri_string, error.message);
    return 1;
  }

  // Create a new client instance
  client = mongoc_client_new_from_uri(uri);
  if (!client) {
    return 1;
  }

  // Get a handle on the database and collection
  database = mongoc_client_get_database(client, db_credentials[3]);
  collection = mongoc_client_get_collection(client, db_credentials[3],
                                            db_credentials[4]);

  printf("Inserting: %s\n", bson_as_json(insert, NULL));

  if (!mongoc_collection_insert_one(collection, insert, NULL, NULL, &error)) {
    fprintf(stderr, "%s\n", error.message);
  } else {
    printf("Data successfully written to DB.\n");
  }

  // Free up memory.
  free(uri_string);

  // Release our handles and clean up libmongoc.
  mongoc_collection_destroy(collection);
  mongoc_database_destroy(database);
  mongoc_uri_destroy(uri);
  mongoc_client_destroy(client);
  mongoc_cleanup();

  return 0;
}

int main(int argc, char *argv[]) {
  // Check for proper amount of arguments provided.
  if (argc != 7) {
    printf("Invalid number of args provided: %d\n", argc - 1);
    exit(EXIT_FAILURE);
  }

  sem_t *sharedSemaphore = sem_open(SHARED_PATH, 0);
  if (sharedSemaphore == SEM_FAILED) {
    printf("Creating the semaphore failed; errno is %d\n", errno);
    exit(EXIT_FAILURE);
  } else {
    printf("Shared semaphore created/opened.\n");
  }

  // Link to shared memory.
  int64_t fileDescriptor = shm_open(SHARED_PATH, O_RDONLY, PERMISSIONS);
  printf("File descriptor: %d\n", fileDescriptor);
  void *sharedMemory = NULL;

  if (fileDescriptor == -1) {
    printf("Creating the shared memory failed; errno is %d", errno);
    close(fileDescriptor);
    exit(EXIT_FAILURE);
  } else {
    // MMap the shared memory
    sharedMemory = mmap((void *)0, (size_t)SHARED_BYTES, PROT_READ,
                        MAP_PRIVATE, fileDescriptor, 0);

    if (sharedMemory == MAP_FAILED) {
      sharedMemory = NULL;
      printf("MMapping the shared memory failed; errno is %d\n", errno);
      close(fileDescriptor);
      exit(EXIT_FAILURE);
    }
  }

  // Setup document insertion function details.
  char *db_credentials[5];
  for (int i = 1; i < 6; i++) db_credentials[i - 1] = argv[i];

  // Get the number of seconds to wait between sensor readings from the
  // arguments.
  uint32_t n_seconds_per_datapoint = abs(strtoul(argv[6], NULL, 10));
  if (n_seconds_per_datapoint == 0) {
    printf(
        "Invalid seconds_per_sample parameter, defaulting to 5 minute "
        "interval.\n");
    n_seconds_per_datapoint = 300;
  }

  // Setup time constructs.
  time_t seconds;
  seconds = time(NULL);
  // Default timestamp to the current milisecond.
  int64_t timestamp_ms = (int64_t)seconds * 1000;
  // Floor timestamp to closest n minutes if a minute interval is provided.
  if (n_seconds_per_datapoint >= 60) {
    uint32_t closest = n_seconds_per_datapoint;
    timestamp_ms = (int64_t)((int64_t)(seconds / closest) * closest) * 1000;
  }

  // Prep a new BSON document for insertion.
  bson_t insert;
  bson_init(&insert);
  BSON_APPEND_DATE_TIME(&insert, "timestamp", timestamp_ms);
  double sensorDataPoints[NUM_DATA_POINTS];

  // Copy the sensor data first.
  sem_wait(sharedSemaphore);
  for (int32_t i = 0; i < NUM_DATA_POINTS; i++) {
    memcpy(&sensorDataPoints[i], sharedMemory + i * sizeof(double),
           sizeof(double));
  }
  sem_post(sharedSemaphore);

  // Then copy the field names and data into the BSON insert.
  for (int32_t i = 0; i < NUM_DATA_POINTS; i++) {
    BSON_APPEND_DOUBLE(&insert, FIELD_NAMES[i], sensorDataPoints[i]);
  }

  // Insert to the db and then cleanup.
  printf("Inserting: %s\n", bson_as_json(&insert, NULL));
  send_sensor_data(db_credentials, &insert);
  bson_destroy(&insert);

  // Unlink memory and semaphore.
  munmap(sharedMemory, (size_t)SHARED_BYTES);
  sem_close(sharedSemaphore);

  exit(EXIT_SUCCESS);
}
