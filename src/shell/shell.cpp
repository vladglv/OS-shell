#include <shell/shell.hpp>

std::vector<std::string> tokenize(std::string in, const std::string& delim) {
    char* token;
    char* str_view = &in[0];
    std::vector<std::string> tokens;

    while ((token = strsep(&str_view, delim.c_str())) != nullptr) {
        for (std::size_t i = 0; i < std::strlen(token); i++) {
            if (token[i] <= 32) {
                token[i] = '\0';
            }
        }

        if (std::strlen(token) > 0) {
            tokens.emplace_back(token);
        }
    }

    return tokens;
}

command getcmd(state& st) {
    std::string cmd_line;
    std::string::size_type pos;
    command cmd;

    // - Display the prompt and fetch the input
    std::cout << st.prompt;
    std::getline(std::cin, cmd_line);

    // - Handle CTRL + D
    if (std::cin.eof()) {
        st.run = false;

        return cmd;
    }

    // - Handle empty input
    if (cmd_line.empty()) {
        return cmd;
    }

    // - Implementation of built-in commands

    // - history
    if (cmd_line == "history") {
        if (st.cmds.empty()) {
            std::cout << "no command history" << std::endl;
        }

        std::size_t i = 1;
        for (auto& e : st.cmds) {
            std::cout << "[" << (i++) << "] \n" << e << std::endl;
        }

        return cmd;
    }

    // - jobs
    else if (cmd_line == "jobs") {
        st.update_jobs();

        if (st.jobs.empty()) {
            std::cout << "no jobs are present" << std::endl;
        }

        std::size_t i = 1;
        for (auto& e : st.jobs) {
            std::cout << "[" << (i++) << "] \n" << e << std::endl;
        }

        return cmd;
    }

    // - !exec command
    else if ((cmd_line.size() > 1) && (cmd_line[0] == '!')) {
        try {
            // - Obtain the number of the command to run
            auto cmd_idx_str = cmd_line.substr(1);
            int cmd_idx = std::stoi(cmd_idx_str);
            if (cmd_idx < 1) {
                throw std::invalid_argument("cmd_idx_str");
            }

            if (static_cast<std::size_t>(cmd_idx) > st.cmds.size()) {
                std::cout << "no command found in history\n";
                return cmd;
            }

            // - Return the command with a default pid.
            auto ccmd = st.cmds[static_cast<std::size_t>(cmd_idx) - 1];
            ccmd.reset_pid();

            return ccmd;
        } catch (...) {
            std::cout << "Incorrect command number provided\n";
        }

        return cmd;
    }

    // - fg
    else if ((cmd_line.size() > 1) && (cmd_line.substr(0, 2) == "fg")) {
        if (cmd_line.size() >= 3) {
            try {
                st.update_jobs();

                // - Obtain the number of the jobs to bring to foreground
                auto job_idx_str = cmd_line.substr(3);
                int job_idx = std::stoi(job_idx_str);
                if (job_idx < 1) {
                    throw std::invalid_argument("job_idx_str");
                }

                if (static_cast<std::size_t>(job_idx) > st.jobs.size()) {
                    std::cout << "no jobs found\n";
                    return cmd;
                }

                return st.jobs[static_cast<std::size_t>(job_idx) - 1];
            } catch (...) {
                std::cout << "Incorrect command number provided\n";
            }
        }

        return cmd;
    }

    // - cd
    else if ((cmd_line.size() > 1) && (cmd_line.substr(0, 2) == "cd")) {
        if (cmd_line.size() >= 3) {

            // - Obtain the directory string
            auto dir = cmd_line.substr(3);
            auto ret = chdir(dir.c_str());
            if (ret != 0) {
                std::cout << "invalid argument to cd\n";
            }
        }

        return cmd;
    }

    // - pwd
    else if (cmd_line == "pwd") {
        std::vector<char> dir(PATH_MAX);

        auto r = getcwd(dir.data(), dir.size());
        if (r != nullptr) {
            std::cout << dir.data() << std::endl;
        }

        return cmd;
    }

    // - exit
    else if (cmd_line == "exit") {
        st.run = false;
        return cmd;
    }

    // - Check for background status
    if ((pos = cmd_line.find_last_of('&')) != cmd_line.npos) {
        cmd.background = true;
        cmd_line[pos] = ' ';
    }

    // - Check for the output redirection
    if ((pos = cmd_line.find_last_of('>')) != cmd_line.npos) {
        cmd.output_redir = true;
        cmd_line[pos] = ' ';
        cmd.output_file = tokenize(cmd_line.substr(pos), st.delim)[0];
        cmd_line = cmd_line.substr(0, pos);
    }

    // - Check for the piping
    auto pipes = std::count(cmd_line.begin(), cmd_line.end(), '|');
    if (pipes > 0) {
        cmd.pipe = true;

        for (auto i = 0; i < pipes + 1; i++) {
            if ((pos = cmd_line.find('|')) != cmd_line.npos) {
                cmd_line[pos] = ' ';
                auto e = cmd_line.substr(0, pos);
                cmd.params.emplace_back(tokenize(e, st.delim));
                cmd_line = cmd_line.substr(pos);
            } else {
                cmd.params.emplace_back(tokenize(cmd_line, st.delim));
            }
        }
    }

    // - Determine the arguments.
    cmd.nil = false;
    if (!cmd.pipe) {
        cmd.params.emplace_back(tokenize(cmd_line, st.delim));
    }

    return cmd;
}

