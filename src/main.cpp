#include <iostream>
#include<cstdlib>
int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  while(true)
  {
    std::cout << "$ ";
    
    std::string input;
    std::getline(std::cin, input);
    if(input.substr(0,4)=="exit")
      exit(0);
    else if(input.substr(0,4)=="echo")
      std::cout<<input.substr(5,input.size()-5)<<"\n";
    else
      std::cout<<input<<": command not found\n";
    

    
    
  }
}
