#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

#define API_URL "https://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s"

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    printf("Not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

int main(int argc, char *argv[]) {
  CURL *curl_handle;
  CURLcode res;
  struct MemoryStruct chunk;
  char *api_key = "<your-api-key>"; // Replace with your own API key
  char *url;
  json_object *root, *main, *weather;
  json_object *temp, *pressure, *humidity, *description;
  const char *city_name;

  if (argc < 2) {
    printf("Usage: %s <city name>\n", argv[0]);
    return 1;
  }

  chunk.memory = malloc(1);
  chunk.size = 0;

  curl_global_init(CURL_GLOBAL_ALL);

  url = (char *)malloc(strlen(API_URL) + strlen(api_key) + strlen(argv[1]) + 1);
  sprintf(url, API_URL, argv[1], api_key);

  curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, url);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

  res = curl_easy_perform(curl_handle);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    return 1;
  }

  root = json_tokener_parse(chunk.memory);
  if (!root) {
    fprintf(stderr, "json_tokener_parse() failed\n");
    return 1;
  }

  main = json_object_object_get(root, "main");
  temp = json_object_object_get(main, "temp");
  pressure = json_object_object_get(main, "pressure");
  humidity = json_object_object_get(main, "humidity");

  weather = json_object_object_get(root, "weather");
  description = json_object_object_get(json_object_array_get_idx(weather, 0), "description");

  city_name = json_object_get_string(json_object_object_get(root, "name"));

  printf("Weather for %s:\n", city_name);
  printf("Temperature: %d C\n", (int)json_object_get_double(temp) - 273);
  printf("Pressure: %d hPa\n", (int)json_object_get_double(pressure));
  printf("Humidity: %d%%\n", (int)json_object_get_double(humidity));
  printf("Description: %s\n", json_object_get_string(description));

  json_object_put(root);

  curl_easy_cleanup(curl_handle);
