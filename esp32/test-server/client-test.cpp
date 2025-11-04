#include <iostream>
#include <curl/curl.h>


int main() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  CURL* curl = curl_easy_init();
  if (!curl) return 1;

  curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8080/upload");

  curl_mime *form = curl_mime_init(curl);
  curl_mimepart *field = curl_mime_addpart(form);
  curl_mime_filedata(field, "data/test-img.jpg");

  curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

  CURLcode result = curl_easy_perform(curl);

  if(result != CURLE_OK) {
    std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(result) << std::endl;
  }

  std::cout << "POST successful." << std::endl;

  curl_mime_free(form);
  curl_easy_cleanup(curl);
  return 0;
}
