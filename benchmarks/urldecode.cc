#include <iostream>
#include <benchmark/benchmark.h>
#include <silicon/backends/urldecode.hh>
#include "symbols.hh"

const char* str = "name=32&age=42";

static void manual_decode(benchmark::State& state) {

  int name, age;
  char* it;

  
  while (state.KeepRunning())
  {
    int pos = 0;
    while (str[pos] != '=') pos++;
    pos++;
    
    name = std::strtol(str + pos, &it, 10);
    pos = it - str;
    while (str[pos] != '=') pos++;
    pos++;
    age = std::strtol(str + pos, nullptr, 10);
  }

  std::cout <<  name  << " " << age << std::endl;
}

  

static void urldecode(benchmark::State& state) {

  auto obj = D(s::_name = int(),
                 s::_age = int());
  while (state.KeepRunning())
  {
    sl::urldecode(iod::stringview(str), obj);
  }
}

BENCHMARK(manual_decode);
BENCHMARK(urldecode);

BENCHMARK_MAIN();
