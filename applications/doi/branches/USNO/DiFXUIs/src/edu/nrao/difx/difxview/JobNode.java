/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.nrao.difx.difxview;

import mil.navy.usno.widgetlib.BrowserNode;
import mil.navy.usno.widgetlib.ActivityMonitorLight;

import mil.navy.usno.plotlib.PlotWindow;
import mil.navy.usno.plotlib.Plot2DObject;
import mil.navy.usno.plotlib.Track2D;

import edu.nrao.difx.difxutilities.DiFXCommand_getFile;
import edu.nrao.difx.difxutilities.DiFXCommand_rm;

import javax.swing.JButton;
import javax.swing.JPopupMenu;
import javax.swing.JMenuItem;
import javax.swing.JProgressBar;
import javax.swing.JSeparator;
import javax.swing.JTextField;

import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;

import java.util.List;
import java.util.Iterator;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.Color;
import java.awt.Component;
import java.awt.Insets;

import java.io.File;

import edu.nrao.difx.xmllib.difxmessage.DifxMessage;
import edu.nrao.difx.xmllib.difxmessage.DifxAlert;
import edu.nrao.difx.xmllib.difxmessage.DifxStatus;
import java.awt.Font;

import edu.nrao.difx.difxdatabase.QueueDBConnection;

public class JobNode extends QueueBrowserNode {
    
    public JobNode( String name, SystemSettings settings ) {
        super( name );
        this.setHeight( 20 );
        _columnColor = Color.LIGHT_GRAY;
        _settings = settings;
        _editorMonitor = new JobEditorMonitor( this, _settings );
    }
    
