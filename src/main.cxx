/**
 *  Execute C/C++ statements in an interactive shell
 *  Copyright (C) 2012  Michał Papierski
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include <cstdlib>
#include <errno.h>

#include "../Config.h"

#if defined(READLINE_FOUND)
#	include <readline/readline.h>
#	include <readline/history.h>
#endif

using namespace std;

/* List of #includes */
static list<string> includes;
/* Link libraries */
static list<string> libraries;
/* Command list */
static vector<string> commands;
/* Compiler to use. */
static string compiler;

/* Source header */
static string templateHeader = "void cshell_stmt(int argc, char* argv[])\n"
	"{\n";
/* Source footer */
static string templateFooter = "}\n"
	"int\n"
	"main(int argc, char* argv[])\n"
	"{\n"
	"\tcshell_stmt(argc, argv);\n"
	"\treturn 0;\n"
	"}";

/**
 * Command line interpreter wrapper
 */
class CLI
{
	private:
		bool running_;
		
	public:
		/**
 		 * Default constructor
 		 * Set running_ to true.
 		 */
		CLI();

		/**
 		 * Read line
 		 * @return Line which we have to do something 
 		 */
		string loop();
		
		/**
 		 * The "safe bool" idiom
 		 * @return Value indicating if we have to run anymore
 		 */
		inline operator const bool() const
		{
			return running_;
		}
		
};

CLI::CLI() :
	running_(true)
{
	
}

string CLI::loop()
{
	static const char prompt[] = ">>";
#if defined(READLINE_FOUND)
	char* input = readline(prompt);
	if (!input)
	{
		running_ = false;
		return "";
	}
	/* TODO: Auto complete */
	// rl_bind_key('\t', rl_complete);
	add_history(input);
	string line(input);
#else
	if (cin.eof())
	{
		running_ = false;
		return "";
	}

	string line;
	cout << ">> ";
	getline(cin, line);
#endif 
	return input;
}

/**
 * Generate source code string.
 * @return Source code ready to compile
 */
static string sourceCode()
{
	/* 1. Write includes */
	ostringstream out;
	for (list<string>::const_iterator it(includes.begin()),
			end(includes.end()); it != end; ++it)
	{
		out << "#include <" << *it << ">" << endl;
	}

	/* 2. Write header (begin of a function) */
	out << templateHeader << endl;

	for (vector<string>::const_iterator it(commands.begin()),
			end(commands.end()); it != end; ++it)
	{
		out << *it << endl;
	}
	
	/* 3. Write footer (end of a function and a main) */
	out << templateFooter << endl;

	/* 4. */
	return out.str();
}

/**
 * Execute command in shell, if error happens throw
 * string with detailed error from the OS.
 * @param commandLine Command line
 */
static void systemOrThrow(const std::string& commandLine)
{
	int result = system(commandLine.c_str());
	if (result == -1)
	{
		/* Failed to do system call. */
		int errcodev = errno;
		char* strerr = strerror(errcodev);
		string strobj(strerr);
		free(strerr);
		throw strobj;
	}
	else if (result > 0)
	{
		/* Child process probably failed and we
 		 * do not know about exit code. */
		throw string("Unknown error.");
	}
}

/**
 * Compile and execute source code passed by string
 * @param inputFile Source code
 * @return True on success
 */
static bool execute(const std::string& inputFile)
{
	/* Produce a command line */
	ostringstream cmdLine;

	cmdLine << compiler << ' ';

	for (list<string>::const_iterator it(libraries.begin()),
		end(libraries.end()); it != end; ++it)
	{
		cmdLine << "-l" << *it << ' ';
	}

	/* Write source code to a temporary file */
	char* temp = strdup("/tmp/cshellXXXXXX");
	close(mkstemp(temp));
	string tempC = string(temp) + ".c";
	
	{
		ofstream fout(tempC.c_str());
		fout << inputFile << endl;
	}

	cmdLine << tempC << " -o " << temp;

	/* Compile */
	try
	{
		/* This behavior is not portable,
 		 * need to return exit code of child process
 		 * rather than result value from system call. */
		systemOrThrow(cmdLine.str());
	} catch (string e)
	{
		cout << "Compile error: " << e << endl;
		return false;
	}

	/* Run the actual output */
	try
	{
		/* Same as above */
 		systemOrThrow(temp);
	} catch (string e)
	{
		cout << "Run error: " << e << endl;
		return false;
	}

	return true;
}

int main(int argc, char **argv)
{
	cout << argv[0] << " Copyright (C) 2012 Michał Papierski" << endl
		<< "This program comes with ABSOLUTELY NO WARRANTY" << endl
		<< "This is free software, and you are welcome to redistribute it"
		<< "under certain condition." << endl << endl;
	/* Detect compiler */
	string comp = (!getenv("COMPILER") ? "" : getenv("COMPILER"));
	if ((comp == "gcc") ||
			(comp == "g++"))
	{
		compiler = comp;
		cout << "Using \"" << compiler << "\" compiler." << endl;
	}
	else
	{
		if (!comp.empty())
			cout << "Provided compiler which will be unsafe: "
				<< comp << endl;
		compiler = "gcc";
	}
	CLI cli;

	while (cli)
	{
		const string& inputLine = cli.loop();
		if (inputLine.empty())
			continue;
		if (inputLine[0] == '+')
		{
			istringstream iss(inputLine);
			vector<string> tokens;
			do
			{
				string sub;
				iss >> sub;
				tokens.push_back(sub);
			} while (iss);

			if (tokens.size() < 2)
				continue;

			if (tokens[0] == "+include")
			{
				includes.push_back(tokens[1]);
				cout << "New include: " << tokens[1] << endl;
			}
			else if (tokens[0] == "+library")
			{
				libraries.push_back("-l" + tokens[1]);
				cout << "New link library: " << tokens[1] << endl;
			}
			else
			{
				cout << "Unknown command: " << tokens[0] << endl;
			}
			continue;
		}
		commands.push_back(inputLine);
		const std::string& source = sourceCode();
		if (!execute(source))
		{
			cout << "Failed to compile." << endl;
			commands.pop_back();
		}
	}
	return 0;
}

