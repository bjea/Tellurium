// $Id: sockets.h,v 1.1 2015-05-12 18:48:40-07 - - $
// base_socket: accepted; client; server

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

//
// class base_socket:
// mostly protected and not used by applications
// Inheritance: but no virtual fcns.

class base_socket {
   private:
      static constexpr size_t MAXRECV = 0xFFFF;
    // We need a special flag (-1) here to know if socket is
    // closed.
      static constexpr int CLOSED_FD = -1;
      int socket_fd {CLOSED_FD};
      sockaddr_in socket_addr;
    // delete: b/c u can't copy file by copy ctor or assignment=
      base_socket (const base_socket&) = delete; // prevent copying
      base_socket& operator= (const base_socket&) = delete;
   protected:
      base_socket(); // only derived classes may construct
      ~base_socket();
      // server_socket initialization
      void create();
      void bind (const in_port_t port);
      void listen() const;
      void accept (base_socket&) const;
      // client_socket initialization
      void connect (const string host, const in_port_t port);
      // accepted_socket initialization
      void set_socket_fd (int fd);
   public:
      void close();
      ssize_t send (const void* buffer, size_t bufsize);
      ssize_t recv (void* buffer, size_t bufsize);
      void set_non_blocking (const bool);
      friend string to_string (const base_socket& sock);
};


//
// class accepted_socket
// used by server when a client connects
//

class accepted_socket: public base_socket {
   public:
      accepted_socket() {}
      accepted_socket(int fd) { set_socket_fd (fd); }
};

//
// class client_socket
// used by client application to connect to server
// can send, recv msgs. and close the socket.

class client_socket: public base_socket {
   public: 
      client_socket (string host, in_port_t port);
};

//
// class server_socket
// single use class by server application
//

class server_socket: public base_socket {
   public:
      server_socket (in_port_t port);
      void accept (accepted_socket& sock) {
         base_socket::accept (sock);
      }
};


//
// class socket_error
// base class for throwing socket errors
//

class socket_error: public runtime_error {
   public:
      explicit socket_error (const string& what): runtime_error(what){}
};

//
// class socket_sys_error
// subclass to record status of extern int errno variable
//

class socket_sys_error: public socket_error {
   public:
      int sys_errno;
      explicit socket_sys_error (const string& what):
      // strerror to explain
               socket_error(what + ": " + strerror (errno)),
               sys_errno(errno) {}
};

//
// class socket_h_error
// subclass to record status of extern int h_errno variable
//

class socket_h_error: public socket_error {
   public:
      int host_errno;
      explicit socket_h_error (const string& what):
               socket_error(what + ": " + hstrerror (h_errno)),
               host_errno(h_errno) {}
};


//
// class hostinfo
// information about a host given hostname or IPv4 address
//

class hostinfo {
   public:
      const string hostname;
      const vector<string> aliases;
      const vector<in_addr> addresses;
      hostinfo (); // localhost
      hostinfo (hostent*);
      hostinfo (const string& hostname);
      hostinfo (const in_addr& ipv4_addr);
    // to_string() doesn't really have to be a friend
      friend string to_string (const hostinfo&);
};

string localhost();
string to_string (const in_addr& ipv4_addr);

#endif
// "abcd" + 2 in C++: return the address of "abcd" plus 2
