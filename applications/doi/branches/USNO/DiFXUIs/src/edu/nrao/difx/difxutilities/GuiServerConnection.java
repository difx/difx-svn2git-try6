/*
 * This class is used to form a connection to the guiServer application that
 * should be running on the DiFX host.  This is a bi-directional TCP connection.
 * Outgoing traffic (TO the guiServer) takes the form of commands, while incoming
 * traffic (FROM guiServer) is limited to a few informational packets (mostly 
 * transmitted upon connection) and relay of multicast traffic on the DiFX cluster (much
 * of which is mk5server traffic).  The design tries to keep this connection
 * fairly simple (and thus hopefully robust) - any additional data relays (for 
 * instance if a command generates return data, or a file needs to be transfered)
 * are done using purpose-formed TCP connections with threads on the guiServer.
 * 
 * This class generates three types of callbacks: "connect" events accompanied by
 * an explanatory String (either the connection status in the case of a change or
 * an exception string in the case of an error); "send" events accompanied
 * by an integer number of bytes sent; and  "receive" events accompanied by an
 * integer number of bytes received.  
 */
package edu.nrao.difx.difxutilities;

import edu.nrao.difx.difxview.SystemSettings;

import java.net.Socket;
import java.net.SocketTimeoutException;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import javax.swing.JOptionPane;

import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.event.EventListenerList;

/**
 *
 * @author johnspitzak
 */
public class GuiServerConnection {
    
    public final int RELAY_PACKET                   = 1;
    public final int RELAY_COMMAND_PACKET           = 2;
    public final int COMMAND_PACKET                 = 3;
    public final int INFORMATION_PACKET             = 4;
    public final int WARNING_PACKET                 = 5;
    public final int ERROR_PACKET                   = 6;
    public final int MULTICAST_SETTINGS_PACKET      = 7;
    public final int GUISERVER_VERSION              = 8;
    public final int GUISERVER_DIFX_VERSION         = 9;
    public final int AVAILABLE_DIFX_VERSION         = 10;
    public final int DIFX_BASE                      = 11;

    
    public GuiServerConnection( SystemSettings settings, String IP, int port, int timeout ) {
        _settings = settings;
        _connectListeners = new EventListenerList();
        _sendListeners = new EventListenerList();
        _receiveListeners = new EventListenerList();
    }
    
    public boolean connect() {
        try {
            _socket = new Socket( _settings.difxControlAddress(), _settings.difxControlPort() );
            _socket.setSoTimeout( _settings.timeout() );
            _in = new DataInputStream( _socket.getInputStream() );
            _out = new DataOutputStream( _socket.getOutputStream() );
            _connected = true;
            connectEvent( "connected" );
            _receiveThread = new ReceiveThread();
            _receiveThread.start();
            //  Request guiServer version information...and anything else it wants
            //  to tell us at startup.
            sendPacket( GUISERVER_VERSION, 0, null );
            return true;
        } catch ( java.net.UnknownHostException e ) {
            _connected = false;
            return false;
        } catch ( java.io.IOException e ) {
            _connected = false;
            return false;
        }
    }
    
    public void close() {
        if ( connected() ) {
            try {
                _socket.close();
            } catch ( java.io.IOException e ) {
                //  Not being able to close the socket probably indicates something out of
                //  the user's ability to fix is wrong, so we won't trouble them by reporting
                //  the problem.
            }
            _connected = false;
            connectEvent( "connection closed" );
        }
    }
    
    /*
     * Send a packet of the given type.  The type and number of bytes in
     * the packet are sent as integers - and thus are swapped (if necessary) to
     * network byte order.  The data are not.
     */
    synchronized public void sendPacket( int packetId, int nBytes, byte[] data ) {
        if ( _connected ) {
            try {
                _out.writeInt( packetId );
                _out.writeInt( nBytes );
                if ( nBytes > 0 ) {
                    _out.write( data );
                }
                sendEvent( nBytes );
            } catch ( Exception e ) {
                java.util.logging.Logger.getLogger("global").log(java.util.logging.Level.SEVERE, null, 
                    e.toString() );
                connectEvent( e.toString() );
            }
        } else {
            sendEvent( -nBytes );
        }
    }
        
    /*
     * This is kind of gross and kludgy....it is meant to simulate waiting for
     * a socket message from difx of a specific type - left over from when the ONLY
     * type of message difx sent was a relay.
     */
    public byte[] getRelay() throws SocketTimeoutException {
        int counter = 0;
        while ( counter < _settings.timeout() ) {
            if ( _difxRelayData != null ) {
                byte [] returnData = _difxRelayData;
                _difxRelayData = null;
                return returnData;
            }
            counter += 10;
            try { Thread.sleep( 10 ); } catch ( Exception e ) {}
        }
        throw new SocketTimeoutException();
    }
    
    protected static int WARNING_SIZE = 1024 * 1024;
    /*
     * This thread recieves data packets of different types from the guiServer.
     */
    protected class ReceiveThread extends Thread {
        
