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
#include<fstream>
using namespace std;

//class dirManage
class pathCheck
{
	public:
	string detectedPathString;
	bool detectedExecutableFlag;
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
		detectedExecutableFlag=false;
		updatePathStringList();
		for(string pathFolder :pathVars )
		{
			string totalPath;
			
			totalPath= pathFolder+'/'+arg2; 
			
			filesystem::path pth(totalPath);
			if(filesystem::exists(pth))
			{
				detectedPathString=totalPath;
				detectedExecutableFlag=true;
				break;
			}            
		}
	}
	void searchContainingPath(string commandParsed, int sz2)
	{
		detectedPathString="";
		detectedExecutableFlag=false;
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
				detectedExecutableFlag=true;
			}            
		}
	}
	void searchExecutableForTypeCommand(string arg2)
	{
		detectedPathString="";
		detectedExecutableFlag=false;
		updatePathStringList();
		for(string pathFolder :pathVars )
		{
			string totalPath;
			
			totalPath= pathFolder+'/'+arg2; 
			
			filesystem::path pth(totalPath);
			if(filesystem::exists(pth))
			{
				detectedPathString=totalPath;
				detectedExecutableFlag=true;
				break;
			}            
		}
	}
	
};


class builtIn
{
	public:
	builtIn()
	{
		
	}
	void performBuiltInCommand(string firstWord,string argString,pathCheck pathChecker,string outputFileString,string arg2,vector<string>unquotedArgs,string command,set<int>escapedList,int outputRedirecOpPosn, int stdErrRedirectPosn)
	{
			
			try
			{
				
				ofstream outputFile;
				ofstream stdErrFile;
				if(outputFileString!="")
				{
					if(!filesystem::exists(outputFileString))
						outputFile=ofstream(outputFileString);
					else
						outputFile.open(outputFileString);
				}
				else
					outputFile.open("/dev/stdout");
				if(stdErrRedirectPosn!=1000000000)
				{
					if(!filesystem::exists(unquotedArgs[stdErrRedirectPosn+1]))
						stdErrFile=ofstream(unquotedArgs[stdErrRedirectPosn+1]);
					else
						stdErrFile.open(unquotedArgs[stdErrRedirectPosn+1]);
					
				}
				else
				{
					stdErrFile.open("/dev/stderr");
				}
				if(firstWord=="exit")
				{
					outputFile.close();
					stdErrFile.close();
					exit(0);
				}
				else if(firstWord=="echo")// basic echo without command substitution
				{ 
					
					int uqSz=unquotedArgs.size();
					uqSz=min(uqSz,outputRedirecOpPosn);
					uqSz=min(uqSz,stdErrRedirectPosn);
					for(int pos=0;pos<uqSz;pos++)
					{
						string wd=unquotedArgs[pos];
						outputFile<<wd;
						//if escaped then don't add space afterward in echo command
				
						if(!(escapedList.find(pos)!=escapedList.end()))
						{
							outputFile<<" ";
						}
						
					}
					outputFile<<"\n";
					stdErrFile<<"";
				
				}
				else if(firstWord=="pwd")
				{
					string directory(get_current_dir_name());
					outputFile<<directory<<"\n";
				}
				else if(firstWord=="type")
				{
					pathChecker.searchExecutableForTypeCommand(arg2);
					if(arg2=="echo" or arg2=="exit" or arg2=="type" or arg2=="pwd")
					{
						outputFile<<arg2<<" is a shell builtin\n";
			
					}
					else if(pathChecker.detectedExecutableFlag==true)
					{
						outputFile<<arg2<<" is "<<pathChecker.detectedPathString<<"\n";
					}
					else
					{
						stdErrFile<<arg2<<": not found\n";
					}
					
				}	
				else if(firstWord=="cd")
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
						
						stdErrFile<<"cd: "<<arg2<<": No such file or directory\n";
					}
				}
				else 
				{

					outputFile<<command<<": command not found\n";
				}
			outputFile.close();
			stdErrFile.close();
			}
		  	catch(const std::exception& e)
			{
				std::cerr << e.what() << '\n';
			}
			
}

};

class parser
{
	
