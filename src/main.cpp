#include<iostream>
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
#include<readline/readline.h>
using namespace std;

#define NO_OPTION 0
#define WRITE_OPTION 1
#define APPEND_OPTION 2

set<string>builtInList={"cd","type","pwd","exit","echo"};
set<string>executableList;
//use autocompletion primitives while taking input

class completer
{
	public:
	
	static char* completion_generator(const char* text, int state) {
		// This function is called with state=0 the first time; subsequent calls are
		// with a nonzero state. state=0 can be used to perform one-time
		// initialization for this completion session.
		vector<string>vocabulary= vector<string>(builtInList.begin(),builtInList.end());
		vector<string>vocabulary2=vector<string>(executableList.begin(),executableList.end());
		static std::vector<std::string> matches;
		static size_t match_index = 0;

		if (state == 0) {
			// During initialization, compute the actual matches for 'text' and keep
			// them in a static vector.
			matches.clear();
			match_index = 0;

			// Collect a vector of matches: vocabulary words that begin with text.
			std::string textstr = std::string(text);
			for (auto word : vocabulary) {
			if (word.size() >= textstr.size() &&
				word.compare(0, textstr.size(), textstr) == 0) {
				matches.push_back(word);
			}
			}
			for (auto word : vocabulary2) {
			if (word.size() >= textstr.size() &&
				word.compare(0, textstr.size(), textstr) == 0) {
				matches.push_back(word);
			}
			}
		}

		if (match_index >= matches.size()) {
			// We return nullptr to notify the caller no more matches are available.
			return nullptr;
		} else {
			// Return a malloc'd char* for the match. The caller frees it.
			return strdup(matches[match_index++].c_str());
		}
		}
	static char** completion(const char* text, int start, int end) {
  // Don't do filename completion even if our generator finds no matches.
  rl_attempted_completion_over = 1;

  // Note: returning nullptr here will make readline use the default filename
  // completer.
  return rl_completion_matches(text,completion_generator);//SOME ISSUE SINCE MY CODE HAS THIS AS A METHOD, NOT SEPARATE FUNCITON
}
	string getInput()
	{
		
		rl_attempted_completion_function = completion;
		char * buf;
		//supports autocomplete and arrow keys editing for input and adds extra space after autocompleted string
		buf =readline("$ ");//read until it encounters a newline
		return string(buf);
	}
	
};

class pathCheck
{
	public:
	string detectedPathString;
	bool detectedExecutableFlag;
	vector<string>pathVars;
	/*Store list of executables names in path for autocompletion*/
	void updateExecutablesInPath()
	{
		updatePathStringList();
		//suppose path strings have been populated already
		for(string & pathValue:pathVars)
		{
			//look at each executable within this pathValue folder and fill it in autocompletion list
		    filesystem::path dirPath(pathValue);
			for(auto &iter : filesystem::directory_iterator(dirPath))
			{
				string pName= iter.path().filename();
				executableList.insert(pName);
			}
			
		}
	}
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

class parser
{
	
