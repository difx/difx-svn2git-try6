/*
 * All settings for the DiFX GUI are contained in this class.  The class provides
 * functions to allow other classes to obtain and set these data, as well as a
 * window that allows the user to view and set them via the GUI.  Facilities for
 * loading settings from files and saving them to files are provided as well.
 * 
 * The original DiFX GUI did what to me appears to be a rather strange thing when
 * it came to reading settings from a file - data were dumped into a DiFXObject and
 * then treated by the same structure that handles messages in the DataModel class.
 * The only reason I can imagine to do this is to allow settings to be issued via
 * network messaging, a procedure for which no facilities exist.  My belief that
 * this was rather strange might mean I didn't understand why it was valuable.
 * In any case it has been swept away.  
 * 
 * This class generates events when items are changed.  For instance, any change to
 * database parameters produces a "databaseChangeEvent".  Other classes can set
 * themselves up to listen for these using the "databaseChangeListener()" function.
 */
package edu.nrao.difx.difxview;

import edu.nrao.difx.difxutilities.BareBonesBrowserLaunch;

import edu.nrao.difx.xmllib.difxmessage.*;
import java.io.File;
import javax.swing.event.EventListenerList;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ComponentEvent;
import java.awt.Color;
import java.awt.Point;
import java.util.Calendar;

import java.io.BufferedWriter;
import java.io.IOException;
import java.io.FileWriter;

import java.net.URL;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.io.IOException;

import javax.swing.Action;
import javax.swing.AbstractAction;
import javax.swing.Timer;

import java.util.HashMap;
import java.util.Map;
import java.util.Iterator;
import java.util.ArrayList;

import javax.swing.JFormattedTextField;
import javax.swing.JLabel;
import javax.swing.JButton;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.UIManager;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.JPasswordField;
import javax.swing.JCheckBox;

import mil.navy.usno.widgetlib.NodeBrowserScrollPane;
import mil.navy.usno.widgetlib.IndexedPanel;
import mil.navy.usno.widgetlib.NumberBox;
import mil.navy.usno.plotlib.PlotWindow;
import mil.navy.usno.plotlib.Plot2DObject;
import mil.navy.usno.plotlib.Track2D;
import mil.navy.usno.widgetlib.PingTest;
import mil.navy.usno.widgetlib.MessageScrollPane;
import mil.navy.usno.widgetlib.MessageNode;
import mil.navy.usno.widgetlib.SaneTextField;
import mil.navy.usno.widgetlib.SimpleTextEditor;

import javax.swing.JFrame;

import edu.nrao.difx.difxdatabase.QueueDBConnection;

import java.sql.ResultSet;

public class SystemSettings extends JFrame {
    
    SystemSettings( String settingsFile ) {
        
        //  The "look and feel" isn't a setting in the sense that these others are...it
        //  must be set prior to building menus, so ONLY the default value will be
        //  used.  However it is put here due to a lack of anywhere better to put it.
        //  It is NOT saved as part of the settings file.  All JFrames should call
        //  the "setLookAndFeel()" function before they create any GUI components.
        //
        //  If left as "null" the look and feel will be whatever the local machine
        //  uses.  Although unpredictable, it does give us native file choosers.
        //  Unfortunately it is not wise to mix the look and feel within an application
        //  (thus we can't use the local one only for file choosers) - at least on the 
        //  Mac it causes confusion in the window manager and odd error messages.
        //_lookAndFeel = null;
        //  The "cross platform" look and feel is consistent across all platforms,
        //  which is why I tend to like it.  It gives us ugly and annoying file
        //  choosers though.
        _lookAndFeel = UIManager.getCrossPlatformLookAndFeelClassName();
        
        //  Some organizational structure here - some items are stored in class
        //  structures that are based on where they apply.  We need to create these
        //  structures here.
        _queueBrowserSettings = new QueueBrowserSettings();
        _windowConfiguration = new WindowConfiguration();
        _defaultNames = new DefaultNames();
        
        //  Create all of the components of the user interface (long, messy function).
        createGUIComponents();
        
        //  Set the default settings for all values (these are hard-coded).
        setDefaults();

        //  Try loading settings from the given file.  If this fails, try to load
        //  the default settings file.
        settingsFileName( settingsFile );
        if ( !_settingsFileRead )
            settingsFileName( this.getDefaultSettingsFileName() );
        //  Buttons for dealing with the settings file.
        //this.newSize();
            
        //  This stuff is used to trap resize events.
		this.addComponentListener(new java.awt.event.ComponentAdapter() {
			public void componentResized( ComponentEvent e ) {
				_this.newSize();
			}
			public void componentMoved( ComponentEvent e ) {
				_this.newSize();
			}
		} );
        

    }
    
