#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

///README file

using namespace std;

//FUNCTION SIMILAR TO SETENV

class User_Variables
{
    private:
        map<string, string> user_var; // map for storing variables and their values

    public:
        //SETTING USER MADE VARIABLE
        void SetVar(vector<string> cmd)
        {
            int n=cmd.size();

            int i;
            for(i=0;i<n;i++)
            {
                if(cmd[i]=="=")
                    break;
            }

            user_var.insert(pair<string, string>(cmd[i-1],cmd[i+1]));
        }

        //CALLING THE VARIABLE
        string CallVar(string command)
        {
            int i=0;
            while(i<(int)command.size())    // checking if user has asked for multiple variables in the same command
            {
                for(;i<(int)command.size();i++)
                {
                    if(command[i]=='$') //checking the starting position of asked variables
                        break;
                }
                if(i>=(int)command.size())
                 break;

                char delim1=' ';
                char delim2='\t';

                int end = min(command.find(delim1,i),command.find(delim2,i));
                if(end==-1)
                    end=command.size();

                string askedvar=command.substr(i+1,end-i-1);
                string answer="";

                map<string,string>::iterator it;    //checking if asked variable is user made
                for(it=user_var.begin();it!=user_var.end();it++)
                {
                    if(it->first==askedvar)
                    {
                        answer=it->second;
                        break;
                    }
                }

                const char* systvar=askedvar.c_str();   //checking if asked variable is system variable
                if(getenv(systvar)!=NULL)
                    answer=getenv(systvar);

                if(answer!="")      //substituting the correct value of variable
                {
                    command.erase(i,end-i);
                    command.insert(i,answer);
                }
                i=i+answer.size();

            }

            return(command); // returning the variable substituted command
        }

        //REMOVING THE USER MADE VARIABLE
        void RemoveVar(string askedvar)
        {
            char delim3=(char)39;
            char delim4=(char)34;

            map<string,string>::iterator it;
            if(askedvar[0]==delim3 || askedvar[0]==delim4)
                askedvar=askedvar.substr(1,askedvar.size()-2);

            it=user_var.find(askedvar);
            if(it!=user_var.end())
                user_var.erase(it);

        }

        //PRINTING THE ELEMENTS OF MAP IF NEEDED
        void PrintMap()
        {
            map<string,string>::iterator it;
            for(it=user_var.begin();it!=user_var.end();it++)
            {
                cout << it->first << "\t" << it->second << endl;
            }
        }
};

//GETTING ALL THE DIFFERENT PATHS
vector<string> ListPaths()
{
    string allpaths=getenv("PATH");
    vector<string> indpath; //vector having individual paths
    int start = 0;//Initialization
    int end = allpaths.find(":");//Initialization
    while(end !=-1 )
    {
        //Adding single single words to the indpath vector
        indpath.push_back(allpaths.substr(start,end-start));

        start = end+1;
        end = allpaths.find(":", start);
    }
    end = allpaths.size();

    if(start!=end)
    {
        indpath.push_back(allpaths.substr(start,end-start));
    }

    return(indpath); //returning vector of strings of paths

}