	public:
	bool outputRedirectionDetect;
	int opRedirectionOperatorPosn;
	int stdErrRedirPosn;
	bool stdErrRedirectionDetect;
	string outputPath;
	string stdErrPath;
	parser()
	{
		this->outputRedirectionDetect=false;
		this->outputPath="";
		this->stdErrPath="";
		this->opRedirectionOperatorPosn=1000000000;
		this->stdErrRedirPosn=1000000000;
		this->stdErrRedirectionDetect=false;
	}
	/*Return executable path which is leftmost ,  enclosed within quotes if quotes are present*/
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

	/*separate the single quoted arguments apart from executable or command name, and store 
	indexes of escaped words for echo command*/
	vector<string> getSpecialArg(string argString,set<int>&escapedList)
	{
		//reset status of variables required for output redirection
		this->outputRedirectionDetect=false;
		this->outputPath="";
		
		this->stdErrPath="";
		this->opRedirectionOperatorPosn=1000000000;
		this->stdErrRedirPosn=1000000000;
		this->stdErrRedirectionDetect=false;
		
		int sz=argString.size();
		//Option1: detect escapes and insert double quotes instead 
		vector<string>unquotedArgs;
		if(argString=="")
		{
			return unquotedArgs;	
		}
		bool singleQuoteStart=false;
		int lastSingleQuoteStart=0;
		string newArg;
		bool spaceStart=true;
		int lastWordStart=0;
		int argNum=0;
		set<int>quotedNums; // add indexes of words present within quotess
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
				this->opRedirectionOperatorPosn=agNum;
				// need to skip operator and file name to avoid being sent in command arguments.
				if(agNum+1>=quotedargCount)
					throw "Missing outputFile Parameter Error";
				this->outputPath=unquotedArgs[agNum+1]; //until input redirection needs to be supported this should be the output file path
				
			}
			else if (unquotedArgs[agNum]=="2>")
			{
				
				this->stdErrRedirPosn=agNum;
				// need to skip operator and file name to avoid being sent in command arguments.
				if(agNum+1>=quotedargCount)
					throw "Missing stderrTarget For redirection Parameter Error";
				this->stdErrPath=unquotedArgs[agNum+1];
				this->stdErrRedirectionDetect=true;
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
	/* execute a system command with stdout or stderr redirected to file path instead */
	void executeCommandFileRedir(string mainCommand,vector<char*>argList,string outputFilePath,string stdErrFilePath,bool executableFoundFlag)
	{  
		if(executableFoundFlag==false)
		{
			ofstream outputFile;
			try{
			if(outputFilePath=="")
				cout<<mainCommand<<": command not found\n";
			else
			{
				outputFile.open(outputFilePath);
				outputFile<<mainCommand<<": command not found\n";
				outputFile.close();
			}
			
			
			}
			catch(exception e){
				cout<<"Exception detected\n";
				cout<<e.what()<<"\n";
				cout<<"Main command was("<<mainCommand<<")\n";
				for (auto val: argList)
					cout<<"Next Argument provided was("<<val<<")\n";
			}
			//outputFile.close();
			return;

		}
		if(outputFilePath=="" and stdErrFilePath=="")
		{
//cout<<"executing without outputRedirection\n";
			executeCommand(mainCommand,argList);

			return;
		}
		int sz=argList.size();
		int pos=0;
		int processPid= fork();
		int fileFD,fileFD2;
		if(processPid==0)//child
		{
			/* redirect write end of pipe of child to put output in file instead of STDOUT*/
			//replace write end file descriptor of this process with STDOUT

			if(outputFilePath!="")
			{
				fileFD=open(outputFilePath.c_str(),O_CREAT | O_RDWR);
				if(fileFD<0)
				{
					throw "Fail to use output file parameter to open filePath  "+outputFilePath+"with mode"+to_string(O_CREAT|O_RDWR);
				}
				int status=dup2(fileFD,STDOUT_FILENO);
				if(status<0)
				{
					throw "Fail to duplicate  STDout file descriptor";
				}
			}
			if(stdErrFilePath!="")
			{
				fileFD2=open(stdErrFilePath.c_str(),O_CREAT | O_RDWR);
				if(fileFD2<0)
				{
					throw "Fail to use stderr file parameter to open filePath "+stdErrFilePath+"with mode"+to_string(O_CREAT|O_RDWR);
				}
				int status2=dup2(fileFD2,STDERR_FILENO);
				if(status2<0)
				{
					throw "Fail to duplicate STDerr file descriptor";
				}
			}	
			
			
			//execute system command
			int result = execvp(mainCommand.c_str(), argList.data());
			//close the redirected files for STDOUT and STDERR
			close(fileFD);
			close(fileFD2);
			// if(result<0)
			// {
			// 	cerr<<mainCommand<<"\n";
			// 	perror("fail with negative code \t");
			// 	exit(1);
			// } 
			
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
	}

};



class Shell
{
	string input;
	parser parseInterface;
	executer executionInterface;
	builtIn builtInPerformer;
	set<string>builtInsList;
	pathCheck pathChecker;
	
	public:
	Shell(parser parserObject, executer executerObject,pathCheck pathChecker,builtIn builtINExecuter)
	{
		this->parseInterface=parserObject;
		this->executionInterface=executerObject;
		this->pathChecker=pathChecker;
		this->builtInPerformer=builtINExecuter;
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
		set<int>escapedList;//if word is escaped then while printing in echo command no space added afterward
		string firstWord;
		stringstream ss;
		string arg2;
		ss<<command;
		ss>>firstWord;
		ss>>arg2;
		//not builtin
		string argString="";
		
		vector<string>unquotedArgs;
		if(this->builtInsList.find(firstWord)==this->builtInsList.end())
		{

			string commandParsed;
			int sz2;
			//QUOTED EXECUTABLE PATH
			if(command[0]=='\'' or command[0]=='\"')
			{
				commandParsed= parseInterface.getMainArg(command);//extract command name including quotes
				sz2=commandParsed.size();
				pathChecker.searchContainingPath(commandParsed,sz2);
			}
			/*parse the path variable into a vector of strings,
		      set flag and break the loop if condition met
		      check flag and output at the end accordingly
		      search executable iterated in a loop over path strings*/
			else //SIMPLE UNQOUTED EXECUTABLE NAME
		    {
			  commandParsed=command;
		      pathChecker.searchExNameInPath(firstWord);
		    }

			//now for both cases when command is present and not present
			int inputSz=command.size();
			int ag1Size=firstWord.size();
			vector<char* >charArgs;
			
			if(commandParsed==firstWord)
			{
				argString="";
				
			}
			else if(command[0]!='\'' and command[0]!='\"')
			{
				argString=command.substr(ag1Size+1);    
				
			}
			else //first word has quotes, 
			{
				sz2=commandParsed.size();
				firstWord=command.substr(1,sz2-2);//now quotes removed          
				argString= command.substr(sz2+1);
				
		
			}    
			charArgs.push_back(const_cast<char*>(firstWord.c_str()));
			unquotedArgs= parseInterface.getSpecialArg(argString,escapedList);
			for(const string &wd:unquotedArgs)
			{   
				if(wd==">" or wd=="1>")
					break;
				if(wd =="2>")
					break;
				charArgs.push_back(const_cast<char*>(wd.c_str())); 
			}
			charArgs.push_back(nullptr);   
			executionInterface.executeCommandFileRedir(firstWord,charArgs,parseInterface.outputPath,parseInterface.stdErrPath,pathChecker.detectedExecutableFlag);
		
		
			
	
		}
		else
		{
			if(firstWord!=command)
				argString=command.substr(firstWord.size()+1);
			else
				argString="";
			unquotedArgs= parseInterface.getSpecialArg(argString,escapedList);
			builtInPerformer.performBuiltInCommand(firstWord,argString,this->pathChecker,parseInterface.outputPath,arg2,unquotedArgs,command,escapedList,parseInterface.opRedirectionOperatorPosn,parseInterface.stdErrRedirPosn);
			
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
		builtIn builtInPerformer= builtIn();
		Shell shell=  Shell(parserObject,executionObject,pathResolver,builtInPerformer);
		shell.run();
	
 	}
 	catch(...)
 	{
 		
 	}
  
  }