    @Override
    public void createAdditionalItems() {
        addSelectionButton( null, null );
        //  Create a popup menu appropriate to a "job".
        _networkActivity = new ActivityMonitorLight();
        _networkActivity.warningTime( 0 );
        _networkActivity.alertTime( 0 );
        showNetworkActivity( true );
        this.add( _networkActivity );
        _state = new ColumnTextArea();
        _state.justify( ColumnTextArea.CENTER );
        _state.setText( "not started" );
        this.add( _state );
        _progress = new JProgressBar( 0, 100 );
        _progress.setValue( 0 );
        _progress.setStringPainted( true );
        this.add( _progress );
        _experiment = new ColumnTextArea();
        _experiment.justify( ColumnTextArea.RIGHT );
        _experiment.setText( "" );
        showExperiment( false );
        this.add( _experiment );
        _pass = new ColumnTextArea();
        _pass.justify( ColumnTextArea.RIGHT );
        _pass.setText( "" );
        showPass( false );
        this.add( _pass );
        _priority = new ColumnTextArea();
        _priority.justify( ColumnTextArea.RIGHT );
        _priority.setText( "" );
        showPriority( false );
        this.add( _priority );
        _queueTime = new ColumnTextArea();
        _queueTime.justify( ColumnTextArea.RIGHT );
        _queueTime.setText( "" );
        showQueueTime( true );
        this.add( _queueTime );
        _correlationStart = new ColumnTextArea();
        _correlationStart.justify( ColumnTextArea.RIGHT );
        _correlationStart.setText( "" );
        showCorrelationStart( false );
        this.add( _correlationStart );
        _correlationEnd = new ColumnTextArea();
        _correlationEnd.justify( ColumnTextArea.RIGHT );
        _correlationEnd.setText( "" );
        showCorrelationEnd( false );
        this.add( _correlationEnd );
        _jobStartText = new ColumnTextArea();
        _jobStartText.justify( ColumnTextArea.RIGHT );
        _jobStartText.setText( "" );
        showJobStart( true );
        this.add( _jobStartText );
        _jobDurationText = new ColumnTextArea();
        _jobDurationText.justify( ColumnTextArea.RIGHT );
        _jobDurationText.setText( "" );
        showJobDuration( true );
        this.add( _jobDurationText );
        _inputFile = new ColumnTextArea();
        _inputFile.justify( ColumnTextArea.RIGHT );
        _inputFile.setText( "" );
        showInputFile( true );
        this.add( _inputFile );
        _outputFile = new ColumnTextArea();
        _outputFile.justify( ColumnTextArea.RIGHT );
        _outputFile.setText( "" );
        showOutputFile( false );
        this.add( _outputFile );
        _outputSize = new ColumnTextArea();
        _outputSize.justify( ColumnTextArea.RIGHT );
        _outputSize.setText( "" );
        showOutputSize( false );
        this.add( _outputSize );
        _difxVersion = new ColumnTextArea();
        _difxVersion.justify( ColumnTextArea.RIGHT );
        _difxVersion.setText( "" );
        showDifxVersion( true );
        this.add( _difxVersion );
        _speedUpFactor = new ColumnTextArea();
        _speedUpFactor.justify( ColumnTextArea.RIGHT );
        _speedUpFactor.setText( "" );
        showSpeedUpFactor( true );
        this.add( _speedUpFactor );
        _numAntennas = new ColumnTextArea();
        _numAntennas.justify( ColumnTextArea.RIGHT );
        _numAntennas.setText( "" );
        showNumAntennas( true );
        this.add( _numAntennas );
        _numForeignAntennas = new ColumnTextArea();
        _numForeignAntennas.justify( ColumnTextArea.RIGHT );
        _numForeignAntennas.setText( "" );
        showNumForeignAntennas( true );
        this.add( _numForeignAntennas );
        _dutyCycleText = new ColumnTextArea();
        _dutyCycleText.justify( ColumnTextArea.RIGHT );
        _dutyCycleText.setText( "" );
        showDutyCycle( true );
        this.add( _dutyCycleText );
        _status = new ColumnTextArea();
        _status.justify( ColumnTextArea.RIGHT );
        _status.setText( "" );
        showStatus( true );
        this.add( _status );
                boolean active = false;
        _statusId = new ColumnTextArea();
        _statusId.justify( ColumnTextArea.RIGHT );
        _statusId.setText( "" );
        showStatusId( false );
        this.add( _statusId );
        _active = new ActivityMonitorLight();
        showActive( true );
        this.add( _active );
        _popup = new JPopupMenu();
        _monitorMenuItem = new JMenuItem( "Control/Monitor for " + name() );
        _monitorMenuItem.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateEditorMonitor();
                _editorMonitor.setVisible( true );
            }
        });
        _popup.add( _monitorMenuItem );
        _popup.add( new JSeparator() );
        JMenuItem selectMenuItem = new JMenuItem( "Toggle Selection" );
        selectMenuItem.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                selectionButtonAction();
            }
        });
        _popup.add( selectMenuItem );
        JMenuItem deleteItem = new JMenuItem( "Delete" );
        deleteItem.setToolTipText( "Delete this experiment.  Deletions also apply to the database (if used)." );
        deleteItem.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                deleteAction();
            }
        });
        _popup.add( deleteItem );
        _popup.add( new JSeparator() );
        JMenuItem menuItem8 = new JMenuItem( "Queue" );
        menuItem8.setToolTipText( "Put this job in the runnable queue." );
        _popup.add( menuItem8 );
        JMenuItem menuItem5 = new JMenuItem( "Start" );
        menuItem5.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateEditorMonitor();
                _editorMonitor.startJob();
            }
        });
        _popup.add( menuItem5 );
        JMenuItem menuItem6 = new JMenuItem( "Pause" );
        menuItem6.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateEditorMonitor();
                _editorMonitor.pauseJob();
            }
        });
        _popup.add( menuItem6 );
        JMenuItem menuItem7 = new JMenuItem( "Stop" );
        menuItem7.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateEditorMonitor();
                _editorMonitor.stopJob();
            }
        });
        _popup.add( menuItem7 );
    }
    
    @Override
    public void positionItems() {
        _colorColumn = false;
        _xOff = _level * 30;
        _networkActivity.setBounds( _xOff, 6, 10, 10 );
        _xOff += 14;
        _label.setBounds( _xOff, 0, _widthName, _ySize );
        _xOff += _widthName;
        _state.setBounds( _xOff + 1, 1, _widthState - 2, 18 );
        _xOff += _widthState;
        _progress.setBounds( _xOff + 1, 1, _widthProgressBar - 2, 18 );
        _xOff += _widthProgressBar;
        if ( _showWeights && _weights != null ) {
            //  The weights are a bit complicated...
            if ( _weights.length > 0 ) {
                int boxSize = _widthWeights / _weights.length / 2;
                for ( int i = 0; i < _weights.length; ++i ) {
                    setTextArea( _antenna[i], boxSize );
                    _antenna[i].setVisible( true );
//                    if ( _showWeightsAsPlots ) {
//                        setTextArea( _weightPlotWindow[i], boxSize );
//                        _weightPlotWindow[i].setVisible( true );
//                        _weight[i].setVisible( false );
//                    }
//                    else {
                        setTextArea( _weight[i], boxSize );
                        _weight[i].setVisible( true );
//                        _weightPlotWindow[i].setVisible( false );
//                    }
                }
            }
            else
                _xOff += _widthWeights;
        }
        else {
            if ( _antenna != null && _weight != null ) {
                for ( int i = 0; i < _antenna.length && i < _weight.length; ++i ) {
                    if ( _antenna[i] != null )
                        _antenna[i].setVisible( false );
                    if ( _weight[i] != null )
                        _weight[i].setVisible( false );
//                    _weightPlotWindow[i].setVisible( false );
                }
            }
        }
//        _startButton.setBounds( _level * 30 + 150, 0, 70, 20 );
//        _editButton.setBounds( _level * 30 + 230, 0, 70, 20 );
        //if ( _state.isVisible() )
        //    setTextArea( _state, _widthState );
        //if ( _progress.isVisible() )
        //    setTextArea( _progress, _widthProgressBar );
        if ( _experiment.isVisible() )
            setTextArea( _experiment, _widthExperiment );
        if ( _pass.isVisible() )
            setTextArea( _pass, _widthPass );
        if ( _priority.isVisible() )
            setTextArea( _priority, _widthPriority );
        if ( _queueTime.isVisible() )
            setTextArea( _queueTime, _widthQueueTime );
        if ( _correlationStart.isVisible() )
            setTextArea( _correlationStart, _widthCorrelationStart );
        if ( _correlationEnd.isVisible() )
            setTextArea( _correlationEnd, _widthCorrelationEnd );
        if ( _jobStartText.isVisible() )
            setTextArea( _jobStartText, _widthJobStart );
        if ( _jobDurationText.isVisible() )
            setTextArea( _jobDurationText, _widthJobDuration );
        if ( _inputFile.isVisible() )
            setTextArea( _inputFile, _widthInputFile );
        if ( _outputFile.isVisible() )
            setTextArea( _outputFile, _widthOutputFile );
        if ( _outputSize.isVisible() )
            setTextArea( _outputSize, _widthOutputSize );
        if ( _difxVersion.isVisible() )
            setTextArea( _difxVersion, _widthDifxVersion );
        if ( _speedUpFactor.isVisible() )
            setTextArea( _speedUpFactor, _widthSpeedUpFactor );
        if ( _numAntennas.isVisible() )
            setTextArea( _numAntennas, _widthNumAntennas );
        if ( _numForeignAntennas.isVisible() )
            setTextArea( _numForeignAntennas, _widthNumForeignAntennas );
        if ( _dutyCycleText.isVisible() )
            setTextArea( _dutyCycleText, _widthDutyCycle );
        if ( _status.isVisible() )
            setTextArea( _status, _widthStatus );
        if ( _active.isVisible() )
            setTextArea( _active, _widthActive );
        if ( _statusId.isVisible() )
            setTextArea( _statusId, _widthStatusId );
    }
    
    /*
     * Private function used repeatedly in positionItems().
     */
    protected void setTextArea( Component area, int xSize ) {
        area.setBounds( _xOff, 1, xSize, _ySize - 2);
        _xOff += xSize;
        if ( _colorColumn )
            area.setBackground( _columnColor );
        else
            area.setBackground( Color.WHITE );
        _colorColumn = !_colorColumn;
    }
    
    @Override
    public void paintComponent( Graphics g ) {
        //  Use anti-aliasing on the text (looks much better)
        Graphics2D g2 = (Graphics2D)g;
        g2.setRenderingHint( RenderingHints.KEY_ANTIALIASING,
                     RenderingHints.VALUE_ANTIALIAS_ON );
        super.paintComponent( g );
    }
    
    /*
     * Delete this job.  It must be removed from the database, files associated with
     * it should be removed from the DiFX host, and then it is removed from
     * its parent "pass".
     */
    public void deleteAction() {
        removeFromDatabase();
        removeFromHost();
        ((BrowserNode)(this.getParent())).removeChild( this );
    }
    
    /*
     * Remove this job from the database.  This is probably done prior to deleting
     * this job.
     */
    public void removeFromDatabase() {
        if ( this.inDatabase() ) {
            QueueDBConnection db = null;
            if ( _settings.useDatabase() ) {
                db = new QueueDBConnection( _settings );
                if ( db.connected() ) {
                    db.deleteJob( _id );
                }
            }
        }
    }
    
    /*
     * Remove the files associated with this job from the DiFX host.  This is done
     * by deleting files of all extensions that match the input file name.  This
     * makes the assumption that this covers everything, which under normal operation
     * is correct.
     */
    public void removeFromHost() {
        String pathname = this.inputFile().substring( 0, this.inputFile().lastIndexOf( '.' ) ) + "*";
        DiFXCommand_rm rm = new DiFXCommand_rm( pathname, "-rf", _settings );
        try { rm.send(); } catch ( Exception e ) {}
    }
    
    /*
     * This is a generic database update function for this object.  It will change
     * a specific field to a specific value - both are strings.  This is only done if
     * this job is in the database.
     */
    public void updateDatabase( String param, String setting ) {
        if ( this.inDatabase() ) {
            QueueDBConnection db = null;
            if ( _settings.useDatabase() ) {
                db = new QueueDBConnection( _settings );
                if ( db.connected() ) {
                    db.updateJob( _id, param, setting );
                }
            }
        }
    }

    /*
     * Internal function used to generate an editor/monitor for this job if one
     * does not exists and update it with current settings, as far as we know them.
     */
    protected void updateEditorMonitor() {
        if ( _editorMonitor == null )
            _editorMonitor = new JobEditorMonitor( this, _settings );
    }
    
    /*
     * This function sends a request to mk5daemon for a copy of an .input file
     * associated with this job.  Mk5daemon will hopefully respond some time soon
     * with the actual data.  
     */
    protected void requestInputFile() {
        requestFile( _inputFile.getText().trim() );
    }
    
    /*
     * Same function, but applied to the .calc file.
     */
    protected void requestCalcFile() {
        if ( _calcFile != null )
            requestFile( _calcFile.trim() );
    }
    
    /*
     * Request an input file from the DiFX Host.  The file will be parsed based on its
     * extension - .input and .calc files are recognized.
     */
    protected void requestFile( String filename ) {
            final DiFXCommand_getFile fileGet = new DiFXCommand_getFile( filename, _settings );
            final String fileStr = filename;
            fileGet.addEndListener( new ActionListener() {
                public void actionPerformed( ActionEvent e ) {
                    //  Check the file size....this will tell us if anything went
                    //  wrong, and to some degree what.
                    int fileSize = fileGet.fileSize();
                    if ( fileSize > 0 ) {
                        //  Was it only partially read?
                        if ( fileSize > fileGet.inString().length() )
                            java.util.logging.Logger.getLogger("global").log(java.util.logging.Level.SEVERE, 
                                    "Warning - connection terminated with "
                                    + fileGet.inString().length() + " of "
                                    + fileSize + " bytes read." );
                        //  Parse the file content based on the extension.
                        String ext = fileStr.substring( fileStr.lastIndexOf( '.' ) + 1 ).trim();
                        if ( ext.contentEquals( "input" ) ) {
                            _editorMonitor.inputFileName( fileStr );
                            _editorMonitor.parseInputFile( fileGet.inString() );
                        }
                        else if ( ext.contentEquals( "calc" ) ) {
                            _editorMonitor.calcFileName( fileStr );
                            _editorMonitor.parseCalcFile( fileGet.inString() );
                        }
                    }
                    else if ( fileSize == 0 ) {
                        java.util.logging.Logger.getLogger("global").log(java.util.logging.Level.SEVERE, 
                            "File \"" + _inputFile.getText() + "\" has zero length." );
                    }
                    else if ( fileSize == -10 ) {
                        java.util.logging.Logger.getLogger("global").log(java.util.logging.Level.SEVERE, 
                            "Socket connection timed out before DiFX host connected." );                                        }
                    else if ( fileSize == -11 ) {
                        java.util.logging.Logger.getLogger("global").log(java.util.logging.Level.SEVERE, 
                            "File transfer failed - " + fileGet.error() );
                    }
                    else if ( fileSize == -1 ) {
                        java.util.logging.Logger.getLogger("global").log(java.util.logging.Level.SEVERE, 
                            "Bad file name (probably the path was not complete." );
                    }
                    else if ( fileSize == -2 ) {
                        java.util.logging.Logger.getLogger("global").log(java.util.logging.Level.SEVERE, 
                            "Requested file \"" + _inputFile.getText() + "\" does not exist." );
                    }
                    else if ( fileSize == -3 ) {
                        java.util.logging.Logger.getLogger("global").log(java.util.logging.Level.SEVERE, 
                            "Error - DiFX user " + _settings.difxControlUser()
                            + " does not have read permission for named file." );
                    }
                    else if ( fileSize == -4 ) {
                        java.util.logging.Logger.getLogger("global").log(java.util.logging.Level.SEVERE, 
                            "DiFX user name " + _settings.difxControlUser() +
                            " not valid on DiFX host." );   
                    }
                    else {
                        java.util.logging.Logger.getLogger("global").log(java.util.logging.Level.SEVERE, 
                            "Unknown error encountered during file transfer of " + _inputFile.getText() + "." );
                    }
                }
            });  
            try {
                fileGet.readString();
            } catch ( Exception e ) {} //  BLAT we should be using the GetFileMonitor class here instead...
    }
    
    /*
     *   Test if this message is intended for a job or not.
     */
    static boolean testJobMessage( DifxMessage difxMsg ) {
        if ( ( difxMsg.getBody().getDifxStatus() != null ) ||
             ( difxMsg.getBody().getDifxAlert() != null ) )
            return true;
        else
            return false;
    }
    
    public void consumeMessage( DifxMessage difxMsg ) {
        
        //  If this job is "running" (it was started by the job editor/monitor) 
        //  then send the message to the monitor.
        _editorMonitor.consumeMessage( difxMsg );
        
        //  Got something...
        _networkActivity.data();
        
        //  See what kind of message this is...try status first.
        if ( difxMsg.getBody().getDifxStatus() != null ) {
            if ( difxMsg.getBody().getDifxStatus().getVisibilityMJD() != null &&
                    difxMsg.getBody().getDifxStatus().getJobstartMJD() != null &&
                    difxMsg.getBody().getDifxStatus().getJobstopMJD() != null )
                _progress.setValue( (int)( 0.5 + 100.0 * ( Double.valueOf( difxMsg.getBody().getDifxStatus().getVisibilityMJD() ) -
                        Double.valueOf( difxMsg.getBody().getDifxStatus().getJobstartMJD() ) ) /
                        ( Double.valueOf( difxMsg.getBody().getDifxStatus().getJobstopMJD() ) -
                        Double.valueOf( difxMsg.getBody().getDifxStatus().getJobstartMJD() ) ) ) );
            else
                _progress.setValue( 0 );
            _state.setText( difxMsg.getBody().getDifxStatus().getState() );
            if ( _state.getText().equalsIgnoreCase( "done" ) || _state.getText().equalsIgnoreCase( "mpidone" ) ) {
                _state.setBackground( Color.GREEN );
                _progress.setValue( 100 );  
            }
            else if ( _state.getText().equalsIgnoreCase( "running" ) )
                _state.setBackground( Color.YELLOW );
            else
                _state.setBackground( Color.LIGHT_GRAY ); 
            List<DifxStatus.Weight> weightList = difxMsg.getBody().getDifxStatus().getWeight();
            //  Create a new list of antennas/weights if one hasn't been created yet.
            if ( _weights == null )
                newWeightDisplay( weightList.size() );
            for ( Iterator<DifxStatus.Weight> iter = weightList.iterator(); iter.hasNext(); ) {
                DifxStatus.Weight thisWeight = iter.next();
                weight( thisWeight.getAnt(), thisWeight.getWt() );
            }
        }
        else if ( difxMsg.getBody().getDifxAlert() != null ) {
            //System.out.println( "this is an alert" );
            //System.out.println( difxMsg.getBody().getDifxAlert().getAlertMessage() );
            //System.out.println( difxMsg.getBody().getDifxAlert().getSeverity() );
        }

    }
    
    /*
     * This function is used to generate antenna/weight display areas.
     */
    protected void newWeightDisplay( int numAntennas ) {
        _weights = new double[ numAntennas ];
        _antennas = new String[ numAntennas ];
        _weight = new ColumnTextArea[ numAntennas ];
        _antenna = new ColumnTextArea[ numAntennas ];
//        _weightPlotWindow = new PlotWindow[ numAntennas ];
//        _weightPlot = new Plot2DObject[ numAntennas ];
//        _weightTrack = new Track2D[ numAntennas ];
//        _weightTrackSize = new int[ numAntennas ];
        //  Give the antennas "default" names.
        for ( Integer i = 0; i < numAntennas; ++i ) {
            _antenna[i] = new ColumnTextArea( i.toString() + ": " );
            _antenna[i].justify( ColumnTextArea.RIGHT );
            this.add( _antenna[i] );
            _weight[i] = new ColumnTextArea( "" );
            this.add( _weight[i] );
            _antennas[i] = i.toString();
//            //  This stuff is used to make a plot of the weight.
//            _weightPlotWindow[i] = new PlotWindow();
//            this.add( _weightPlotWindow[i] );
//            _weightPlot[i] = new Plot2DObject();
//            _weightPlotWindow[i].add2DPlot( _weightPlot[i] );
//            _weightTrack[i] = new Track2D();
//            _weightPlot[i].name( "Weight Plot " + i.toString() );
//            _weightPlot[i].drawBackground( true );
//            _weightPlot[i].drawFrame( true );
//            _weightPlot[i].frameColor( Color.GRAY );
//            _weightPlot[i].clip( true );
//            _weightPlot[i].addTopGrid( Plot2DObject.X_AXIS, 10.0, Color.BLACK );
//            _weightTrack[i] = new Track2D();
//            _weightTrack[i].fillCurve( true );
//            _weightPlot[i].addTrack( _weightTrack[i] );
//            _weightTrack[i].color( Color.GREEN );
//            _weightTrack[i].sizeLimit( 200 );
//            _weightPlot[i].frame( 0.0, 0.0, 1.0, 1.0 );
//            _weightPlot[i].backgroundColor( Color.BLACK );
//            _weightTrackSize[i] = 0;
        }
    }
    
    public void experiment( String newVal ) { _experiment.setText( newVal ); }
    public String experiment() { return _experiment.getText(); }
    public void pass( String newVal ) { _pass.setText( newVal ); }
    public String pass() { return _pass.getText(); }
    public void priority( int newVal ) { _priority.setText( String.format( "%10d", newVal ) ); }
    public int priority() { return new Integer( _priority.getText() ).intValue(); }
    public void queueTime( String newVal ) { _queueTime.setText( newVal ); }
    public String queueTime() { return _queueTime.getText(); }
    public void correlationStart( String newVal ) { _correlationStart.setText( newVal ); }
    public String correlationStart() { return _correlationStart.getText(); }
    public void correlationEnd( String newVal ) { _correlationEnd.setText( newVal ); }
    public String correlationEnd() { return _correlationEnd.getText(); }
    public void jobStart( double newVal ) { 
        _jobStartText.setText( String.format( "%10.3f", newVal ) );
        _jobStart = newVal;
    }
    public Double jobStart() { 
        return _jobStart;
    }
    public void jobDuration( Double newVal ) { 
        _jobDurationText.setText( String.format( "%10.3f", newVal ) );
        _jobDuration = newVal;
    }
    public Double jobDuration() { return _jobDuration; }
    public void inputFile( String newVal ) { 
        _inputFile.setText( newVal );
        //  Convert to a file to extract the directory path...
        File tryFile = new File( newVal );
        _directoryPath = tryFile.getParent();
        //  Request the contents of this input file from mk5daemon.
        requestInputFile();
        updateUI();
    }
    public String inputFile() { return _inputFile.getText(); }
    public void calcFile( String newVal ) {
        _calcFile = newVal;
        requestCalcFile();
    }
    public String calcFile() { return _calcFile; }
    public void fullName( String newVal ) { _fullName = newVal; }
    public String fullName() { return _fullName; }
    public void outputFile( String newVal ) { _outputFile.setText( newVal ); }
    public String outputFile() { return _outputFile.getText(); }
    public void outputSize( int newVal ) { _outputSize.setText( String.format( "%10d", newVal ) ); }
    public int outputSize() { return new Integer( _outputSize.getText() ).intValue(); }
    public void difxVersion( String newVal ) { _difxVersion.setText( newVal ); }
    public String difxVersion() { return _difxVersion.getText(); }
    public void speedUpFactor( double newVal ) { _speedUpFactor.setText( String.format( "%10.3f", newVal ) ); }
    public double speedUpFactor() { return new Double( _speedUpFactor.getText() ).doubleValue(); }
    public void numAntennas( int newVal ) {
        _numAntennas.setText( String.format( "%10d", newVal ) );
        newWeightDisplay( newVal );
    }
    public Integer numAntennas() { return Integer.parseInt( _numAntennas.getText().trim() ); }
    public void weight( String antenna, String newString ) {
        double newVal = Double.valueOf( newString );
        for ( int i = 0; i < _weights.length; ++i ) {
            if ( _antennas[i].contentEquals( antenna ) ) {
                _weights[i] = newVal;
                _weight[i].setText( newString );
//                _weightPlot[i].limits( (double)(_weightTrackSize[i] - 20), (double)(_weightTrackSize[i]), 0.0, 1.05 );
//                _weightTrack[i].add( (double)(_weightTrackSize[i]), newVal );
//                _weightTrackSize[i] += 1;
//                _weightPlotWindow[i].updateUI();
            }
        }
    }
    public double weight( String antenna ) {
        for ( int i = 0; i < _weights.length; ++i )
            if ( _antennas[i].contentEquals( antenna ) )
                return _weights[i];
        return 0.0;
    }
    public void antennaName( int i, String name ) {
        if ( i < _antennas.length )
            _antennas[i] = name;
    }
    public void numForeignAntennas( int newVal ) { _numForeignAntennas.setText( String.format( "%10d", newVal ) ); }
    public int numForeignAntennas() { return new Integer( _numForeignAntennas.getText() ).intValue(); }
    public void dutyCycle( Double newVal ) { 
        _dutyCycleText.setText( String.format( "%10.3f", newVal ) );
        _dutyCycle = newVal;
    }
    public Double dutyCycle() { return _dutyCycle; }
    public void status( String newVal ) { _status.setText( newVal ); }
    public String status() { return _status.getText(); }
    public void active( boolean newVal ) { _active.on( newVal ); }
    public boolean active() { return _active.on(); }
    public void statusId( int newVal ) { _statusId.setText( String.format( "%10d", newVal ) ); }
    public int statusId() { return new Integer( _statusId.getText() ).intValue(); }
    public String directoryPath() { return _directoryPath; }
    
    public void showNetworkActivity( boolean newVal ) { _networkActivity.setVisible( newVal ); }
    public void showName( boolean newVal ) { _label.setVisible( newVal ); }
    public void showProgressBar( boolean newVal ) { _progress.setVisible( newVal ); }
    public void showState( boolean newVal ) { _state.setVisible( newVal ); }
    public void showExperiment( boolean newVal ) { _experiment.setVisible( newVal ); }
    public void showPass( boolean newVal ) { _pass.setVisible( newVal ); }
    public void showPriority( boolean newVal ) { _priority.setVisible( newVal ); }
    public void showQueueTime( boolean newVal ) { _queueTime.setVisible( newVal ); }
    public void showCorrelationStart( boolean newVal ) { _correlationStart.setVisible( newVal ); }
    public void showCorrelationEnd( boolean newVal ) { _correlationEnd.setVisible( newVal ); }
    public void showJobStart( boolean newVal ) { _jobStartText.setVisible( newVal ); }
    public void showJobDuration( boolean newVal ) { _jobDurationText.setVisible( newVal ); }
    public void showInputFile( boolean newVal ) { _inputFile.setVisible( newVal ); }
    public void showOutputFile( boolean newVal ) { _outputFile.setVisible( newVal ); }
    public void showOutputSize( boolean newVal ) { _outputSize.setVisible( newVal ); }
    public void showDifxVersion( boolean newVal ) { _difxVersion.setVisible( newVal ); }
    public void showSpeedUpFactor( boolean newVal ) { _speedUpFactor.setVisible( newVal ); }
    public void showNumAntennas( boolean newVal ) { _numAntennas.setVisible( newVal ); }
    public void showNumForeignAntennas( boolean newVal ) { _numForeignAntennas.setVisible( newVal ); }
    public void showDutyCycle( boolean newVal ) { _dutyCycleText.setVisible( newVal ); }
    public void showStatus( boolean newVal ) { _status.setVisible( newVal ); }
    public void showActive( boolean newVal ) { _active.setVisible( newVal ); }
    public void showStatusId( boolean newVal ) { _statusId.setVisible( newVal ); }
    public void showWeights( boolean newVal ) { 
        _showWeights = newVal;
        this.updateUI();
    }