//PARSING INPUT
vector<string> Parse_Input(string command)
{
    vector<string> cmd; // vector of string containing individual parsed tokens

    //Initializing our delimiters
    char delim1=' ';
    char delim2='\t';
    char delim3=(char)39;
    char delim4=(char)34;

    {
        int negspaces[(int)command.size()]={0};//To neglect spaces in quotes, array denoting 1 at positions to neglect parsing there
        int quote_start=0;
        int quote_end=0;
        while(quote_start<(int)command.size())

        {
            quote_start=min(command.find(delim3,quote_end+1),command.find(delim4,quote_end+1));
            if(quote_start==-1)
                break;
            if(quote_start==(int)command.find(delim3,quote_end))
            {
                quote_end=command.find(delim3,quote_start+1);
            }
            else
                quote_end=command.find(delim4,quote_start+1);

            for(int i=quote_start;i<=quote_end;i++)
                    negspaces[i]=1;
        }

        //taking care that '=','<','>',"<<",'|' are placed as a single string in cmd vector
        auto it=command.begin();
        for(it=command.begin();it<command.end();it++)
        {
            if(*it=='=' || *it=='<' || *it=='>' || *it=='|')
            {
                command.insert(it,1,delim1);
                command.insert(it+2,delim1);
                it+=3;
            }

            if(*it=='<' && *(it+1)=='<')
            {
                command.insert(it,1,delim1);
                command.insert(it+3,1,delim1);
                it+=4;
            }
        }

        //Initializing our start and end index counters while parsing the input command
        int start=0;
        int end = min(command.find(delim1),command.find(delim2));
        while(negspaces[end]==1)
            end=min(command.find(delim1,end+1),command.find(delim2,end+1));

        if(start!=end)//verifying that no white spaces are there in start of command
        {
            if(end==-1)
                end=command.size();
            cmd.push_back(command.substr(start,end-start));
            start=end;
        }
        while(command[start]==delim1 || command[start]==delim2)//skipping the white spaces
            start++;

        while(start<(int)command.size())
        {
            if(command[start]==delim3 || command[start]==delim4)//managing input for single and double quotes
            {
                if(command[start]==delim3)//adds content in single quotes as string
                {
                    end=command.find(delim3,start+1);
                    if(command[end+1]=='/')
                    {
                        end=min(command.find(delim1,end+1),command.find(delim2,end+1));
                        while(negspaces[end]==1) //changing the end counter if its position has 1 in negspaces array
                        {
                            end=min(command.find(delim1,end+1),command.find(delim2,end+1));
                            if(end==-1)
                                break;
                        }
                    }

                    if(end==-1)
                        end=command.size()-1;
                    cmd.push_back(command.substr(start,end-start+1));
                    start=end+1;
                }
                if(command[start]==delim4)//adds content in double quotes as string
                {
                    end=command.find(delim4,start+1);
                    if(command[end+1]=='/')
                    {
                        end=min(command.find(delim1,end+1),command.find(delim2,end+1));
                        while(negspaces[end]==1)
                        {
                            end=min(command.find(delim1,end+1),command.find(delim2,end+1));
                            if(end==-1)
                                break;
                        }
                    }

                    if(end==-1)
                        end=command.size()-1;
                    cmd.push_back(command.substr(start,end-start+1));
                    start=end+1;
                }
            }
            else
            {
                end=min(command.find(delim1,start+1),command.find(delim2,start+1));
                while(negspaces[end]==1)
                {
                    end=min(command.find(delim1,end+1),command.find(delim2,end+1));
                    if(end==-1)
                        break;
                }

                if(end==-1)
                {
                    end=command.size();
                    cmd.push_back(command.substr(start,end-start));
                    start=end;
                }
                else
                {
                    cmd.push_back(command.substr(start,end-start)); // pushing individual elements
                    start=end;
                }

            }

            while(command[start]==delim1 || command[start]==delim2)//skipping the white spaces
                start++;
            end=start;

        }
    }
    return(cmd); // returning the cmd vector
}

