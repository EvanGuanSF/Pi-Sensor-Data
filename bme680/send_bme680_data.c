#include <mongoc/mongoc.h>
#include <sys/file.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

int send_sensor_data(char *db_credentials[], bson_t *insert) {
  printf("Attempting to write sensor data to DB...\n");

  int32_t max_uri_length = 250;
  char *uri_string = malloc(max_uri_length * sizeof(char));
  snprintf(uri_string, max_uri_length, "mongodb://%s:%s@%s", db_credentials[0], db_credentials[1], db_credentials[2]);

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
  if(!uri) {
    fprintf (stderr,
              "failed to parse URI: %s\n"
              "error message:       %s\n",
              uri_string,
              error.message);
    return 1;
  }

  // Create a new client instance
  client = mongoc_client_new_from_uri(uri);
  if (!client) {
    return 1;
  }

  // Get a handle on the database and collection
  database = mongoc_client_get_database(client, db_credentials[3]);
  collection = mongoc_client_get_collection(client, db_credentials[3], db_credentials[4]);

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
  if(argc != 7) {
    printf("Invalid number of args provided: %d\n", argc - 1);
    return 1;
  }

  // Setup document insertion function details.
  char *db_credentials[5];
  for(int i = 1; i < 6; i++)
    db_credentials[i - 1] = argv[i];

  // Get the number of minutes to wait between sensor readings from the arguments.
  uint32_t n_minutes_per_datapoint = abs(strtoul(argv[6], NULL, 10));
  if(n_minutes_per_datapoint == 0) {
    printf("Invalid minutes_per_sample parameter, defaulting to 5 minute interval.\n");
    n_minutes_per_datapoint = 5;
  }

  // Setup time constructs.
  time_t seconds;
  seconds = time(&seconds);
  // Floor to closest n minutes.
  uint32_t closest = n_minutes_per_datapoint * 60;
  int64_t timestamp_ms = (int64_t) ((int64_t)(seconds / closest) * closest) * 1000;

  // Prep a new BSON document for insertion.
  bson_t insert;
  bson_init (&insert);
  BSON_APPEND_DATE_TIME(&insert, "timestamp", timestamp_ms);

  
  char file_name[128] = "";
  strcpy(file_name, getenv("HOME"));
  strcat(file_name, "/.pi_sensor_data/bme680_data.txt");
  puts(file_name);

  // Get the data from the file. We are going to assume that the format is "field_name_string:sensor_data_double".
  // Acquire a lock and open the file for reading.
  FILE *input_file;
  input_file = fopen(file_name, "r");
  if (flock(fileno(input_file), LOCK_EX) < 0) {
    puts("Failed to get a lock for the file ~/.pi_sensor_data/bme680_data.txt\n");
    return 0;
  }
  const int buffer_size = 256;
  char buffer[buffer_size];
  
  // Check if file exists
  if (input_file == NULL) {
    puts("Could not open file ~/.pi_sensor_data/bme680_data.txt\n");
    return 0;
  }

  // Read the file line by line and set BSON data.
  int tok_index = 0;
  char *field_name;
  double data = 0.0;

  while(fgets(buffer, buffer_size - 1, (FILE*)input_file)) {
    char *token = strtok(buffer, ":");
    // Get the data for the current line.
    // We will assume that the datatype is double.
    while(token != NULL) {
      if(tok_index == 0)
        field_name = token;
      else
        data = strtod(token, NULL);

      token = strtok(NULL, buffer);
      ++tok_index;
    }

    // Add it to the BSON document.
    BSON_APPEND_DOUBLE(&insert, field_name, data);

    // Reset data vars.
    field_name = "";
    data = 0.0;
    tok_index = 0;
  }

  // Release the lock and close the file.
  int release = flock(fileno(input_file), LOCK_UN);
  fclose(input_file);

  printf("Inserting: %s\n", bson_as_json(&insert, NULL));

  send_sensor_data(db_credentials, &insert);
  bson_destroy(&insert);

  return 0;
}
