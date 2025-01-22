#include <iostream>
#include<cstdlib>
#include<sstream>
#include<cstring>
#include<vector>
#include<filesystem>
#include <sys/wait.h>

#include<unistd.h>
using namespace std;
// separate single quoted arguments
vector<string> getSpecialArg(string argString)
{
  int sz=argString.size();
  vector<string>unquotedArgs;
  bool quoteStart=false;
  int lastQuoteStart=0;
  string newArg;
  bool spaceStart=true;
  int lastWordStart=0;
  for( int pos=0;pos<sz;pos++)
  {
    //cases of quote
    
    if (quoteStart ==true and argString[pos]=='\'')
    {
      //extra pos-1 and lastQuoteStart+1 to remove quotes
      newArg=argString.substr(lastQuoteStart+1,((pos-1)-(lastQuoteStart+1)+1));
      quoteStart=false;
      unquotedArgs.push_back(newArg);  
      
    } 
    else if(quoteStart==true and argString[pos]!='\'' )
    {
      continue;
      
    }
    //cases without quote
    else if(quoteStart==false )
    {
      if(quoteStart==false and  argString[pos]=='\'')
      {
        quoteStart=true;
        lastQuoteStart=pos;
        spaceStart=false;
        
        if(spaceStart==true)
        {
          newArg=argString.substr(lastWordStart,((pos-2)-lastWordStart+1));
          unquotedArgs.push_back(newArg);
          spaceStart=false;
          
        }
      }
      else if( argString[pos]==' ' and spaceStart==false )
      {
        
        continue;
      }
      else if(argString[pos]==' ' and spaceStart==true )
      {
        newArg=argString.substr(lastWordStart,((pos-1)-lastWordStart+1));
        unquotedArgs.push_back(newArg);
        spaceStart=false;
          
      }
      
      else if(argString[pos]!=' ' and spaceStart==false)// not a space and not a quote
      {
        lastWordStart=pos;
        spaceStart=true;
        
      }
      
    }
  }
  if(argString[sz-1]!='\'' and argString[sz-1]!=' ')
  {
    newArg=argString.substr(lastWordStart,((sz-1)-lastWordStart+1));
    unquotedArgs.push_back(newArg);
  }
  return unquotedArgs;
}
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
      string argString=input.substr(5);
      //cout<<"("<<argString<<")";
      vector <string> unquotedArgs= getSpecialArg(argString);
      for(string wd:unquotedArgs)
      {
        //cout<<"("<<wd<<") ";
        cout<<wd<<" ";
      }
      cout<<"\n";
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
        string argString=input.substr(4);
        vector <string> unquotedArgs= getSpecialArg(argString);
        vector<char* >charArgs;
        for(string wd:unquotedArgs)
        {
          charArgs.push_back(const_cast<char*>(wd.c_str()));
        }
         executeCommand(arg1,charArgs);
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
