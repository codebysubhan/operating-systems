# Operating Systems
## Assignment 1 - Creating a UNIX Shell

### Overview
This repository contains the source code for a custom UNIX shell, developed by Subhan Ali as part of an Operating Systems assignment.

#### gpt part :>
The shell is built in six versions, with each version adding new functionalities, culminating in a feature-rich command-line interface that supports external commands, I/O redirection, background processing, command history, built-in commands, and variable management.

---

### Version Descriptions

#### Version 1: Basic Shell
- Displays a prompt and accepts user input for external commands.
- Uses `fork` and `exec` to execute commands.
- Waits for each command to complete before showing the prompt again.
- Exits on `CTRL+D`.

#### Version 2: I/O Redirection
- Added support for input (`<`) and output (`>`) redirection.
- Allows commands like `mycmd < infile > outfile` to redirect `stdin` and `stdout`.

#### Version 3: Background Processing
- Supports running processes in the background using `&` at the end of a command.
- Returns the prompt immediately after launching background processes.

#### Version 4: Command History
- Maintains a history of the last 10 commands.
- Supports repeating commands using `!number` (e.g., `!1` for the first command) and `!-1` for the last command.

#### Version 5: Built-in Commands
- Implements built-in commands including:
  - `cd`: Change the working directory.
  - `exit`: Terminate the shell.
  - `jobs`: List currently running background jobs.
  - `kill`: Terminate a specified background job.
  - `help`: Display a list of available built-in commands.

#### Version 6: Variable Management (BONUS)
- Adds support for local and environment variables.
- Allows assigning values to variables and retrieving their values.
- Implements a simple storage system to distinguish between local and environment variables.

---

### Features
- **External Command Execution**: Supports standard UNIX commands by searching paths and executing binaries.
- **I/O Redirection**: Enables redirection of `stdin`, `stdout`, and append mode for output.
- **Background Process Handling**: Run processes in the background without waiting for completion.
- **Command History**: Access previous commands for quick execution.
- **Built-in Commands**: Includes custom commands such as `cd`, `exit`, `jobs`, `kill`, and `help`.
- **Variable Management**: Handles user-defined and environment variables, allowing dynamic variable assignment.

---

### Installation
1. Clone this repository to your local machine.
    ```bash
    git clone https://github.com/codebysubhan/operating-systems.git
    ```
2. Navigate to the directory.
    ```bash
    cd operating-systems
    ```
3. Compile the shell program.
    ```bash
    gcc shell_v1.c -o shell_v1
    ```

---

### Usage
Run the shell by executing the compiled binary:
```bash
./shell_v1