//FUNCTIONALITIES
int Func_Exec(string command,vector<string> cmd,string history_path, User_Variables* A)
{
    char buf[1000];
    char delim3=(char)39;
    char delim4=(char)34;

    //Help command
    if(cmd[0]=="help")
    {
        getcwd(buf, sizeof(buf));
        string current_loc = buf;
        const char* destination=history_path.c_str();
        chdir(destination);
        string line;
        ifstream myfile("help.txt");
        if (myfile.is_open())
        {
            while ( getline (myfile,line) )
            {
                cout << line << '\n';
            }
            myfile.close();
        }
        else
            cout << "Unable to open file";

        chdir(current_loc.c_str());

        return 0;
    }

    //Exiting the shell
    if((cmd.size()==1 || cmd.size()==2) && cmd[0]=="exit")
    {
        return -1;
    }

    //SETTING THE VARIABLE
    if((int)command.find('=')>0 && (int)command.find('=')<(int)command.size()-1 && cmd.size()==3)
    {
        A->SetVar(cmd);
    }

    //CALLING THE VARIABLE
    if((int)command.find('$')!=-1)
    {
        command=A->CallVar(command);
        cmd.clear();
        cmd=Parse_Input(command);
    }

    //UNSETTING THE VARIABLE
    if((cmd.size()==2 && cmd[0]=="unset"))
    {
        A->RemoveVar(cmd[1]);
    }

    //ECHO FUNCTION
    if(cmd[0]=="echo")
    {
        for(int i=1;i<(int)cmd.size();i++)
        {
            string partial_output = cmd[i];
            if(cmd[i][0]==delim3 || cmd[i][0]==delim4)
            {
                partial_output = cmd[i].substr(1,(int)cmd[i].size()-2);
            }

            cout << partial_output << " ";
        }
        cout << endl;
        return 0;
    }

    int arg1size=cmd[1].size();

    //CHANGING THE CURRENT WORKING DIRECTORY
    const char* chdirpath;//Declaration of final changed path/Argument of chdir()
    if(cmd[0]=="cd" && cmd.size()==2)
    {
        if(cmd[1]=="~")//Navigating to home directory
        {
            string homedir=getenv("HOME");
            chdirpath=homedir.c_str();
            chdir(chdirpath);
            return 0;
        }

        if(cmd[1]=="/")//Navigating to root directory
        {
            while((string)buf!="/")
            {
                //cout << "hello root directory process\n";
                chdir("..");
                getcwd(buf, sizeof(buf));
            }
            return 0;
        }

        if((int)cmd[1].find(' ')!=-1)//Managing the case for directories containing space in their name
        {
            string changedir;
            for(int j=0;j<arg1size;j++)
            {
                if(cmd[1][j]!=delim3 && cmd[1][j]!=delim4)// if the tokens contain quotes at start and end then remove them
                    changedir+=cmd[1][j];
            }
            chdirpath=changedir.c_str();
        }
        else
            chdirpath = cmd[1].c_str();
        chdir(chdirpath);

        return 0;
    }

    if(cmd[0]=="cd" && cmd.size()==1)//Navigating to home directory
    {
        string homedir=getenv("HOME");
        chdirpath=homedir.c_str();
        chdir(chdirpath);
        return 0;
    }

    int pipefd_1[2]; //creating file descriptors for pipe
    pipe(pipefd_1); //Creating pipe for communication between child and parent process
    pid_t pid;

    //EXECUTING THE EXTERNAL COMMANDS BY CALLING THE SCRIPTS
    {
        vector<string> paths=ListPaths();
        pid=fork(); //forking and creating child and parent

        if(pid==0)
        {

            close(pipefd_1[0]);    // close reading end in the child

            dup2(pipefd_1[1], STDOUT_FILENO);  // send stdout to the pipe
            dup2(pipefd_1[1], STDERR_FILENO);  // send stderr to the pipe

            close(pipefd_1[1]);    // this descriptor is no longer needed

            char *exe_args[(int)cmd.size()+1]; //creating the argument of execv in proper data types
            exe_args[(int)cmd.size()]={NULL};

            for(int a=1;a<(int)cmd.size();a++)
            {
                exe_args[a]=const_cast<char*>(cmd[a].c_str());
            }

            for(int i=0;i<(int)paths.size();i++)//searching executable in different path locations
            {
                if(cmd[0]=="echo")//preventing running echo executable
                    break;
                string external_commands=paths[i] + "/" + cmd[0];// making absolute path by merging path and command executable name
                exe_args[0]={const_cast<char*>(external_commands.c_str())};
                execv(exe_args[0],exe_args);
                perror("execv"); //checking error for execv

                exit(0);//exitting child
            }

        }
        else if(pid>0)
        {
            wait(NULL); //waiting for child process to get over
            char readbuf[1024];
            int bytes_read = 0;
            close(pipefd_1[1]); // closing the write end, as their is nothing to write

            while ((bytes_read = read(pipefd_1[0], readbuf, sizeof(readbuf)-1 > 0))) // reading the contents form pipe
            {
                readbuf[bytes_read]='\0';
                cout << readbuf; // printing the output
            }
            close(pipefd_1[0]); // closing the reading end

        }
        else // error if fork is not executed
        {
            cout << "Could not succesfully create a child process\n";
        }

    }
    return 0;
}

