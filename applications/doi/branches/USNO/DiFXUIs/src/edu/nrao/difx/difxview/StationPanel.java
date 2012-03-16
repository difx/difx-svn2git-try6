/*
 * This panel is contained in the ExperimentEditor, and is meant to hold all of
 * the information about a station.  It was originally developed as a class
 * entirely contained within the ExperimentEditor class, which makes sense (as that's
 * the only place it is used), but Netbeans continually crashes when confronted
 * with files that are above a few hundred lines.  Something to do with the Mac
 * implementation of Java.  Anyway, this is the only way to maintain sanity.
 */
package edu.nrao.difx.difxview;

import mil.navy.usno.widgetlib.SaneTextField;
import mil.navy.usno.widgetlib.BrowserNode;
import mil.navy.usno.widgetlib.NumberBox;

import edu.nrao.difx.difxutilities.DiFXCommand_ls;

import javax.swing.JLabel;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JTextField;
import javax.swing.JButton;

import java.awt.Color;
import java.awt.Cursor;

import java.util.Iterator;
import java.util.ArrayList;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import mil.navy.usno.widgetlib.NodeBrowserScrollPane;
import mil.navy.usno.widgetlib.IndexedPanel;

import javax.swing.event.EventListenerList;

public class StationPanel extends IndexedPanel {

