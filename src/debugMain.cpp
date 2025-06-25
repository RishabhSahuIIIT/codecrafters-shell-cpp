//to check echo "script  world"  "shell""hello"
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
		this->stdErrRedirectionStatus=NO_OPTION;
	}
	/*Separate by pipes and redirection*/
	vector<string>separateCommands(string cPipeline)
	{
		stringstream pipeSep;
		pipeSep<<cPipeline;
		vector<string>commands;
		string token="";
		this->outputPath="";
		this->stdErrPath="";
		this->outputRedirectionStatus=NO_OPTION;
		this->stdErrRedirectionStatus=NO_OPTION;
		//assuming that pipe symbol character doesn't appear within any command parameter string
		while(getline(pipeSep,token,'|'))
		{
			commands.push_back(token);
		}
		
		//check for redirection operator presence
		string last=(*(commands.rbegin()));
		if(commands.size()==0) //if commands are not separated by '|'
		{
			last.clear();
			last=cPipeline;
			pipeSep.clear();
			pipeSep.str("");
			pipeSep<<last;
		}
		else
		{
			commands.pop_back();
			pipeSep.clear();
			pipeSep.str("");
			pipeSep<<last;
		}
		
		if(last.find('>')==string::npos) //no redirection operator present
		{	
			commands.push_back(last);
	
			return commands;
		}
		else
		{
			int pos=last.find('>');
			string mainCommand=last.substr(0,pos);
			if(last.find("1>>")!=string::npos or last.find("2>>")!= string :: npos or last.find("1>")!= string::npos  or last.find("2>")!=string::npos)
			{
				mainCommand=last.substr(0, pos-1);
			}
			commands.push_back(mainCommand);
		}
		//assuming redirection operator found above doesn't appear within some string paremeter
		while(pipeSep>>token) 
		{
			if(token==">" or token=="1>")
			{
				this->outputRedirectionStatus=WRITE_OPTION;
				string opFilePath;
				try
				{
					/* code */
					pipeSep>>opFilePath;
					this->outputPath=opFilePath; //until input redirection needs to be supported this should be the output file path
				}
				catch(const std::exception& e)
				{
					std::cerr << e.what() << '\n';
				}
				
			}
			else if(token==">>" or token== "1>>")
			{
				this->outputRedirectionStatus=APPEND_OPTION;
				string opFilePath;
				try
				{
					/* code */
					
					pipeSep>>opFilePath;
					this->outputPath=opFilePath; //until input redirection needs to be supported this should be the output file path
				}
				catch(const std::exception& e)
				{
					std::cerr << e.what() << '\n';
				}

			}
			if(token=="2>")
			{
				
				// need to skip operator and file name to avoid being sent in command arguments.
				
				this->stdErrRedirectionStatus=WRITE_OPTION;
				string stdErrFile;
				pipeSep>>stdErrFile;
				this->stdErrPath=stdErrFile;

			}
			else if(token=="2>>")
			{
				this->stdErrRedirectionStatus=APPEND_OPTION;
				string stdErrFile;
				pipeSep>>stdErrFile;
				this->stdErrPath=stdErrFile;
			}
		}
		return commands;


	}
	/*Return executable path which is leftmost ,  enclosed within quotes if quotes are present*/
	string getMainArg(string singleCommand)
	{
		
		string command="";
		int sz=singleCommand.size();
		int k;
		if(singleCommand[0]=='\"')
		{

			for(int pos=1 ;pos<sz;pos++)
			{
				if(singleCommand[pos]=='\"') 
				{
					k=pos;
					break;
				}
			}
			command=singleCommand.substr(0,k+1);
		}
		else if(singleCommand[0]=='\'')
		{
			for(int pos=1 ;pos<sz;pos++)
			{
				if(singleCommand[pos]=='\'') 
				{
					k=pos;
					break;
				}
			}
			command=singleCommand.substr(0,k+1);
		}
		return command;
	}

	/*separate the single quoted arguments apart from executable or command name, and remove the quotation characters 
	as well as  store indexes of escaped characters in the list of separated arguments for echo command 
	String withing Double quotes or single quotes : preserve spaces and escapes
	
	*/
	vector<string> processParameters(string singleCommandString,set<int>&escapedList)
	{
		//reset status of variables required for output redirection

		int sz=singleCommandString.size();
		vector<string>unquotedArgs;
		if(singleCommandString=="")
		{
			return unquotedArgs;	
		}
		bool singleQuoteStart=false;
		int lastSingleQuoteStart=0;
		string newArg;
		bool notWithinQuotes=true;
		int lastWordStart=0;
		int argNum=0;
		set<int>contiguousQuotedNums; // add indexes of words present within quotess
		//special characters for double quotes  ‘$’, ‘`’, ‘"’, ‘\’, 
		bool doubleQuoteStart=false;
		int lastDoubleQuoteStart=0;
		
		for( int pos=0;pos<sz;pos++)
		{
			notWithinQuotes= !(singleQuoteStart or doubleQuoteStart);

			if(doubleQuoteStart==true)
			{
				if(singleCommandString[pos]=='\"' )
				{
					newArg=singleCommandString.substr(lastDoubleQuoteStart+1,((pos-1)-(lastDoubleQuoteStart+1)+1));
					doubleQuoteStart=false;
					singleQuoteStart=false;
					unquotedArgs.push_back(newArg); //A double quote pair is complete
					contiguousQuotedNums.insert(argNum);// found a word enclosed within double quotes
// cout<<"inserted"<<argNum<<"\n";
 //cout<<"A"<<newArg<<argNum<<")\n";

					
					if(pos+1<sz and singleCommandString[pos+1]!=' ')//if next character not a space then put in set to avoid adding space in echo
					 	escapedList.insert(argNum);
					argNum++;      
					
					
					
				}
				else if(singleCommandString[pos]=='\\')//backslash within double quotes
				{
					if(singleCommandString[pos+1]=='\\' or singleCommandString[pos+1]=='`' or singleCommandString[pos+1]=='$' or singleCommandString[pos+1]=='\"')
					{
						singleCommandString.erase(pos,1);//ERASE EXTRA BACKSLASH BEFORE ESCAPED CHARACTER  , now escaped character at pos+1
						pos++;//skip processing at pos+1 th character in original command to avoid detection as quote
						if(singleCommandString[pos]=='\"') // for \\" case 
						{
							newArg=singleCommandString.substr(lastDoubleQuoteStart+1,((pos-1)-(lastDoubleQuoteStart+1)+1));
							unquotedArgs.push_back(newArg);
							
//cout<<"B"<<newArg<<argNum<<")\n";
							argNum++;
							doubleQuoteStart=false;
						}
						sz=singleCommandString.size();
					}
				}
			}
			else //if(doubleQuoteStart==false)
			{
				if(singleCommandString[pos]=='\"' and !( (pos>0) and singleCommandString[pos-1]=='\\' ) and singleQuoteStart==false) //double quote starts now
				{
					doubleQuoteStart=true;
					lastDoubleQuoteStart=pos;
					
					//remove argNum-1 from list of contiguous quotedNums since it wasn't quoted
					if(contiguousQuotedNums.find(argNum-1)!=contiguousQuotedNums.end() )
					{
							
						if(pos>0  and !(singleCommandString[pos-1]=='\'' or singleCommandString[pos-1]=='\"'))
							contiguousQuotedNums.erase(argNum-1);  
					}
					//separate word appearing before double quotes
					newArg=singleCommandString.substr(lastWordStart,((pos-1)-lastWordStart+1));
					
					if(newArg!="" and !( pos>0 and (singleCommandString[pos-1]==' ' or singleCommandString[pos-1]=='\"'))) /*dont add empty string and avoid adding the same term again if previous character is space*/ 
					{
//cout<<"C"<<newArg<<argNum<<")\n";

						unquotedArgs.push_back(newArg); //an unqouted word appearing before double quote is complete
						argNum++;
					}
					else if(newArg=="")
					{
						if(pos+1<sz and singleCommandString[pos+1]!=' ')
							escapedList.insert(argNum+2);
					}

				}
				else if (singleQuoteStart==true)
				{
					if (singleCommandString[pos]=='\'' ) //close previously started single quote
					{
					//extra pos-1 and lastSingleQuoteStart+1 to remove quotes
						newArg=singleCommandString.substr(lastSingleQuoteStart+1,((pos-1)-(lastSingleQuoteStart+1)+1));
						singleQuoteStart=false;
						if(newArg!="")
						{
							unquotedArgs.push_back(newArg);  // a single quoted string is complete
							contiguousQuotedNums.insert(argNum);//found a word enclosed within single quotes
							argNum++;

						}
						else
						{
//cout<<"empty word"<<argNum<<"\n";
							if(singleCommandString[pos-1]!= ' '  and pos+1<sz and  singleCommandString[pos+1]!=' ')
							{
								escapedList.insert(argNum-1);
//cout<<argNum-1<<"added to escapedList\n";
							}
						}
						
// cout<<"inserted"<<argNum<<"\n";
 //cout<<"D"<<newArg<<argNum<<")\n";

						

						
					} 
					else if(singleCommandString[pos]!='\'' )
					{
						continue;
					}
				}
				//cases without quote
				else //if(singleQuoteStart==false )
				{
					if(singleCommandString[pos]=='\\')// backslash outside any quotes
					{          
						if(pos+1<sz ) // if previous was space then the word preceding is already stored
						{
							
							if(notWithinQuotes==true and !(pos>1 and singleCommandString[pos-2]=='\\')) //need to store any preceding lettered word as well
							{
								newArg=singleCommandString.substr(lastWordStart,((pos-1)-lastWordStart+1));
								unquotedArgs.push_back(newArg);
								escapedList.insert(argNum);
//cout<<"E"<<newArg<<argNum<<")\n";

								argNum++;
							}
							
							//next charcater is escaped assuming it exists
							escapedList.insert(argNum);						
							newArg=singleCommandString.substr(pos+1,1);
							pos+=1;
							unquotedArgs.push_back(newArg);
//cout<<"F"<<newArg<<argNum<<")\n";

							escapedList.insert(argNum);


							lastWordStart=pos+1;
//cout<<"G"<<newArg<<argNum<<")\n";

							argNum++;
							
						}
					}
					else if( singleCommandString[pos]=='\'')//single quote starts
					{
						singleQuoteStart=true;
						if(notWithinQuotes==true and singleCommandString[pos-1]!='\'' and singleCommandString[pos-1]!='\"' and singleCommandString[pos-1]!=' ') //end any previous non quoted word
						{
							
							newArg=singleCommandString.substr(lastWordStart,((pos-1)-lastWordStart+1));// an unqouted word appearing before single quote is complete
							if(newArg!="")
							{
								unquotedArgs.push_back(newArg);
//cout<<"GH"<<newArg<<argNum<<")\n";							
								argNum++;
							}
							
						}
						lastSingleQuoteStart=pos;
						
						//previous extracted word was quoted but immediate previous character was not a quote
						if(contiguousQuotedNums.find(argNum-1)!=contiguousQuotedNums.end() and singleCommandString[pos-1]==' ')
						{
							contiguousQuotedNums.erase(argNum-1); 
//cout<<"removed"<<argNum-1<<"\n";
//cout<<"found";
						}
						
						
						
					
					}
					else if( singleCommandString[pos]==' ' and notWithinQuotes==false )
					{
						
						continue;

					}
					else if(singleCommandString[pos]==' ' and notWithinQuotes==true and singleCommandString[pos-1]!='\'' and singleCommandString[pos-1]!='\"' and singleCommandString[pos-1]!=' ')
					{
						newArg=singleCommandString.substr(lastWordStart,((pos-1)-lastWordStart+1));// an unqouted word appearing before space is complete
//cout<<"H"<<notWithinQuotes<<doubleQuoteStart<<newArg<<argNum<<")\n";

						unquotedArgs.push_back(newArg);
						argNum++;
						
					}
					else if(singleCommandString[pos]!=' ' and notWithinQuotes==false)// not a space and not a quote
					{
						if(pos>0 and singleCommandString[pos-1]==' ')
							lastWordStart=pos;
						
						
					}
					else if(singleCommandString[pos]!=' ' and notWithinQuotes==true)
					{
						if(singleCommandString[pos-1]=='\"' or singleCommandString[pos-1]=='\'' or singleCommandString[pos-1]==' ')
						{
							lastWordStart=pos;
//cout<<"changedLastWorsdStartto"<<pos<<"\n";
						}
					}
				}      
			
			}
		} 
		//last word consisting of letters
		if(singleCommandString[sz-1]!='\'' and singleCommandString[sz-1]!=' ' and singleCommandString[sz-1]!='\"' and notWithinQuotes==true)
		{
			newArg=singleCommandString.substr(lastWordStart,((sz-1)-lastWordStart+1)); //an unqoted word appearing at the end is complete
//cout<<"I"<<newArg<<argNum<<")\n";
			unquotedArgs.push_back(newArg);
			argNum++;
		}
		//last word with escaped special quote or space
		else if((singleCommandString[sz-1]=='\'' or singleCommandString[sz-1]==' ' or singleCommandString[sz-1]=='\"') and singleCommandString.at(sz-1)=='\\' and notWithinQuotes==true)
		{
			newArg=singleCommandString.substr(lastWordStart,((sz-1)-lastWordStart+1)); //an unqoted word appearing at the end is complete
//cout<<"J"<<newArg<<argNum<<")\n";

			unquotedArgs.push_back(newArg);
			argNum++;
		}
		
		vector<string>quotedMerged;
		int quotedargCount=unquotedArgs.size();
//cout<<quotedargCount<<"isquotedARgCount\n";
// for(int val:escapedList)
// {
// 	cout<<val<<"\t";
// }
// cout<<"\n";
		for(int agNum=0;agNum<quotedargCount;agNum++)
		{
//cout<<agNum<<"isAgNum\n";
			//join two arguments at agNum and agNum+1 th position separated only by quotes (without any space or character)
			if(contiguousQuotedNums.find(agNum)!=contiguousQuotedNums.end())
			{
				if(contiguousQuotedNums.find(agNum+1)!=contiguousQuotedNums.end() )
				{
					unquotedArgs[agNum+1]=unquotedArgs[agNum]+unquotedArgs[agNum+1];
//cout<<"found pair"<<agNum<<" "<<agNum+1<<" "<<unquotedArgs[agNum]<<"\n";
					// modify set by assigning a new set to avoid errors due to undefined behavior
					//modifying the same set while iterating it in for loop will cause segmentation fault due to undefined behavior 
					set<int> newEscapedList;
					for(int val : escapedList)
					{
						if(val > agNum)
							newEscapedList.insert(val - 1);
						else
							newEscapedList.insert(val);
					}
					escapedList = newEscapedList;				
				}
				else
				{
//cout<<"(A"<<unquotedArgs[agNum]<<agNum<<")\n";
					quotedMerged.push_back(unquotedArgs[agNum]);        
				}
			}
			
			else
			{
//cout<<"(B"<<unquotedArgs[agNum]<<agNum<<")\n";
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
					
					for(int pos=0;pos<uqSz;pos++)
					{
						string wd=unquotedArgs[pos];
						outputFile<<wd;
						//if escaped then don't add space afterward in echo command
				
						if(escapedList.find(pos)==escapedList.end())
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
			executeCommand(mainCommand,argList);

			return;
		}
		int sz=argList.size();
		int pos=0;
		int processPid= fork();
		int outputFDD,errorFileFD;
		int tempIn,tempOut,tempError;
		int status,status2;
		if(opRedirectStatus==WRITE_OPTION)
		{
			outputFDD=open(outputFilePath.c_str(),O_CREAT |O_RDWR, 0644);
			tempOut=dup(STDOUT_FILENO);
			status=dup2(outputFDD,STDOUT_FILENO);
			close(outputFDD);
			// if(status<0)
			// 	perror("dup2 outputRedirect");
			
			
		}
		else if(opRedirectStatus==APPEND_OPTION)
		{
			outputFDD=open(outputFilePath.c_str(),O_CREAT | O_APPEND|O_RDWR, 0644);
			tempOut=dup(STDOUT_FILENO);
			status=dup2(outputFDD,STDOUT_FILENO);
			close(outputFDD);
			// if(status<0)
			// 	perror("dup2 outputRedirect append");
			
			
		}
		else if(opRedirectStatus!= NO_OPTION)
		{
			throw runtime_error("Invalid output Redirection Status detected on line number"+std::to_string(__LINE__));
		}
		if(stdRedirectStatus==WRITE_OPTION)
		{
			errorFileFD=open(stdErrFilePath.c_str(),O_CREAT | O_RDWR);
			tempError=dup(STDERR_FILENO);
			status2=dup2(errorFileFD,STDERR_FILENO);
			close(errorFileFD);
			// if(status<0)
			// 	perror("stderr Redirect");
			
			
		}	
		else if(stdRedirectStatus==APPEND_OPTION)
		{
			errorFileFD=open(stdErrFilePath.c_str(),O_CREAT | O_APPEND|O_RDWR, 0644);
			tempError=dup(STDERR_FILENO);
			status2=dup2(errorFileFD,STDERR_FILENO);
			close(errorFileFD);
			// if(status2<0)
			// 	perror("stderr Redirect append");
			
		}
		else if(stdRedirectStatus!=NO_OPTION)
		{
			throw runtime_error("Invalid stderr Redirection Status detected on line number"+std::to_string(__LINE__));
		}
		

		if(processPid==0)//child
		{
			/* redirect write end of pipe of child to put output in file instead of STDOUT*/
			//replace write end file descriptor of this process with STDOUT

			
			
			//execute system command
			int result = execvp(mainCommand.c_str(), argList.data());
			if(result<0)
				perror("");
			//close the redirected files for STDOUT and STDERR
			
			
			
		}
		else if (processPid>0) //parent
		{
			
			
			int status;
			waitpid(processPid, &status, 0);
			close(outputFDD);
			close(errorFileFD);
			dup2(tempOut,STDOUT_FILENO);
			dup2(tempError,STDERR_FILENO);
		}
		else // process pid ==-1 or error
		{
			throw runtime_error("Error in process ccreation detected on line number"+std::to_string(__LINE__));
		}
	}

	void execRedirPiped(vector<string> mainCommands,vector<vector<char*>>argLists,parser& parseInterface)
	{  
		//initialize all pipes
		
		
		string outputFilePath=parseInterface.outputPath;
		string stdErrFilePath=parseInterface.stdErrPath;
		int opRedirectStatus=parseInterface.outputRedirectionStatus;
		int stdRedirectStatus=parseInterface.stdErrRedirectionStatus;
		//assuming 2 commands for current implementation stage "Dual command pipeline"
		int fds[2];
		if(pipe(fds)<0)
		{
			perror("pipe generation");
			return;
		}
		pid_t pr0Id=fork();
		if(pr0Id<0)
		{
			perror("Fork for process0 fail");
			return ;
		}
		
		if(pr0Id==0)//child process
		{
			//close read end of pipe since read is by stdin
			close(fds[0]);
			// set stdout to write end of pipe
			dup2(fds[1],STDOUT_FILENO);
			close(fds[1]);
// Before calling execvp, add this in your child processes:
// for(int i = 0; argLists[0][i] != nullptr; i++) {
//     cerr << "Command 0 arg[" << i << "]: '" << argLists[0][i] << "'" << endl;
// }

			execvp(mainCommands[0].c_str(),argLists[0].data());
			//in case of successful execution it should not return 
			perror("fail in process0 exec");
			exit(1);

		}
		int fileFD,status,fileFD2,status2;
		pid_t pr1Id=fork();
		if(pr1Id<0)
		{
			perror("Fork for process1 fail");
			return ;
		}
		if(pr1Id==0)//child process
		{
			//close write end of pipe since write is to output file or stdout
			close(fds[1]);
			//set output file 
			if(opRedirectStatus==WRITE_OPTION)
			{
				fileFD=open(outputFilePath.c_str(),O_CREAT |O_RDWR, 0644);
				
				status=dup2(fileFD,STDOUT_FILENO);
				close(fileFD);
				
			}
			else if(opRedirectStatus==APPEND_OPTION)
			{
				fileFD=open(outputFilePath.c_str(),O_CREAT | O_APPEND|O_RDWR, 0644);
				
				status=dup2(fileFD,STDOUT_FILENO);
				close(fileFD);
				
			}
			else if(opRedirectStatus!= NO_OPTION)
			{
				throw runtime_error("Invalid output Redirection Status detected on line number"+std::to_string(__LINE__));
			}
			if(stdRedirectStatus==WRITE_OPTION)
			{
				fileFD2=open(stdErrFilePath.c_str(),O_CREAT | O_RDWR);
				
				status2=dup2(fileFD2,STDERR_FILENO);
				close(fileFD2);
				
			}	
			else if(stdRedirectStatus==APPEND_OPTION)
			{
				fileFD2=open(stdErrFilePath.c_str(),O_CREAT | O_APPEND|O_RDWR, 0644);
				
				status2=dup2(fileFD2,STDERR_FILENO);
				close(fileFD2);
			}
			else if(stdRedirectStatus!=NO_OPTION)
			{
				throw runtime_error("Invalid stderr Redirection Status detected on line number"+std::to_string(__LINE__));
			}
			


			// set stdin to read end of pipe
			dup2(fds[0],STDIN_FILENO);
			close(fds[0]);
			execvp(mainCommands[1].c_str(),argLists[1].data());
			//in case of successful execution it should not return 
			perror("fail in process1 exec");
			exit(1);

		}
		//close pipe
		close(fds[0]);
		close(fds[1]);
		int waitStatus;
		waitpid(pr0Id,&waitStatus,0);
		waitpid(pr1Id,&waitStatus,0);

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
		vector<string>commands=parseInterface.separateCommands(command);
		int commandCount=commands.size();
		vector<set<int>>escapedLists;
		vector<string>fws;//firstwords
		vector<string>ag2s;//arg2 s 
		vector<vector<char*>>charAgs;//charArgs
		stringstream ss;
		int pos=0;
		string firstWord,arg2;
		string argString="";
		
		//No requirement to check escaped list for non builtins in piping in current stage
		set<int>escapedList;//if word is escaped then while printing in echo command no space added afterward
		if(commandCount>1)
		{
			for(int pos=0;pos<commandCount;pos++)
			{
				argString="";
				string comString=commands[pos];
				// separation of main command ,arguments and quoting
				ss.clear();
				ss.str("");
				ss<<comString;
				firstWord="";
				ss>>firstWord;

				arg2="";
				ss>>arg2;
				ag2s.push_back(arg2);
				vector<string>unquotedArgs;
				string commandParsed;
				int sz2;
				//QUOTED EXECUTABLE PATH
				if(comString[0]=='\'' or comString[0]=='\"')
				{
					commandParsed= parseInterface.getMainArg(comString);//extract command name including quotes
					sz2=commandParsed.size();
					pathChecker.searchContainingPath(commandParsed,sz2);
				}
				/*parse the path variable into a vector of strings,
				set flag and break the loop if condition met
				check flag and output at the end accordingly
				search executable iterated in a loop over path strings*/
				else //SIMPLE UNQOUTED EXECUTABLE NAME
				{
					commandParsed=comString;
					pathChecker.searchExNameInPath(firstWord);
				}

				//now for both cases when command is present and not present
				int inputSz=comString.size();
				int ag1Size=firstWord.size();
				
				//vector<char* >charArgs;
				
				if(commandParsed==firstWord)//true if arguments not present
				{
					argString="";
					
				}
				else if(comString[0]!='\'' and comString[0]!='\"')
				{
					argString=comString.substr(ag1Size+1);    
					
				}
				else //first word has quotes, 
				{
					sz2=commandParsed.size();
					firstWord=commandParsed.substr(1,sz2-2);//now quotes removed          
					argString= comString.substr(sz2+1);
					
				}    
				unquotedArgs= parseInterface.processParameters(argString,escapedList); //includes firstWord
				vector<string> persistentArgs;
				persistentArgs.push_back(firstWord);

				for(const string &wd:unquotedArgs) {
					if(wd!="")
						persistentArgs.push_back(wd);
				}
				vector<char*> charArgs;
				//IMPORTANT, WE USED STRDUP INSTEAD OF CONST CHAR* CAST AND IT HELPED TO SOLVE DANGLING POINTER PROBLEM
				for(const string &arg : persistentArgs) {
					charArgs.push_back(strdup(arg.c_str()));
				}
				charArgs.push_back(nullptr);
 
				fws.push_back(firstWord);
				charAgs.push_back(charArgs);

			}
			executionInterface.execRedirPiped(fws,charAgs,parseInterface);
			// After using charArgs, free the allocated memory:
			for(vector<char*>charArgs:charAgs){
			for(char* arg : charArgs) {
				if(arg != nullptr) {
					free(arg);
				}
			}
		}
		}
		else
		{
			string commandNew=commands[0];


			//past implementation: separates main command, arguments and quoting , then executes command with redirection
			//new : separate into pipes, do separation of main command , arguments and quoting for each command in pipeline, execute commands in loop

			ss.clear();
			ss.str("");
			ss<<commandNew;
			ss>>firstWord;
			ss>>arg2;
			//not builtin
			

			vector<string>unquotedArgs;
			if(builtInList.find(firstWord)==builtInList.end())
			{

				string commandParsed;
				int sz2;
				//QUOTED EXECUTABLE PATH
				if(command[0]=='\'' or command[0]=='\"')
				{
					commandParsed= parseInterface.getMainArg(commandNew);//extract command name including quotes
					sz2=commandParsed.size();
					pathChecker.searchContainingPath(commandParsed,sz2);
				}
				/*parse the path variable into a vector of strings,
				set flag and break the loop if condition met
				check flag and output at the end accordingly
				search executable iterated in a loop over path strings*/
				else //SIMPLE UNQOUTED EXECUTABLE NAME
				{
					commandParsed=commandNew;
					pathChecker.searchExNameInPath(firstWord);
				
				}

				//now for both cases when command is present and not present
				int inputSz=commandNew.size();
				int ag1Size=firstWord.size();
				//vector<char* >charArgs;
				
				if(commandParsed==firstWord)//true if arguments not present
				{
					argString="";
					
				}
				else if(commandNew[0]!='\'' and commandNew[0]!='\"')
				{
					argString=commandNew.substr(ag1Size+1);    
					
				}
				else //first word has quotes, 
				{
					sz2=commandParsed.size();
					firstWord=commandParsed.substr(1,sz2-2);//now quotes removed          
					argString= commandNew.substr(sz2+1);
					
				}    
				//charArgs.push_back(const_cast<char*>(firstWord.c_str()));
				vector<string> persistentArgs;
				unquotedArgs=parseInterface.processParameters(commandNew,escapedList);// includes first word
				for(const string &wd:unquotedArgs) {
					if(wd!="")
						persistentArgs.push_back(wd);
				}
				vector<char*> charArgs;
				for(const string &arg : persistentArgs) {
					charArgs.push_back(strdup(arg.c_str()));

				}
				charArgs.push_back(nullptr);
 
				executionInterface.executeCommandFileRedir(firstWord,charArgs,parseInterface,pathChecker.detectedExecutableFlag);	
				// After using charArgs, free the allocated memory:
				for(char* arg : charArgs) {
					if(arg != nullptr) {
						free(arg);
					}
				}
		
			}
			else
			{
				if(firstWord!=commandNew)
					argString=commandNew.substr(firstWord.size()+1);
				else
					argString="";
				unquotedArgs= parseInterface.processParameters(argString,escapedList);
				builtInPerformer.performBuiltInCommand(firstWord,argString,this->pathChecker,parseInterface,arg2,unquotedArgs,commandNew,escapedList);
				
			}
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