	public:
	int outputRedirectionStatus; 
	int opRedirectionOperatorPosn;
	int stdErrRedirPosn;
	int stdErrRedirectionStatus; 
	string outputPath;
	string stdErrPath;
	parser()
	{
		this->outputRedirectionStatus=NO_OPTION;
		this->outputPath="";
		this->stdErrPath="";
		this->opRedirectionOperatorPosn=1000000000;
		this->stdErrRedirPosn=1000000000;
		this->stdErrRedirectionStatus=NO_OPTION;
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
		this->outputRedirectionStatus=NO_OPTION;
		this->outputPath="";
		
		this->stdErrPath="";
		this->opRedirectionOperatorPosn=1000000000;
		this->stdErrRedirPosn=1000000000;
		this->stdErrRedirectionStatus=NO_OPTION;
		
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
				this->outputRedirectionStatus=WRITE_OPTION;
				this->opRedirectionOperatorPosn=agNum;
				// need to skip operator and file name to avoid being sent in command arguments.
				if(agNum+1>=quotedargCount)
					throw runtime_error("Missing outputFile Parameter Error on line"+std::to_string(__LINE__));
				this->outputPath=unquotedArgs[agNum+1]; //until input redirection needs to be supported this should be the output file path
				
			}
			else if(unquotedArgs[agNum]==">>" or unquotedArgs[agNum]=="1>>")
			{
				this->outputRedirectionStatus=APPEND_OPTION;
				this->opRedirectionOperatorPosn=agNum;
				// need to skip operator and file name to avoid being sent in command arguments.
				if(agNum+1>=quotedargCount)
					throw runtime_error("Missing outputFile Parameter Error on line"+std::to_string(__LINE__));
				this->outputPath=unquotedArgs[agNum+1]; //until input redirection needs to be supported this should be the output file path
			}
			else if (unquotedArgs[agNum]=="2>")
			{
				
				this->stdErrRedirPosn=agNum;
				// need to skip operator and file name to avoid being sent in command arguments.
				if(agNum+1>=quotedargCount)
					throw runtime_error("missing stdErrFile parameter Error on line "+std::to_string(__LINE__));
				this->stdErrPath=unquotedArgs[agNum+1];
				this->stdErrRedirectionStatus=true;
			}
			else if( unquotedArgs[agNum]=="2>>")
			{
				this->stdErrRedirectionStatus=APPEND_OPTION;
				this->stdErrRedirPosn=agNum;
				// need to skip operator and file name to avoid being sent in command arguments.
				if(agNum+1>=quotedargCount)
					throw runtime_error("missing stdErrFile parameter Error on line "+std::to_string(__LINE__));
				this->outputPath=unquotedArgs[agNum+1]; 
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

class builtIn
{
	public:
	builtIn()
	{
		
	}
	void performBuiltInCommand(string firstWord,string argString,pathCheck pathChecker,parser parseInterface,string arg2,vector<string>unquotedArgs,string command,set<int>escapedList)
	{
		
		string outputFileString=parseInterface.outputPath;
		string stdErrFileString=parseInterface.stdErrPath;
		int outputRedirecOpPosn=parseInterface.opRedirectionOperatorPosn;
		int stdErrRedirectPosn=parseInterface.stdErrRedirPosn;
		int opRedirecStatus=parseInterface.outputRedirectionStatus;
		int stdErrStatus=parseInterface.stdErrRedirectionStatus;
			try
			{
				
				ofstream outputFile;
				ofstream stdErrFile;
				if(opRedirecStatus==NO_OPTION)
				{
					outputFile.open("/dev/stdout");
				}
				else if(opRedirecStatus==WRITE_OPTION)
				{
					if(!filesystem::exists(outputFileString))
						outputFile=ofstream(outputFileString);
					else
						outputFile.open(outputFileString);
				}
				else if (opRedirecStatus==APPEND_OPTION) 
				{
					if(!filesystem::exists(outputFileString))
						outputFile=ofstream(outputFileString,ios_base::app);
					else
						outputFile.open(outputFileString,ios_base::app);
				}
				else
				{
					throw runtime_error("Incorrect output redirection option detected in line number"+std::to_string(__LINE__));
				}
				if(stdErrStatus==NO_OPTION)
				{
					stdErrFile.open("/dev/stderr");
				}
				else if(stdErrStatus==WRITE_OPTION)
				{
					if(!filesystem::exists(stdErrFileString))
						stdErrFile=ofstream(stdErrFileString);
					else
						stdErrFile.open(stdErrFileString);
					
				}
				else if(stdErrStatus==APPEND_OPTION)
				{
					if(!filesystem::exists(outputFileString))
						stdErrFile=ofstream(outputFileString,ios_base::app);
					else
						stdErrFile.open(outputFileString,ios_base::app);
				}
				else
				{
					throw runtime_error("Incorrect output redirection option detected in line number"+std::to_string(__LINE__));
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
			
			throw runtime_error("Error in process execution detected on line number "+std::to_string(__LINE__));
			
			} 
		}
		else if (processPid>0) //parent
		{
			int status;
			waitpid(processPid, &status, 0);
		}
		else // process pid ==-1 or error
		{
			throw runtime_error("Error in process creation detected on line number "+std::to_string(__LINE__));
		}

	}
	/* execute a system command with stdout or stderr redirected to file path instead */
	void executeCommandFileRedir(string mainCommand,vector<char*>argList,parser parseInterface,bool executableFoundFlag)
	{  
		string outputFilePath=parseInterface.outputPath;
		string stdErrFilePath=parseInterface.stdErrPath;
		int opRedirectStatus=parseInterface.outputRedirectionStatus;
		int stdRedirectStatus=parseInterface.stdErrRedirectionStatus;
		if(executableFoundFlag==false)
		{
			ofstream outputFile;
			try{
			if(opRedirectStatus==NO_OPTION)
				cout<<mainCommand<<": command not found\n";
			else if(opRedirectStatus==WRITE_OPTION or opRedirectStatus==APPEND_OPTION)
			{
				outputFile.open(outputFilePath);
				outputFile<<mainCommand<<": command not found\n";
				outputFile.close();
			}
			else
			{
				throw runtime_error("invalid opRedirect status");
			}
			
			
			}
			catch(exception e){
				cerr<<"Exception detected\n";
				cerr<<e.what()<<"\n";
				cerr<<"Main command was("<<mainCommand<<")\n";
				for (auto val: argList)
					cerr<<"Next Argument provided was("<<val<<")\n";
			}
			
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
		int status,status2;
		if(processPid==0)//child
		{
			/* redirect write end of pipe of child to put output in file instead of STDOUT*/
			//replace write end file descriptor of this process with STDOUT

			
			if(opRedirectStatus==WRITE_OPTION)
			{
				fileFD=open(outputFilePath.c_str(),O_CREAT |O_RDWR, 0644);
				
				status=dup2(fileFD,STDOUT_FILENO);
				
			}
			else if(opRedirectStatus==APPEND_OPTION)
			{
				fileFD=open(outputFilePath.c_str(),O_CREAT | O_APPEND|O_RDWR, 0644);
				
				status=dup2(fileFD,STDOUT_FILENO);
				
			}
			else if(opRedirectStatus!= NO_OPTION)
			{
				throw runtime_error("Invalid output Redirection Status detected on line number"+std::to_string(__LINE__));
			}
			if(stdRedirectStatus==WRITE_OPTION)
			{
				fileFD2=open(stdErrFilePath.c_str(),O_CREAT | O_RDWR);
				
				status2=dup2(fileFD2,STDERR_FILENO);
				
			}	
			else if(stdRedirectStatus==APPEND_OPTION)
			{
				fileFD=open(outputFilePath.c_str(),O_CREAT | O_APPEND|O_RDWR, 0644);
				
				status=dup2(fileFD,STDERR_FILENO);
			}
			else if(stdRedirectStatus!=NO_OPTION)
			{
				throw runtime_error("Invalid stderr Redirection Status detected on line number"+std::to_string(__LINE__));
			}
			
			
			//execute system command
			int result = execvp(mainCommand.c_str(), argList.data());
			//close the redirected files for STDOUT and STDERR
			if(opRedirectStatus!=NO_OPTION)
				close(fileFD);
			if(stdRedirectStatus!=NO_OPTION)
				close(fileFD2);
			
			
		}
		else if (processPid>0) //parent
		{
			int status;
			waitpid(processPid, &status, 0);
		}
		else // process pid ==-1 or error
		{
			throw runtime_error("Error in process ccreation detected on line number"+std::to_string(__LINE__));
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
	completer autoC;
	
	public:
	/*passing class objects by reference*/
	Shell(parser &parserObject, executer& executerObject,pathCheck& pathChecker,builtIn& builtINExecuter, completer& autoCompleter)
	{
		this->parseInterface=parserObject;
		this->executionInterface=executerObject;
		this->pathChecker=pathChecker;
		this->builtInPerformer=builtINExecuter;
		this->autoC=autoCompleter;
		
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
		if(builtInList.find(firstWord)==builtInList.end())
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
			
			if(commandParsed==firstWord)//true if arguments not present
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
				firstWord=commandParsed.substr(1,sz2-2);//now quotes removed          
				argString= command.substr(sz2+1);
				
			}    
			charArgs.push_back(const_cast<char*>(firstWord.c_str()));
			unquotedArgs= parseInterface.getSpecialArg(argString,escapedList);
			for(const string &wd:unquotedArgs)
			{   
				if(wd==">" or wd=="1>" or wd==">>" or wd =="1>>" or wd=="2>" or wd=="2>>")
					break;
				charArgs.push_back(const_cast<char*>(wd.c_str())); 
			}
			charArgs.push_back(nullptr);   
			executionInterface.executeCommandFileRedir(firstWord,charArgs,parseInterface,pathChecker.detectedExecutableFlag);	
	
		}
		else
		{
			if(firstWord!=command)
				argString=command.substr(firstWord.size()+1);
			else
				argString="";
			unquotedArgs= parseInterface.getSpecialArg(argString,escapedList);
			builtInPerformer.performBuiltInCommand(firstWord,argString,this->pathChecker,parseInterface,arg2,unquotedArgs,command,escapedList);
			
		}
		
	}
	void run()
	{
		
		while(true)
	  	{
			// uses readline for input and autocompletion
			this->pathChecker.updateExecutablesInPath();
			string input=this->autoC.getInput();	
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
		pathResolver.updatePathStringList();
		builtIn builtInPerformer= builtIn();
		completer autoCompletionInterface=completer();
		Shell shell=  Shell(parserObject,executionObject,pathResolver,builtInPerformer,autoCompletionInterface);
		shell.run();
	
 	}
 	catch(runtime_error e)
 	{
 		cout<<e.what();
 	}
  
  }
