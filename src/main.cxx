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
#include <string>

#include "../Config.h"

#if defined(HAVE_READLINE)
#	include <readline/readline.h>
#	include <readline/history.h>
#endif

using namespace std;

#define READLINE

static string templateHeader = "#include <iostream>";
static string templateEnd = "\treturn 0;\n}";

class CLI
{
	private:
		bool running_;
		
	public:
		CLI();
		void loop();
		
		inline operator const bool() const
		{
			return running_;
		}
		
};

CLI::CLI() :
	running_(false)
{
	
}

void CLI::loop()
{
#if defined(HAVE_READLINE)
 // getting the current user 'n path
        snprintf(shell_prompt, sizeof(shell_prompt), "%s:%s $ ", getenv("USER"), getcwd(NULL, 1024));
        // inputing...
        input = readline(shell_prompt);
        // eof
        if (!input)
            break;
        // path autocompletion when tabulation hit
        rl_bind_key('\t', rl_complete);
        // adding the previous input into history
        add_history(input);
#else
	string line;
	cout << ">> ";
	getline(cin, line);
#endif 
	cout << "Input(" << line << ")" << endl;
}

int main(int argc, char **argv)
{
	CLI cli;
	while (cli)
	{
		cli.loop();
	}
	return 0;
}

