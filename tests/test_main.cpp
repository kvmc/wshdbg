#include "test.hpp"
int main(){int failures=0;for(const auto& test:tests()){try{test.fn();std::cout<<"[pass] "<<test.name<<'\n';}catch(const std::exception& ex){++failures;std::cerr<<"[fail] "<<test.name<<": "<<ex.what()<<'\n';}}return failures==0?0:1;}
