#include <mongoc/mongoc.h>
#include <stdio.h>

int send_sensor_data(int64_t *timestamp_ms, double *temperature, double *humidity, char *db_credentials[]) {
  printf("Attempting to write sensor data to DB...\n");

  if(!timestamp_ms)
    *timestamp_ms = (int64_t) 0;
  if(!temperature)
    *temperature = (double) 0.0;
  if(!humidity)
    *humidity = (double) 0.0;

  int32_t max_uri_length = 250;
  char *uri_string = malloc(max_uri_length * sizeof(char));

  mongoc_uri_t *uri;
  mongoc_client_t *client;
  mongoc_database_t *database;
  mongoc_collection_t *collection;
  bson_t insert;
  bson_error_t error;
  bool retval;

  // Required to initialize libmongoc's internals
  mongoc_init ();

  // Safely create a MongoDB URI object from the given string
  uri = mongoc_uri_new_with_error (uri_string, &error);
  if (!uri) {
    fprintf (stderr,
              "failed to parse URI: %s\n"
              "error message:       %s\n",
              uri_string,
              error.message);
    return 1;
  }

  // Create a new client instance
  client = mongoc_client_new_from_uri (uri);
  if (!client) {
    return 1;
  }

  // Get a handle on the database and collection
  database = mongoc_client_get_database (client, db_credentials[3]);
  collection = mongoc_client_get_collection (client, db_credentials[3], db_credentials[4]);

  // Prep a new BSON document for insertion.
  bson_init (&insert);
  BSON_APPEND_DATE_TIME(&insert, "timestamp", *timestamp_ms);
  BSON_APPEND_DOUBLE(&insert, "temperature_c", *temperature);
  BSON_APPEND_DOUBLE(&insert, "relative_humidity", *humidity);

  printf("Inserting: %s\n", bson_as_json (&insert, NULL));

  if (!mongoc_collection_insert_one (collection, &insert, NULL, NULL, &error)) {
    fprintf (stderr, "%s\n", error.message);
  } else {
    printf("Data successfully written to DB.\n");
  }

  // Free up memory.
  bson_destroy (&insert);
  free(uri_string);

  // Release our handles and clean up libmongoc.
  mongoc_collection_destroy (collection);
  mongoc_database_destroy (database);
  mongoc_uri_destroy (uri);
  mongoc_client_destroy (client);
  mongoc_cleanup ();

  return 0;
}