//REDIRECTING INPUT
void Redirect_Input(string command,vector<string>cmd,User_Variables* A,string file_name2)
{
    char *exe_args[(int)cmd.size()+1];

    for(int a=0;a<(int)cmd.size();a++)
    {
        exe_args[a]=const_cast<char*>(cmd[a].c_str());
    }

    exe_args[(int)cmd.size()]={NULL};

    int pipefd_3[2]; // Creating file descriptors
    pipe(pipefd_3); // Creating pipe between child and parent
    pid_t pid;

    //EXECUTING THE EXTERNAL COMMANDS BY CALLING THE SCRIPTS

    pid=fork(); //Froking process

    if(pid==0) // if child is succesfully formed
    {
        int inp= open(file_name2.c_str(), O_RDONLY ); // open the file and read only
        dup2(inp,0);
        close(inp); // stop reading and close file

        close(pipefd_3[0]);    // close reading end in the child

        dup2(pipefd_3[1], STDOUT_FILENO);  // send stdout to the pipe
        dup2(pipefd_3[1], STDERR_FILENO);  // send stderr to the pipe

        close(pipefd_3[1]);    // this descriptor is no longer needed*/

        execvp(exe_args[0],exe_args);
        perror("execvp");

        exit(0);

    }
    else if(pid>0) // parent
    {
        wait(NULL); // waiting for child to complete
        char readbuf[1024];
        int bytes_read = 0;
        close(pipefd_3[1]); //closing the write end

        while ((bytes_read = read(pipefd_3[0], readbuf, sizeof(readbuf)-1 > 0))) // reading output from read end
        {
            readbuf[bytes_read]='\0';
            cout << readbuf;
        }
        close(pipefd_3[0]); //closing the read end

    }
    else    //error in forking child
    {
        cout << "Could not succesfully create a child process\n";
    }

}

//REDIRECTING OUTPUT
void Redirect_Output(string command,vector<string>cmd,string history_path,User_Variables* A,string file_name1)
{
    ofstream out(file_name1.c_str());
    streambuf *coutbuf = cout.rdbuf(); //saving old buf
    cout.rdbuf(out.rdbuf()); //redirecting cout to file_name1!

    auto it = find(cmd.begin(),cmd.end(), "y8w3b64f1s8");

    if(it!=cmd.end()) // checking if input redirection is also present
    {
        string file_name2=*(it + 1);
        cmd.erase(it);
        cmd.erase(it);
        Redirect_Input(command,cmd,A,file_name2);
    }
    else    // function normally without input redirection
        Func_Exec(command,cmd,history_path,A);

    cout.rdbuf(coutbuf);    // restoring the output buf to screen
}

//REDIRECTING OUTPUT IN APPEND MODE
void Append_Output(string command,vector<string>cmd,string history_path,User_Variables* A,string file_name3)
{
    streambuf *coutbuf = cout.rdbuf();
    fstream file; //stream object for appending the file
    file.open(file_name3.c_str(),ios_base::app);
    cout.rdbuf(file.rdbuf());

    auto it = find(cmd.begin(),cmd.end(), "y8w3b64f1s8");   // checking if input redirection is also present

    if(it!=cmd.end())
    {
        string file_name2=*(it + 1);
        cmd.erase(it);
        cmd.erase(it);

        Redirect_Input(command,cmd,A,file_name2);
    }
    else    // function normally without input redirection
        Func_Exec(command,cmd,history_path,A);

    cout.rdbuf(coutbuf);    // restoring the output buf to screen
    file.close();   //closing the file
}

