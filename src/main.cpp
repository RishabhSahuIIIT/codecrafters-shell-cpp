#include <iostream>
#include<cstdlib>
#include<sstream>
#include<cstring>
#include<vector>
#include<filesystem>
using namespace std;
int main() {
  // Flush after every std::cout / std:cerr
  cout << std::unitbuf;
  cerr << std::unitbuf;

  while(true)
  {
    cout << "$ ";
    
    string input;
    getline(std::cin, input);
    if(input.substr(0,4)=="exit")
    {
      exit(0);
    }
    else if(input.substr(0,4)=="echo")
    { 
      cout<<input.substr(5,input.size()-5)<<"\n";
    }
    else if(input.substr(0,4)=="type")
    {
      stringstream ss;
       string arg1;
       string arg2;
       string arg3;
      ss<<input;
      ss>>arg1>>arg2;
      bool flag=false;
      string paths=string(getenv("PATH"));
        stringstream tokenizer(paths);
        string token;
        vector<string>pathVars;
        string detectedPathString;
        //parse the path variable into a vector of strings,
        
            //set flag and break the loop if condition met
            //check flag and output at the end accordingly

        while(getline(tokenizer,token,':'))
        {
          pathVars.push_back(token);
        }
        
        
        //search executable iterated in a loop over path strings
        for(string pathFolder :pathVars )
        {
            string totalPath= pathFolder+'/'+arg2;
            filesystem::path pth(totalPath);
            

            
            flag=flag | (filesystem::exists(pth))    ;
            if(filesystem::exists(pth))
            {
              detectedPathString=totalPath;
            }
            

        }
        
      if(arg2=="echo" or arg2=="exit" or arg2=="type")
      {
        cout<<arg2<<" is a shell builtin\n";

      }
      else if(flag) //check if command is present in any of the directories of the path variable
      {
        
			
		
        //check for executable in ea
        //for(string st:pathVars)
        //{
        	//check if executable exists under this path string
          //filesystem::exists()
        //}
        cout<<arg2<<" is "<<detectedPathString<<"\n";
      }
      else
      {
        cout<<arg2<<": not found\n";
      }

    }
    else
    {
      cout<<input<<": command not found\n";
    }
    

    
    
  }
}
