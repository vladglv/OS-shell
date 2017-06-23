/***
 *
 * Vlad Glv
 *
 * ECSE-427 (Operating Systems)
 *
 * Assignment 1
 *
 ***/

// - System specific
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
// - Standard C++
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// - Macro to inhibit unused variable warnings.
#define UNUSED(expr)  \
    do {              \
        (void)(expr); \
    } while(0)

// - Define a command class
class command {
  public:
    typedef std::vector<std::string> string_vec;

    std::size_t number;               //!< The number of the command in history
    std::vector<string_vec> params;   //!< The parameter list
    bool background;                  //!< The status of the process
    bool nil;                         //!< The job status
    pid_t pid;                        //!< Process ID if > 0
    static const pid_t default_ = -1; //!< Default value of a nil process
    bool output_redir;                //!< Output redirection flag
    std::string output_file;          //!< File where to write stdout
    bool pipe;                        //!< Pipe flag

    command()
        : number(1)
        , background(false)
        , nil(true)
        , pid(default_)
        , output_redir(false)
        , pipe(false) {
    }

    /**
     * @brief Resets the pid values to its default value
     */
    void reset_pid() {
        this->pid = this->default_;
    }

    /**
     * @brief Checks if actual parameters are present
     * @return True if parameters are empty and false otherwise
     */
    bool empty_params() {
        std::string tmp;
        for(const auto& l : this->params) {
            for(const auto& e : l)
                tmp += e;
        }

        return tmp.empty();
    }

    /**
     * @brief Overload of << operator for convenient printing of the command
     */
    friend std::ostream& operator<<(std::ostream& stream, const command& cmd) {
        stream << "<Command #\t: " << cmd.number << std::endl;

        command::string_vec::size_type i = 0;
        for(const auto& l : cmd.params) {
            stream << " Arguments " << (++i) << "\t: ";
            for(const auto& e : l)
                stream << e << " ";
            stream << std::endl;
        }

        stream << " Background\t: " << cmd.background << std::endl;
        stream << " Piped\t\t: " << cmd.pipe << std::endl;
        stream << " Output redir\t: " << cmd.output_redir << ", to '"
               << cmd.output_file << "'\n";
        stream << " Last known PID\t: " << cmd.pid << ">" << std::endl;

        return stream;
    }
};

// - Define a state of the shell
class state {
  public:
    std::vector<command> cmds; //!< The list of commands
    std::vector<command> jobs; //!< The list of jobs
    std::string prompt;        //!< The prompt message of the shell
    bool run;                  //!< Continue running the shell
    std::size_t cmd_count;     //!< Command counter
    std::string delim;         //!< Delimiter for parsing 'params' of a command

    state()
        : prompt(">> ")
        , run(true)
        , cmd_count(1)
        , delim(" \t\n") {
    }

    /**
     * @brief Adds a command 'cmd' to history. If a command is a jobs then it is
     * added to a job list.
     * @param cmd Command to be added
     */
    void add_command(command& cmd) {
        cmd.number = this->cmd_count++;
        this->cmds.emplace_back(cmd);

        if(cmd.background)
            this->jobs.emplace_back(cmd);
    }

    /**
     * @brief Update the job list with currently active jobs
     */
    void update_jobs() {
        // - Copy and clear jobs
        std::vector<command> tmp(jobs);
        jobs.clear();

        // - Check for running jobs
        for(auto& e : tmp) {
            auto r = waitpid(e.pid, nullptr, WNOHANG);
            if(r == 0)
                jobs.emplace_back(e);
        }
    }
};

/**
 * @brief Tokenizes a string 'in' using a delimiter 'delim'
 * @param in Input string
 * @param delim Delimiter used to separate tokens
 * @return A vector of string tokens
 */
std::vector<std::string> tokenize(std::string in, std::string delim) {
    char* token;
    char* str_view = &in[0];
    std::vector<std::string> tokens;

    while((token = strsep(&str_view, delim.c_str())) != nullptr) {
        for(std::size_t i = 0; i < std::strlen(token); i++)
            if(token[i] <= 32)
                token[i] = '\0';

        if(std::strlen(token) > 0)
            tokens.emplace_back(token);
    }

    return tokens;
}