//RUNNING FIRST COMMAND OF PIPING
void Run_Source(int pipefd_2[],string command1, vector<string> cmd1, User_Variables* A)	// run the first part of the pipeline, cmd1
{
	int pid;

    char *exe_args[(int)cmd1.size()+1];

    for(int a=0;a<(int)cmd1.size();a++)
    {
        exe_args[a]=const_cast<char*>(cmd1[a].c_str());
    }

    exe_args[(int)cmd1.size()]={NULL};

	switch (pid = fork())
	{
        case 0:             // child
        {
            cout << "going to execute first function\n";
            dup2(pipefd_2[1], 1);	// this end of the pipe becomes the standard output
            close(pipefd_2[0]); 		// this process don't need the other end
            execvp(exe_args[0],exe_args);	// run the command
        }

        default: // parent does nothing
            break;

        case -1:
            perror("fork"); // error checking in fork
            exit(1);
	}
}

//RUNNING SECOND COMMAND OF PIPING
void Run_Dest(int pipefd_2[],string command2,vector<string> cmd2, User_Variables* A)	// run the second part of the pipeline, cmd2
{
	int pid;

    char *exe_args[(int)cmd2.size()+1];

    for(int a=0;a<(int)cmd2.size();a++)
    {
        exe_args[a]=const_cast<char*>(cmd2[a].c_str());
    }

    exe_args[(int)cmd2.size()]={NULL};

	switch (pid = fork()) {

	case 0: /* child */
		{
            dup2(pipefd_2[0], 0);	// this end of the pipe becomes the standard input
            close(pipefd_2[1]);        // this process doesn't need the other end
            execvp(exe_args[0],exe_args);
            perror("execvp");   // it failed!
            exit(0);
        }
	default: // parent does nothing
		break;

	case -1:    // error checking in forking
		perror("fork");
		exit(1);
	}
}

//PIPING COMMAND
void Piping(string command,vector<string> cmd,User_Variables* A)
{
    auto vecpipe_loc=find(cmd.begin()+1,cmd.end()-1,"|");
    int strpipe_loc=command.find("|");
    vector<string> cmd1;    // creating vector of strings for first command before pipe
    vector<string> cmd2;    // creating vector of strings for second command after pipe
    string command1=command.substr(0,strpipe_loc+1);    // making corresponding first command string
    string command2=command.substr(strpipe_loc+1);  // making corresponding second command string

    for(auto it=cmd.begin();it!=vecpipe_loc;it++)   // splitting first command for piping
    {
        cmd1.push_back(*it);
    }
    for(auto it=vecpipe_loc+1;it!=cmd.end();it++)   // splitting second command for piping
    {
        cmd2.push_back(*it);
    }

    int pipefd_2[2];    // creating file descriptors
    int pid, status;
    pipe(pipefd_2);     // creating a pipe for interprocess communication

	Run_Source(pipefd_2,command1,cmd1,A);   // running first command
	Run_Dest(pipefd_2,command2,cmd2,A);     // running second command
	close(pipefd_2[0]);     // closing read end of pipe
	close(pipefd_2[1]);     // closing write end of pipe

    while((pid = wait(&status)) != -1)
    {
        //waiting for children to finish their process

    }
    return ;

}

