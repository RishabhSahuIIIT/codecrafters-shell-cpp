#include <iostream>
#include<cstdlib>
#include<sstream>
#include<cstring>
#include<vector>
#include<filesystem>
#include <sys/wait.h>

#include<unistd.h>
using namespace std;
void executeCommand(string mainCommand,vector<string>argList)
{
  int sz=argList.size();
  char * const *  argListC = (char* const *)malloc(sizeof(char* const )*sz);
  int pos=0;
  for(string arg:argList)
  {
    strcpy(argListC[pos],arg.c_str());
    pos++;
  }
  int processPid= fork();
  if(processPid==0)//child
  { 
    cout<<"childProcessStarted\n";
    execvp(mainCommand.c_str(),argListC);
    
  }
  else if (processPid>0) //parent
  {
    int status;
    waitpid(processPid, &status, 0);
    cout<<status;
  }
  else // process pid ==-1 or error
  {
    cout<<"Error";
  }

  //option1 : use location of command and execute as a normal program
  //option2: use command name without location ,executing as a shell command

}
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
    else
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
            string totalPath;
            if(arg1=="type")
              totalPath= pathFolder+'/'+arg2;
            else
              totalPath= pathFolder+'/'+arg1;
            
            filesystem::path pth(totalPath);
            
            flag=flag | (filesystem::exists(pth))    ;
            if(filesystem::exists(pth))
            {
              detectedPathString=totalPath;
              
              break;
            }
            

        }

        if(flag==false)
        {
          cout<<input<<": command not found\n";
        }
        else if(input.substr(0,4)=="type")
        {
          if(arg2=="echo" or arg2=="exit" or arg2=="type")
          {
            cout<<arg2<<" is a shell builtin\n";

          }
          else if(flag)
          {
  
              cout<<arg2<<" is "<<detectedPathString<<"\n";
          }
          else
          {
            cout<<arg2<<": not found\n";
          }
        }    
        else // command detected in path and needs to be executed from argument list
        {
          vector<string> argumentsList;
          argumentsList.push_back(arg2);//arguments list apart from the main command
          string par;
          while(!ss.eof())
          {
            ss>>par;
            argumentsList.push_back(par);
          }
          executeCommand(arg1,argumentsList);
        }
    

    
    
  }
}
}
