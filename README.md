### HTTP Server

## About
This project is an HTTP Server coded in C following a custom set of rules as well as RFC 9110 and RFC 9112. This project originated from a school project and derived into a personnal project. This binary is able to run both as an in-command-line binary or as a daemon depending on the user's choice.

### Built with
This project was built on Ubuntu 22.04 using:
  * C Language
  * GCC compiler
  * Python
  * Pytest library

## Getting started
### Prerequisites
In order to build this project, you will need:
  * GCC compiler
  * Make

### Installation
1. Clone the repository
2. Run: ```make```

### Usage
There are two ways to use this project. You can either directly launch the binary or use the provided `config_reader.sh` shell script for ease of use.

### Directly launching binary
You can launch the binary directly to start the server using the following command:
```./http-server [OPTIONS]```

The options are as follows:
  * `-h | --help` Display the binary's help message
  * `--pid_file <path>` Absolute path of the pid file (required)
  * `--log <true|false>` Enable or disable logging. If a value other than 'true' or 'false' is given, logging will be disabled. Default: true (optionnal)
  * `--log_file <path>` Relative path of the desired log file. If none is specified and server is not running as a daemon, defaults to STDOUT. If server is running as a daemon, defaults to `HTTP.log` (optionnal)
  * `--server_name <name>` Name of the server (required)
  * `--port <port>` Port on which the server will receive requests (required)
  * `--ip <address>` IP address on which the server will run (required)
  * `--root_dir <path>` Relative path of the server's root directory. Default: `./` (optionnal)
  * `--default_file <name>` Name of the default file when none is specified in HTTP request. Default: `index.html` (optionnal)
  * `--daemon <start|stop|restart>` Start, stop or restart the daemon. If start is given and a daemon with the same pid_file is already running, program throws an error. If user tries to stop a daemon that is not running, the program does nothing. Restarting a daemon that was not running is equivalent to starting a new daemon. (optionnal)

Since the command line can get a little large, a `config.txt` and `config_reader.sh` file are provided. They make for an easier use of the project and centralize the server's configuration in `config.txt`.
Note: `config_reader.sh` was not made by myself.

### Utilizing the configuration script
Here is the command line to execute the configuration reader script: `./config_reader.sh --path-config <config_file> --path-bin <path_binary> --daemon <option>`

The options are detailed below:
  * `--path-config <path>` The path to the configuration file, a sample file `config.txt` is provided with this project (required)
  * `--path-bin <path>` The path to the binary (required)
  * `--daemon <option>` Option for daemonizing the server, works the same as for the binary (optionnal)
  * `-h | --help` Display the script's help message

A sample configuration file is given in the project's repository. The following section will present how to edit this file to adapt the server's configuration to the user's needs.

### Editing the configuration file
The file is seperated in two sections. The global options and the vhosts section. These sections are marked by the `[[global]]` and `[[vhosts]]` tags. A section has to end with an empty line.
Inside these sections you can set the server's configuration as follows:
1. Global section
  - pid_file, log_file, log
2. Vhosts section
  - server_name, port, ip, root_dir, default_file

Here is an example of how to set an option in the configuration file:
`log = true`

## License
Distributed under MIT license. Please refer to `LICENSE` for more information.
