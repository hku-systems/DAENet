//
// Created by jianyu on 5/27/19.
//

#ifndef ANONYMOUSP2P_C_NETWORK_H
#define ANONYMOUSP2P_C_NETWORK_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace c_network {
    /*  Prepare to accept connections on socket FD.
        N connection requests will be queued before further requests are refused.
        Returns 0 on success, -1 for errors.  */
    extern "C" int listen (int __fd, int __n) __THROW;

    /*  Await a connection on socket FD.
        When a connection arrives, open a new socket to communicate with it,
        set *ADDR (which is *ADDR_LEN bytes long) to the address of the connecting
        peer and *ADDR_LEN to the address's actual length, and return the
        new socket's descriptor, or -1 for errors.

        This function is a cancellation point and therefore not marked with
        __THROW.  */
    extern "C" int accept (int __fd, __SOCKADDR_ARG __addr,
                           socklen_t *__restrict __addr_len);

    /* Create a new socket of type TYPE in domain DOMAIN, using
        protocol PROTOCOL.  If PROTOCOL is zero, one is chosen automatically.
        Returns a file descriptor for the new socket, or -1 for errors.  */
    extern "C" int socket (int __domain, int __type, int __protocol) __THROW;

    /* Give the socket FD the local address ADDR (which is LEN bytes long).  */
    extern "C" int bind (int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len)
    __THROW;

    /* This function is a cancellation point and therefore not marked with
    __THROW.  */
    extern "C" int connect (int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);

    /* Send N bytes of BUF to socket FD.  Returns the number sent or -1.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
    extern "C" ssize_t send (int __fd, const void *__buf, size_t __n, int __flags);

/* Read N bytes into BUF from socket FD.
   Returns the number read or -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
    extern "C" ssize_t recv (int __fd, void *__buf, size_t __n, int __flags);

/* Send N bytes of BUF on socket FD to peer at address ADDR (which is
   ADDR_LEN bytes long).  Returns the number sent, or -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
    extern "C" ssize_t sendto (int __fd, const void *__buf, size_t __n,
                int __flags, __CONST_SOCKADDR_ARG __addr,
                socklen_t __addr_len);

/* Read N bytes into BUF through socket FD.
   If ADDR is not NULL, fill in *ADDR_LEN bytes of it with tha address of
   the sender, and store the actual size of the address in *ADDR_LEN.
   Returns the number of bytes read or -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
    extern "C" ssize_t recvfrom (int __fd, void *__restrict __buf, size_t __n,
                int __flags, __SOCKADDR_ARG __addr,
                socklen_t *__restrict __addr_len);

    extern "C" void close (int __fd);
}

#endif //ANONYMOUSP2P_C_NETWORK_H
