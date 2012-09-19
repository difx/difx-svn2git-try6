#ifndef NETWORK_UDPSOCKET_H
#define NETWORK_UDPSOCKET_H
//==============================================================================
//
//   network::UDPSocket Class
//
//!  The UDP Socket can work both to broadcast and to receive.
//!
//!  The UDPSocket Class creates and handles a socket set
//!  up to send and receive UDP packets.  It can be created in 
//!  broadcast, multicast, unicast or receive mode.  In broadcast mode
//!  messages will be sent to all addresses on a subnet.  In
//!  unicast mode there will be a single (specified) target
//!  address.  Multicast can be set to send to a range of addresses.
//!  In receive mode the socket will be able to
//!  receive UDP packets from any of the above types of
//!  sockets.  In all cases the port number is specified.  If
//!  a socket is in broadcast or receive mode, the IPaddress
//!  is assumed to be NULL.
//!
//!  In reality, receive is possible in all modes.
//!
//!  The destructor will close the socket connection.
//
//==============================================================================
#include <sys/time.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <network/GenericSocket.h>

namespace network {

    class UDPSocket : public GenericSocket  {

    public :

        //!  UDP Socket types
        enum UDPSocketType  {
            BROADCAST   = 1,
            UNICAST     = 2,
            RECEIVE     = 3,
            MULTICAST   = 4
        };