/**
 * @brief Obtains a command from std::cin
 * @param st State of the shell
 * @return Obtained command if any
 */
command getcmd(state& st) {
    std::string cmd_line;
    std::string::size_type pos;
    command cmd;

    // - Display the prompt and fetch the input
    std::cout << st.prompt;
    std::getline(std::cin, cmd_line);

    // - Handle CTRL + D
    if(std::cin.eof()) {
        st.run = false;

        return cmd;
    }

    // - Handle empty input
    if(cmd_line.size() <= 0) {
        return cmd;
    }

    // - Implementation of built-in commands

    // - history
    if(cmd_line == "history") {
        if(st.cmds.empty())
            std::cout << "no command history" << std::endl;

        std::size_t i = 1;
        for(auto& e : st.cmds)
            std::cout << "[" << (i++) << "] \n" << e << std::endl;

        return cmd;
    }

    // - jobs
    else if(cmd_line == "jobs") {
        st.update_jobs();

        if(st.jobs.empty())
            std::cout << "no jobs are present" << std::endl;

        std::size_t i = 1;
        for(auto& e : st.jobs)
            std::cout << "[" << (i++) << "] \n" << e << std::endl;

        return cmd;
    }

    // - !exec command
    else if((cmd_line.size() > 1) && (cmd_line[0] == '!')) {
        try {
            // - Obtain the number of the command to run
            auto cmd_idx_str = cmd_line.substr(1);
            int cmd_idx = std::stoi(cmd_idx_str);
            if(cmd_idx < 1)
                throw std::invalid_argument("cmd_idx_str");

            if(static_cast<std::size_t>(cmd_idx) > st.cmds.size()) {
                std::cout << "no command found in history\n";
                return cmd;
            }

            // - Return the command with a default pid.
            auto ccmd = st.cmds[cmd_idx - 1];
            ccmd.reset_pid();

            return ccmd;
        } catch(...) {
            std::cout << "Incorrect command number provided\n";
        }

        return cmd;
    }

    // - fg
    else if((cmd_line.size() > 1) && (cmd_line.substr(0, 2) == "fg")) {
        if(cmd_line.size() >= 3) {
            try {
                st.update_jobs();

                // - Obtain the number of the jobs to bring to foreground
                auto job_idx_str = cmd_line.substr(3);
                int job_idx = std::stoi(job_idx_str);
                if(job_idx < 1)
                    throw std::invalid_argument("job_idx_str");

                if(static_cast<std::size_t>(job_idx) > st.jobs.size()) {
                    std::cout << "no jobs found\n";
                    return cmd;
                }

                return st.jobs[job_idx - 1];
            } catch(...) {
                std::cout << "Incorrect command number provided\n";
            }
        }

        return cmd;
    }

    // - cd
    else if((cmd_line.size() > 1) && (cmd_line.substr(0, 2) == "cd")) {
        if(cmd_line.size() >= 3) {

            // - Obtain the directory string
            auto dir = cmd_line.substr(3);
            auto ret = chdir(dir.c_str());
            if(ret != 0)
                std::cout << "invalid argument to cd\n";
        }

        return cmd;
    }

    // - pwd
    else if(cmd_line == "pwd") {
        std::vector<char> dir(PATH_MAX);

        auto r = getcwd(dir.data(), dir.size());
        if(r)
            std::cout << dir.data() << std::endl;

        return cmd;
    }

    // - exit
    else if(cmd_line == "exit") {
        st.run = false;
        return cmd;
    }

    // - Check for background status
    if((pos = cmd_line.find_last_of('&')) != cmd_line.npos) {
        cmd.background = true;
        cmd_line[pos] = ' ';
    }

    // - Check for the output redirection
    if((pos = cmd_line.find_last_of('>')) != cmd_line.npos) {
        cmd.output_redir = true;
        cmd_line[pos] = ' ';
        cmd.output_file = tokenize(cmd_line.substr(pos), st.delim)[0];
        cmd_line = cmd_line.substr(0, pos);
    }

    // - Check for the piping
    auto pipes = std::count(cmd_line.begin(), cmd_line.end(), '|');
    if(pipes > 0) {
        cmd.pipe = true;

        for(auto i = 0; i < pipes + 1; i++) {
            if((pos = cmd_line.find('|')) != cmd_line.npos) {
                cmd_line[pos] = ' ';
                auto e = cmd_line.substr(0, pos);
                cmd.params.emplace_back(tokenize(e, st.delim));
                cmd_line = cmd_line.substr(pos);
            } else
                cmd.params.emplace_back(tokenize(cmd_line, st.delim));
        }
    }

    // - Determine the arguments.
    cmd.nil = false;
    if(!cmd.pipe)
        cmd.params.emplace_back(tokenize(cmd_line, st.delim));

    return cmd;
}