    public StationPanel( VexFileParser.Station station, SystemSettings settings ) {
        super( station.name );
        _settings = settings;
        _this = this;
        _changeListeners = new EventListenerList();
        this.closedHeight( 20 );
        this.open( false );
        //this.drawFrame( false );
        this.resizeOnTopBar( true );
        _useCheck = new JCheckBox( "" );
        _useCheck.setBounds( 100, 2, 18, 16 );
        _useCheck.setSelected( true );
        _useCheck.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent evt ) {
                dispatchChangeCallback();
            }
        } );
        this.add( _useCheck );
        _contentPane = new NodeBrowserScrollPane();
        _contentPane.setLevel( 2 );
        _contentPane.drawFrame( false );
        _contentPane.respondToResizeEvents( true );
        _contentPane.noTimer();
        this.addScrollPane( _contentPane );
        
        //  The data source panel lets the user specify where data for this station
        //  come from.
        _dataSourcePanel = new IndexedPanel( "Data Source: unknown" );
        _dataSourcePanel.closedHeight( 20 );
        _dataSourcePanel.open( false );
        _dataSourcePanel.drawFrame( false );
        _dataSourcePanel.resizeOnTopBar( true );
        _contentPane.addNode( _dataSourcePanel );
        _vsnCheck = new JCheckBox( "" );
        _vsnCheck.setBounds( 200, 30, 25, 25 );
        _vsnCheck.setSelected( false );
        _vsnCheck.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent evt ) {
                setEnabledItems( _vsnCheck );
                dispatchChangeCallback();
            }
        } );
        _dataSourcePanel.add( _vsnCheck );
        JLabel vsnLabel = new JLabel( "Module: " );
        vsnLabel.setBounds( 100, 30, 95, 25 );
        vsnLabel.setHorizontalAlignment( JLabel.RIGHT );
        _dataSourcePanel.add( vsnLabel );
        _vsnList = new JComboBox();
        _vsnList.setBounds( 230, 30, 210, 25 );
        _vsnList.setToolTipText( "VSN of module containing data for this antenna." );
        _vsnList.setEditable( true );
        //  This little bit causes a typed-in item to be treated as a module name.
        _vsnList.getEditor().addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _dataSourcePanel.name( "Data Source: " + (String)_vsnList.getEditor().getItem() );
                //  If not already in the list of VSNs, add this name.
                if ( !_settings.dataSourceInList( (String)_vsnList.getEditor().getItem(), "VSN" ) ) {
                    if ( ((String)_vsnList.getEditor().getItem()).length() > 0 )
                        _settings.addDataSource( (String)_vsnList.getEditor().getItem(), "VSN", "hardware" );
                }
                dispatchChangeCallback();
            }
        });
        _vsnList.setBackground( Color.WHITE );
        _vsnList.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _dataSourcePanel.name( "Data Source: " + (String)_vsnList.getSelectedItem() );
                dispatchChangeCallback();
            }
        });
        _dataSourcePanel.add( _vsnList );
        _vsnList.setEnabled( true );
        _dirListLocation = new SaneTextField();
        _dirListLocation.setToolTipText( "Location on the DiFX Host of the file containing a directory listing for this module." );
        _dirListLocation.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _dataSourcePanel.name( "Data Source: " + (String)_vsnList.getSelectedItem() );
                _settings.defaultNames().dirListLocation = _dirListLocation.getText();
            }
        });
        _dirListLocation.setText( _settings.defaultNames().dirListLocation );
        _dirListLocation.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                dispatchChangeCallback();
            }
        });
        _dataSourcePanel.add( _dirListLocation );
        JLabel dirListLabel = new JLabel( "Listing:" );
        dirListLabel.setBounds( 450, 30, 55, 25 );
        dirListLabel.setHorizontalAlignment( JLabel.RIGHT );
        _dataSourcePanel.add( dirListLabel );
        Iterator<SystemSettings.DataSource> iter = _settings.listDataSources( "VSN" ).iterator();
        for ( ; iter.hasNext(); )
            _vsnList.addItem( iter.next().name );
        _fileCheck = new JCheckBox( "" );
        _fileCheck.setBounds( 200, 80, 25, 25 );
        _fileCheck.setSelected( false );
        _fileCheck.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent evt ) {
                setEnabledItems( _fileCheck );
                dispatchChangeCallback();
            }
        } );
        _dataSourcePanel.add( _fileCheck );
        JLabel fileLabel = new JLabel( "Files: " );
        fileLabel.setBounds( 100, 80, 95, 25 );
        fileLabel.setHorizontalAlignment( JLabel.RIGHT );
        _dataSourcePanel.add( fileLabel );
        _fileFilter = new SaneTextField();
        _fileFilter.setEnabled( false );
        _fileFilter.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent evt ) {
                fileFilterCallback();
            }
        } );
        _dataSourcePanel.add( _fileFilter );
        _fileList = new NodeBrowserScrollPane();
        _fileList.setBackground( Color.WHITE );
        _dataSourcePanel.add( _fileList );
        JLabel fileFilterLabel = new JLabel( "Filter:" );
        fileFilterLabel.setHorizontalAlignment( JLabel.RIGHT );
        fileFilterLabel.setBounds( 195, 80, 80, 25 );
        _dataSourcePanel.add( fileFilterLabel );
        _eVLBICheck = new JCheckBox( "" );
        _eVLBICheck.setBounds( 200, 55, 25, 25 );
        _eVLBICheck.setSelected( false );
        _eVLBICheck.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent evt ) {
                setEnabledItems( _eVLBICheck );
                dispatchChangeCallback();
            }
        } );
        _dataSourcePanel.add( _eVLBICheck );
        JLabel eVLBILabel = new JLabel( "E-Transfer: " );
        eVLBILabel.setBounds( 100, 55, 95, 25 );
        eVLBILabel.setHorizontalAlignment( JLabel.RIGHT );
        _dataSourcePanel.add( eVLBILabel );
        //  Default setup.  This should come from the SystemSettings class so
        //  it is saved between runs.
        setEnabledItems( _vsnCheck );
        
        //  The antenna panel contains information about the antenna - mount, offsets,
        //  size, etc.  This is filled in by a function call.
        _antennaPanel = new IndexedPanel( "Antenna: " + station.antenna );
        _antennaPanel.closedHeight( 20 );
        _antennaPanel.openHeight( 100 );
        _antennaPanel.open( false );
        _antennaPanel.drawFrame( false );
        _antennaPanel.resizeOnTopBar( true );
        _contentPane.addNode( _antennaPanel );
        
        //  The site panel contains information about the physical site....location,
        //  etc.  The content of this panel is added by a function call.
        _sitePanel = new IndexedPanel( "Site: " + station.site );
        _sitePanel.closedHeight( 20 );
        _sitePanel.openHeight( 100 );
        _sitePanel.open( false );
        _sitePanel.drawFrame( false );
        _sitePanel.resizeOnTopBar( true );
        _contentPane.addNode( _sitePanel );
        
        //  The Settings panel contains a bunch of things that don't fit into the
        //  above panels.
        _settingsPanel = new IndexedPanel( "Settings" );
        _settingsPanel.closedHeight( 20 );
        _settingsPanel.openHeight( 100 );
        _settingsPanel.open( false );
        _settingsPanel.drawFrame( false );
        _settingsPanel.resizeOnTopBar( true );
        _contentPane.addNode( _settingsPanel );
        _deltaClock = new NumberBox();
        _deltaClock.setBounds( 150, 20, 120, 25 );
        _deltaClock.precision( 3 );
        _deltaClock.value( 0.0 );
        _deltaClock.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent evt ) {
                dispatchChangeCallback();
            }
        } );
        _settingsPanel.add( _deltaClock );
        JLabel deltaClockLabel = new JLabel( "Delta Clock (\u03bcs)" );
        deltaClockLabel.setBounds( 10, 20, 135, 25 );
        deltaClockLabel.setHorizontalAlignment( JLabel.RIGHT );
        _settingsPanel.add( deltaClockLabel );
        
    }

    /*
     * This function is used to make things visible/enabled/etc as
     * fits each data source choice.  The selected data source check box is
     * used to determine which was selected.
     */
    protected void setEnabledItems( JCheckBox selector ) {
        //  Turn everything off first...
        _vsnCheck.setSelected( false );
        _fileCheck.setSelected( false );
        _eVLBICheck.setSelected( false );
        _vsnList.setEnabled( false );
        _dirListLocation.setEnabled( false );
        _fileFilter.setEnabled( false );
        _fileList.setVisible( false );
        _dataSourcePanel.staticHeight( 100 );
        //  Then turn appropriate stuff back on.
        if ( selector == _vsnCheck ) {
            _vsnCheck.setSelected( true );
            _vsnList.setEnabled( true );
            _dirListLocation.setEnabled( true );
            _dataSourcePanel.name( "Data Source: " + (String)_vsnList.getSelectedItem() );
        }
        else if ( selector == _fileCheck ) {
            _fileCheck.setSelected( true );
            _dataSourcePanel.name( "Data Source: " + "files " + _fileFilter.getText().trim() + "*" );
            _fileFilter.setEnabled( true );
            _fileList.setVisible( true );
            _dataSourcePanel.staticHeight( 230 );
        }
        else if ( selector == _eVLBICheck ) {
            _eVLBICheck.setSelected( true );
            _dataSourcePanel.name( "Data Source: " + "eVLBI" );
        }
        this.updateUI();
    }
    
    /*
     * Use site information to create and populate the site panel.
     */
    public void addSiteInformation( VexFileParser.Site site ) {
        JLabel siteName = new JLabel( site.name );
        siteName.setBounds( 150, 20, 100, 25 );
        _sitePanel.add( siteName );
        JLabel siteNameLabel = new JLabel( "Name:" );
        siteNameLabel.setBounds( 60, 20, 85, 25 );
        siteNameLabel.setHorizontalAlignment( JLabel.RIGHT );
        _sitePanel.add( siteNameLabel );
        JLabel siteId = new JLabel( site.id );
        siteId.setBounds( 150, 45, 100, 25 );
        _sitePanel.add( siteId );
        JLabel siteIdLabel = new JLabel( "ID:" );
        siteIdLabel.setBounds( 60, 45, 85, 25 );
        siteIdLabel.setHorizontalAlignment( JLabel.RIGHT );
        _sitePanel.add( siteIdLabel );
        JLabel siteType = new JLabel( site.type );
        siteType.setBounds( 150, 70, 100, 25 );
        _sitePanel.add( siteType );
        JLabel siteTypeLabel = new JLabel( "Type:" );
        siteTypeLabel.setBounds( 60, 70, 85, 25 );
        siteTypeLabel.setHorizontalAlignment( JLabel.RIGHT );
        _sitePanel.add( siteTypeLabel );
        //  Parse out the x, y, and z positions from the site position string.
        String posString = site.position.trim();
        int end = posString.indexOf( 'm' );
        _xpos = Double.parseDouble( posString.substring( 0, end ).trim() );
        posString = posString.substring( end + 3 ).trim();
        end = posString.indexOf( 'm' );
        _ypos = Double.parseDouble( posString.substring( 0, end ).trim() );
        posString = posString.substring( end + 3 ).trim();
        end = posString.indexOf( 'm' );
        _zpos = Double.parseDouble( posString.substring( 0, end ).trim() );
        _positionX = new NumberBox();
        _positionX.setBounds( 320, 20, 120, 25 );
        _sitePanel.add( _positionX );
        _positionX.value( _xpos );
        _positionX.precision( 3 );
        _positionX.setHorizontalAlignment( JTextField.RIGHT );
        _positionX.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent evt ) {
                dispatchChangeCallback();
            }
        } );
        JLabel positionXLabel = new JLabel( "X Position (m):" );
        positionXLabel.setBounds( 220, 20, 95, 25 );
        _sitePanel.add( positionXLabel );
        JButton resetX = new JButton( "Reset" );
        resetX.setToolTipText( "Reset the X position to its original value." );
        resetX.setBounds( 445, 20, 80, 24 );
        resetX.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent evt ) {
                _positionX.value( _xpos );
                dispatchChangeCallback();
            }
        } );
        _sitePanel.add( resetX );
        _positionY = new NumberBox();
        _positionY.setBounds( 320, 45, 120, 25 );
        _sitePanel.add( _positionY );
        _positionY.value( _ypos );
        _positionY.precision( 3 );
        _positionY.setHorizontalAlignment( JTextField.RIGHT );
        _positionY.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent evt ) {
                dispatchChangeCallback();
            }
        } );
        JLabel positionYLabel = new JLabel( "Y Position (m):" );
        positionYLabel.setBounds( 220, 45, 95, 25 );
        _sitePanel.add( positionYLabel );
        JButton resetY = new JButton( "Reset" );
        resetY.setToolTipText( "Reset the Y position to its original value." );
        resetY.setBounds( 445, 45, 80, 24 );
        resetY.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent evt ) {
                _positionY.value( _ypos );
                dispatchChangeCallback();
            }
        } );
        _sitePanel.add( resetY );
        _positionZ = new NumberBox();
        _positionZ.setBounds( 320, 70, 120, 25 );
        _sitePanel.add( _positionZ );
        _positionZ.value( _zpos );
        _positionZ.precision( 3 );
        _positionZ.setHorizontalAlignment( JTextField.RIGHT );
        _positionZ.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent evt ) {
                dispatchChangeCallback();
            }
        } );
        JLabel positionZLabel = new JLabel( "Z Position (m):" );
        positionZLabel.setBounds( 220, 70, 95, 25 );
        _sitePanel.add( positionZLabel );
        JButton resetZ = new JButton( "Reset" );
        resetZ.setToolTipText( "Reset the Z position to its original value." );
        resetZ.setBounds( 445, 70, 80, 24 );
        resetZ.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent evt ) {
                _positionZ.value( _zpos );
                dispatchChangeCallback();
            }
        } );
        _sitePanel.add( resetZ );
    }

    /*
     * Use site information to create and populate the site panel.
     */
    public void addAntennaInformation( VexFileParser.Antenna antenna ) {
    }

    /*
     * Change things to match a new width.
     */
    public void newWidth( int w ) {
        _fileFilter.setBounds( 280, 80, w - 305, 25 );
        _fileList.setBounds( 230, 110, w - 255, 120 );
        _dirListLocation.setBounds( 510, 30, w - 535, 25 );
        _contentPane.setBounds( 0, 20, w - 2, _contentPane.dataHeight() );
    }

    /*
     * This function is called when the file filter is changed.  It gets a list
     * of files from the DiFX Host that match the current filter.
     */
    public void fileFilterCallback() {
        final ArrayList<String> newList = new ArrayList<String>();
        final Cursor cursor = this.getCursor();
        final IndexedPanel _this = this;
        this.setCursor( Cursor.getPredefinedCursor( Cursor.WAIT_CURSOR ) );
        DiFXCommand_ls ls = new DiFXCommand_ls( _fileFilter.getText().trim() + "*", _settings );
        ls.addEndListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _this.setCursor( cursor );
                //  Found anything at all?
                if ( newList.size() > 0 ) {
                    Iterator<String> iter = newList.iterator();
                    String commonString = iter.next();
                    clearFileList();
                    addToFileList( commonString, _settings.difxControlAddress() );
                    for ( ; iter.hasNext(); ) {
                        String newStr = iter.next();
                        addToFileList( newStr, _settings.difxControlAddress() );
                        int i = 0;
                        while ( commonString.regionMatches( 0, newStr, 0, i ) )
                            ++i;
                        commonString = commonString.substring( 0, i + 1 );
                    }
                    //  The common string becomes the new contents of the filter...
                    _fileFilter.setText( commonString );
                    _fileFilter.setCaretPosition( commonString.length() );
                    _dataSource.setText( "files " + _fileFilter.getText().trim() + "*" );
                }
                dispatchChangeCallback();
            }
        });
        ls.addIncrementalListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                newList.add( e.getActionCommand().trim() );
            }
        });
        ls.send();
    }

    protected class FileListItem extends IndexedPanel {

        FileListItem( String filename, String sourceNode, boolean use ) {
            super( filename );
            this.alwaysOpen( true );
            this.openHeight( 20 );
            this.noArrow( true );
            this.backgroundColor( Color.WHITE );
            this.drawFrame( false );
            _use = new JCheckBox( "" );
            _use.setBounds( 5, 0, 20, 20 );
            _use.setBackground( Color.WHITE );
            //  Any change in the "use" needs to trigger a change event.
            _use.addActionListener( new ActionListener() {
                public void actionPerformed( ActionEvent evt ) {
                    dispatchChangeCallback();
                }
            } );
            if ( use )
                _use.setSelected( true );
            this.add( _use );
            sourceNode( sourceNode );
        }

        public boolean use() { return _use.isSelected(); }
        public void use( boolean newVal ) { _use.setSelected( newVal ); }
        public String sourceNode() { return _sourceNode; }
        public void sourceNode( String newVal ) { _sourceNode = newVal; }

        protected JCheckBox _use;
        protected String _sourceNode;

    }

    /*
     * Add a new item to the list of files - presumably a new file name.
     */
    public void addToFileList( String newFile, String sourceNode ) {
        //  Make sure this doesn't terminate with a "/" character - so we
        //  know its a file.
        if ( newFile.charAt( newFile.length() - 1 ) != '/' )
            _fileList.addNode( new FileListItem( newFile, sourceNode, true ) );
    }

    /*
     * Clear all items from the list of files.
     */
    public void clearFileList() {
        _fileList.browserTopNode().clearChildren();
    }

    /*
     * Return a list of all filenames listed in the file list.  Only those
     * items with the "use" check are listed.
     */
    public ArrayList<String> fileList() {
        ArrayList<String> newList = new ArrayList<String>();
        for ( Iterator<BrowserNode> iter = _fileList.browserTopNode().childrenIterator(); iter.hasNext(); ) {
            FileListItem newItem = (FileListItem)iter.next();
            if ( newItem.use() )
                newList.add( newItem.name() );
        }
        return newList;
    }

    /*
     * Return the machine name associated with the given file item.
     */
    public String machineForFile( String filename ) {
        for ( Iterator<BrowserNode> iter = _fileList.browserTopNode().childrenIterator(); iter.hasNext(); ) {
            FileListItem newItem = (FileListItem)iter.next();
            if ( newItem.name().contentEquals( filename ) )
                return newItem.sourceNode();
        }
        return null;
    }

    public boolean use() { return _useCheck.isSelected(); }
    public boolean useVsn() { return _vsnCheck.isSelected(); }
    public boolean useFile() { return _fileCheck.isSelected(); }
    public boolean useEVLBI() { return _eVLBICheck.isSelected(); }
    public String vsnSource() { return (String)_vsnList.getSelectedItem(); }
    public String dirListLocation() { return _dirListLocation.getText(); }
    
    public boolean positionChange() {
        if ( _positionX.value() != _xpos ||
             _positionY.value() != _ypos ||
             _positionZ.value() != _zpos )
            return true;
        else
            return false;
    }
    public boolean positionXChange() {
        return ( _positionX.value() != _xpos );
    }
    public boolean positionYChange() {
        return ( _positionY.value() != _ypos );
    }
    public boolean positionZChange() {
        return ( _positionZ.value() != _zpos );
    }
    public Double positionX() { return _positionX.value(); }
    public Double positionY() { return _positionY.value(); }
    public Double positionZ() { return _positionZ.value(); }
    
    public boolean deltaClockChange() {
        return ( _deltaClock.value() != 0.0 );
    }
    public Double deltaClock() { return _deltaClock.value(); }

    /*
     * Add a "listener" to callbacks when any changes to button states occur.
     */
    public void addChangeListener( ActionListener a ) {
        _changeListeners.add( ActionListener.class, a );
    }

    protected void dispatchChangeCallback() {
        Object[] listeners = _changeListeners.getListenerList();
        // loop through each listener and pass on the event if needed
        int numListeners = listeners.length;
        for ( int i = 0; i < numListeners; i+=2 ) {
            if ( listeners[i] == ActionListener.class )
                ((ActionListener)listeners[i+1]).actionPerformed( new ActionEvent( this, ActionEvent.ACTION_PERFORMED, "" ) );
        }
    }

    protected JCheckBox _useCheck;
    protected JCheckBox _vsnCheck;
    protected JCheckBox _fileCheck;
    protected JCheckBox _eVLBICheck;
    protected JComboBox _vsnList;
    protected JLabel _dataSource;
    protected EventListenerList _changeListeners;
    protected SaneTextField _fileFilter;
    protected NodeBrowserScrollPane _fileList;
    protected StationPanel _this;
    protected SaneTextField _dirListLocation;
    
    protected SystemSettings _settings;
    
    protected NodeBrowserScrollPane _contentPane;
    protected IndexedPanel _dataSourcePanel;
    protected IndexedPanel _antennaPanel;
    protected IndexedPanel _sitePanel;
    protected IndexedPanel _settingsPanel;
    
    protected NumberBox _positionX;
    protected NumberBox _positionY;
    protected NumberBox _positionZ;
    protected double _xpos;
    protected double _ypos;
    protected double _zpos;
    
    protected NumberBox _deltaClock;

}
