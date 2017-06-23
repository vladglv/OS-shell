# OS-shell

A simple shell for Linux environment that supports built-in and external commands.

List of build-in commands:
1. history - keeps track of recently executed commands. The history is not preserved between runs.
2. cd
3. pwd
4. exit
5. jobs
6. fg

List of features:
1. Output redirection via ">".
2. Command piping via "|". Multiple piping is supported.
3. Background execution of jobs via "&".
4. All of the items above can be used together in the following order: piping then output redirection then background execution
