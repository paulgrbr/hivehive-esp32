#include <iostream>
#include <sstream>
#include <curl/curl.h>

static size_t write_cb(char* data, size_t size, size_t nmemb, void* userp) {
  auto* out = static_cast<std::ostringstream*>(userp);
  out->write(data, static_cast<std::streamsize>(size * nmemb));
  return size * nmemb;
}

int main() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  CURL* handle = curl_easy_init();
  if (!handle) return 1;

  std::ostringstream response;
  curl_easy_setopt(handle, CURLOPT_URL, "http://127.0.0.1:8080/");
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response);

  CURLcode res = curl_easy_perform(handle);
  if (res != CURLE_OK) {
    std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << '\n';
  } else {
    std::cout << response.str() << '\n';
  }

  curl_easy_cleanup(handle);
  curl_global_cleanup();
}
