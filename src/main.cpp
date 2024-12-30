#include <iostream>
#include<cstdlib>
#include<sstream>
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
    else if(input.substr(0,4)=="type")
    {
      std::stringstream ss;
      std:: string arg1;
      std:: string arg2;
      std:: string arg3;
      ss<<input;
      ss>>arg1>>arg2;
      if(arg2=="echo" or arg2=="exit" or arg2=="type")
      {
        std::cout<<arg2<<" is a shell builtin\n";

      }
      else
      {
        std::cout<<arg2<<": not found\n";
      }

    }
    else
      std::cout<<input<<": command not found\n";
    

    
    
  }
}