//    public void showWeightsAsPlots( boolean newVal ) { 
//        _showWeightsAsPlots = newVal;
//        this.updateUI();
//    }
    
    public void widthName( int newVal ) { _widthName = newVal; }
    public void widthProgressBar( int newVal ) { _widthProgressBar = newVal; }
    public void widthState( int newVal ) { _widthState = newVal; }
    public void widthExperiment( int newVal ) { _widthExperiment = newVal; }
    public void widthPass( int newVal ) { _widthPass = newVal; }
    public void widthPriority( int newVal ) { _widthPriority = newVal; }
    public void widthQueueTime( int newVal ) { _widthQueueTime = newVal; }
    public void widthCorrelationStart( int newVal ) { _widthCorrelationStart = newVal; }
    public void widthCorrelationEnd( int newVal ) { _widthCorrelationEnd = newVal; }
    public void widthJobStart( int newVal ) { _widthJobStart = newVal; }
    public void widthJobDuration( int newVal ) { _widthJobDuration = newVal; }
    public void widthInputFile( int newVal ) { _widthInputFile = newVal; }
    public void widthOutputFile( int newVal ) { _widthOutputFile = newVal; }
    public void widthOutputSize( int newVal ) { _widthOutputSize = newVal; }
    public void widthDifxVersion( int newVal ) { _widthDifxVersion = newVal; }
    public void widthSpeedUpFactor( int newVal ) { _widthSpeedUpFactor = newVal; }
    public void widthNumAntennas( int newVal ) { _widthNumAntennas = newVal; }
    public void widthNumForeignAntennas( int newVal ) { _widthNumForeignAntennas = newVal; }
    public void widthDutyCycle( int newVal ) { _widthDutyCycle = newVal; }
    public void widthStatus( int newVal ) { _widthStatus = newVal; }
    public void widthActive( int newVal ) { _widthActive = newVal; }
    public void widthStatusId( int newVal ) { _widthStatusId = newVal; }
    public void widthWeights( int newVal ) { _widthWeights = newVal; }
    
    public PassNode passNode() {
        return _passNode;
    }
    public void passNode( PassNode newNode ) {
        _passNode = newNode;
    }
    
    public JobEditorMonitor editorMonitor() { return _editorMonitor; }
    
    public boolean running() {
        return _running;
    }
    public void running( boolean newVal ) {
        _running = newVal;
    }
    
    protected PassNode _passNode;
    
    protected JButton _startButton;
    protected JButton _editButton;
    protected JobEditorMonitor _editorMonitor;
    protected int _xOff;
    protected int _widthName;
    protected JProgressBar _progress;
    protected int _widthProgressBar;
    protected ActivityMonitorLight _networkActivity;
    protected ColumnTextArea _state;
    protected int _widthState;
    protected ColumnTextArea _experiment;
    protected int _widthExperiment;
    protected ColumnTextArea _pass;
    protected int _widthPass;
    protected ColumnTextArea _priority;
    protected int _widthPriority;
    protected ColumnTextArea _queueTime;
    protected int _widthQueueTime;
    protected ColumnTextArea _correlationStart;
    protected int _widthCorrelationStart;
    protected ColumnTextArea _correlationEnd;
    protected int _widthCorrelationEnd;
    protected ColumnTextArea _jobStartText;
    protected int _widthJobStart;
    protected Double _jobStart;
    protected ColumnTextArea _jobDurationText;
    protected int _widthJobDuration;
    protected Double _jobDuration;
    protected ColumnTextArea _inputFile;
    protected int _widthInputFile;
    protected ColumnTextArea _outputFile;
    protected int _widthOutputFile;
    protected ColumnTextArea _outputSize;
    protected int _widthOutputSize;
    protected ColumnTextArea _difxVersion;
    protected int _widthDifxVersion;
    protected ColumnTextArea _speedUpFactor;
    protected int _widthSpeedUpFactor;
    protected ColumnTextArea _numAntennas;
    protected int _widthNumAntennas;
    protected ColumnTextArea _numForeignAntennas;
    protected int _widthNumForeignAntennas;
    protected ColumnTextArea _dutyCycleText;
    protected int _widthDutyCycle;
    protected Double _dutyCycle;
    protected ColumnTextArea _status;
    protected int _widthStatus;
    protected ActivityMonitorLight _active;
    protected int _widthActive;
    protected ColumnTextArea _statusId;
    protected int _widthStatusId;
    protected double[] _weights;
    protected String[] _antennas;
    protected boolean _showWeights;
    protected boolean _showWeightsAsPlots;
    protected int _widthWeights;
    protected ColumnTextArea[] _weight;
    protected ColumnTextArea[] _antenna;
//    protected PlotWindow[] _weightPlotWindow;
//    protected Plot2DObject[] _weightPlot;
//    protected Track2D[] _weightTrack;
    protected int[] _weightTrackSize;
    
    protected JMenuItem _monitorMenuItem;
    
    protected boolean _colorColumn;
    protected Color _columnColor;

    protected String _directoryPath;
    protected String _calcFile;
    protected String _fullName;
    
    protected SystemSettings _settings;
    
    protected boolean _running;
//    protected Integer _databaseJobId;
}
