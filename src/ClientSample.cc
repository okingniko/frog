#include "HttpClient.h"

#include <iostream>
#include <thread>
#include <chrono>
using namespace std;
//using namespace frog;

using HttpClient = frog::Client<frog::HTTP>;

int main(const int argc, char *argv[])
{
  HttpClient test_client("localhost:8080");

  
  //this_thread::sleep_for(chrono::seconds(3));

  string json_str = "{\"firstname\": \"wang\", \"lastname\": \"mumu\", \"sex\": \"male\"}";
  auto resp2 = test_client.Request("POST", "/json", json_str);
  cout << "resp2:\n" << resp2->content.rdbuf() << endl;

  auto resp3 = test_client.Request("POST", "/string", json_str);
  cout << "resp3:\n" << resp3->content.rdbuf() << endl;

  auto resp4 = test_client.Request("GET", "/id/12358");
  cout << "resp4:\n" << resp4->content.rdbuf() << endl;

  auto resp1 = test_client.Request("GET", "/reqinfo");
  cout << "rep1:\n" << resp1->content.rdbuf() << endl;
  return 0;
}


