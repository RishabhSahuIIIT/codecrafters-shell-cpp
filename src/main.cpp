#include <iostream>
#include<cstdlib>
#include<sstream>
#include<cstring>
#include<vector>
#include<filesystem>
#include <sys/wait.h>

#include<unistd.h>
using namespace std;
void executeCommand(string mainCommand,vector<char*>argList)
{
  
  int sz=argList.size();

  int pos=0;
  int processPid= fork();
  if(processPid==0)//child
  { 
    execvp(mainCommand.c_str(),argList.data());
  }
  else if (processPid>0) //parent
  {
    int status;
    waitpid(processPid, &status, 0);
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
    else if(input.substr(0,4)=="echo")// basic echo without command substitution
    { 
      //TODO :Manage single quotes
      if(input[5]=='`')
      {
        string quotedText= input.substr(6,input.size()-7);
        cout<<quotedText<<"\n";
      }
      else
        cout<<input.substr(5,input.size()-5)<<"\n";
    }
    else if(input.substr(0,3)=="pwd")
    {
      string directory(get_current_dir_name());
      cout<<directory<<"\n";
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

      
      if(input.substr(0,4)=="type")
      {
        if(arg2=="echo" or arg2=="exit" or arg2=="type" or arg2=="pwd")
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
      else if(input.substr(0,2)=="cd")
      {
        int errorCode;
        if(arg2=="~")
        {
          string pathString= string(getenv("HOME"));
          errorCode=chdir(pathString.c_str());
        }
        else
        {
          errorCode=chdir(arg2.c_str());
        }
        
        if(errorCode<0)
        {
          cout<<"cd: "<<arg2<<": No such file or directory\n";
        }
      }  
      else if(flag==false)
      {
        cout<<input<<": command not found\n";
      } 
      else if(arg1=="cat")//single quote support for cat
      {
        if(arg2[0]=='`')
        {
          arg2=arg2.substr(1,arg2.size()-2);
        }
        string par;
        vector<char*> argumentsList;
        while(!ss.eof())
        {
          ss>>par;
          if(par[0]=='`')
          {
            par=par.substr(1,par.size()-2);
          }
          argumentsList.push_back(const_cast<char*>(par.c_str()));
        }
        executeCommand(arg1,argumentsList);
      }
      else // external command detected in path and needs to be executed from argument list
      {
        
        vector<char*> argumentsList;
        //cout<<arg2; //printed name parameter 
        argumentsList.push_back(const_cast<char*>(arg1.c_str()));
        argumentsList.push_back(const_cast<char*>(arg2.c_str()));//arguments list apart from the main command
        string par;
        while(!ss.eof())
        {
          ss>>par;
          argumentsList.push_back(const_cast<char*>(par.c_str()));
        }
        executeCommand(arg1,argumentsList);
      }
  
  }
  //cout<<"yet another output\n";
}
//cout<<"yet another yet another output\n";
}