        public void run() {
            while ( _connected ) {
                byte[] data = null;
                try {
                    int packetId = _in.readInt();
                    int nBytes = _in.readInt();
                    if ( nBytes > WARNING_SIZE ) {
                        java.util.logging.Logger.getLogger("global").log(java.util.logging.Level.WARNING, 
                                "trying to read " + nBytes + "of data - packetID is " + packetId );
                        java.util.logging.Logger.getLogger("global").log(java.util.logging.Level.WARNING, 
                                "Message has NOT BEEN READ" );
                    }
                    else {
                        data = new byte[nBytes];
                        _in.readFully( data );
                        //  Sort out what to do with this packet.
                        if ( packetId == RELAY_PACKET && data != null ) {
                            _difxRelayData = data;
                        }
                        else if ( packetId == GUISERVER_VERSION ) {
                            //  This is a report of the version of guiServer that is running.
                            _settings.guiServerVersion( new String( data ) );
                        }
                        else if ( packetId == GUISERVER_DIFX_VERSION ) {
                            //  This is the difx version for which the guiServer was compiled.  At the
                            //  moment this only changes the message parser (difxio).
                            _settings.guiServerDifxVersion( new String( data ) );
                            _settings.difxVersion( new String( data ), false );
                        }
                        else if ( packetId == AVAILABLE_DIFX_VERSION ) {
                            //  Add an available DiFX version to the list in settings.
                            _settings.addDifxVersion( new String( data ) );
                            if ( _settings.guiServerDifxVersion() != null )
                                _settings.difxVersion( _settings.guiServerDifxVersion(), false );
                        }
                        else if ( packetId == INFORMATION_PACKET ) {
                            _settings.messageCenter().message( 0, "guiServer", new String( data ) );
                        }
                        else if ( packetId == WARNING_PACKET ) {
                            _settings.messageCenter().warning( 0, "guiServer", new String( data ) );
                        }
                        else if ( packetId == ERROR_PACKET ) {
                            _settings.messageCenter().error( 0, "guiServer", new String( data ) );
                        }
                        else if ( packetId == DIFX_BASE ) {
                            //  The DiFX base is the path below which all "setup" files
                            //  exist.
                            _settings.clearDifxVersion();
                            _settings.difxBase( new String( data ) );
                        }
                        receiveEvent( data.length );
                    }
                } catch ( SocketTimeoutException e ) {
                    //  Timeouts are actually expected and should not cause alarm.
                } catch ( java.io.IOException e ) {
                    _connected = false;
                    connectEvent( e.toString() );
                }
            try { Thread.sleep( 20 ); } catch ( Exception e ) {}
            }
        }
        
    }
        
    
    /*
     * This turns on (or off) the broadcast relay capability.
     */
    public void relayBroadcast( boolean on ) {
        ByteBuffer b = ByteBuffer.allocate( 4 );
        b.order( ByteOrder.BIG_ENDIAN );
        if ( on )
            b.putInt( 1 );
        else
            b.putInt( 0 );
        sendPacket( RELAY_PACKET, 4, b.array() );
    }
    
    public boolean connected() { return _connected; }
    
    public void addConnectionListener( ActionListener a ) {
        _connectListeners.add( ActionListener.class, a );
    }

    public void addSendListener( ActionListener a ) {
        _sendListeners.add( ActionListener.class, a );
    }

    public void addReceiveListener( ActionListener a ) {
        _receiveListeners.add( ActionListener.class, a );
    }
    
    protected void connectEvent( String mess ) {
        Object[] listeners = _connectListeners.getListenerList();
        int numListeners = listeners.length;
        for ( int i = 0; i < numListeners; i+=2 ) {
            if ( listeners[i] == ActionListener.class )
                ((ActionListener)listeners[i+1]).actionPerformed( new ActionEvent( this, ActionEvent.ACTION_PERFORMED, mess ) );
        }
    }

    protected void sendEvent( Integer nBytes ) {
        Object[] listeners = _sendListeners.getListenerList();
        int numListeners = listeners.length;
        for ( int i = 0; i < numListeners; i+=2 ) {
            if ( listeners[i] == ActionListener.class )
                ((ActionListener)listeners[i+1]).actionPerformed( new ActionEvent( this, ActionEvent.ACTION_PERFORMED, nBytes.toString() ) );
        }
    }

    protected void receiveEvent( Integer nBytes ) {
        Object[] listeners = _receiveListeners.getListenerList();
        int numListeners = listeners.length;
        for ( int i = 0; i < numListeners; i+=2 ) {
            if ( listeners[i] == ActionListener.class )
                ((ActionListener)listeners[i+1]).actionPerformed( new ActionEvent( this, ActionEvent.ACTION_PERFORMED, nBytes.toString() ) );
        }
    }


    protected Socket _socket;
    protected DataInputStream _in;
    protected DataOutputStream _out;
    protected boolean _connected;
    protected EventListenerList _connectListeners;
    protected EventListenerList _sendListeners;
    protected EventListenerList _receiveListeners;
    protected byte[] _difxRelayData;
    protected ReceiveThread _receiveThread;
    protected SystemSettings _settings;
}
