#include <iostream>
#include<cstdlib>
#include<sstream>
#include<cstring>
#include<vector>
#include<set>
#include<filesystem>
#include<sys/wait.h>
#include<unistd.h>
#include<fcntl.h>
using namespace std;

//class dirManage

class builtIn
{};
class parser
{
	bool outputRedirectionDetect;
	int redirectionOperatorPosn;
	public:
	parser()
	{
		this->outputRedirectionDetect=false;
	}
	/*Return executable path which is leftmost and  enclosed within quotes */
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

	/*separate the single quoted arguments apart from executable or command name*/
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
		
		vector<string>quotedMerged;
		int quotedargCount=unquotedArgs.size();
		for(int agNum=0;agNum<quotedargCount;agNum++)
		{
			//detect output redirection (right now expecting only single redirection)
			if(unquotedArgs[agNum]==">" or unquotedArgs[agNum]=="1>")
			{
				this->outputRedirectionDetect=true;
				this->redirectionOperatorPosn=agNum;
			}

			//join two or more arguments separated only by quotes 
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

};
class pathCheck
{
	public:
	string detectedPathString;
	bool flag;
	vector<string>pathVars;
	/* finds system wide path for given executable if found in PATH and stores in class member
	variable as well as set flag if detected the path*/
	void updatePathStringList()
	{
		string paths=string(getenv("PATH"));
		
		stringstream tokenizer(paths);
		string token;
		
		/*parse the path variable into a vector of strings, set flag and break the loop if condition met
		check flag and output at the end accordingly*/
		
		while(getline(tokenizer,token,':'))
		{
			pathVars.push_back(token);
		}

	}
	void searchExNameInPath(string arg2)
	{
		detectedPathString="";
		flag=false;
		updatePathStringList();
		for(string pathFolder :pathVars )
		{
			string totalPath;
			
			totalPath= pathFolder+'/'+arg2; 
			
			filesystem::path pth(totalPath);
			if(filesystem::exists(pth))
			{
				detectedPathString=totalPath;
				flag=true;
				break;
			}            
		}
	}
	void searchContainingPath(string commandParsed, int sz2)
	{
		detectedPathString="";
		flag=false;
		updatePathStringList();
		//search executable iterated in a loop over path strings
		for(string pathFolder :pathVars )
		{
			string totalPath;            
			totalPath= pathFolder+'/'+commandParsed.substr(1,sz2-2);
			filesystem::path pth(totalPath);
			if(filesystem::exists(pth))
			{
				detectedPathString=totalPath;
				flag=true;
			}            
		}
	}
	void searchExecutableForTypeCommand(string arg2)
	{
		detectedPathString="";
		flag=false;
		updatePathStringList();
		for(string pathFolder :pathVars )
		{
			string totalPath;
			
			totalPath= pathFolder+'/'+arg2; 
			
			filesystem::path pth(totalPath);
			if(filesystem::exists(pth))
			{
				detectedPathString=totalPath;
				flag=true;
				break;
			}            
		}
	}
	
};
class executer
{
	
	public:
	executer()
	{

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

};



class Shell
{
	string input;
	parser parseInterface;
	executer executionInterface;
	set<string>builtInsList;
	pathCheck pathChecker;
	public:
	Shell(parser parserObject, executer executerObject,pathCheck pathChecker)
	{
		this->parseInterface=parserObject;
		this->executionInterface=executerObject;
		this->pathChecker=pathChecker;
		this->builtInsList.insert("cd");
		this->builtInsList.insert("type");
		this->builtInsList.insert("pwd");
		this->builtInsList.insert("exit");
		this->builtInsList.insert("echo");
	}
	//NEW APPROACH
			//pick first word 
				//	if builtin(echo, type,pwd,cd,exit), use builtin class, 
					//for type : PATH CHECK FOR TYPE
					//echo : QUOTING
				//  if not builtin, 
					//	check if present in path : PATH CHECK FOR NOT TYPE
					// if present in path and quoted : PARSING or  QUOTING then EXECUTE
					// If present in path but not quoted: EXECUTE
		