        //----------------------------------------------------------------------------
        //!  Open a new UDP socket at the specified port.  This can be used to
        //!  broadcast or receive.  The IP address can be NULL unless a unicast
        //!  socket is requested.
        //----------------------------------------------------------------------------
        UDPSocket( UDPSocketType mode, char *IPaddress, int port )  {
            _fd = 0;
            int on = 1;

            _ignoreOwn = true;
            _ipValid = 0;

            //  Fix things if we have a messed up mode
            if ( ( mode == UNICAST || mode == MULTICAST ) && IPaddress == NULL )  {
                perror( "UDPSocket: Unicast/multicast impossible without IP address - making this a broadcast" );
                mode = BROADCAST;
            }

            //  Open the socket
            if ( ( _fd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )  {
                perror( "UDPSocket: can't open socket" );
                _fd = -1;
                return;
            }

            //  Options only for a broadcast socket
            if ( mode == BROADCAST )  {
                if ( setsockopt( _fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof ( on ) ) < 0 ) {
                    perror( "UDPSocket: trouble with setsockopt" );
                    close( _fd );
                    _fd = -1;
                    return;
                }
            }

            bzero( (char *) &addr, sizeof (addr) );
            addr.sin_family = AF_INET;
            addr.sin_port = htons( port );

            //  only for receive
            if ( mode == RECEIVE ) {
                addr.sin_addr.s_addr = htonl( INADDR_ANY );
            	if ( setsockopt( _fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof( on ) ) < 0 ) {
                    perror( "UDPSocket: trouble with setsockopt" );
                    close( _fd );
                    _fd = -1;
                    return;
                }
                bind( _fd, (struct sockaddr *) &addr, sizeof( addr ) );
                //  Receive based on a "group ID", which is given as the address...this is the receive
                //  side of a MULTICAST socket.  Honestly not sure if the group id does anything at all.
                if ( IPaddress != NULL ) {
                	inet_aton( IPaddress, &mreq.imr_multiaddr );
                	mreq.imr_interface.s_addr = htonl( INADDR_ANY );
                	if ( setsockopt( _fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof( struct ip_mreq ) ) < 0 ) {
                        perror( "UDPSocket: trouble with setsockopt" );
                        close( _fd );
                        _fd = -1;
                        return;
                    }
                }
                //  Add a 1/10th second timeout on receives...this lets us close the socket (use closefd());
                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 100000;
                if ( setsockopt( _fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof( tv ) ) < 0 ) {
                    perror( "UDPSocket: trouble with setsockopt" );
                    close( _fd );
                    _fd = -1;
                    return;
                }
            }

            //  only for MULTICAST
            if ( mode == MULTICAST ) {
                unsigned char ttl = 3;    // time-to-live.  Max hops before discard 
                setsockopt( _fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof( ttl ) );
            }

            //  unicast or multicast
            if ( mode == UNICAST || mode == MULTICAST )
                addr.sin_addr.s_addr = inet_addr( IPaddress );


            if ( mode == BROADCAST )  {
                addr.sin_addr.s_addr = INADDR_BROADCAST;
            }

            getHostAddress();
        }
        
        ~UDPSocket() {
            close( _fd );
        }

        //----------------------------------------------------------------------------
        //!  Fill a string with the IP address from which messages
        //!  have been received.  The address is only valid after the reader
        //!  has been called!
        //----------------------------------------------------------------------------
        int fromIPAddress( char *address )  {
            char blotch[4];
            if ( !_ipValid )  return( -1 );

            bcopy( (char *)&(fromaddr.sin_addr.s_addr), blotch, 4 );
            sprintf( address, "%d.%d.%d.%d", (unsigned char)(blotch[0]), (unsigned char)(blotch[1]), 
            (unsigned char)(blotch[2]), (unsigned char)(blotch[3]) );
            return (0);
        }
        
        //----------------------------------------------------------------------------
        //!  Receive a broadcast of the specified length and store it in the
        //!  character string.  The number of characters received is returned
        //!  (-1 is something goes wrong).  This function will ignore broadcasts of
        //!  the same host if the _ignoreOwn flag is set (by default, it is).
        //----------------------------------------------------------------------------
        virtual int reader( char *message, int messagelength )  {
            int fromaddrlength = sizeof( fromaddr );
            int charcount;

            do  {
                //  Wait until we get some data as long as the socket is good.
                while ( _fd > -1 ) {
                    charcount = recvfrom( _fd, message, messagelength, 0,
                                          (sockaddr *)&fromaddr, (socklen_t *)&fromaddrlength );
                    if ( charcount < 0 )  {
                        //  Bail out on errors that are not timeouts.
                        if ( errno != EWOULDBLOCK ) {
                            perror( "recvbroadcast" );
                            return( -1 );
                        }
                    }
                    else
                        break;
                }

            } while ( _ignoreOwn && (fromaddr.sin_addr.s_addr == hostaddr.sin_addr.s_addr) );

            _ipValid = true;
            message[charcount] = 0;
            return( charcount );
        }

        //----------------------------------------------------------------------------
        //!  Send the specified data over the UDP socket.
        //----------------------------------------------------------------------------
        virtual int writer( char *message, int messagelength )  {
            return( sendto( _fd, message, messagelength, 0, (sockaddr *)&addr, sizeof(addr) ) );
        }
        
        struct sockaddr_in addr;  //  this can't be private!
        struct sockaddr_in hostaddr;
        struct sockaddr_in fromaddr;
	    struct ip_mreq mreq;
        
        //----------------------------------------------------------------------------
        //!  "IgnoreOwn" determines whether broadcast messages from your machine are
        //!  ignored or not.  By default it is "true".
        //----------------------------------------------------------------------------
        const bool ignoreOwn() { return _ignoreOwn; }
        void ignoreOwn( const bool newVal ) { _ignoreOwn = newVal; }
        
        int fd() { return _fd; }
        void closeFd() {
            _fd = -1;
            close( _fd );
            usleep( 200000 );
        }

    protected :

        int _fd;
        bool _ipValid;
        bool _ignoreOwn;

        //----------------------------------------------------------------------------
        //!  Get the present host address.  This is used internally.
        //----------------------------------------------------------------------------
        int getHostAddress()  {
            char hostname[100];
            struct hostent *hp;

            if ( gethostname( hostname, sizeof( hostname ) ) < 0)
                return( -1 );
            hp = gethostbyname( hostname );
            if ( !hp )
                return( -1 );
            hostaddr.sin_family = AF_INET;
            hostaddr.sin_port = 0;
            bcopy( hp->h_addr, (char *)&(hostaddr.sin_addr), sizeof(hostaddr.sin_addr) );
            return( 0 );
        }

    };

}

#endif
