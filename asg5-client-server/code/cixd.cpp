// $Id: cixd.cpp,v 1.7 2016-05-09 16:01:56-07 - - $

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstring>
using namespace std;

#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>

#include "protocol.h"
#include "logstream.h"
#include "sockets.h"

logstream log (cout);
struct cix_exit: public exception {};

void reply_ls (accepted_socket& client_sock, cix_header& header) {
   const char* ls_cmd = "ls -l 2>&1";
   FILE* ls_pipe = popen (ls_cmd, "r");
   if (ls_pipe == NULL) { 
      log << "ls -l: popen failed: " << strerror (errno) << endl;
      header.command = cix_command::NAK;
      header.nbytes = errno;
      send_packet (client_sock, &header, sizeof header);
      return;
   }
   string ls_output;
   char buffer[0x1000];
   for (;;) {
      char* rc = fgets (buffer, sizeof buffer, ls_pipe);
      if (rc == nullptr) break;
      ls_output.append (buffer);
   }
   int status = pclose (ls_pipe);
   if (status < 0) log << ls_cmd << ": " << strerror (errno) << endl;
              else log << ls_cmd << ": exit " << (status >> 8)
                       << " signal " << (status & 0x7F)
                       << " core " << (status >> 7 & 1) << endl;
   header.command = cix_command::LSOUT;
   header.nbytes = ls_output.size();
   memset (header.filename, 0, FILENAME_SIZE);
   log << "sending header " << header << endl;
   send_packet (client_sock, &header, sizeof header);
   send_packet (client_sock, ls_output.c_str(), ls_output.size());
   log << "sent " << ls_output.size() << " bytes" << endl;
}

void reply_get (accepted_socket& client_sock, cix_header& header) {
  ifstream file {header.filename};

  char c;
  string fileContent;
  if(file.is_open()){
    while(file.good()){
      file.get(c);
      fileContent.push_back(c);
    }
  }else{
    log << "No such file: " << header.filename << endl;
    header.command = cix_command::NAK;
    header.nbytes = errno;
    log << "sending header " << header << endl;
    send_packet(client_sock, &header, sizeof header);
    return;
  }

  
   file.close();
   header.command = cix_command::FILE;
   header.nbytes = fileContent.size();
   memset (header.filename, 0, FILENAME_SIZE);
   log << "sending header " << header << endl;
   send_packet (client_sock, &header, sizeof header);
   send_packet (client_sock, fileContent.c_str(), fileContent.size());
   log << "sent " << fileContent.size() << " bytes" << endl;  
}

void reply_rm(accepted_socket& client_sock, cix_header& header){
  int rc = unlink(header.filename);
  if(rc != 0){ 
    log << "Error: " << header.filename << ": " 
                << strerror (errno) << endl;
    header.command = cix_command::NAK;
    header.nbytes = errno;
    log << "sending header " << header << endl;
    send_packet (client_sock, &header, sizeof header);
    return;
  }
  
  log << header.filename << ": Succesfully removed: "<< endl;
  header.command = cix_command::ACK;
  memset (header.filename, 0, FILENAME_SIZE);
  log << "sending header " << header << endl;
  send_packet (client_sock, &header, sizeof header);
}

void reply_put(accepted_socket& client_sock, cix_header& header){
  char buffer[header.nbytes + 1];
  recv_packet (client_sock, buffer, header.nbytes);
  log << "received " << header.nbytes << " bytes" << endl;
  buffer[header.nbytes] = '\0';
  ofstream file {header.filename};
  if(file.fail()){
    log << header.filename << ": ifstream failed: " 
                            << strerror (errno) << endl;
    header.command = cix_command::NAK;
    header.nbytes = errno;
    send_packet (client_sock, &header, sizeof header);
    return;
  }
  file.write (buffer, sizeof buffer);
  file.close();
  cout << header.filename << " has been created"<< endl; 
  header.command = cix_command::ACK;
  memset (header.filename, 0, FILENAME_SIZE);
  header.nbytes = 0;
  log << "sending header " << header << endl;
  send_packet (client_sock, &header, sizeof header);
      
}


void run_server (accepted_socket& client_sock) {
   log.execname (log.execname() + "-server");
   log << "connected to " << to_string (client_sock) << endl;
   try {   
      for (;;) {
         cix_header header; 
         recv_packet (client_sock, &header, sizeof header);
         log << "received header " << header << endl;
         switch (header.command) {
            case cix_command::LS: 
               reply_ls (client_sock, header);
               break;
            case cix_command::GET: 
               reply_get (client_sock, header);
               break;
            case cix_command::RM: 
               reply_rm (client_sock, header);
               break;
            case cix_command::PUT: 
               reply_put (client_sock, header);
               break;
            default:
               log << "invalid header from client:" << header << endl;
               break;
         }
      }
   }catch (socket_error& error) {
      log << error.what() << endl;
   }catch (cix_exit& error) {
      log << "caught cix_exit" << endl;
   }
   log << "finishing" << endl;
   throw cix_exit();
}

void fork_cixserver (server_socket& server, accepted_socket& accept) {
   pid_t pid = fork();
   if (pid == 0) { // child
      server.close();
      run_server (accept);
      throw cix_exit();
   }else {
      accept.close();
      if (pid < 0) {
         log << "fork failed: " << strerror (errno) << endl;
      }else {
         log << "forked cixserver pid " << pid << endl;
      }
   }
}


void reap_zombies() {
   for (;;) {
      int status;
      pid_t child = waitpid (-1, &status, WNOHANG);
      if (child <= 0) break;
      log << "child " << child
          << " exit " << (status >> 8)
          << " signal " << (status & 0x7F)
          << " core " << (status >> 7 & 1) << endl;
   }
}

void signal_handler (int signal) {
   log << "signal_handler: caught " << strsignal (signal) << endl;
   reap_zombies();
}

void signal_action (int signal, void (*handler) (int)) {
   struct sigaction action;
   action.sa_handler = handler;
   sigfillset (&action.sa_mask);
   action.sa_flags = 0;
   int rc = sigaction (signal, &action, nullptr);
   if (rc < 0) log << "sigaction " << strsignal (signal) << " failed: "
                   << strerror (errno) << endl;
}


int main (int argc, char** argv) {
   log.execname (basename (argv[0]));
   log << "starting" << endl;
   vector<string> args (&argv[1], &argv[argc]);
   signal_action (SIGCHLD, signal_handler);
   in_port_t port = get_cix_server_port (args, 0);
   try {
      server_socket listener (port);
      for (;;) {
         log << to_string (hostinfo()) << " accepting port "
             << to_string (port) << endl;
         accepted_socket client_sock;
         for (;;) {
            try {
               listener.accept (client_sock);
               break;
            }catch (socket_sys_error& error) {
               switch (error.sys_errno) {
                  case EINTR:
                     log << "listener.accept caught "
                         << strerror (EINTR) << endl;
                     break;
                  default:
                     throw;
               }
            }
         }
         log << "accepted " << to_string (client_sock) << endl;
         try {
            fork_cixserver (listener, client_sock);
            reap_zombies();
         }catch (socket_error& error) {
            log << error.what() << endl;
         }
      }
   }catch (socket_error& error) {
      log << error.what() << endl;
   }catch (cix_exit& error) {
      log << "caught cix_exit" << endl;
   }
   log << "finishing" << endl;
   return 0;
}

