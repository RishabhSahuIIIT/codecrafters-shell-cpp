#include <iostream>
#include<cstdlib>
#include<sstream>
#include<cstring>
#include<vector>
#include<set>
#include<filesystem>
#include <sys/wait.h>
#include<unistd.h>
#include<fcntl.h>
using namespace std;

//OLD APPROACH
	    //checking input's first word , presently used substr
			//exit : exit(0)
			//echo : QUOTING
			//pwd : get_current_dir...()
		//if not either of exit, echo, pwd , , then for else case following subcases are there
		
		
		    //check for quotes, in case of quotes present,then it could be command for quoted path, 
		    //else it could be a command present in PATH  ,or type builtin, and for unquoted or type builting
          //we need to search for executable path
			//in either of above subcases we checked if the quoted path is actually an executable 
			//or if unquoted command has a valid executable and set flag detectedPathString to true if it does


			//after above two subcases, following cases checked independently in either case
				//type: , check if specified command is present in path or is inbuilt
				// cd: chdir() if valid directory
				//external command  QUOTING
	    

// separate single quoted arguments
vector<string> getSpecialArg(string argString,set<int>&escapedList)
{
  
  int sz=argString.size();
  //Option1: detect escapes and insert double quotes instead 
  vector<string>unquotedArgs;
  bool singleQuoteStart=false;
  int lastSingleQuoteStart=0;
  string newArg;
  bool spaceStart=true;
  int lastWordStart=0;
  int argNum=0;
  set<int>quotedNums;
  bool escapeLock=false;
  //special characters for double quotes  ‘$’, ‘`’, ‘"’, ‘\’, 
  bool doubleQuoteStart=false;
  int lastDoubleQuoteStart=0;
  for( int pos=0;pos<sz;pos++)
  {

    if(doubleQuoteStart==true)
    {
      if(argString[pos]=='\"' )
      {
        newArg=argString.substr(lastDoubleQuoteStart+1,((pos-1)-(lastDoubleQuoteStart+1)+1));
        doubleQuoteStart=false;
        unquotedArgs.push_back(newArg); 
        quotedNums.insert(argNum);
        if(pos+1<sz and argString[pos+1]!=' ')
          escapedList.insert(argNum);
        argNum+=1;        
      }
      else if(argString[pos]=='\\')//backslash within double quotes
      {
        if(argString[pos+1]=='\\' or argString[pos+1]=='`' or argString[pos+1]=='$' or argString[pos+1]=='\"')
        {
          argString.erase(pos,1);
          sz=argString.size();
          
        }
      }
    }
    else if(doubleQuoteStart==false)
    {
      if(argString[pos]=='\"' and argString[pos-1]!='\\' and singleQuoteStart==false) //double quote starts now
      {
        doubleQuoteStart=true;
        lastDoubleQuoteStart=pos;
        spaceStart=false;
        if(quotedNums.find(argNum-1)!=quotedNums.end() and argString[pos-1]!='\'' and argString[pos-1]!='\"')
        {
          quotedNums.erase(argNum-1);   
        }
      }
      else if (singleQuoteStart==true)
      {
        if (argString[pos]=='\'' ) //close previously started single quote
        {
          //extra pos-1 and lastSingleQuoteStart+1 to remove quotes
          newArg=argString.substr(lastSingleQuoteStart+1,((pos-1)-(lastSingleQuoteStart+1)+1));
          singleQuoteStart=false;
          unquotedArgs.push_back(newArg);  
          quotedNums.insert(argNum);
          argNum+=1;        
        } 
        else if(argString[pos]!='\'' )
        {
          continue;
        }
      }
      //cases without quote
      else if(singleQuoteStart==false )
      {
        if(argString[pos]=='\\')// backslash outside any quotes
        {          
          if(pos+1<sz ) // if previous was space then the word preceding is already stored
          {
            if(spaceStart==true) //need to store any preceding lettered word as well
            {
              newArg=argString.substr(lastWordStart,((pos-1)-lastWordStart+1));
              unquotedArgs.push_back(newArg);
              escapedList.insert(argNum);                
              escapedList.insert(argNum+1); 
              argNum+=1;
            }
            else
            {
              escapedList.insert(argNum+1);
              
            }
            newArg=argString.substr(pos+1,1);
            lastWordStart=pos+2;
            unquotedArgs.push_back(newArg);
            argNum+=1;
            pos+=1;
          }
        }
        else if( argString[pos]=='\'')//single quote starts
        {
          singleQuoteStart=true;
          lastSingleQuoteStart=pos;
          spaceStart=false;

          if(quotedNums.find(argNum-1)!=quotedNums.end() and argString[pos-1]!='\'' and argString[pos-1]!='\"')
          {
            quotedNums.erase(argNum-1); 
          }
          if(spaceStart==true) //end any previous word
          {
            newArg=argString.substr(lastWordStart,((pos-2)-lastWordStart+1));
            unquotedArgs.push_back(newArg);
            spaceStart=false;
            escapedList.insert(argNum);
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
  //join two or more arguments separated only by quotes
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
      cerr<<mainCommand<<"\n";
      perror("fail with negative code \t");
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

string getMainArg(string input)
{
  string command="";
  int sz=input.size();
  int k;
  if(input[0]=='\"')
  {

    for(int pos=1 ;pos<sz;pos++)
    {
     if(input[pos]=='\"') 
     {
       k=pos;
       break;
     }
    }
    command=input.substr(0,k+1);
  }
  else if(input[0]=='\'')
  {
    for(int pos=1 ;pos<sz;pos++)
    {
     if(input[pos]=='\'') 
     {
       k=pos;
       break;
     }
    }
    command=input.substr(0,k+1);
  }
  return command;
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
      set<int>escapedList;
      
      if(input.substr(0,4)=="exit")
      {
        exit(0);
      }
      else if(input.substr(0,4)=="echo")// basic echo without command substitution
      { 
        //TODO :Manage single quotes
        string argString=input.substr(5);
        vector <string> unquotedArgs= getSpecialArg(argString,escapedList);
        int uqSz=unquotedArgs.size();
        bool addSpace;
        
        for(int pos=0;pos<uqSz;pos++)
        {
          string wd=unquotedArgs[pos];
          cout<<wd;
          //if escaped then don't add space afterward?
  
          if(escapedList.find(pos)!=escapedList.end())
          {
            addSpace=false;
          }
          else 
          {
           addSpace=true; 
          }
          
          if(addSpace)
          {
            cout<<" ";
          }
  
  
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
        bool flag;
        string command;
        int sz2;
        string argString;
        string detectedPathString;
        vector<string>pathVars;
        if(input[0]=='\'' or input[0]=='\"')
        {
        
          command= getMainArg(input);
          sz2=command.size();
          string paths=string(getenv("PATH"));
          stringstream tokenizer(paths);
          string token;
          /*parse the path variable into a vector of strings, set flag and break the loop if condition met
          check flag and output at the end accordingly*/
          
          while(getline(tokenizer,token,':'))
          {
            pathVars.push_back(token);
          }
    
          //search executable iterated in a loop over path strings
          for(string pathFolder :pathVars )
          {
              string totalPath;            
              totalPath= pathFolder+'/'+command.substr(1,sz2-2);
              filesystem::path pth(totalPath);
              flag=flag | (filesystem::exists(pth))    ;
              if(filesystem::exists(pth))
              {
                detectedPathString=totalPath;
                break;
              }            
          }
          
        }
        else
        {
          ss<<input;
          ss>>arg1>>arg2;
          flag=false;
          string paths=string(getenv("PATH"));
          stringstream tokenizer(paths);
          string token;
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
          if(input[0]!='\'' and input[0]!='\"')
            argString=input.substr(ag1Size+1);    
          else
          {
            sz2=command.size();
            arg1=command.substr(1,sz2-2);          
            argString= input.substr(sz2+1);
  
          }    
          
          vector <string> unquotedArgs= getSpecialArg(argString,escapedList);
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
  }
  }
  