int main()
{
    char buf[1000];
    getcwd(buf, sizeof(buf));
    string history_path=buf;
    string history_path1= history_path + "/" + "history.txt";   // making history path for history file
    class User_Variables var;   //making class User_Variables for dealing with user variables
    while(1)
    {

        string command;

        //Its printing current directory
        if (getcwd(buf, sizeof(buf)) != NULL)
            cout << buf << " : ";
        else
        perror("Error:");

        //Taking input command
        getline(cin,command);

        //STORING EACH COMMAND IN HISTORY FILE
        {
            //storing the input command in history.txt file
            ofstream outfile;
            outfile.open(history_path1, ios_base::app);//append mode
            outfile << command << endl;
            outfile.close();
        }

        //as my code had a problem of recieving <,>,>>,| as individual vector elements
        //i replace them initially for parsing and then restore it back to normal
        //i have assigned 13 word token to each of them

        int a=command.find('>'); // finding locations of >,>>
        int c=-1;
        if(command[a+1]=='>')
        {
            c=a;
            a=-1;
        }

        if(a!=-1) // token substitution for '>'
        {
            command.erase(a,1);
            command.insert(a," o1p9k19r2f4 ");
        }
        if(c!=-1) // token substitution for ">>"
        {
            command.erase(c,2);
            command.insert(c," g5o2z07k4q3 ");
        }

        int b=command.find('<');
        if(b!=-1)   // token substitution for '<'
        {
            command.erase(b,1);
            command.insert(b," y8w3b64f1s8 ");
        }

        int d=command.find('|');
        if(d!=-1)   // token substitution for '|'
        {
            command.erase(d,1);
            command.insert(d," f6n8d40l9x4 ");
        }

        command=var.CallVar(command); // Replacing if variables are called

        vector<string> cmd = Parse_Input(command); // recieving the parsed command vector

        //ALIAS FOR HISTORY COMMAND
        if(cmd[0]=="history" && (cmd.size()==1 || cmd.size()==2))
        {
            command = "cat history.txt";
            cmd.clear();
            cmd.push_back("cat");
            cmd.push_back(history_path1);
        }

        // Replacing back the tokens in vector strings as their original identity
        {
            string passoutput=" o1p9k19r2f4 ";
            string passinput=" y8w3b64f1s8 ";
            string passappend=" g5o2z07k4q3 ";
            string passpipe=" f6n8d40l9x4 ";
            for(int j=0;j<(int)command.size()-(int)passoutput.size()+1;j++)
            {
                string matching1 = command.substr(j,passoutput.size());
                if(passoutput==matching1)
                {
                    command.erase(j,passoutput.size());
                    command.insert(j,">");
                }
                string matching2 = command.substr(j,passinput.size());
                if(passinput==matching2)
                {
                    command.erase(j,passinput.size());
                    command.insert(j,"<");
                }
                string matching3 = command.substr(j,passappend.size());
                if(passappend==matching3)
                {
                    command.erase(j,passappend.size());
                    command.insert(j,">>");
                }
                string matching4 = command.substr(j,passpipe.size());
                if(passpipe==matching4)
                {
                    command.erase(j,passpipe.size());
                    command.insert(j,"|");
                }

            }

            auto it=find(cmd.begin(),cmd.end(),"f6n8d40l9x4");
            if(it!=cmd.end())
            {
                cmd.at(it-cmd.begin())="|";
            }
        }


        if(a!=-1)   // Entering output redirection mode
        {
            string file_name1=cmd[cmd.size()-1]; // identifying the file name
            cmd.pop_back();
            cmd.pop_back();

            Redirect_Output(command,cmd,history_path,&var,file_name1); // calling the output redirection function
        }
        else if(c!=-1)  // Entering output redirection in append mode
        {
            string file_name3=cmd[cmd.size()-1];    // identifying the file name
            cmd.pop_back();
            cmd.pop_back();

            Append_Output(command,cmd,history_path,&var,file_name3); // calling the append output function
        }
        else if(b!=-1)  // Entering input redirection mode
        {
            auto index_it = find(cmd.begin(),cmd.end(), "y8w3b64f1s8");
            string file_name2=*(index_it + 1);  // identifying the file name
            cmd.erase(index_it);
            cmd.erase(index_it);

            Redirect_Input(command,cmd,&var,file_name2);    // calling the input redirection function
        }
        else if(d!=-1)  // Entering piping mode
        {
            Piping(command,cmd,&var);   // calling the piping function
        }
        else    // if none of the above follows then running the functionalities plainly
        {
            if(Func_Exec(command,cmd,history_path,&var)==-1)
            {
                break; // breaking from the while loop
            }
        }
        cout << endl;
    }

    cout << "\n\nThankyou for using my minishell.\n";
    cout << "Hope you liked it !!!\n";
    return 0;
}
