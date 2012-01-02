/**
 *  This file is part of cshell.
 *
 *  cshell is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  cshell is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with cshell.  If not, see <http://www.gnu.org/licenses/>.
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

static list<string> includes;
static list<string> libraries;
static list<string> commands;

static string templateHeader = "void cshell_stmt(int argc, char* argv[])\n"
	"{\n";
static string templateFooter = "}\n"
	"int\n"
	"main(int argc, char* argv[])\n"
	"{\n"
	"\tcshell_stmt(argc, argv);\n"
	"\treturn 0;\n"
	"}";

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

static string sourceCode()
{
	ostringstream out;
	for (list<string>::const_iterator it(includes.begin()),
			end(includes.end()); it != end; ++it)
	{
		out << "#include <" << *it << ">" << endl;
	}

	out << templateHeader << endl;

	for (list<string>::const_iterator it(commands.begin()),
			end(commands.end()); it != end; ++it)
	{
		out << *it << endl;
	}
	out << templateFooter << endl;

	return out.str();
}

static bool execute(const std::string& inputFile)
{
	vector<char*> params;
	params.resize(libraries.size());
	int i = 0;
	for (list<string>::const_iterator it(libraries.begin()),
		end(libraries.end()); it != end; ++it)
	{
		/*
 		 * Remember not to write to params,
 		 * because weird things might happen
 		 */
		params[i++] = const_cast<char*>(it->c_str());
	}
	/* Write source code to a temporary file */
	char* temp = strdup("/tmp/cshellXXXXXX");
	close(mkstemp(temp));
	string tempC = string(temp) + ".c";
	
	{
		ofstream fout(tempC.c_str());
		fout << inputFile << endl;
	}

	/* Complete parameters... */
		string compiler = "gcc";
		params.insert(params.begin(), const_cast<char*>(compiler.c_str()));
		string outputParam = "-o";
		params.push_back(const_cast<char*>(tempC.c_str()));
		params.push_back(const_cast<char*>(outputParam.c_str()));
		params.push_back(temp);
		params.push_back(NULL);
	for (vector<char*>::iterator it(params.begin()), end(params.end()); it != end; ++it)
	{
		cout << "\"" << *it << "\" ";
	}
	cout << endl;
	
	/* Fork from this point */
	if (fork() == 0)
	{
		int resultCode = execvp("/usr/bin/gcc", &(params[0]));
		if (resultCode == -1)
		{
			cout << "ERROR: #" << errno << " (" << strerror(errno) << ")" << endl;
		}
		return -1;
	}
	cout << endl;
	return true;
}

int main(int argc, char **argv)
{
	CLI cli;
	includes.push_back("stdlib.h");
	includes.push_back("stdio.h");

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
		}
	}
	return 0;
}

