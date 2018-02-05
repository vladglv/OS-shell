/***
 *
 * Vlad Glv
 *
 * ECSE-427 (Operating Systems)
 *
 * Assignment 1
 *
 ***/
#pragma once

#ifdef __clang__
#pragma clang diagnostic ignored "-Wc++98-compat"
#pragma clang diagnostic ignored "-Wpadded"
#endif

// - System specific
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// - Standard C++
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// - Macro to inhibit unused variable warnings.
#define UNUSED(expr)                                                           \
    do {                                                                       \
        (void)(expr);                                                          \
    } while (0)

// - Define a command class
class command {
  public:
    using string_vec = std::vector<std::string>;

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
        : number(1), background(false), nil(true), pid(default_),
          output_redir(false), pipe(false) {}

    /**
     * @brief Resets the pid values to its default value
     */
    void reset_pid() { this->pid = this->default_; }

    /**
     * @brief Checks if actual parameters are present
     * @return True if parameters are empty and false otherwise
     */
    bool empty_params() {
        std::string tmp;
        for (const auto& l : this->params) {
            for (const auto& e : l) {
                tmp += e;
            }
        }

        return tmp.empty();
    }

    /**
     * @brief Overload of << operator for convenient printing of the command
     */
    friend std::ostream& operator<<(std::ostream& stream, const command& cmd) {
        stream << "<Command #\t: " << cmd.number << std::endl;

        command::string_vec::size_type i = 0;
        for (const auto& l : cmd.params) {
            stream << " Arguments " << (++i) << "\t: ";
            for (const auto& e : l) {
                stream << e << " ";
            }
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

    state() : prompt(">> "), run(true), cmd_count(1), delim(" \t\n") {}

    /**
     * @brief Adds a command 'cmd' to history. If a command is a jobs then it is
     * added to a job list.
     * @param cmd Command to be added
     */
    void add_command(command& cmd) {
        cmd.number = this->cmd_count++;
        this->cmds.emplace_back(cmd);

        if (cmd.background) {
            this->jobs.emplace_back(cmd);
        }
    }

    /**
     * @brief Update the job list with currently active jobs
     */
    void update_jobs() {
        // - Copy and clear jobs
        std::vector<command> tmp(jobs);
        jobs.clear();

        // - Check for running jobs
        for (auto& e : tmp) {
            auto r = waitpid(e.pid, nullptr, WNOHANG);
            if (r == 0) {
                jobs.emplace_back(e);
            }
        }
    }
};

/**
 * @brief Tokenizes a string 'in' using a delimiter 'delim'
 * @param in Input string
 * @param delim Delimiter used to separate tokens
 * @return A vector of string tokens
 */
std::vector<std::string> tokenize(std::string in, const std::string& delim);

/**
 * @brief Obtains a command from std::cin
 * @param st State of the shell
 * @return Obtained command if any
 */
command getcmd(state& st);

/**
 * @brief Executes a command
 *
 * @param cmd Command to be executed
 * @param idx Index of the command's parameters to be executed
 * @param last_pipe Tells whether or not this is the last command. If so, the
 * output will be redirected when 'cmd.output_redir' is set
 */
void exec_(command& cmd, command::string_vec::size_type idx, bool last_pipe);

/**
 * @brief This is the shell's entry point function
 */
void shell_entry_point();