	void handleInput(string command)
	{
		set<int>escapedList;
		string firstWord;
		stringstream ss;
		ss<<command;
		ss>>firstWord;
		string arg2;
		//not builtin
		if(this->builtInsList.find(firstWord)==this->builtInsList.end())
		{

			bool flag;
			string commandParsed;
			int sz2;
			string argString;
			//QUOTED EXECUTABLE PATH
			if(command[0]=='\'' or command[0]=='\"')
			{
			
				commandParsed= parseInterface.getMainArg(command);
				sz2=commandParsed.size();
				pathChecker.searchContainingPath(commandParsed,sz2);
			}
			/*parse the path variable into a vector of strings,
		      set flag and break the loop if condition met
		      check flag and output at the end accordingly
		      search executable iterated in a loop over path strings*/
			else //SIMPLE UNQOUTED EXECUTABLE NAME
		    {
		      
		      pathChecker.searchExNameInPath(firstWord);
		
		    }

			if(pathChecker.flag==true)//command is found
			{
				int inputSz=command.size();
				int ag1Size=firstWord.size();
				if(command[0]!='\'' and command[0]!='\"')
				argString=command.substr(ag1Size+1);    
				else
				{
				sz2=commandParsed.size();
				firstWord=commandParsed.substr(1,sz2-2);          
				argString= command.substr(sz2+1);
		
				}    
				
				vector <string> unquotedArgs= parseInterface.getSpecialArg(argString,escapedList);
				vector<char* >charArgs;
				charArgs.push_back(const_cast<char*>(firstWord.c_str()));
				for(const string &wd:unquotedArgs)
				{   
					charArgs.push_back(const_cast<char*>(wd.c_str())); 
				}
				charArgs.push_back(nullptr);        
				executionInterface.executeCommand(firstWord,charArgs);
			
			}
			else
			{
				cout<<command<<": command not found\n";
			}
	
		}
		else
		{

			if(firstWord=="exit")
			{
				exit(0);
			}
			else if(firstWord=="echo")// basic echo without command substitution
			{ 
				string argString=command.substr(5);
				vector <string> unquotedArgs= parseInterface.getSpecialArg(argString,escapedList);
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
			else if(firstWord=="pwd")
			{
				string directory(get_current_dir_name());
				cout<<directory<<"\n";
			}
			else
			{
				if(firstWord=="type")
				{
					ss>>arg2;
		      	
				  	pathChecker.searchExecutableForTypeCommand(arg2);
					if(arg2=="echo" or arg2=="exit" or arg2=="type" or arg2=="pwd")
					{
					cout<<arg2<<" is a shell builtin\n";
			
					}
					else if(pathChecker.flag==true)
					{
						cout<<arg2<<" is "<<pathChecker.detectedPathString<<"\n";
					}
					else
					{
					cout<<arg2<<": not found\n";
					}
				}
			else if(firstWord=="cd")
			{
				ss>>arg2;
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
			
	      else // external command including cat detected in path and needs to be executed from argument list
	      {
	        cout<<command<<": command not found\n";
	      }
	  
			}
			
		}
		
	}
	void run()
	{
		while(true)
	  	{
			cout<<"$ ";
	    	getline(std::cin, input);
	    	handleInput(input); 
		
		}
		
	}
};

int main() {
  // Flush after every std::cout / std:cerr
  cout << std::unitbuf;
  cerr << std::unitbuf;
 	try
 	{
		parser parserObject= parser();
		executer executionObject=executer();
		pathCheck pathResolver= pathCheck();
		Shell shell=  Shell(parserObject,executionObject,pathResolver);
		shell.run();
	
 	}
 	catch(...)
 	{
 		
 	}
  
  }