    /*
     * This function creates all of the components of the user interface for the
     * settings window (there are many components).  It does not initialize values
     * for user-changeable fields (that is done in setDefaults()), nor does it
     * determine the size and position of components unless they are fixed (usually
     * labels and things).  Sizes are set on the fly in the "newSize()" function.
     * The settings are contained in a browsable list of panes, matching the design
     * of the overall DiFX GUI.
     */
    public void createGUIComponents() {
        
        //  Use the universal "look and feel" setting for this window.  This MUST
        //  be used (at least on the Mac) or Java barfs errors and doesn't draw
        //  things correctly.
        this.setLookAndFeel();

        //  One file chooser for all settings operations.  This means it will pop
        //  up with whatever directory the user previously gave it unless otherwise
        //  specified.
        _fileChooser = new JFileChooser();
        
        //  Build a user interface for all settings items and load with default
        //  values.
        _this = this;
        this.setLayout( null );
        this.setSize( 800, 775 );
        this.setTitle( "DiFX GUI Settings" );
        _menuBar = new JMenuBar();
        JMenu helpMenu = new JMenu( "  Help  " );
        _menuBar.add( helpMenu );
        JMenuItem settingsHelpItem = new JMenuItem( "Settings Help" );
        settingsHelpItem.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                launchGUIHelp( "settings.html" );
            }
        } );
        helpMenu.add( settingsHelpItem );
        JMenuItem helpIndexItem = new JMenuItem( "Help Index" );
        helpIndexItem.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                launchGUIHelp( "index.html" );
            }
        } );
        helpMenu.add( helpIndexItem );
        this.add( _menuBar );
        _scrollPane = new NodeBrowserScrollPane();
        _scrollPane.addTimeoutEventListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _this.newSize();
            }
        } );
        this.add( _scrollPane );
        
        IndexedPanel settingsFilePanel = new IndexedPanel( "Settings File" );
        settingsFilePanel.openHeight( 100 );
        settingsFilePanel.closedHeight( 20 );
        _scrollPane.addNode( settingsFilePanel );
        _settingsFileName = new JFormattedTextField();
        _settingsFileName.setFocusLostBehavior( JFormattedTextField.COMMIT );
        settingsFilePanel.add( _settingsFileName );
        JLabel settingsFileLabel = new JLabel( "Current:" );
        settingsFileLabel.setBounds( 10, 25, 100, 25 );
        settingsFileLabel.setHorizontalAlignment( JLabel.RIGHT );
        settingsFilePanel.add( settingsFileLabel );
        JButton loadFromFileButton = new JButton( "Open..." );
        loadFromFileButton.setBounds( 115, 55, 100, 25 );
        loadFromFileButton.setToolTipText( "Open a settings file" );
        loadFromFileButton.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                openSettingsFile();
            }
        } );
        settingsFilePanel.add( loadFromFileButton );
        JButton saveButton = new JButton( "Save" );
        saveButton.setBounds( 220, 55, 100, 25 );
        saveButton.setToolTipText( "Save current settings to the given file name." );
        saveButton.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                saveSettings();
            }
        } );
        settingsFilePanel.add( saveButton );
        JButton saveAsButton = new JButton( "Save As..." );
        saveAsButton.setBounds( 325, 55, 100, 25 );
        saveAsButton.setToolTipText( "Save the current settings to a new file name." );
        saveAsButton.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                saveSettingsAs();
            }
        } );
        settingsFilePanel.add( saveAsButton );
        JButton defaultsButton = new JButton( "Defaults" );
        defaultsButton.setBounds( 430, 55, 100, 25 );
        defaultsButton.setToolTipText( "Reset all settings to internal default values." );
        defaultsButton.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                setDefaults();
            }
        } );
        settingsFilePanel.add( defaultsButton );
        
        IndexedPanel difxControlPanel = new IndexedPanel( "DiFX Control Connection" );
        difxControlPanel.openHeight( 210 );
        difxControlPanel.closedHeight( 20 );
        _scrollPane.addNode( difxControlPanel );
        _difxControlAddress = new JFormattedTextField();
        _difxControlAddress.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _difxControlAddress.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                //generateControlChangeEvent();
            }
        } );
        difxControlPanel.add( _difxControlAddress );
        JLabel ipAddressLabel = new JLabel( "Host:" );
        ipAddressLabel.setBounds( 10, 25, 150, 25 );
        ipAddressLabel.setHorizontalAlignment( JLabel.RIGHT );
        difxControlPanel.add( ipAddressLabel );
        _difxControlPort = new NumberBox();
        _difxControlPort.setHorizontalAlignment( NumberBox.LEFT );
        _difxControlPort.minimum( 0 );
        _difxControlPort.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                //generateControlChangeEvent();
            }
        } );
        difxControlPanel.add( _difxControlPort );
        JLabel portLabel = new JLabel( "Control Port:" );
        portLabel.setBounds( 10, 55, 150, 25 );
        portLabel.setHorizontalAlignment( JLabel.RIGHT );
        difxControlPanel.add( portLabel );
        _difxTransferPort = new NumberBox();
        _difxTransferPort.setHorizontalAlignment( NumberBox.LEFT );
        _difxTransferPort.minimum( 0 );
        difxControlPanel.add( _difxTransferPort );
        JLabel transferPortLabel = new JLabel( "Transfer Port:" );
        transferPortLabel.setBounds( 210, 55, 150, 25 );
        transferPortLabel.setHorizontalAlignment( JLabel.RIGHT );
        difxControlPanel.add( transferPortLabel );
        _difxControlUser = new JFormattedTextField();
        _difxControlUser.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _difxControlUser.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                //generateControlChangeEvent();
            }
        } );
        difxControlPanel.add( _difxControlUser );
        JLabel userAddressLabel = new JLabel( "Username:" );
        userAddressLabel.setBounds( 10, 85, 150, 25 );
        userAddressLabel.setHorizontalAlignment( JLabel.RIGHT );
        difxControlPanel.add( userAddressLabel );
        _difxControlPWD = new JPasswordField();
        _difxControlPWD.setHorizontalAlignment( NumberBox.LEFT );
        _difxControlPWD.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                //generateControlChangeEvent();
            }
        } );
        difxControlPanel.add( _difxControlPWD );
        JLabel pwdLabel = new JLabel( "Password:" );
        pwdLabel.setBounds( 10, 115, 150, 25 );
        pwdLabel.setHorizontalAlignment( JLabel.RIGHT );
        difxControlPanel.add( pwdLabel );
        _difxVersion = new JFormattedTextField();
        _difxVersion.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _difxVersion.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                //generateControlChangeEvent();
            }
        } );
        difxControlPanel.add( _difxVersion );
        JLabel versionAddressLabel = new JLabel( "DiFX Version:" );
        versionAddressLabel.setBounds( 10, 145, 150, 25 );
        versionAddressLabel.setHorizontalAlignment( JLabel.RIGHT );
        difxControlPanel.add( versionAddressLabel );
        _difxPath = new SaneTextField();
        _difxPath.setToolTipText( "Path to DiFX directory tree on the DiFX host (\"bin\" and \"setup\" should be below this)." );
        difxControlPanel.add( _difxPath );
        JLabel difxPathLabel = new JLabel( "DiFX Path:" );
        difxPathLabel.setBounds( 10, 175, 150, 25 );
        difxPathLabel.setHorizontalAlignment( JLabel.RIGHT );
        difxControlPanel.add( difxPathLabel );
        
        IndexedPanel networkPanel = new IndexedPanel( "Broadcast Network" );
        networkPanel.openHeight( 180 );
        networkPanel.closedHeight( 20 );
        _scrollPane.addNode( networkPanel );
        _ipAddress = new JFormattedTextField();
        _ipAddress.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _ipAddress.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                generateBroadcastChangeEvent();
            }
        } );
        networkPanel.add( _ipAddress );
        JLabel difxControlAddressLabel = new JLabel( "Group IP Address:" );
        difxControlAddressLabel.setBounds( 10, 25, 150, 25 );
        difxControlAddressLabel.setHorizontalAlignment( JLabel.RIGHT );
        networkPanel.add( difxControlAddressLabel );
        _port = new NumberBox();
        _port.setHorizontalAlignment( NumberBox.LEFT );
        _port.minimum( 0 );
        _port.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                generateBroadcastChangeEvent();
            }
        } );
        networkPanel.add( _port );
        JLabel controlPortLabel = new JLabel( "Port:" );
        controlPortLabel.setBounds( 10, 55, 150, 25 );
        controlPortLabel.setHorizontalAlignment( JLabel.RIGHT );
        networkPanel.add( controlPortLabel );
        _bufferSize = new NumberBox();
        _bufferSize.setHorizontalAlignment( NumberBox.LEFT );
        _bufferSize.minimum( 0 );
        _bufferSize.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                generateBroadcastChangeEvent();
            }
        } );
        networkPanel.add( _bufferSize );
        JLabel bufferSizeLabel = new JLabel( "Buffer Size:" );
        bufferSizeLabel.setBounds( 10, 85, 150, 25 );
        bufferSizeLabel.setHorizontalAlignment( JLabel.RIGHT );
        networkPanel.add( bufferSizeLabel );
        _timeout = new NumberBox();
        _timeout.setHorizontalAlignment( NumberBox.LEFT );
        _timeout.minimum( 0 );
        _timeout.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                generateBroadcastChangeEvent();
            }
        } );
        networkPanel.add( _timeout );
        JLabel timeoutLabel = new JLabel( "Timeout (ms):" );
        timeoutLabel.setBounds( 10, 115, 150, 25 );
        timeoutLabel.setHorizontalAlignment( JLabel.RIGHT );
        networkPanel.add( timeoutLabel );
        _plotWindow = new PlotWindow();
        _plotWindow.backgroundColor( this.getBackground() );
        networkPanel.add( _plotWindow );
        _broadcastPlot = new Plot2DObject();
        _broadcastPlot.title( "Packet Traffic (bytes/buffer size)", Plot2DObject.LEFT_JUSTIFY );
        _broadcastPlot.titlePosition( 0.0, 4.0 );
        _broadcastTrack = new Track2D();
        _broadcastPlot.addTrack( _broadcastTrack );
        _broadcastTrack.color( Color.GREEN );
        _broadcastTrack.sizeLimit( 1000 );
        _broadcastPlot.frame( 10, 23, 0.95, 90 );
        _broadcastPlot.backgroundColor( Color.BLACK );
        _plotWindow.add2DPlot( _broadcastPlot );
        _suppressWarningsCheck = new JCheckBox( "Suppress \"Unknown Message\" Warnings" );
        _suppressWarningsCheck.setBounds( 165, 145, 450, 25 );
        networkPanel.add( _suppressWarningsCheck );
        
        IndexedPanel databasePanel = new IndexedPanel( "Database Configuration" );
        databasePanel.openHeight( 275 );
        databasePanel.closedHeight( 20 );
        databasePanel.labelWidth( 300 );
        _scrollPane.addNode( databasePanel );
        _dbUseDataBase = new JCheckBox();
        _dbUseDataBase.setBounds( 165, 25, 25, 25 );
        databasePanel.add( _dbUseDataBase );
        JLabel dbUseDataBaseLabel = new JLabel( "Use Data Base:" );
        dbUseDataBaseLabel.setBounds( 10, 25, 150, 25 );
        dbUseDataBaseLabel.setHorizontalAlignment( JLabel.RIGHT );
        databasePanel.add( dbUseDataBaseLabel );
        _dbVersion = new JFormattedTextField();
        _dbVersion.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _dbVersion.setBounds( 285, 25, 180, 25 );
        databasePanel.add( _dbVersion );
        JLabel dbVersionLabel = new JLabel( "Version:" );
        dbVersionLabel.setBounds( 200, 25, 80, 25 );
        dbVersionLabel.setHorizontalAlignment( JLabel.RIGHT );
        databasePanel.add( dbVersionLabel );
        _dbHost = new JFormattedTextField();
        _dbHost.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _dbHost.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                setDbURL();
            }
        } );
        databasePanel.add( _dbHost );
        JLabel dbHostLabel = new JLabel( "Host:" );
        dbHostLabel.setBounds( 10, 55, 150, 25 );
        dbHostLabel.setHorizontalAlignment( JLabel.RIGHT );
        databasePanel.add( dbHostLabel );
        _dbSID = new JFormattedTextField();
        _dbSID.setFocusLostBehavior( JFormattedTextField.COMMIT );
        databasePanel.add( _dbSID );
        _dbSID.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                setDbURL();
            }
        } );
        JLabel dbSIDLabel = new JLabel( "User:" );
        dbSIDLabel.setBounds( 10, 85, 150, 25 );
        dbSIDLabel.setHorizontalAlignment( JLabel.RIGHT );
        databasePanel.add( dbSIDLabel );
        _dbPWD = new JPasswordField();
        _dbPWD.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                generateDatabaseChangeEvent();
            }
        } );
        databasePanel.add( _dbPWD );
        JLabel dbPWDLabel = new JLabel( "Password:" );
        dbPWDLabel.setBounds( 10, 115, 150, 25 );
        dbPWDLabel.setHorizontalAlignment( JLabel.RIGHT );
        databasePanel.add( dbPWDLabel );
        _dbName = new JFormattedTextField();
        _dbName.setFocusLostBehavior( JFormattedTextField.COMMIT );
        databasePanel.add( _dbName );
        JLabel dbNameLabel = new JLabel( "DiFX Database:" );
        dbNameLabel.setBounds( 10, 145, 150, 25 );
        dbNameLabel.setHorizontalAlignment( JLabel.RIGHT );
        databasePanel.add( dbNameLabel );
        _jdbcDriver = new JFormattedTextField();
        _jdbcDriver.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _jdbcDriver.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                generateDatabaseChangeEvent();
            }
        } );
        databasePanel.add( _jdbcDriver );
        JLabel oracleDriverLabel = new JLabel( "JDBC Driver:" );
        oracleDriverLabel.setBounds( 10, 175, 150, 25 );
        oracleDriverLabel.setHorizontalAlignment( JLabel.RIGHT );
        databasePanel.add( oracleDriverLabel );
        _jdbcPort = new JFormattedTextField();
        _jdbcPort.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _jdbcPort.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                setDbURL();
            }
        } );
        databasePanel.add( _jdbcPort );
        JLabel oraclePortLabel = new JLabel( "JDBC Port:" );
        oraclePortLabel.setBounds( 10, 205, 150, 25 );
        oraclePortLabel.setHorizontalAlignment( JLabel.RIGHT );
        databasePanel.add( oraclePortLabel );
        _dbAutoUpdate = new JCheckBox();
        _dbAutoUpdate.setBounds( 165, 235, 25, 25 );
        databasePanel.add( _dbAutoUpdate );
        JLabel dbAutoUpdateLabel = new JLabel( "Periodic Update:" );
        dbAutoUpdateLabel.setBounds( 10, 235, 150, 25 );
        dbAutoUpdateLabel.setHorizontalAlignment( JLabel.RIGHT );
        databasePanel.add( dbAutoUpdateLabel );
        _dbAutoUpdateInterval = new NumberBox();
        _dbAutoUpdateInterval.setBounds( 230, 235, 50, 25 );
        _dbAutoUpdateInterval.minimum( 1.0 );
        databasePanel.add( _dbAutoUpdateInterval );
        JLabel dbAutoUpdateIntervalLabel1 = new JLabel( "every" );
        dbAutoUpdateIntervalLabel1.setBounds( 130, 235, 95, 25 );
        dbAutoUpdateIntervalLabel1.setHorizontalAlignment( JLabel.RIGHT );
        databasePanel.add( dbAutoUpdateIntervalLabel1 );
        JLabel dbAutoUpdateIntervalLabel2 = new JLabel( "seconds" );
        dbAutoUpdateIntervalLabel2.setBounds( 285, 235, 65, 25 );
        databasePanel.add( dbAutoUpdateIntervalLabel2 );
        _pingHostButton = new JButton( "Ping Host" );
        _pingHostButton.setToolTipText( "ping the database host" );
        _pingHostButton.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                pingDatabaseHost();
            }
        } );
        databasePanel.add( _pingHostButton );
        _testDatabaseButton = new JButton( "Test Database" );
        _testDatabaseButton.setToolTipText( "Run a connection test using the current database settings." );
        _testDatabaseButton.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                testDatabaseAction();
            }
        } );
        databasePanel.add( _testDatabaseButton );
        _databaseMessages = new MessageScrollPane();
        databasePanel.add( _databaseMessages );
         
        _addressesPanel = new IndexedPanel( "Documentation Locations" );
        _addressesPanel.openHeight( 155 );
        _addressesPanel.closedHeight( 20 );
        _addressesPanel.labelWidth( 250 );
        JLabel guiDocPathLabel = new JLabel( "GUI Docs:" );
        guiDocPathLabel.setBounds( 10, 25, 100, 25 );
        guiDocPathLabel.setHorizontalAlignment( JLabel.RIGHT );
        guiDocPathLabel.setToolTipText( "Directory (or web address) containing all GUI documentation." );
        _guiDocPath = new JFormattedTextField();
        _guiDocPath.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _guiDocPath.setToolTipText( "Directory (or web address) containing all GUI documentation." );
        _addressesPanel.add( guiDocPathLabel );
        _addressesPanel.add( _guiDocPath );
        _guiDocPathBrowseButton = new JButton( "Browse..." );
        _guiDocPathBrowseButton.setToolTipText( "Browse the local directory structure for the location of documentation." );
        _guiDocPathBrowseButton.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                guiDocPathBrowse();
            }
        } );
        _addressesPanel.add( _guiDocPathBrowseButton );
        JLabel difxUsersGroupURLLabel = new JLabel( "Users Group:" );
        difxUsersGroupURLLabel.setBounds( 10, 55, 100, 25 );
        difxUsersGroupURLLabel.setHorizontalAlignment( JLabel.RIGHT );
        difxUsersGroupURLLabel.setToolTipText( "URL of the DiFX Users Group." );
        _difxUsersGroupURL = new JFormattedTextField();
        _difxUsersGroupURL.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _difxUsersGroupURL.setToolTipText( "URL of the DiFX Users Group." );
        _addressesPanel.add( difxUsersGroupURLLabel );
        _addressesPanel.add( _difxUsersGroupURL );
        JLabel difxWikiURLLabel = new JLabel( "DiFX Wiki:" );
        difxWikiURLLabel.setBounds( 10, 85, 100, 25 );
        difxWikiURLLabel.setHorizontalAlignment( JLabel.RIGHT );
        difxWikiURLLabel.setToolTipText( "URL of the DiFX Wiki." );
        _difxWikiURL = new JFormattedTextField();
        _difxWikiURL.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _difxWikiURL.setToolTipText( "URL of the DiFX Wiki." );
        _addressesPanel.add( difxWikiURLLabel );
        _addressesPanel.add( _difxWikiURL );
        JLabel difxSVNLabel = new JLabel( "DiFX SVN:" );
        difxSVNLabel.setBounds( 10, 115, 100, 25 );
        difxSVNLabel.setHorizontalAlignment( JLabel.RIGHT );
        difxSVNLabel.setToolTipText( "URL of the DiFX Subversion repository." );
        _difxSVN = new JFormattedTextField();
        _difxSVN.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _difxSVN.setToolTipText( "URL of the DiFX Subversion repository." );
        _addressesPanel.add( difxSVNLabel );
        _addressesPanel.add( _difxSVN );

        _scrollPane.addNode( _addressesPanel );
        
        IndexedPanel jobSettingsPanel = new IndexedPanel( "Job Settings" );
        jobSettingsPanel.openHeight( 85 );
        jobSettingsPanel.closedHeight( 20 );
        jobSettingsPanel.labelWidth( 300 );
        _scrollPane.addNode( jobSettingsPanel );
        JLabel workingDirectoryLabel = new JLabel( "Working Directory:" );
        workingDirectoryLabel.setBounds( 10, 25, 150, 25 );
        workingDirectoryLabel.setHorizontalAlignment( JLabel.RIGHT );
        workingDirectoryLabel.setToolTipText( "Root directory on the DiFX host for Experiment data." );
        _workingDirectory = new JFormattedTextField();
        _workingDirectory.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _workingDirectory.setToolTipText( "Root directory on the DiFX host for Experiment data." );
        jobSettingsPanel.add( workingDirectoryLabel );
        jobSettingsPanel.add( _workingDirectory );
        _stagingArea = new JFormattedTextField();
        _stagingArea.setFocusLostBehavior( JFormattedTextField.COMMIT );
        _stagingArea.setToolTipText( "Staging area root directory on the DiFX host." );
        jobSettingsPanel.add( _stagingArea );
        JLabel useStagingAreaLabel = new JLabel( "Use Staging Area:" );
        useStagingAreaLabel.setBounds( 10, 55, 120, 25 );
        useStagingAreaLabel.setHorizontalAlignment( JLabel.RIGHT );
        useStagingAreaLabel.setToolTipText( "Use the staging area to run jobs (or don't)." );
        _useStagingArea = new JCheckBox( "" );
        _useStagingArea.setBounds( 133, 55, 25, 25 );
        _useStagingArea.setToolTipText( "Use the staging area to run jobs (or don't)." );
        jobSettingsPanel.add( useStagingAreaLabel );
        jobSettingsPanel.add( _useStagingArea );
        
        IndexedPanel eopSettingsPanel = new IndexedPanel( "EOP Settings" );
        //  These editors may or may not be displayed, but they are used to hold
        //  EOP and leap second data regardless.
        _eopText = new SimpleTextEditor();
        _leapSecondText = new SimpleTextEditor();
        eopSettingsPanel.openHeight( 145 );
        eopSettingsPanel.closedHeight( 20 );
        eopSettingsPanel.labelWidth( 300 );
        _scrollPane.addNode( eopSettingsPanel );
        _eopURL = new SaneTextField();
        _eopURL.setToolTipText( "URL of the file containing EOP data." );
        _eopURL.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateEOPNow();
            }
        } );
        eopSettingsPanel.add( _eopURL );
        JLabel eopURLLabel = new JLabel( "EOP URL:" );
        eopURLLabel.setBounds( 10, 25, 150, 25 );
        eopURLLabel.setHorizontalAlignment( JLabel.RIGHT );
        eopSettingsPanel.add( eopURLLabel );
        _viewEOPFile = new JButton( "View" );
        _viewEOPFile.setToolTipText( "View the contents of the most recent EOP file (load new data if necessary)." );
        _viewEOPFile.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                if ( _eopDisplay == null ) {
                    _eopDisplay = new JFrame();
                    _eopDisplay.setTitle( "Current EOP Data" );
                    Point pt = _viewEOPFile.getLocationOnScreen();
                    _eopDisplay.setBounds( pt.x + 100, pt.y + 50, 500, 500 );
                    _eopDisplay.add( _eopText );
                }
                if ( _eopText.text() == null || _eopText.text().length() == 0 )
                    updateEOPNow();
                _eopDisplay.setVisible( true );
            }
        } );
        eopSettingsPanel.add( _viewEOPFile );
        _leapSecondsURL = new SaneTextField();
        _leapSecondsURL.setToolTipText( "URL of the file containing leap second data." );
        _leapSecondsURL.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateEOPNow();
            }
        } );
        eopSettingsPanel.add( _leapSecondsURL );
        _viewLeapSecondsFile = new JButton( "View" );
        _viewLeapSecondsFile.setToolTipText( "View the contents of the most recent leap seconds file (load new data if necessary)." );
        _viewLeapSecondsFile.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                if ( _leapSecondDisplay == null ) {
                    _leapSecondDisplay = new JFrame();
                    _leapSecondDisplay.setTitle( "Current Leap Second Data" );
                    Point pt = _viewLeapSecondsFile.getLocationOnScreen();
                    _leapSecondDisplay.setBounds( pt.x + 100, pt.y + 50, 500, 500 );
                    _leapSecondDisplay.add( _leapSecondText );
                }
                if ( _leapSecondText.text() == null || _leapSecondText.text().length() == 0 )
                    updateEOPNow();
                _leapSecondDisplay.setVisible( true );
            }
        } );
        eopSettingsPanel.add( _viewLeapSecondsFile );
        JLabel leapSecondsURLLabel = new JLabel( "Leap Seconds URL:" );
        leapSecondsURLLabel.setBounds( 10, 55, 130, 25 );
        leapSecondsURLLabel.setHorizontalAlignment( JLabel.RIGHT );
        eopSettingsPanel.add( leapSecondsURLLabel );
        _useLeapSecondsURL = new JCheckBox( "" );
        _useLeapSecondsURL.setBounds( 140, 55, 20, 25 );
        _useLeapSecondsURL.setToolTipText( "Obtain leap second data from the given URL." );
        _useLeapSecondsURL.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                leapSecondChoice( _useLeapSecondsURL );
            }
        } );
        eopSettingsPanel.add( _useLeapSecondsURL );
        JLabel leapSecondsValueLabel = new JLabel( "static value:" );
        leapSecondsValueLabel.setBounds( 10, 85, 130, 25 );
        leapSecondsValueLabel.setHorizontalAlignment( JLabel.RIGHT );
        eopSettingsPanel.add( leapSecondsValueLabel );
        _useLeapSecondsValue = new JCheckBox( "" );
        _useLeapSecondsValue.setBounds( 140, 85, 20, 25 );
        _useLeapSecondsValue.setToolTipText( "Use the given static value as the number of leap seconds." );
        _useLeapSecondsValue.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                leapSecondChoice( _useLeapSecondsValue );
            }
        } );
        eopSettingsPanel.add( _useLeapSecondsValue );
        _leapSecondsValue = new NumberBox();
        _leapSecondsValue.setBounds( 165, 85, 120, 25 );
        _leapSecondsValue.precision( 0 );
        eopSettingsPanel.add( _leapSecondsValue );
        _autoUpdateEOP = new JCheckBox( "" );
        _autoUpdateEOP.setBounds( 140, 115, 20, 25 );
        _autoUpdateEOP.setToolTipText( "Periodically update the EOP and leap second data." );
        _autoUpdateEOP.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                if ( _autoUpdateEOP.isSelected() ) {
                    updateEOPNow();
                    _eopTimer.start();
                }
                else {
                    _eopTimer.stop();
                }
            }
        } );
        eopSettingsPanel.add( _autoUpdateEOP );
        JLabel autoUpdateLabel = new JLabel( "auto update:" );
        autoUpdateLabel.setBounds( 10, 115, 130, 25 );
        autoUpdateLabel.setHorizontalAlignment( JLabel.RIGHT );
        eopSettingsPanel.add( autoUpdateLabel );
        _autoUpdateSeconds = new NumberBox();
        _autoUpdateSeconds.setBounds( 220, 115, 120, 25 );
        _autoUpdateSeconds.precision( 0 );
        _autoUpdateSeconds.minimum( 1 );
        _autoUpdateSeconds.setToolTipText( "Number of seconds between automatic updates of EOP and leap second data." );
        eopSettingsPanel.add( _autoUpdateSeconds );
        JLabel autoUpdateLabel1 = new JLabel( "every:" );
        autoUpdateLabel1.setBounds( 160, 115, 55, 25 );
        autoUpdateLabel1.setHorizontalAlignment( JLabel.RIGHT );
        eopSettingsPanel.add( autoUpdateLabel1 );
        JLabel autoUpdateLabel2 = new JLabel( "seconds" );
        autoUpdateLabel2.setBounds( 345, 115, 100, 25 );
        eopSettingsPanel.add( autoUpdateLabel2 );
        _updateEOPNow = new JButton( "Update Now" );
        _updateEOPNow.setToolTipText( "Update the EOP and leap second data now." );
        _updateEOPNow.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateEOPNow();
            }
        } );
        eopSettingsPanel.add( _updateEOPNow );
        Action updateDrawingAction = new AbstractAction() {
            @Override
            public void actionPerformed( ActionEvent e ) {
                ++_eopTimerCount;
                if ( _eopTimerCount >= _autoUpdateSeconds.intValue() )
                    updateEOPNow();
            }
        };
        _eopTimer = new Timer( 1000, updateDrawingAction );

        _allObjectsBuilt = true;
        
        //  This seems to be required to get the browser to draw the first time.
        //  Annoying and kludgey, but harmless.
        this.newSize();
        
    }
    
    @Override
    public void setBounds( int x, int y, int w, int h ) {
        super.setBounds( x, y, w, h );
        newSize();
    }
    
    public void newSize() {
        int w = this.getWidth();
        int h = this.getHeight();
        if ( _menuBar != null )
            _menuBar.setBounds( 0, 0, w, 25 );
        if ( _allObjectsBuilt ) {
            _scrollPane.setBounds( 0, 25, w, h - 47 );
            _settingsFileName.setBounds( 115, 25, w - 135, 25 );
            //  DiFX Controll Connection settings
            _difxControlAddress.setBounds( 165, 25, 300, 25 );
            _difxControlPort.setBounds( 165, 55, 100, 25 );
            _difxTransferPort.setBounds( 365, 55, 100, 25 );
            _difxControlUser.setBounds( 165, 85, 300, 25 );
            _difxControlPWD.setBounds( 165, 115, 300, 25 );
            _difxVersion.setBounds( 165, 145, 300, 25 );
            _difxPath.setBounds( 165, 175, w - 195, 25 );
            //  Broadcast network settings
            _ipAddress.setBounds( 165, 25, 300, 25 );
            _port.setBounds( 165, 55, 300, 25 );
            _bufferSize.setBounds( 165, 85, 300, 25 );
            _timeout.setBounds( 165, 115, 300, 25 );
            _plotWindow.setBounds( 470, 25, w - 495, 120 );
            //  Database Configuration
            _dbHost.setBounds( 165, 55, 300, 25 );
            _dbSID.setBounds( 165, 85, 300, 25 );
            _dbPWD.setBounds( 165, 115, 300, 25 );
            _dbName.setBounds( 165, 145, 300, 25 );
            _jdbcDriver.setBounds( 165, 175, 300, 25 );
            _jdbcPort.setBounds( 165, 205, 300, 25 );
            _pingHostButton.setBounds( 480, 55, 125, 25 );
            _testDatabaseButton.setBounds( 610, 55, 125, 25 );
            _databaseMessages.setBounds( 480, 85, w - 505, 145 );
            //  Documentation Addresses
            _guiDocPath.setBounds( 115, 25, w - 240, 25 );
            _guiDocPathBrowseButton.setBounds( w - 120, 25, 100, 25 );
            _difxUsersGroupURL.setBounds( 115, 55, w - 135, 25 );
            _difxWikiURL.setBounds( 115, 85, w - 135, 25 );
            _difxSVN.setBounds( 115, 115, w - 135, 25 );
            //  Job settings
            _workingDirectory.setBounds( 165, 25, w - 185, 25 );
            _stagingArea.setBounds( 165, 55, w - 185, 25 );
            _eopURL.setBounds( 165, 25, w - 295, 25 );
            _viewEOPFile.setBounds( w - 125, 25, 100, 25 );
            _leapSecondsURL.setBounds( 165, 55, w - 295, 25 );
            _viewLeapSecondsFile.setBounds( w - 125, 55, 100, 25 );
            _updateEOPNow.setBounds( w - 250, 115, 120, 25 );
        }
    }
    
    /*
     * Called when one of the checks associated with leap seconds is picked.
     */
    protected void leapSecondChoice( JCheckBox check ) {
        if ( check == _useLeapSecondsURL ) {
            _useLeapSecondsURL.setSelected( true );
            _useLeapSecondsValue.setSelected( false );
            _leapSecondsURL.setEnabled( true );
            _leapSecondsValue.setEnabled( false );
            updateEOPNow();
        }
        else {
            _useLeapSecondsURL.setSelected( false );
            _useLeapSecondsValue.setSelected( true );
            _leapSecondsURL.setEnabled( false );
            _leapSecondsValue.setEnabled( true );
        }
    }
    
    /*
     * Called whenever an update is required for the EOP and leap second data.
     */
    protected void updateEOPNow() {
        //  Read the specified EOP data.
        try {
            URL url = new URL( _eopURL.getText() );
            url.openConnection();
            InputStream reader = url.openStream();
            byte[] buffer = new byte[100000];
            int bytesRead = 0;
            _eopText.text( "" );
            while ( ( bytesRead = reader.read( buffer, 0, 99999 ) ) > 0 ) {
                _eopText.addText( new String( buffer ).substring( 0, bytesRead ) );
        }
        } catch ( IOException e ) {
            java.util.logging.Logger.getLogger( "global" ).log( java.util.logging.Level.SEVERE,
                "EOP URL " + _eopURL.getText() + " triggered exception: " + e.getMessage() );
        }
        //  If the leap second data is being read from a URL, do that.
        if ( _useLeapSecondsURL.isSelected() ) {
            try {
                URL url = new URL( _leapSecondsURL.getText() );
                url.openConnection();
                InputStream reader = url.openStream();
                byte[] buffer = new byte[100000];
                int bytesRead = 0;
                _leapSecondText.text( "" );
                while ( ( bytesRead = reader.read( buffer, 0, 99999 ) ) > 0 ) {
                    _leapSecondText.addText( new String( buffer ).substring( 0, bytesRead ) );
            }
            } catch ( IOException e ) {
                java.util.logging.Logger.getLogger( "global" ).log( java.util.logging.Level.SEVERE,
                    "Leap Second URL " + _leapSecondsURL.getText() + " triggered exception: " + e.getMessage() );
            }
        }
        _eopTimerCount = 0;
        //  In case anyone is out there listening, generate callbacks indicating
        //  new EOP data exist.
        generateEOPChangeEvent();
        eopData( 2447302.0 - 3.0, 2447302.0 + 3.0 );
    }
    
    public class EOPStructure {
        double date;     //  Julian day
        double tai_utc;  //  Leap second for the day
        double xPole;    //  X pole coordinate in 0.1 arcsec
        double yPole;    //  Y pole coordinate in 0.1 arcsec
        double ut1_tai;  //  UT1 - TAI in microseconds of time
        double sXPole;   //  X pole uncertainty in 0.1 arcsec
        double sYPole;   //  Y pole uncertainty in 0.1 arcsec
        double sUt1_tai; //  UT1-TAI uncertainty in microseconds of time
    }

    /*
     * Generate an array list of structures containing EOP data for a specified range
     * of days.
     */
    public ArrayList<EOPStructure> eopData( double before, double after ) {
        ArrayList<EOPStructure> newList = new ArrayList<EOPStructure>();
        int fromIndex = 0;
        String content = _eopText.text();
        int toIndex = content.indexOf( '\n', 0 );
        while ( toIndex != -1 ) {
            try {
                double testDate = Double.parseDouble( content.substring( fromIndex, fromIndex + 9 ) );
                if ( testDate > before && testDate < after ) {
                    EOPStructure newEOP = new EOPStructure();
                    newEOP.date = testDate;
                    newEOP.tai_utc = leapSecond( testDate );
                    newEOP.xPole = Double.parseDouble( _eopText.text().substring( fromIndex + 10, fromIndex + 17 ) );
                    newEOP.yPole = Double.parseDouble( _eopText.text().substring( fromIndex + 18, fromIndex + 26 ) );
                    newEOP.ut1_tai = Double.parseDouble( _eopText.text().substring( fromIndex + 26, fromIndex + 35 ) );
                    newEOP.sXPole = Double.parseDouble( _eopText.text().substring( fromIndex + 36, fromIndex + 42 ) );
                    newEOP.sYPole = Double.parseDouble( _eopText.text().substring( fromIndex + 43, fromIndex + 49 ) );
                    newEOP.sUt1_tai = Double.parseDouble( _eopText.text().substring( fromIndex + 50, fromIndex + 57 ) );
                    newList.add( newEOP );
                }
            } catch ( java.lang.NumberFormatException e ) {
                //  We expect a few of these...comments, etc.
            }
            fromIndex = toIndex + 1;
            toIndex = content.indexOf( '\n', fromIndex );
        }
        return newList;
    }
    
    /*
     * Generate a leap second from the leap second data for a specific Julian date.
     */
    public double leapSecond( double date ) {
        //  See if we are using a constant value...in which case we just return
        //  that.
        if ( _useLeapSecondsValue.isSelected() ) {
            return _leapSecondsValue.value();
        }
        else {
            //  Plod through the leap second data looking for the correct date.
            if ( _leapSecondText.text() == null || _leapSecondText.text().length() == 0 ) {
                java.util.logging.Logger.getLogger( "global" ).log( java.util.logging.Level.SEVERE,
                    "No current leap second data available." );
                return 0.0;
            }
            else {
                //  Plod through the leap second data looking for the nearest date
                //  before the date we are looking for.
                int fromIndex = 0;
                int toIndex = _leapSecondText.text().indexOf( '\n', 0 );
                boolean found = false;
                double testDay = 0.0;
                double testLeap = 0.0;
                int count = 0;
                while ( toIndex != -1 && !found ) {
                    testDay = Double.parseDouble( _leapSecondText.text().substring( fromIndex + 17, fromIndex + 27 ) );
                    if ( testDay > date )
                        found = true;
                    testLeap = Double.parseDouble( _leapSecondText.text().substring( fromIndex + 38, fromIndex + 49 ) );
                    fromIndex = toIndex + 1;
                    toIndex = _leapSecondText.text().indexOf( '\n', fromIndex );
                    ++count;
                }
                //  See what happened...
                if ( found && count == 0 ) {
                    java.util.logging.Logger.getLogger( "global" ).log( java.util.logging.Level.SEVERE,
                        "Request for Julian date (" + date + ") prior to earliest date in leap second data ("
                            + testDay + ")." );
                }
                else if ( !found && count == 0 ) {
                    java.util.logging.Logger.getLogger( "global" ).log( java.util.logging.Level.SEVERE,
                        "Leap second data appears to be empty." );
                }
                else if ( !found ) {
                    java.util.logging.Logger.getLogger( "global" ).log( java.util.logging.Level.SEVERE,
                        "Request for Julian date (" + date + ") after last date in leap second data ("
                            + testDay + ")." );
                }
                return testLeap;
            }
        }
    }
    
    /*
     * Open a new file containing settings.  This uses the file chooser.
     */
    public void openSettingsFile() {
        _fileChooser.setDialogTitle( "Open System Settings File..." );
        _fileChooser.setFileSelectionMode( JFileChooser.FILES_AND_DIRECTORIES );
        _fileChooser.setApproveButtonText( "Open" );
        _fileChooser.setCurrentDirectory( new File( this.settingsFileName() ) );
        int ret = _fileChooser.showOpenDialog( this );
        if ( ret == JFileChooser.APPROVE_OPTION )
            this.settingsFileName( _fileChooser.getSelectedFile().getAbsolutePath() );
    }
    
    /*
     * Browse for the directory that contains GUI documentation.
     */
    public void guiDocPathBrowse() {
        _fileChooser.setDialogTitle( "GUI Documentation Directory..." );
        _fileChooser.setFileSelectionMode( JFileChooser.DIRECTORIES_ONLY );
        _fileChooser.setApproveButtonText( "Select" );
        int ret = _fileChooser.showOpenDialog( this );
        if ( ret == JFileChooser.APPROVE_OPTION )
            _guiDocPath.setText( "file://" + _fileChooser.getSelectedFile().getAbsolutePath() );
    }
    
    /*
     * Save setting to the current settings file.  If this is null, go to "saveSettingsAs()".
     */
    public void saveSettings() {
        if ( _settingsFileName.getText().equals( this.getDefaultSettingsFileName() ) ) {
            Object[] options = { "Continue", "Cancel", "Save to..." };
            int ans = JOptionPane.showOptionDialog( this, 
                    "This will overwrite the default settings file!\nAre you sure you want to do this?",
                    "Overwrite Defaults Warning",
                    JOptionPane.YES_NO_CANCEL_OPTION, JOptionPane.WARNING_MESSAGE, null, options, options[2] );
            if ( ans == 0 )
                saveSettingsToFile( _settingsFileName.getName() );
            else if ( ans == 1 )
                return;
            else
                saveSettingsAs();
        }
        else
            saveSettingsToFile( _settingsFileName.getName() );
    }
    
    /*
     * Open a file chooser to pick a file for saving the current settings.
     */
    public void saveSettingsAs() {
        _fileChooser.setDialogTitle( "Save System Settings to File..." );
        int ret = _fileChooser.showSaveDialog( this );
        if ( ret == JFileChooser.APPROVE_OPTION )
            saveSettingsToFile( _fileChooser.getSelectedFile().getAbsolutePath() );
    }
    
    /*
     * Set all settings to their internal system defaults.
     */
    public void setDefaults() {
        _jaxbPackage = "edu.nrao.difx.xmllib.difxmessage";
        _home = "/home/swc/difx";
        //_resourcesFile = "/cluster/difx/DiFX_trunk_64/conf/resources.difx";
        _loggingEnabled = false;
        _statusValidDuration = 2000l;
        _ipAddress.setText( "224.2.2.1" );
        _port.intValue( 52525 );
        _bufferSize.intValue( 1500 );
        _timeout.intValue( 100 );
        _difxControlAddress.setText( "swc01.usno.navy.mil" );
        _difxControlPort.intValue( 50200 );
        _difxTransferPort.intValue( 50300 );
        _difxControlUser.setText( "difx" );
        _difxControlPWD.setText( "difx2010" );
        _difxVersion.setText( "trunk" );
        _difxPath.setText( "/usr/local/swc/difx" );
        _dbUseDataBase.setSelected( true );
        _dbVersion.setText( "unknown" );
        _dbHost.setText( "swc02.usno.navy.mil" );
        _dbSID.setText( "difx" );
        _dbPWD.setText( "difx2010" );
        _dbName.setText( "difxdb" );
        _jdbcDriver.setText( "com.mysql.jdbc.Driver" );
        _jdbcPort.setText( "3306" );
        this.setDbURL();
        _reportLoc = "/users/difx/Desktop";
        _guiDocPath.setText( "file://" + System.getProperty( "user.dir" ) + "/doc" );
        _difxUsersGroupURL.setText( "http://groups.google.com/group/difx-users/topics" );
        _difxWikiURL.setText( "http://cira.ivec.org/dokuwiki/doku.php/difx/start" );
        _difxSVN.setText( "https://svn.atnf.csiro.au/trac/difx" );
        _dbAutoUpdate.setSelected( false );
        _dbAutoUpdateInterval.intValue( 10 );
        _workingDirectory.setText( "/" );
        _stagingArea.setText( "/queue" );
        _useStagingArea.setSelected( false );
        _queueBrowserSettings.showCompleted = true;
        _queueBrowserSettings.showIncomplete = true;
        _queueBrowserSettings.showSelected = true;
        _queueBrowserSettings.showUnselected = true;
        _windowConfiguration.mainX = 0;
        _windowConfiguration.mainY = 0;
        _windowConfiguration.mainW = 1400;
        _windowConfiguration.mainH = 800;
        _windowConfiguration.verticalPanels = false;
        _windowConfiguration.mainDividerLocation = 650;
        _windowConfiguration.topDividerLocation = 700;
        _windowConfiguration.queueBrowserTearOff = false;
        _windowConfiguration.queueBrowserX = 0;
        _windowConfiguration.queueBrowserY = 0;
        _windowConfiguration.queueBrowserW = 350;
        _windowConfiguration.queueBrowserH = 600;
        _windowConfiguration.hardwareMonitorTearOff = false;
        _windowConfiguration.hardwareMonitorX = 0;
        _windowConfiguration.hardwareMonitorY = 0;
        _windowConfiguration.hardwareMonitorW = 350;
        _windowConfiguration.hardwareMonitorH = 600;
        _windowConfiguration.experimentEditorW = 500;
        _windowConfiguration.experimentEditorH = 430;
        _windowConfiguration.settingsWindowW = 800;
        _windowConfiguration.settingsWindowH = 775;
        this.setSize( _windowConfiguration.settingsWindowW, _windowConfiguration.settingsWindowH );
        _windowConfiguration.jobEditorMonitorWindowW = 900;
        _windowConfiguration.jobEditorMonitorWindowW = 500;
        _defaultNames.vexFileSource = "";
        _defaultNames.viaHttpLocation = "";
        _defaultNames.viaFtpLocation = "";
        _defaultNames.localFileLocation = "";
        _defaultNames.createPassOnExperimentCreation = true;
        _defaultNames.singleInputFile = false;
        _defaultNames.scanBasedJobNames = true;
        _defaultNames.dirListLocation = "";
        _defaultNames.jobCreationSanityCheck = true;
        _eopURL.setText( "http://gemini.gsfc.nasa.gov/solve_save/usno_finals.erp" );
        _leapSecondsURL.setText( "http://gemini.gsfc.nasa.gov/500/oper/solve_apriori_files/ut1ls.dat" );
        _leapSecondsValue.value( 34 );
        _autoUpdateSeconds.value( 3600 );
        leapSecondChoice( _useLeapSecondsURL );
        _autoUpdateEOP.setSelected( true );
        _eopTimer.start();
    }
    
    /*
     * Close this window.  At the moment this operation is not complicated.
     */
    public void closeWindow() {
        this.setVisible( false );
    }

    public void settingsFileName( String newVal ) {
        _settingsFileName.setText( newVal );
        //  Attempt to read the new settings.
        _settingsFileRead = getSettingsFromFile( settingsFileName() );
    }
    public String settingsFileName() { return _settingsFileName.getText(); }
    
    public void jaxbPackage( String newVal ) { _jaxbPackage = newVal; }
    public String jaxbPackage() { return _jaxbPackage; }
    
    public void home( String newVal ) { _home = newVal; }
    public String home() { return _home; }
    
    //public void resourcesFile( String newVal ) { _resourcesFile = newVal; }
    //public String resourcesFile() { return _resourcesFile; }
    
    public void loggingEnabled( boolean newVal ) { _loggingEnabled = newVal; }
    public boolean loggingEnabled() { return _loggingEnabled; }
    
    public void statusValidDuration( long newVal ) { _statusValidDuration = newVal; }
    public long statusValidDuration() { return _statusValidDuration; }
    
    public void difxControlAddress( String newVal ) { _difxControlAddress.setText( newVal ); }
    public String difxControlAddress() { return _difxControlAddress.getText(); }
    
    public void difxControlPort( int newVal ) { _difxControlPort.intValue( newVal ); }
    public int difxControlPort() { return _difxControlPort.intValue(); }
    public void difxControlPort( String newVal ) { difxControlPort( Integer.parseInt( newVal ) ); }

    /*
     * Set the TCP "transfer" port.  The actual port used will increment up to 100 more than
     * this setting, at which point it will cycle back and use the original.  The
     * idea is that we want to be able to have multiple TCP sessions open at once.
     */
    public void difxTransferPort( int newVal ) { _difxTransferPort.intValue( newVal ); }
    public int difxTransferPort() { return _difxTransferPort.intValue(); }
    public void difxTransferPort( String newVal ) { difxTransferPort( Integer.parseInt( newVal ) ); }
    public int newDifxTransferPort() {
        ++_newDifxTransferPort;
        if ( _newDifxTransferPort > 100 )
            _newDifxTransferPort = 0;
        return _newDifxTransferPort + _difxTransferPort.intValue();
    }

    public void difxControlUser( String newVal ) { _difxControlUser.setText( newVal ); }
    public String difxControlUser() { return _difxControlUser.getText(); }
    
    public void difxControlPassword( String newVal ) { _difxControlPWD.setText( newVal ); }
    public String difxControlPassword() { return new String( _difxControlPWD.getPassword() ); }
    
    public void difxVersion( String newVal ) { _difxVersion.setText( newVal ); }
    public String difxVersion() { return _difxVersion.getText(); }
    
    public void difxPath( String newVal ) { _difxPath.setText( newVal ); }
    public String difxPath() { return _difxPath.getText(); }
    
    public void ipAddress( String newVal ) { _ipAddress.setText( newVal ); }
    public String ipAddress() { return _ipAddress.getText(); }
    
    public void port( int newVal ) { _port.intValue( newVal ); }
    public int port() { return _port.intValue(); }
    public void port( String newVal ) { port( Integer.parseInt( newVal ) ); }
    
    public void bufferSize( int newVal ) { _bufferSize.intValue( newVal ); }
    public int bufferSize() { return _bufferSize.intValue(); }
    public void bufferSize( String newVal ) { bufferSize( Integer.parseInt( newVal ) ); }
    
    public void timeout( int newVal ) { _timeout.intValue( newVal ); }
    public int timeout() { return _timeout.intValue(); }
    public void timeout( String newVal ) { timeout( Integer.parseInt( newVal ) ); }
    
    public boolean useDataBase() { return _dbUseDataBase.isSelected(); }
    public String dbVersion() { return _dbVersion.getText(); }
    public void dbHost( String newVal ) { 
        _dbHost.setText( newVal );
        setDbURL();
    }
    public String dbHost() { return _dbHost.getText(); }
    
    public void dbSID( String newVal ) { 
        _dbSID.setText( newVal );
        setDbURL();
    }
    public String dbSID() { return _dbSID.getText(); }
    
    public void dbPWD( String newVal ) { 
        _dbPWD.setText( newVal );
        generateDatabaseChangeEvent();
    }
    public String dbPWD() { return new String( _dbPWD.getPassword() ); }
    
    public void dbName( String newVal ) { 
        _dbName.setText( newVal );
    }
    public String dbName() { return _dbName.getText(); }
    
    public void jdbcDriver( String newVal ) { 
        _jdbcDriver.setText( newVal );
        generateDatabaseChangeEvent();
    }
    public String jdbcDriver() { return _jdbcDriver.getText(); }
    
    public void jdbcPort( String newVal ) { 
        _jdbcPort.setText( newVal );
        setDbURL();
    }
    public String jdbcPort() { return _jdbcPort.getText(); }
    
    public String dbURL() { return _dbURL; }
    public void dbURL( String newVal ) { 
        _dbURL = newVal;
        generateDatabaseChangeEvent();
    }
    protected void setDbURL() {
        //  Sets the dbURL using other items - this is not accessible to the outside.
        //_dbURL = "jdbc:oracle:thin:@" + _dbHost.getText() + ":" + _oracleJdbcPort.getText() + 
        _dbURL = "jdbc:mysql://" + _dbHost.getText() + ":" + _jdbcPort.getText() + "/mysql";
        generateDatabaseChangeEvent();
    }
    public boolean dbAutoUpdate() { return _dbAutoUpdate.isSelected(); }
    public void dbAutoUpdate( boolean newVal ) { _dbAutoUpdate.setSelected( newVal ); }
    public int dbAutoUpdateInterval() { return _dbAutoUpdateInterval.intValue(); }
    
    public String workingDirectory() { return _workingDirectory.getText(); }
    public void workingDirectory( String newVal ) { _workingDirectory.setText( newVal ); }
    
    public String stagingArea() { return _stagingArea.getText(); }
    public void stagingArea( String newVal ) { _stagingArea.setText( newVal ); }
    
    public boolean useStagingArea() { return _useStagingArea.isSelected(); }
    public void useStagingArea( boolean newVal ) { _useStagingArea.setSelected( newVal ); }
    
    public String guiDocPath() { return _guiDocPath.getText().substring( 7 ); }
    /*
     * Set the look and feel for a new JFrame.  This needs to be called before any
     * GUI components are created.
     */
    public void setLookAndFeel() {
        //  Don't do anything if the look and feel is "null".
        if ( _lookAndFeel == null )
            return;
        try {
            UIManager.setLookAndFeel( UIManager.getCrossPlatformLookAndFeelClassName() );
        }
        catch ( Exception e ) {
            //  This thing throws exceptions, but we ignore them.  Shouldn't hurt
            //  us - if the look and feel isn't set, the default should at least
            //  be visible.
        }
    }
    
    public void launchGUIHelp( String topicAddress ) {
        File file = new File( _guiDocPath.getText().substring( 7 ) + "/" + topicAddress );
        if ( file.exists() )
            BareBonesBrowserLaunch.openURL( _guiDocPath.getText() + "/" + topicAddress );
        else {
            //  The named file couldn't be found.  Since this file is formed by
            //  the GUI, the name is probably not wrong, so the path probably is.
            //  Generate a temporary file with instructions for setting the documentation
            //  path.
            try {
                BufferedWriter out = new BufferedWriter( new FileWriter( "/tmp/tmpIndex.html" ) );
                out.write( "<h2>Requested Documentation Not Found</h2>\n" );
                out.write( "\n" );
                out.write( "<p>The document you requested:\n" );
                out.write( "<br>\n" );
                out.write( "<pre>\n" );
                out.write( _guiDocPath.getText() + "/" + topicAddress + "\n" );
                out.write( "</pre>\n" );
                out.write( "was not found.\n" );
                out.write( "\n" );
                out.write( "<p>The most likely reasons for this is\n" );
                out.write( "that the GUI Documentation Path is not set correctly.\n" );
                out.write( "Check the \"GUI Docs\" setting under the \"Documentation Locations\"\n" );
                out.write( "subset in the Settings Window (to launch the Settings Window pick\n" );
                out.write( "\"Settings/Show Settings\" from the menu bar of the main DiFX GUI Window).\n" );
                out.write( "<p>Note that the path should start with \"<code>file:///</code>\" followed by a complete\n" );
                out.write( "pathname, as in \"<code>file:///tmp/foo.html</code>\".\n" );
                out.close();
                BareBonesBrowserLaunch.openURL( "file:///tmp/tmpIndex.html" );
            } catch ( IOException e ) {
            }
        }
    }
    
    public void launchDiFXUsersGroup() {
        BareBonesBrowserLaunch.openURL( _difxUsersGroupURL.getText() );
    }
    
    public void launchDiFXWiki() {
        BareBonesBrowserLaunch.openURL( _difxWikiURL.getText() );
    }
    
    public void launchDiFXSVN() {
        BareBonesBrowserLaunch.openURL( _difxSVN.getText() );
    }
    
    /*
     * Add a new listener for database changes.
     */
    public void databaseChangeListener( ActionListener a ) {
        if ( _databaseChangeListeners == null )
            _databaseChangeListeners = new EventListenerList();
        _databaseChangeListeners.add( ActionListener.class, a );
    }

    /*
     * Inform all listeners of a change to database-related items.
     */
    protected void generateDatabaseChangeEvent() {
        if ( _databaseChangeListeners == null )
            return;
        Object[] listeners = _databaseChangeListeners.getListenerList();
        // loop through each listener and pass on the event if needed
        int numListeners = listeners.length;
        for ( int i = 0; i < numListeners; i+=2 ) {
            if ( listeners[i] == ActionListener.class )
                ((ActionListener)listeners[i+1]).actionPerformed( null );
        }
    }
    
    /*
     * Add a new listener for broadcast changes.
     */
    public void broadcastChangeListener( ActionListener a ) {
        if ( _broadcastChangeListeners == null )
            _broadcastChangeListeners = new EventListenerList();
        _broadcastChangeListeners.add( ActionListener.class, a );
    }

    /*
     * Inform all listeners of a change to broadcast-related items.
     */
    protected void generateBroadcastChangeEvent() {
        if ( _broadcastChangeListeners == null )
            return;
        Object[] listeners = _broadcastChangeListeners.getListenerList();
        // loop through each listener and pass on the event if needed
        int numListeners = listeners.length;
        for ( int i = 0; i < numListeners; i+=2 ) {
            if ( listeners[i] == ActionListener.class )
                ((ActionListener)listeners[i+1]).actionPerformed( null );
        }
    }
    
    /*
     * Add a new listener for EOP data changes.
     */
    public void eopChangeListener( ActionListener a ) {
        if ( _eopChangeListeners == null )
            _eopChangeListeners = new EventListenerList();
        _eopChangeListeners.add( ActionListener.class, a );
    }

    /*
     * Inform all listeners of a change to broadcast-related items.
     */
    protected void generateEOPChangeEvent() {
        if ( _eopChangeListeners == null )
            return;
        Object[] listeners = _eopChangeListeners.getListenerList();
        // loop through each listener and pass on the event if needed
        int numListeners = listeners.length;
        for ( int i = 0; i < numListeners; i+=2 ) {
            if ( listeners[i] == ActionListener.class )
                ((ActionListener)listeners[i+1]).actionPerformed( null );
        }
    }
    
    /*
     * Parse a settings file (XML).  We are set up to deal with "old" settings
     * files - those that may not have parameters we are looking for.  We try not
     * to over-write our default settings when such situations arise, although
     * different things happen depending on the variable type.  String settings
     * that don't exist return "null", which is easy to trap.  Numeric settings
     * return 0, which is easy to deal with unless 0 is a valid setting (there are some
     * of these).  Booleans return false when they don't exist, which we just have
     * to assume is the right setting.
     */
    public boolean getSettingsFromFile( String filename ) {
        //  Can't read a non-existent filename
        if ( filename == null )
            return false;
        //  Or a non-existent file
        File theFile = new File( filename );
        if ( !theFile.exists() ) {
            java.util.logging.Logger.getLogger( "global" ).log( java.util.logging.Level.SEVERE,
                "Settings file " + filename + " does not exist." );
            return false;
        }
        //  Now parse the thing, or try to.
        ObjectFactory factory = new ObjectFactory();
        DoiSystemConfig doiConfig = factory.createDoiSystemConfig();
        try {
            javax.xml.bind.JAXBContext jaxbCtx = javax.xml.bind.JAXBContext.newInstance(doiConfig.getClass().getPackage().getName());
            javax.xml.bind.Unmarshaller unmarshaller = jaxbCtx.createUnmarshaller();
            doiConfig = (DoiSystemConfig) unmarshaller.unmarshal( theFile );
            if ( doiConfig.getDifxHome() != null )
                this.home( doiConfig.getDifxHome() );
            //if ( doiConfig.getResourcesFile() != null )   // BLAT do we still use this thing?
            //    this.resourcesFile( doiConfig.getResourcesFile() );
            if ( doiConfig.getDbHost() != null )
                this.dbHost( doiConfig.getDbHost() );
            if ( doiConfig.getDbSID() != null )
                this.dbSID( doiConfig.getDbSID() );
            if ( doiConfig.getDbPassword() != null )
                this.dbPWD( doiConfig.getDbPassword() );
            if ( doiConfig.getDbJdbcDriver() != null )
                this.jdbcDriver( doiConfig.getDbJdbcDriver() );
            if ( doiConfig.getDbJdbcPort() != null )
                this.jdbcPort( doiConfig.getDbJdbcPort() );
            if ( doiConfig.getDbUrl() != null )
                this.dbURL( doiConfig.getDbUrl() );
            setDbURL();  //  BLAT not sure we need to do this anymore
            if ( doiConfig.getIpAddress() != null )
                this.ipAddress( doiConfig.getIpAddress() );
            if ( doiConfig.getPort() != 0 )
                this.port( doiConfig.getPort() );
            if ( doiConfig.getBufferSize() != 0 )
                this.bufferSize( doiConfig.getBufferSize() );
            _suppressWarningsCheck.setSelected( doiConfig.isSuppressUnknownMessageWarnings() );
            this.loggingEnabled( doiConfig.isLoggingEnabled() );
            if ( doiConfig.getStatusValidDuration() != 0 )
                this.statusValidDuration( doiConfig.getStatusValidDuration() );
            
            if ( doiConfig.getJaxbPackage() != null )
                this.jaxbPackage( doiConfig.getJaxbPackage() );
            if ( doiConfig.getTimeout() != 0 )
                this.timeout( doiConfig.getTimeout() );
            if ( doiConfig.getDifxControlAddress() != null )
                this.difxControlAddress( doiConfig.getDifxControlAddress() );
            if ( doiConfig.getDifxControlPort() != 0 )
                this.difxControlPort( doiConfig.getDifxControlPort() );
            if ( doiConfig.getDifxTransferPort() != 0 )
                this.difxTransferPort( doiConfig.getDifxTransferPort() );
            if ( doiConfig.getDifxControlUser() != null )
                this.difxControlUser( doiConfig.getDifxControlUser() );
            if ( doiConfig.getDifxControlPWD() != null )
                this.difxControlPassword( doiConfig.getDifxControlPWD() );
            if ( doiConfig.getDifxVersion() != null )
                this.difxVersion( doiConfig.getDifxVersion() );
            if ( doiConfig.getDifxPath() != null )
                this.difxPath( doiConfig.getDifxPath() );
            _dbUseDataBase.setSelected( doiConfig.isDbUseDataBase() );
            if ( doiConfig.getDbVersion() != null )
                _dbVersion.setText( doiConfig.getDbVersion() );
            if ( doiConfig.getDbName() != null )
                this.dbName( doiConfig.getDbName() );
            if ( doiConfig.getReportLoc() != null )
                _reportLoc = doiConfig.getReportLoc();
            if ( doiConfig.getGuiDocPath() != null )
                _guiDocPath.setText( doiConfig.getGuiDocPath() );
            if ( doiConfig.getDifxUsersGroupURL() != null )
                _difxUsersGroupURL.setText( doiConfig.getDifxUsersGroupURL() );
            if ( doiConfig.getDifxWikiURL() != null )
                _difxWikiURL.setText( doiConfig.getDifxWikiURL() );
            if ( doiConfig.getDifxSVN() != null )
                _difxSVN.setText( doiConfig.getDifxSVN() );
            this.dbAutoUpdate( doiConfig.isDbAutoUpdate() );
            if ( doiConfig.getDbAutoUpdateInterval() != 0 )
                _dbAutoUpdateInterval.intValue( doiConfig.getDbAutoUpdateInterval() );
            if ( doiConfig.getWorkingDirectory() != null )
                _workingDirectory.setText( doiConfig.getWorkingDirectory() );
            if ( doiConfig.getStagingArea() != null )
                _stagingArea.setText( doiConfig.getStagingArea() );
            _useStagingArea.setSelected( doiConfig.isUseStagingArea() );

            _queueBrowserSettings.showCompleted = doiConfig.isQueueShowCompleted();
            _queueBrowserSettings.showIncomplete = doiConfig.isQueueShowIncomplete();
            _queueBrowserSettings.showSelected = doiConfig.isQueueShowSelected();
            _queueBrowserSettings.showUnselected = doiConfig.isQueueShowUnselected();
            if ( doiConfig.getWindowConfigMainX() != 0 )
                _windowConfiguration.mainX = doiConfig.getWindowConfigMainX();
            if ( doiConfig.getWindowConfigMainY() != 0 )
                _windowConfiguration.mainY = doiConfig.getWindowConfigMainY();
            if ( doiConfig.getWindowConfigMainW() != 0 )
                _windowConfiguration.mainW = doiConfig.getWindowConfigMainW();
            if ( doiConfig.getWindowConfigMainH() != 0 )
                _windowConfiguration.mainH = doiConfig.getWindowConfigMainH();
            _windowConfiguration.verticalPanels = doiConfig.isWindowConfigVerticalPanels();
            if ( doiConfig.getWindowConfigMainDividerLocation() != 0 )
                _windowConfiguration.mainDividerLocation = doiConfig.getWindowConfigMainDividerLocation();
            if ( doiConfig.getWindowConfigTopDividerLocation() != 0 )
                _windowConfiguration.topDividerLocation = doiConfig.getWindowConfigTopDividerLocation();
            _windowConfiguration.queueBrowserTearOff = doiConfig.isWindowConfigQueueBrowserTearOff();
            if ( doiConfig.getWindowConfigQueueBrowserX() != 0 )
                _windowConfiguration.queueBrowserX = doiConfig.getWindowConfigQueueBrowserX();
            if ( doiConfig.getWindowConfigQueueBrowserY() != 0 )
                _windowConfiguration.queueBrowserY = doiConfig.getWindowConfigQueueBrowserY();
            if ( doiConfig.getWindowConfigQueueBrowserW() != 0 )
                _windowConfiguration.queueBrowserW = doiConfig.getWindowConfigQueueBrowserW();
            if ( doiConfig.getWindowConfigQueueBrowserH() != 0 )
                _windowConfiguration.queueBrowserH = doiConfig.getWindowConfigQueueBrowserH();
            _windowConfiguration.hardwareMonitorTearOff = doiConfig.isWindowConfigHardwareMonitorTearOff();
            if ( doiConfig.getWindowConfigHardwareMonitorX() != 0 )
                _windowConfiguration.hardwareMonitorX = doiConfig.getWindowConfigHardwareMonitorX();
            if ( doiConfig.getWindowConfigHardwareMonitorY() != 0 )
                _windowConfiguration.hardwareMonitorY = doiConfig.getWindowConfigHardwareMonitorY();
            if ( doiConfig.getWindowConfigHardwareMonitorW() != 0 )
                _windowConfiguration.hardwareMonitorW = doiConfig.getWindowConfigHardwareMonitorW();
            if ( doiConfig.getWindowConfigHardwareMonitorH() != 0 )
                _windowConfiguration.hardwareMonitorH = doiConfig.getWindowConfigHardwareMonitorH();
            if ( doiConfig.getWindowConfigExperimentEditorW() != 0 )
                _windowConfiguration.experimentEditorW = doiConfig.getWindowConfigExperimentEditorW();
            if ( doiConfig.getWindowConfigExperimentEditorH() != 0 )
                _windowConfiguration.experimentEditorH = doiConfig.getWindowConfigExperimentEditorH();
            if ( doiConfig.getWindowConfigSettingsWindowW() != 0 )
                _windowConfiguration.settingsWindowW = doiConfig.getWindowConfigSettingsWindowW();
            if ( doiConfig.getWindowConfigSettingsWindowH() != 0 )
                _windowConfiguration.settingsWindowH = doiConfig.getWindowConfigSettingsWindowH();
            this.setSize( _windowConfiguration.settingsWindowW, _windowConfiguration.settingsWindowH );
            if ( doiConfig.getWindowConfigJobEditorMonitorWindowW() != 0 )
                _windowConfiguration.jobEditorMonitorWindowW = doiConfig.getWindowConfigJobEditorMonitorWindowW();
            if ( doiConfig.getWindowConfigJobEditorMonitorWindowH() != 0 )
                _windowConfiguration.jobEditorMonitorWindowH = doiConfig.getWindowConfigJobEditorMonitorWindowH();
            if ( doiConfig.getDefaultNamesVexFileSource() != null )
                _defaultNames.vexFileSource = doiConfig.getDefaultNamesVexFileSource();
            if ( doiConfig.getDefaultNamesViaHttpLocation() != null )
                _defaultNames.viaHttpLocation = doiConfig.getDefaultNamesViaHttpLocation();
            if ( doiConfig.getDefaultNamesViaFtpLocation() != null )
                _defaultNames.viaFtpLocation = doiConfig.getDefaultNamesViaFtpLocation();
            if ( doiConfig.getDefaultNamesLocalFileLocation() != null )
                _defaultNames.localFileLocation = doiConfig.getDefaultNamesLocalFileLocation();
            _defaultNames.singleInputFile = doiConfig.isDefaultSingleInputFile();
            _defaultNames.scanBasedJobNames = doiConfig.isDefaultNamesScanBasedJobNames();
            if ( doiConfig.getDefaultNamesDirListLocation() != null )
                _defaultNames.dirListLocation = doiConfig.getDefaultNamesDirListLocation();
            _defaultNames.jobCreationSanityCheck = doiConfig.isDefaultJobCreationSanityCheck();
            if ( doiConfig.getEopURL() != null )
                _eopURL.setText( doiConfig.getEopURL() );
            if ( doiConfig.getLeapSecondsURL() != null && doiConfig.getLeapSecondsURL().length() > 0 );
                _leapSecondsURL.setText( doiConfig.getLeapSecondsURL() );
            _useLeapSecondsURL.setSelected( doiConfig.isUseLeapSecondsURL() );
            if ( _useLeapSecondsURL.isSelected() )
                leapSecondChoice( _useLeapSecondsURL );
            else
                leapSecondChoice( _useLeapSecondsValue );
            if ( doiConfig.getLeapSecondsValue() != 0 )
                _leapSecondsValue.intValue( doiConfig.getLeapSecondsValue() );
            _autoUpdateEOP.setSelected( doiConfig.isAutoUpdateEOP() );
            if ( doiConfig.getAutoUpdateSeconds() != 0 )
                _autoUpdateSeconds.value( doiConfig.getAutoUpdateSeconds() );
            updateEOPNow();
            generateBroadcastChangeEvent();
            generateDatabaseChangeEvent();
            
        } catch (javax.xml.bind.JAXBException ex) {
            // XXXTODO Handle exception
            java.util.logging.Logger.getLogger("global").log( java.util.logging.Level.SEVERE, null, ex );
        }
        return false;
    }

    /*
     * Save all current settings to the given filename.
     */
    public void saveSettingsToFile( String filename ) {
        System.out.println( "write to " + filename );
        ObjectFactory factory = new ObjectFactory();
        DoiSystemConfig doiConfig = factory.createDoiSystemConfig();
        doiConfig.setDifxHome( this.home() );
        //doiConfig.setResourcesFile( this.resourcesFile() );
        doiConfig.setDbHost( this.dbHost() );
        doiConfig.setDbSID( this.dbSID() );
        doiConfig.setDbPassword( this.dbPWD() );
        doiConfig.setDbJdbcDriver( this.jdbcDriver() );
        doiConfig.setDbJdbcPort( this.jdbcPort() );
        doiConfig.setDbUrl( this.dbURL() );
        doiConfig.setIpAddress( this.ipAddress() );
        doiConfig.setPort( this.port() );
        doiConfig.setBufferSize( this.bufferSize() );
        doiConfig.setSuppressUnknownMessageWarnings( _suppressWarningsCheck.isSelected() );
        doiConfig.setLoggingEnabled( this.loggingEnabled() );
        doiConfig.setStatusValidDuration( this.statusValidDuration() );
        
        doiConfig.setJaxbPackage( this.jaxbPackage() );
        doiConfig.setTimeout( this.timeout() );
        doiConfig.setDifxControlAddress( this.difxControlAddress() );
        doiConfig.setDifxControlPort( this.difxControlPort() );
        doiConfig.setDifxTransferPort( this.difxTransferPort() );
        doiConfig.setDifxControlUser( this.difxControlUser() );
        doiConfig.setDifxControlPWD( new String( this.difxControlPassword() ) );
        doiConfig.setDifxVersion( this.difxVersion() );
        doiConfig.setDifxPath( this.difxPath() );
        doiConfig.setDbUseDataBase( this.useDataBase() );
        doiConfig.setDbVersion( this.dbVersion() );
        doiConfig.setDbName( this.dbName() );
        doiConfig.setReportLoc( _reportLoc );
        doiConfig.setGuiDocPath( _guiDocPath.getText() );
        doiConfig.setDifxUsersGroupURL( _difxUsersGroupURL.getText() );
        doiConfig.setDifxWikiURL( _difxWikiURL.getText() );
        doiConfig.setDifxSVN( _difxSVN.getText() );
        doiConfig.setDbAutoUpdate( this.dbAutoUpdate() );
        doiConfig.setDbAutoUpdateInterval( _dbAutoUpdateInterval.intValue() );
        doiConfig.setQueueShowCompleted( _queueBrowserSettings.showCompleted );
        doiConfig.setQueueShowIncomplete( _queueBrowserSettings.showIncomplete );
        doiConfig.setQueueShowSelected( _queueBrowserSettings.showSelected );
        doiConfig.setQueueShowUnselected( _queueBrowserSettings.showUnselected );
        
        doiConfig.setWorkingDirectory( _workingDirectory.getText() );
        doiConfig.setStagingArea( _stagingArea.getText() );
        doiConfig.setUseStagingArea( _useStagingArea.isSelected() );
        
        doiConfig.setWindowConfigMainX( _windowConfiguration.mainX );
        doiConfig.setWindowConfigMainY( _windowConfiguration.mainY );
        doiConfig.setWindowConfigMainW( _windowConfiguration.mainW );
        doiConfig.setWindowConfigMainH( _windowConfiguration.mainH );
        doiConfig.setWindowConfigVerticalPanels( _windowConfiguration.verticalPanels );
        doiConfig.setWindowConfigMainDividerLocation( _windowConfiguration.mainDividerLocation );
        doiConfig.setWindowConfigTopDividerLocation( _windowConfiguration.topDividerLocation );
        doiConfig.setWindowConfigQueueBrowserTearOff( _windowConfiguration.queueBrowserTearOff );
        doiConfig.setWindowConfigQueueBrowserX( _windowConfiguration.queueBrowserX );
        doiConfig.setWindowConfigQueueBrowserY( _windowConfiguration.queueBrowserY );
        doiConfig.setWindowConfigQueueBrowserW( _windowConfiguration.queueBrowserW );
        doiConfig.setWindowConfigQueueBrowserH( _windowConfiguration.queueBrowserH );
        doiConfig.setWindowConfigHardwareMonitorTearOff( _windowConfiguration.hardwareMonitorTearOff );
        doiConfig.setWindowConfigHardwareMonitorX( _windowConfiguration.hardwareMonitorX );
        doiConfig.setWindowConfigHardwareMonitorY( _windowConfiguration.hardwareMonitorY );
        doiConfig.setWindowConfigHardwareMonitorW( _windowConfiguration.hardwareMonitorW );
        doiConfig.setWindowConfigHardwareMonitorH( _windowConfiguration.hardwareMonitorH );
        doiConfig.setWindowConfigExperimentEditorW( _windowConfiguration.experimentEditorW );
        doiConfig.setWindowConfigExperimentEditorH( _windowConfiguration.experimentEditorH );
        doiConfig.setWindowConfigSettingsWindowW( this.getWidth() );
        doiConfig.setWindowConfigSettingsWindowH( this.getHeight() );
        doiConfig.setWindowConfigJobEditorMonitorWindowW( _windowConfiguration.jobEditorMonitorWindowW );
        doiConfig.setWindowConfigJobEditorMonitorWindowH( _windowConfiguration.jobEditorMonitorWindowH );
        
        doiConfig.setDefaultNamesVexFileSource( _defaultNames.vexFileSource );
        doiConfig.setDefaultNamesViaHttpLocation( _defaultNames.viaHttpLocation );
        doiConfig.setDefaultNamesViaFtpLocation( _defaultNames.viaFtpLocation );
        doiConfig.setDefaultNamesLocalFileLocation( _defaultNames.localFileLocation );
        doiConfig.setDefaultSingleInputFile( _defaultNames.singleInputFile );
        doiConfig.setDefaultNamesScanBasedJobNames( _defaultNames.scanBasedJobNames );
        doiConfig.setDefaultNamesDirListLocation( _defaultNames.dirListLocation );
        doiConfig.setDefaultJobCreationSanityCheck( _defaultNames.jobCreationSanityCheck );
        
        doiConfig.setEopURL( _eopURL.getText() );
        doiConfig.setLeapSecondsURL( _leapSecondsURL.getText() );
        doiConfig.setUseLeapSecondsURL( _useLeapSecondsURL.isSelected() );
        doiConfig.setLeapSecondsValue( _leapSecondsValue.intValue() );
        doiConfig.setAutoUpdateEOP( _autoUpdateEOP.isSelected() );
        doiConfig.setAutoUpdateSeconds( _autoUpdateSeconds.intValue() );
        
        try {
            javax.xml.bind.JAXBContext jaxbCtx = javax.xml.bind.JAXBContext.newInstance( doiConfig.getClass().getPackage().getName() );
            javax.xml.bind.Marshaller marshaller = jaxbCtx.createMarshaller();
            File theFile = new File( filename );
            theFile.createNewFile();
            marshaller.marshal( doiConfig, theFile );
        } catch ( java.io.IOException e ) {
                System.out.println( "SystemSettings: can't write file \"" + filename + "\" - some appropriate complaint here." );
        } catch (javax.xml.bind.JAXBException ex) {
            // XXXTODO Handle exception
            java.util.logging.Logger.getLogger("global").log( java.util.logging.Level.SEVERE, null, ex );
        }
   }
    
    public String getDefaultSettingsFileName() {
        //  See if a DIFXROOT environment variable has been defined.  If not,
        //  guess that the current working directory is the DIFXROOT.
        String difxRoot = System.getenv( "DIFXROOT" );
        if (difxRoot == null) {
            difxRoot = System.getProperty( "user.dir" );
        }
        return difxRoot + "/conf/DOISystemConfig.xml";
    }
    
    /*
     * This function is called from the thread that receives broadcasts each time
     * it cycles.  It gives us the size of a received packet, or a 0 if this thread
     * timed out.  The information is added to a plot if this window is visible
     * (otherwise it is kind of a waste of time).
     */
    public void gotPacket( int newSize ) {
//        if ( this.isVisible() ) {
//            _broadcastPlot.limits( (double)(_broadcastTrackSize - _broadcastPlot.w()), (double)(_broadcastTrackSize), -.05, 1.0 );
//            _broadcastTrack.add( (double)(_broadcastTrackSize), (double)(newSize)/(double)bufferSize() );
//            _broadcastTrackSize += 1;
//            _plotWindow.updateUI();
//        }
//        else {
//            _broadcastTrack.clear();
//            _broadcastTrackSize = 0;
//        }
        if ( this.isVisible() )
            _broadcastPlot.limits( (double)(_broadcastTrackSize - _broadcastPlot.w()), (double)(_broadcastTrackSize), -.05, 1.0 );
        _broadcastTrack.add( (double)(_broadcastTrackSize), (double)(newSize)/(double)bufferSize() );
        _broadcastTrackSize += 1;
        _plotWindow.updateUI();
    }
    
    /*
     * Make a test connection to the database amd 
     */
    public void testDatabaseAction() {
        databaseSuccess( "" );
        databaseWarning( "Connecting to database (" + this.dbURL() + ")..." );
        QueueDBConnection db = new QueueDBConnection( this );
        try {
            databaseSuccess( "Connection successful...reading DiFX jobs..." );
            ResultSet jobInfo = db.jobList();

            Integer n = 0;
            while ( jobInfo.next() )
                ++n;
            databaseSuccess( "Database contains " + n.toString() + " jobs." );
            databaseSuccess( "" );

        } catch ( java.sql.SQLException e ) {
            databaseFailure( "SQLException: " + e.getMessage() );
            databaseSuccess( "" );
        } catch ( Exception e ) {
            databaseFailure( "Connection Failure (Exception): " + e.getMessage()  );
            databaseSuccess( "" );
        }
    }
    
    /*
     * Perform a "ping" test on the specified database host.
     */
    public void pingDatabaseHost() {
        databaseSuccess( "" );
        databaseWarning( "Starting ping test..." );
        final PingTest tester = new PingTest( _dbHost.getText() );
        tester.pings( 6 );
        tester.addSuccessListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                databaseSuccess( tester.lastMessage() );
            }
        } );            
        tester.addFailureListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                databaseFailure( tester.lastError() );
            }
        } );            
        tester.start();
    }
    
    protected void databaseWarning( String message ) {
        MessageNode node = new MessageNode( Calendar.getInstance().getTimeInMillis(), MessageNode.WARNING, null, message );
        node.showDate( false );
        node.showTime( true );
        node.showSource( false );
        node.showWarnings( true );
        _databaseMessages.addMessage( node );
        _databaseMessages.scrollToEnd();
    }
    
    protected void databaseSuccess( String message ) {
        MessageNode node = new MessageNode( 0, MessageNode.INFO, null, message );
        node.showDate( false );
        node.showTime( false );
        node.showSource( false );
        node.showMessages( true );
        _databaseMessages.addMessage( node );
        _databaseMessages.scrollToEnd();
    }
    
    protected void databaseFailure( String message ) {
        MessageNode node = new MessageNode( 0, MessageNode.ERROR, null, message );
        node.showDate( false );
        node.showTime( false );
        node.showSource( false );
        node.showErrors( true );
        _databaseMessages.addMessage( node );
        _databaseMessages.scrollToEnd();
    }
    
    public void hardwareMonitor( HardwareMonitorPanel newMonitor ) {
        _hardwareMonitor = newMonitor;
    }
    public HardwareMonitorPanel hardwareMonitor() {
        return _hardwareMonitor;
    }
    
    public void queueBrowser( QueueBrowserPanel newBrowser ) {
        _queueBrowser = newBrowser;
    }
    public QueueBrowserPanel queueBrowser() {
        return _queueBrowser;
    }
    public QueueBrowserSettings queueBrowserSettings() { return _queueBrowserSettings; }
    public WindowConfiguration windowConfiguration() { return _windowConfiguration; }
    public DefaultNames defaultNames() { return _defaultNames; }
    
    /*
     * Return the current list of experiment status types, or try to create one
     * from the database if it doesn't exist yet.
     */
    public Map<Integer, ExperimentStatusEntry> experimentStatusList() {
        if ( _experimentStatusList == null )
            _experimentStatusList = new HashMap<Integer, ExperimentStatusEntry>();
        if ( _experimentStatusList.isEmpty() ) {            
            QueueDBConnection db = new QueueDBConnection( this );
            if ( db.connected() ) {
                ResultSet dbExperimentStatusList = db.experimentStatusList();
                try {
                    while ( dbExperimentStatusList.next() ) {
                        _experimentStatusList.put( dbExperimentStatusList.getInt( "id" ),
                                new ExperimentStatusEntry( dbExperimentStatusList.getInt( "statuscode" ),
                                        dbExperimentStatusList.getString( "experimentstatus" ) ) );
                    }
                } catch ( Exception e ) {
                    java.util.logging.Logger.getLogger( "global" ).log( java.util.logging.Level.SEVERE, null, e );
                }
            }
        }
        return _experimentStatusList;
    }
    
    /*
     * Obtain the "id" of an experiment status from its string form.
     */
    public Integer experimentStatusID( String status ) {
        if ( _experimentStatusList == null )
            return 0;
        Iterator iter = _experimentStatusList.entrySet().iterator();
        for ( ; iter.hasNext(); ) {
            Map.Entry m = (Map.Entry)iter.next();
            if ( ((ExperimentStatusEntry)m.getValue()).status.contentEquals( status ) )
                return ((Integer)m.getKey());
        }
        return 0;
    }
    
    /*
     * Obtain the string form of an experiment status from its ID.
     */
    public String experimentStatusString( Integer id ) {
        if ( _experimentStatusList == null )
            return null;
        Iterator iter = _experimentStatusList.entrySet().iterator();
        for ( ; iter.hasNext(); ) {
            Map.Entry m = (Map.Entry)iter.next();
            if ( (Integer)m.getKey() == id )
                return ((ExperimentStatusEntry)m.getValue()).status;
        }
        return null;
    }
    
    /*
     * Return the current list of job status types, or try to create one
     * from the database if it doesn't exist yet.
     */
    public Map<Integer, JobStatusEntry> jobStatusList() {
        if ( _jobStatusList == null )
            _jobStatusList = new HashMap<Integer, JobStatusEntry>();
        if ( _jobStatusList.isEmpty() ) {            
            QueueDBConnection db = new QueueDBConnection( this );
            if ( db.connected() ) {
                ResultSet dbJobStatusList = db.jobStatusList();
                try {
                    while ( dbJobStatusList.next() ) {
                        _jobStatusList.put( dbJobStatusList.getInt( "id" ),
                                new JobStatusEntry( dbJobStatusList.getInt( "active" ),
                                        dbJobStatusList.getString( "status" ) ) );
                    }
                } catch ( Exception e ) {
                    java.util.logging.Logger.getLogger( "global" ).log( java.util.logging.Level.SEVERE, null, e );
                }
            }
        }
        return _jobStatusList;
    }
    
    /*
     * Obtain the "id" of an experiment status from its string form.
     */
    public Integer jobStatusID( String status ) {
        Map<Integer, JobStatusEntry> jobStatusList = jobStatusList();
        if ( jobStatusList == null )
            return 0;
        Iterator iter = jobStatusList.entrySet().iterator();
        for ( ; iter.hasNext(); ) {
            Map.Entry m = (Map.Entry)iter.next();
            if ( ((JobStatusEntry)m.getValue()).status.contentEquals( status ) )
                return ((Integer)m.getKey());
        }
        return 0;
    }
    
    /*
     * Obtain the string form of an experiment status from its ID.
     */
    public String jobStatusString( Integer id ) {
        if ( _jobStatusList == null )
            return null;
        Iterator iter = _jobStatusList.entrySet().iterator();
        for ( ; iter.hasNext(); ) {
            Map.Entry m = (Map.Entry)iter.next();
            if ( (Integer)m.getKey() == id )
                return ((JobStatusEntry)m.getValue()).status;
        }
        return null;
    }
    
    /*
     * Return the current list of pass types, or try to create one
     * from the database if it doesn't exist yet.
     */
    public Map<Integer, String> passTypeList() {
        if ( _passTypeList == null )
            _passTypeList = new HashMap<Integer, String>();
        if ( _passTypeList.isEmpty() ) {            
            QueueDBConnection db = new QueueDBConnection( this );
            if ( db.connected() ) {
                ResultSet dbPassTypeList = db.passTypeList();
                try {
                    while ( dbPassTypeList.next() ) {
                        _passTypeList.put( dbPassTypeList.getInt( "id" ),
                                dbPassTypeList.getString( "Type" ) );
                    }
                } catch ( Exception e ) {
                    java.util.logging.Logger.getLogger( "global" ).log( java.util.logging.Level.SEVERE, null, e );
                }
            }
        }
        return _passTypeList;
    }
    
    /*
     * Obtain the "id" of a pass type from its string form.
     */
    public Integer passTypeID( String status ) {
        if ( _passTypeList == null )
            return 0;
        Iterator iter = _passTypeList.entrySet().iterator();
        for ( ; iter.hasNext(); ) {
            Map.Entry m = (Map.Entry)iter.next();
            if ( ((String)m.getValue()).contentEquals( status ) )
                return ((Integer)m.getKey());
        }
        return 0;
    }
    
    /*
     * Obtain the string form of an pass type from its ID.
     */
    public String passTypeString( Integer id ) {
        if ( _passTypeList == null )
            return null;
        Iterator iter = _passTypeList.entrySet().iterator();
        for ( ; iter.hasNext(); ) {
            Map.Entry m = (Map.Entry)iter.next();
            if ( (Integer)m.getKey() == id )
                return ((String)m.getValue());
        }
        return null;
    }
    
    public boolean suppressWarnings() { return _suppressWarningsCheck.isSelected(); }
    
    /*
     * Add a new data source.  The "name" is ostensibly what it is called - the VSN
     * for a module, full path for a file, etc.  The type tells us what kind it is -
     * module, file, eVLBI, etc.  The "source" describes where we got the above
     * information - from the database, because the module was detected in a data
     * node, or whatever.
     */
    public void addDataSource( String name, String type, String source ) {
        DataSource src = new DataSource();
        src.name = name;
        src.type = type;
        src.source = source;
        if ( _dataSourceList == null )
            _dataSourceList = new ArrayList<DataSource>();
        _dataSourceList.add( src );
    }
    
    /*
     * Return a list of all data sources of the given type.
     */
    public ArrayList<DataSource> listDataSources( String type ) {
        ArrayList<DataSource> list = new ArrayList<DataSource>();
        if ( _dataSourceList != null ) {
            for ( Iterator<DataSource> iter = _dataSourceList.iterator(); iter.hasNext(); ) {
                DataSource src = iter.next();
                if ( src.type.contentEquals( type ) )
                    list.add( src );
            }
        }
        return list;
    }
    
    /*
     * Determine whether the given data source name and type exist in the list.
     */
    public boolean dataSourceInList( String name, String type ) {
        if ( _dataSourceList != null ) {
            for ( Iterator<DataSource> iter = _dataSourceList.iterator(); iter.hasNext(); ) {
                DataSource src = iter.next();
                if ( src.name.contentEquals( name ) && src.type.contentEquals( type ) )
                    return true;
            }
        }
        return false;
    }
    
    protected SystemSettings _this;
    
    protected boolean _allObjectsBuilt;

    protected JMenuBar _menuBar;
    protected JFormattedTextField _settingsFileName;
    protected boolean _settingsFileRead;
    
    protected String _jaxbPackage;
    protected String _home;
    //protected String _resourcesFile;
    protected boolean _loggingEnabled;
    protected long _statusValidDuration;
    //  DiFX Control Connection
    protected JFormattedTextField _difxControlAddress;
    protected NumberBox _difxControlPort;
    protected NumberBox _difxTransferPort;
    protected int _newDifxTransferPort;
    protected JFormattedTextField _difxControlUser;
    protected JPasswordField _difxControlPWD;
    protected JFormattedTextField _difxVersion;
    protected SaneTextField _difxPath;
    //  Broadcast network
    protected JFormattedTextField _ipAddress;
    protected NumberBox _port;
    protected NumberBox _bufferSize;
    protected NumberBox _timeout;
    PlotWindow _plotWindow;
    Plot2DObject _broadcastPlot;
    Track2D _broadcastTrack;
    int _broadcastTrackSize;
    protected JCheckBox _suppressWarningsCheck;
    //  Database configuration
    protected JCheckBox _dbUseDataBase;
    protected JFormattedTextField _dbVersion;
    protected JFormattedTextField _dbHost;
    protected JFormattedTextField _dbSID;
    protected JPasswordField _dbPWD;
    protected JFormattedTextField _dbName;
    protected JFormattedTextField _jdbcDriver;
    protected JFormattedTextField _jdbcPort;
    protected String _dbURL;
    protected JCheckBox _dbAutoUpdate;
    protected NumberBox _dbAutoUpdateInterval;
    protected JButton _pingHostButton;
    protected JButton _testDatabaseButton;
    protected MessageScrollPane _databaseMessages;
    //  Default report location
    protected String _reportLoc;
    
    //  These are locations for "help" - GUI and DiFX documentation.
    protected JFormattedTextField _guiDocPath;
    protected JButton _guiDocPathBrowseButton;
    protected JFormattedTextField _difxUsersGroupURL;
    protected JFormattedTextField _difxWikiURL;
    protected JFormattedTextField _difxSVN;
    
    //  Items that govern the creation and running of jobs.
    protected JFormattedTextField _workingDirectory;
    protected JFormattedTextField _stagingArea;
    protected JCheckBox _useStagingArea;
    
    //  EOP Settings items.
    protected SaneTextField _eopURL;
    protected SaneTextField _leapSecondsURL;
    protected JButton _viewEOPFile;
    protected JButton _viewLeapSecondsFile;
    protected JCheckBox _useLeapSecondsURL;
    protected JCheckBox _useLeapSecondsValue;
    protected NumberBox _leapSecondsValue;
    protected JCheckBox _autoUpdateEOP;
    protected NumberBox _autoUpdateSeconds;
    protected JButton _updateEOPNow;
    
    protected Timer _eopTimer;
    protected int _eopTimerCount;
    protected SimpleTextEditor _eopText;
    protected SimpleTextEditor _leapSecondText;
    protected JFrame _eopDisplay;
    protected JFrame _leapSecondDisplay;
    
    //  The "look and feel" that applies to all GUI components.
    protected String _lookAndFeel;
    
    //  Settings in the queue browser.
    public class QueueBrowserSettings {
        boolean showSelected;
        boolean showUnselected;
        boolean showCompleted;
        boolean showIncomplete;
    }
    protected QueueBrowserSettings _queueBrowserSettings;
    
    //  Dimensions and other configurations related to windows
    public class WindowConfiguration {
        int mainX;
        int mainY;
        int mainW;
        int mainH;
        boolean verticalPanels;
        int mainDividerLocation;
        int topDividerLocation;
        boolean queueBrowserTearOff;
        int queueBrowserX;
        int queueBrowserY;
        int queueBrowserW;
        int queueBrowserH;
        boolean hardwareMonitorTearOff;
        int hardwareMonitorX;
        int hardwareMonitorY;
        int hardwareMonitorW;
        int hardwareMonitorH;
        int experimentEditorW;
        int experimentEditorH;
        int settingsWindowW;
        int settingsWindowH;
        int jobEditorMonitorWindowW;
        int jobEditorMonitorWindowH;
    }
    protected WindowConfiguration _windowConfiguration;
    
    //  Defaults for a bunch of things that the user would likely change.
    public class DefaultNames {
        String vexFileSource;
        String viaHttpLocation;
        String viaFtpLocation;
        String localFileLocation;
        boolean createPassOnExperimentCreation;
        boolean singleInputFile;
        boolean scanBasedJobNames;
        boolean jobCreationSanityCheck;
        String dirListLocation;
    }
    protected DefaultNames _defaultNames;
    
    //  Different lists of event listeners.  Other classes can be informed of
    //  setting changes by adding themselves to these lists.
    EventListenerList _databaseChangeListeners;
    EventListenerList _broadcastChangeListeners;
    EventListenerList _eopChangeListeners;
    
    //  All settings use the same file chooser.
    JFileChooser _fileChooser;
    
    NodeBrowserScrollPane _scrollPane;
    IndexedPanel _addressesPanel;
    
    //  These items are used by multiple classes - they are put here as a matter
    //  of convenience as all locations have access to this class.
    HardwareMonitorPanel _hardwareMonitor;
    QueueBrowserPanel _queueBrowser;
    
    //  These lists contain "status" values that can be applied to different things.
    //  Nominally they come from the database, but in the absense of the database the
    //  user can create their own.
    public class ExperimentStatusEntry {
        public ExperimentStatusEntry( Integer newCode, String newStatus ) {
            code = newCode;
            status = newStatus;
        }
        public Integer code;
        public String status;
    }
    protected Map<Integer, ExperimentStatusEntry> _experimentStatusList;
    public class JobStatusEntry {
        public JobStatusEntry( Integer newActive, String newStatus ) {
            active = newActive;
            status = newStatus;
        }
        public Integer active;
        public String status;
    }
    protected Map<Integer, JobStatusEntry> _jobStatusList;
    
    //  These lists contain "type" values that can be applied to passes.
    //  Nominally they come from the database, but in the absense of the database the
    //  user can create their own.
    protected Map<Integer, String> _passTypeList;
    
    //  This class is used to contain information about a "data source", including
    //  its name, type, and where we got information about it (the "source" of the source).
    //  Since these are going to be used in a bunch of different ways, everything
    //  is a string.
    public class DataSource {
        String name;
        String type;
        String source;
    }
    
    //  Our list of the above class types.
    protected ArrayList<DataSource> _dataSourceList;
    
    
    
}
