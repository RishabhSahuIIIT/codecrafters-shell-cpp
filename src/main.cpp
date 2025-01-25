#include <iostream>
#include<cstdlib>
#include<sstream>
#include<cstring>
#include<vector>
#include<set>
#include<filesystem>
#include <sys/wait.h>

#include<unistd.h>
using namespace std;

// separate single quoted arguments
vector<string> getSpecialArg(string argString)
{
  
  int sz=argString.size();
  vector<string>unquotedArgs;
  bool singleQuoteStart=false;
  int lastSingleQuoteStart=0;
  string newArg;
  bool spaceStart=true;
  int lastWordStart=0;
  int argNum=0;
  set<int>quotedNums;
  
  //special characters for double quotes  ‘$’, ‘`’, ‘"’, ‘\’, 
  bool doubleQuoteStart=false;
  int lastDoubleQuoteStart=0;
  for( int pos=0;pos<sz;pos++)
  {
    //cases of quote
    if( doubleQuoteStart==true and argString[pos]=='\\') //may delete this
    {
      if((pos+1<sz) and( argString[pos+1]== '\\'  or argString[pos+1]=='$' or argString[pos+1]=='`' or argString[pos+1]=='\"') )
      {
        argString.erase(pos);
        sz=argString.size();
      }
    }
    else if(doubleQuoteStart==true and argString[pos]=='\"')
    {
        newArg=argString.substr(lastDoubleQuoteStart+1,((pos-1)-(lastDoubleQuoteStart+1)+1));
        doubleQuoteStart=false;
        unquotedArgs.push_back(newArg);  
        quotedNums.insert(argNum);
        argNum+=1;
    }
    else if(doubleQuoteStart==true and argString[pos]!='\"')
    {
      continue;
    }
    else if(doubleQuoteStart==false)
    {
      if(argString[pos]=='\"')
      {
        doubleQuoteStart=true;
        lastDoubleQuoteStart=pos;
        spaceStart=false;
        if(quotedNums.find(argNum-1)!=quotedNums.end() and argString[pos-1]!='\'' and argString[pos-1]!='\"')
        {
          quotedNums.erase(argNum-1);
        }

        
      }
      else if (singleQuoteStart ==true and argString[pos]=='\'' )
      {
        //extra pos-1 and lastSingleQuoteStart+1 to remove quotes
        newArg=argString.substr(lastSingleQuoteStart+1,((pos-1)-(lastSingleQuoteStart+1)+1));
        singleQuoteStart=false;
        unquotedArgs.push_back(newArg);  
        quotedNums.insert(argNum);
        argNum+=1;
        
      } 
      else if(singleQuoteStart==true and argString[pos]!='\'' )
      {
        continue;
      }
      //cases without quote
      else if(singleQuoteStart==false )
      {
        if(singleQuoteStart==false  and argString[pos]=='\\')// backslash outside any quotes
        {
          if(pos-1>=0 and argString[pos-1]!=' ')
          {
            argString.erase(pos,1);
            sz=argString.size();
            newArg=argString.substr(lastWordStart,((pos-1)-lastWordStart+1));
            unquotedArgs.push_back(newArg);
            argNum+=1;
            
          }
          else if(pos+1<sz and argString[pos+1]==' ') // if previous was space then the word preceding is already stored
          {
            //newArg=argString.substr(pos+1,1);
            newArg="";//modification suited for echo command, until some situation requires passing space as arguments 
            //to path functions
            lastWordStart=pos+2;
            unquotedArgs.push_back(newArg);
            argNum+=1;
            spaceStart=true;
            pos+=1;
          }
          
        }
        else if(singleQuoteStart==false and  argString[pos]=='\'')
        {
          singleQuoteStart=true;
          lastSingleQuoteStart=pos;
          spaceStart=false;
          if(quotedNums.find(argNum-1)!=quotedNums.end() and argString[pos-1]!='\'' and argString[pos-1]!='\"')
          {
            quotedNums.erase(argNum-1);
          }
          
          if(spaceStart==true)
          {
            newArg=argString.substr(lastWordStart,((pos-2)-lastWordStart+1));
            unquotedArgs.push_back(newArg);
            spaceStart=false;
            argNum+=1;
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
          argNum+=1;
        }
        else if(argString[pos]!=' ' and spaceStart==false)// not a space and not a quote
        {
          lastWordStart=pos;
          spaceStart=true;
        }
      }
    }
  }
    
    
  if(argString[sz-1]!='\'' and argString[sz-1]!=' ' and argString[sz-1]!='\"' and spaceStart==true)
  {
    newArg=argString.substr(lastWordStart,((sz-1)-lastWordStart+1));
    unquotedArgs.push_back(newArg);
    argNum+=1;
  }
  vector<string>quotedMerged;
  int quotedargCount=unquotedArgs.size();
  for(int agNum=0;agNum<quotedargCount;agNum++)
  {
    if(quotedNums.find(agNum)!=quotedNums.end())
    {
      if(quotedNums.find(agNum+1)!=quotedNums.end())
      {
        unquotedArgs[agNum+1]=unquotedArgs[agNum]+unquotedArgs[agNum+1];
      }
      else
      {
        quotedMerged.push_back(unquotedArgs[agNum]);        
      }
    }
    
    else
    {
      quotedMerged.push_back(unquotedArgs[agNum]);      
    }
  }
  return quotedMerged;
}

void executeCommand(string mainCommand,vector<char*>argList)
{  
  int sz=argList.size();
  int pos=0;
  int processPid= fork();
  if(processPid==0)//child
  {
    int result = execvp(mainCommand.c_str(), argList.data());
    if(result<0)
    {
      cout<<"fail message in cout\n";
      perror("fail with negative code\n");
      exit(1);
    } 
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
      
      else // external command including cat detected in path and needs to be executed from argument list
      {
        int inputSz=input.size();
        int ag1Size=arg1.size();
        
        string argString=input.substr(ag1Size+1);        
        vector <string> unquotedArgs= getSpecialArg(argString);
        vector<char* >charArgs;
        charArgs.push_back(const_cast<char*>(arg1.c_str()));
        for(const string &wd:unquotedArgs)
        {   
          charArgs.push_back(const_cast<char*>(wd.c_str())); 
        }
        charArgs.push_back(nullptr);        
        executeCommand(arg1,charArgs);
      }
  
  }
  //cout<<"yet another output\n";
}
//cout<<"yet another yet another output\n";
}
