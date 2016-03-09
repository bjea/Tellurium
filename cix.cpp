// $Id: cix.cpp,v 1.2 2015-05-12 18:59:40-07 - - $

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>

#include "protocol.h"
#include "logstream.h"
#include "sockets.h"

logstream log (cout);
struct cix_exit: public exception {};

unordered_map<string,cix_command> command_map {
   {"exit", CIX_EXIT},
   {"get" , CIX_GET },
   {"help", CIX_HELP},
   {"ls"  , CIX_LS  },
};

void cix_get (client_socket& server, string& filename)
{
   cix_header header;
   header.command = CIX_GET;
   size_t n = filename.length();
   size_t found = filename.find("/");
   if (n > 58 || found != string::npos)
   {
      CIX_ERROR;   // ??? TO-DO!
   }
   strcpy(header.filename, filename.c_str());
   header.filename[n] = '\0';
   log << "sending header " << header << endl;
   send_packet (server, &header, sizeof header);
   recv_packet (server, &header, sizeof header);
   log << "received header " << header << endl;
   if (header.command != CIX_FILE) {
      log << "sent CIX_GET, server did not return CIX_FILE" << endl;
      log << "server returned " << header << endl;
   }else {
      char buffer[header.nbytes + 1];
      recv_packet (server, buffer, header.nbytes);
      log << "received " << header.nbytes << " bytes" << endl;
      buffer[header.nbytes] = '\0';
      //cout << buffer; // TO-DO: write buffer to file.
      ofstream out_file (filename, ofstream::binary);
      //out_file.open(filename);
      //if (out_file.is_open())
      //{
         //out_file << buffer; // TO-DO: write buffer to file.
      out_file.write(buffer, header.nbytes);
      out_file.close();
      //}
      /*else
      {
         log << "Unable to open file: " << filename << endl;
      }*/
   }


}

void cix_help() {
   static vector<string> help = {
      "exit         - Exit the program.  Equivalent to EOF.",
      "get filename - Copy remote file to local host.",
      "help         - Print help summary.",
      "ls           - List names of files on remote server.",
      "put filename - Copy local file to remote host.",
      "rm filename  - Remove file from remote server.",
   };
   for (const auto& line: help) cout << line << endl;
}

void cix_ls (client_socket& server) {
   cix_header header;
   header.command = CIX_LS;
   log << "sending header " << header << endl;
   send_packet (server, &header, sizeof header);
   recv_packet (server, &header, sizeof header);
   log << "received header " << header << endl;
   if (header.command != CIX_LSOUT) {
      log << "sent CIX_LS, server did not return CIX_LSOUT" << endl;
      log << "server returned " << header << endl;
   }else {
      char buffer[header.nbytes + 1];
      recv_packet (server, buffer, header.nbytes);
      log << "received " << header.nbytes << " bytes" << endl;
      buffer[header.nbytes] = '\0';
      cout << buffer;
   }
}

void cix_put (client_socket& server, string& filename)
{
   cix_header header;
   header.command = CIX_PUT;
   size_t n = filename.length();
   size_t found = filename.find("/");
   if (n > 58 || found != string::npos)
   {
      CIX_ERROR;   // ??? TO-DO!
   }

   ifstream file (filename, ifstream::binary);
   if (file) {
      strcpy(header.filename, filename.c_str());
      header.filename[n] = '\0';
      file.seekg(0, file.end);
      long size = file.tellg();
      file.seekg(0, file.beg); // Or file.seekg(0); ?
      //file.seekg(0, file.beg);

      char *buffer = new char[size];
      log << "Reading " << size << " characters." << endl;
      file.read(buffer, size);
      log << "sending header " << header << endl;
      header.nbytes = size;
      //send_packet(server, &header, sizeof header);
      send_packet(server, &header, size);

      send_packet(server, buffer, sizeof buffer);
      //header.command = CIX_FILE;
      //header.nbytes = size;
      //memset (header.filename, 0, FILENAME_SIZE);
/*
      const long FIXED_SIZE = 1024;
      if (size > FIXED_SIZE)
      {
         long i = 0;
         do {
            char* buffer_temp = new char[FIXED_SIZE];
            for (; i < FIXED_SIZE and i < size; ++i) {
               buffer_temp[i] = buffer[i];
            }

            send_packet(server, buffer_temp, sizeof buffer_temp);
            log << "sent " << sizeof buffer_temp
                << " bytes" << endl;
         } while (i < size);
      }
      else
      {
         send_packet(server, buffer, sizeof buffer);
         log << "sent " << size << " bytes" << endl;
      }
*/

      delete[] buffer;
      file.close();
   }
   else
   {
      log << "put filename: " << filename << " open failed: "
          << strerror (errno) << endl;
      //header.command = CIX_NAK;
      header.nbytes = errno;
      CIX_ERROR;   // ??? TO-DO!
   }

}


void usage() {
   cerr << "Usage: " << log.execname() << " [host] [port]" << endl;
   throw cix_exit();
}

int main (int argc, char** argv) {
   log.execname (basename (argv[0]));
   log << "starting" << endl;
   vector<string> args (&argv[1], &argv[argc]);
   if (args.size() > 2) usage();
   string host = get_cix_server_host (args, 0);
   in_port_t port = get_cix_server_port (args, 1);
   log << to_string (hostinfo()) << endl;
   try {
      log << "connecting to " << host << " port " << port << endl;
      client_socket server (host, port);
      log << "connected to " << to_string (server) << endl;
      for (;;) {
         string line;
         getline (cin, line);
         if (cin.eof()) throw cix_exit();
         log << "command " << line << endl;
         const auto& itor = command_map.find (line);
         cix_command cmd = itor == command_map.end()
                         ? CIX_ERROR : itor->second;
         switch (cmd) {
            case CIX_EXIT:
               throw cix_exit();
               break;

            case CIX_HELP:
               cix_help();
               break;
            case CIX_LS:
               cix_ls (server);
               break;
            default:
               log << line << ": invalid command" << endl;
               break;
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