void exec_(command& cmd, command::string_vec::size_type idx = 0,
           bool last_pipe = true) {
    // - Check if we have enough arguments
    if (idx >= cmd.params.size()) {
        return;
    }

    // - Prepare commands
    std::vector<char*> argv_;
    for (auto& e : cmd.params[idx]) {
        argv_.emplace_back(&e[0]);
    }

    argv_.emplace_back(nullptr);

    // - Redirect the output
    if (cmd.output_redir && last_pipe) {
        if (close(1) == 0) {
            auto fd = open(&cmd.output_file[0],
                           O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC,
                           S_IRWXU | S_IRGRP | S_IROTH);
            if (fd == -1) {
                std::cout << "Cannot redirect output to '" << cmd.output_file
                          << "'\n";
            }
        } else {
            std::cout << "Failed to close stdout\n";
        }
    }

    // - Execute the command
    auto r = execvp(argv_[0], &argv_[0]);
    if (r == -1) {
        std::cout << "An error occurred when trying to execute the "
                     "command\n";
    }

    // - Kill the child process
    exit(EXIT_FAILURE);
}

// - Entry point
void shell_entry_point() {
    state cmd_state;
    while (cmd_state.run) {
        command cmd = getcmd(cmd_state);

        // - Ensure that we actually have a command
        if (!(cmd.nil || cmd.empty_params())) {
            pid_t child_pid;

            // - fg waiting
            if (cmd.background && cmd.pid != cmd.default_) {
                // - Print the command
                std::cout << cmd << std::endl;

                while (waitpid(cmd.pid, nullptr, WCONTINUED) != cmd.pid) {
                    ;
                }
                cmd.reset_pid();
            }

            // - Process a new command
            else if ((child_pid = fork()) != 0) {
                // - Add the command to history
                cmd.pid = child_pid;
                cmd_state.add_command(cmd);

                // - Print the command
                std::cout << cmd << std::endl;

                // - Wait for a normal process
                if (!cmd.background) {
                    while (waitpid(child_pid, nullptr, WCONTINUED) !=
                           child_pid) {
                        ;
                    }
                }
                // - Child process executing the command
            } else {
                if (cmd.pipe) {
                    // - Set up the pipe
                    int fd[2];
                    int in = 0;
                    command::string_vec::size_type i = 0;

                    // - Fork, rewire the child, execute the i-th command
                    auto spawn_child = [i, &cmd](int stdin_,
                                                 int stdout_) -> void {
                        if (fork() == 0) {
                            if (stdin_ != 0) {
                                dup2(stdin_, 0);
                                close(stdin_);
                            }

                            if (stdout_ != 1) {
                                dup2(stdout_, 1);
                                close(stdout_);
                            }

                            exec_(cmd, i, false);
                        }
                    };

                    for (; i < cmd.params.size() - 1u; i++) {
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
                    if (in != 0) {
                        dup2(in, 0);
                    }

                    // - Execute the last command
                    exec_(cmd, i);
                } else {
                    exec_(cmd, 0);
                }
            }
        }
    }

    // - Newline
    std::cout << std::endl;
}