void exec_(command& cmd,
           command::string_vec::size_type idx = 0,
           bool last_pipe = true) {
    // - Check if we have enough arguments
    if(idx >= cmd.params.size())
        return;

    // Prepare commands
    std::vector<char*> argv_;
    for(auto& e : cmd.params[idx])
        argv_.emplace_back(&e[0]);
    argv_.emplace_back(nullptr);

    // - Redirect the output
    if(cmd.output_redir && last_pipe) {
        if(close(1) == 0) {
            auto fd = open(&cmd.output_file[0],
                           O_CREAT | O_WRONLY | O_TRUNC,
                           S_IRWXU | S_IRGRP | S_IROTH);
            if(fd == -1)
                std::cout << "Cannot redirect output to '" << cmd.output_file
                          << "'\n";
        } else
            std::cout << "Failed to close stdout\n";
    }

    // - Execute the command
    auto r = execvp(argv_[0], &argv_[0]);
    if(r == -1)
        std::cout << "An error occurred when trying to execute the "
                     "command\n";

    // - Kill the child process
    exit(EXIT_FAILURE);
}

// - Entry point
int main(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    state cmd_state;
    while(cmd_state.run) {
        command cmd = getcmd(cmd_state);

        // - Ensure that we actually have a command
        if(!(cmd.nil || cmd.empty_params())) {
            pid_t child_pid;

            // - fg waiting
            if(cmd.background && cmd.pid != cmd.default_) {
                // - Print the command
                std::cout << cmd << std::endl;

                while(waitpid(cmd.pid, nullptr, WCONTINUED) != cmd.pid)
                    ;
                cmd.reset_pid();
            }

            // - Process a new command
            else if((child_pid = fork()) != 0) {
                // - Add the command to history
                cmd.pid = child_pid;
                cmd_state.add_command(cmd);

                // - Print the command
                std::cout << cmd << std::endl;

                // - Wait for a normal process
                if(!cmd.background) {
                    while(waitpid(child_pid, nullptr, WCONTINUED) != child_pid)
                        ;
                }
                // - Child process executing the command
            } else {
                if(cmd.pipe) {
                    // - Set up the pipe
                    int fd[2];
                    int in = 0;
                    command::string_vec::size_type i = 0;

                    // - Fork, rewire the child, execute the i-th command
                    auto spawn_child =
                        [i, &cmd](int stdin, int stdout) -> void {
                        if(fork() == 0) {
                            if(stdin != 0) {
                                dup2(stdin, 0);
                                close(stdin);
                            }

                            if(stdout != 1) {
                                dup2(stdout, 1);
                                close(stdout);
                            }

                            exec_(cmd, i, false);
                        }
                    };

                    for(; i < cmd.params.size() - 1u; i++) {
                        // - Create a pipe
                        pipe(fd);

                        // - Spawn the child
                        spawn_child(in, fd[1]);

                        // - The child writes here
                        close(fd[1]);

                        // - The next child reads from here
                        in = fd[0];
                    }

                    // - Connect the output of the previous child to the input
                    // - of the current one
                    if(in != 0)
                        dup2(in, 0);

                    // - Execute the last command
                    exec_(cmd, i);
                } else
                    exec_(cmd, 0);
            }
        }
    }

    // - Newline
    std::cout << std::endl;

    return EXIT_SUCCESS;
}
