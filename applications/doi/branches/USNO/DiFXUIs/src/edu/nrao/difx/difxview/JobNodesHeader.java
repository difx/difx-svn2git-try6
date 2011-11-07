/*
 * Provides a header line for jobs in the queue browser with titles for different
 * displayed items.  Items can be added, removed, and resized.
 */
package edu.nrao.difx.difxview;

import mil.navy.usno.widgetlib.BrowserNode;
import javax.swing.JMenuItem;
import javax.swing.JMenu;
import javax.swing.JPopupMenu;
import javax.swing.JSeparator;
import javax.swing.JCheckBoxMenuItem;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseEvent;
import java.awt.Cursor;
import java.awt.Component;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;

import java.util.Iterator;
import java.util.ArrayDeque;

public class JobNodesHeader extends BrowserNode {
    
    public JobNodesHeader() {
        super( "" );
        _normalCursor = this.getCursor();
        _columnAdjustCursor = new Cursor( Cursor.W_RESIZE_CURSOR );
        initializeDisplaySettings();
        setColumnWidths();
        _popupButton.setVisible( true );
    }
    
    @Override
    public void createAdditionalItems() {
        
        //  Holds a list of jobs that can be changed to reflect changes in this
        //  header.
        _jobs = new ArrayDeque<JobNode>();
        
        //  Create a popup menu that allows us to turn things on and off
        _popup = new JPopupMenu();
        JMenuItem menuItem;
        menuItem = new JMenuItem( _label.getText() + " Display Options:" );
        _popup.add( menuItem );
        _popup.add( new JSeparator() );
        JMenuItem allItem = new JMenuItem( "Show All" );
        allItem.addActionListener( new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                activateAll();
            }
        } );
        _popup.add( allItem );
        _showNetworkActivity = new JCheckBoxMenuItem( "Network Activity" );
        _showNetworkActivity.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showNetworkActivity );
        _showName = new JCheckBoxMenuItem( "Job Name" );
        _showName.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showName );
        _showState = new JCheckBoxMenuItem( "State" );
        _showState.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showState );
        _showProgressBar = new JCheckBoxMenuItem( "Progress" );
        _showProgressBar.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showProgressBar );
        _showWeights = new JCheckBoxMenuItem( "Weights" );
        _showWeights.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showWeights );
        JMenu weightMenu = new JMenu( "Show Weights As..." );
        _popup.add( weightMenu );
        _showWeightsAsNumbers = new JCheckBoxMenuItem( "Numbers" );
        _showWeightsAsNumbers.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                setWeightsAsPlots( false );
                updateDisplayedData();
            }
        });
        weightMenu.add( _showWeightsAsNumbers );
        _showWeightsAsPlots = new JCheckBoxMenuItem( "Plots" );
        _showWeightsAsPlots.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                setWeightsAsPlots( true );
                updateDisplayedData();
            }
        });
        weightMenu.add( _showWeightsAsPlots );
        _showExperiment = new JCheckBoxMenuItem( "Experiment" );
        _showExperiment.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showExperiment );
        _showPass = new JCheckBoxMenuItem( "Pass" );
        _showPass.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showPass );
        _showPriority = new JCheckBoxMenuItem( "Priority" );
        _showPriority.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showPriority );
        _showQueueTime = new JCheckBoxMenuItem( "Queue Time" );
        _showQueueTime.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showQueueTime );
        _showCorrelationStart = new JCheckBoxMenuItem( "Correlation Start" );
        _showCorrelationStart.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showCorrelationStart );
        _showCorrelationEnd = new JCheckBoxMenuItem( "Correlation End" );
        _showCorrelationEnd.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showCorrelationEnd );
        _showJobStart = new JCheckBoxMenuItem( "Job Start" );
        _showJobStart.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showJobStart );
        _showJobDuration = new JCheckBoxMenuItem( "Job Duration");
        _showJobDuration.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showJobDuration );
        _showInputFile = new JCheckBoxMenuItem( "Input File" );
        _showInputFile.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showInputFile );
        _showOutputFile = new JCheckBoxMenuItem( "Output File" );
        _showOutputFile.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showOutputFile );
        _showOutputSize = new JCheckBoxMenuItem( "Output Size" );
        _showOutputSize.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showOutputSize );
        _showDifxVersion = new JCheckBoxMenuItem( "DiFX Version" );
        _showDifxVersion.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showDifxVersion );
        _showSpeedUpFactor = new JCheckBoxMenuItem( "Speed Up Factor" );
        _showSpeedUpFactor.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showSpeedUpFactor );
        _showNumAntennas = new JCheckBoxMenuItem( "# Antennas" );
        _showNumAntennas.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showNumAntennas );
        _showNumForeignAntennas = new JCheckBoxMenuItem( "# Foreign Antennas" );
        _showNumForeignAntennas.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showNumForeignAntennas );
        _showDutyCycle = new JCheckBoxMenuItem( "Duty Cycle" );
        _showDutyCycle.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showDutyCycle );
        _showStatus = new JCheckBoxMenuItem( "Status" );
        _showStatus.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showStatus );
        _showActive = new JCheckBoxMenuItem( "Active" );
        _showActive.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showActive );
        _showStatusId = new JCheckBoxMenuItem( "Status ID" );
        _showStatusId.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showStatusId );
        _popupButton.setVisible( true );

        //  Create column headers
        _nameArea = new ColumnTextArea( "Job Name" );
        _nameArea.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showName.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _nameArea );
        _progressBar = new ColumnTextArea( "Progress" );
        _progressBar.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showProgressBar.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _progressBar );
        _state = new ColumnTextArea( "State" );
        _state.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showState.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _state );
        _experiment = new ColumnTextArea( "Experiment" );
        _experiment.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showExperiment.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _experiment );
        _pass = new ColumnTextArea( "Pass" );
        _pass.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showPass.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _pass );
        _priority = new ColumnTextArea( "Priority" );
        _priority.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showPriority.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _priority );
        _queueTime = new ColumnTextArea( "Queue Time" );
        _queueTime.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showQueueTime.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _queueTime );
        _correlationStart = new ColumnTextArea( "Correlation Start" );
        _correlationStart.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showCorrelationStart.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _correlationStart );
        _correlationEnd = new ColumnTextArea( "Correlation End" );
        _correlationEnd.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showCorrelationEnd.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _correlationEnd );
        _jobStart = new ColumnTextArea( "Job Start" );
        _jobStart.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showJobStart.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _jobStart );
        _jobDuration = new ColumnTextArea( "Job Duration");
        _jobDuration.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showJobDuration.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _jobDuration );
        _inputFile = new ColumnTextArea( "Input File" );
        _inputFile.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showInputFile.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _inputFile );
        _outputFile = new ColumnTextArea( "Output File" );
        _outputFile.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showOutputFile.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _outputFile );
        _outputSize = new ColumnTextArea( "Output Size" );
        _outputSize.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showOutputSize.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _outputSize );
        _difxVersion = new ColumnTextArea( "DiFX Version" );
        _difxVersion.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showDifxVersion.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _difxVersion );
        _speedUpFactor = new ColumnTextArea( "Speed Up Factor" );
        _speedUpFactor.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showSpeedUpFactor.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _speedUpFactor );
        _numAntennas = new ColumnTextArea( "# Antennas" );
        _numAntennas.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showNumAntennas.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _numAntennas );
        _numForeignAntennas = new ColumnTextArea( "# Foreign Antennas" );
        _numForeignAntennas.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showNumForeignAntennas.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _numForeignAntennas );
        _dutyCycle = new ColumnTextArea( "Duty Cycle" );
        _dutyCycle.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showDutyCycle.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _dutyCycle );
        _status = new ColumnTextArea( "Status" );
        _status.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showStatus.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _status );
        _active = new ColumnTextArea( "Active" );
        _active.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showActive.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _active );
        _statusId = new ColumnTextArea( "Status ID" );
        _statusId.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showStatusId.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _statusId );
        _weights = new ColumnTextArea( "Weights" );
        _weights.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showWeights.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _weights );
    }
    
    public void setWeightsAsPlots( boolean newVal ) {
        _showWeightsAsPlots.setState( newVal );
        _showWeightsAsNumbers.setState( !newVal );
    }
    
    /*
     * This function makes everything in the popup menu visible.
     */
    public void activateAll() {
        _showNetworkActivity.setState( true );
        _showName.setState( true );
        _showProgressBar.setState( true );
        _showState.setState( true );
        _showExperiment.setState( true );
        _showPass.setState( true );
        _showPriority.setState( true );
        _showQueueTime.setState( true );
        _showCorrelationStart.setState( true );
        _showCorrelationEnd.setState( true );
        _showJobStart.setState( true );
        _showJobDuration.setState( true );
        _showInputFile.setState( true );
        _showOutputFile.setState( true );
        _showOutputSize.setState( true );
        _showDifxVersion.setState( true );
        _showSpeedUpFactor.setState( true );
        _showNumAntennas.setState( true );
        _showNumForeignAntennas.setState( true );
        _showDutyCycle.setState( true );
        _showStatus.setState( true );
        _showActive.setState( true );
        _showStatusId.setState( true );
        _showWeights.setState( true );
        updateDisplayedData();
    }

    @Override
    public void positionItems() {
        super.positionItems();
        _xOff = 90;
        if ( _showNetworkActivity.getState() )
            _xOff += 14;
        _popupButton.setBounds( _xOff - 20, 0, 18, _ySize - 2 );
        if ( _showName.getState() ) {
            setTextArea( _nameArea, _widthName );
            _positionName = _xOff;
        }
        else
            _positionName = -100;
        if ( _showState.getState() ) {
            setTextArea( _state, _widthState );
            _positionState = _xOff;
        }
        else
            _positionState = -100;
        if ( _showProgressBar.getState() ) {
            setTextArea( _progressBar, _widthProgressBar );
            _positionProgressBar = _xOff;
        }
        else
            _positionProgressBar = -100;
        if ( _showWeights.getState() ) {
            setTextArea( _weights, _widthWeights );
            _positionWeights = _xOff;
        }
        else
            _positionWeights = -100;
        if ( _showExperiment.getState() ) {
            setTextArea( _experiment, _widthExperiment );
            _positionExperiment = _xOff;
        }
        else
            _positionExperiment = -100;
        if ( _showPass.getState() ) {
            setTextArea( _pass, _widthPass );
            _positionPass = _xOff;
        }
        else
            _positionPass = -100;
        if ( _showPriority.getState() ) {
            setTextArea( _priority, _widthPriority );
            _positionPriority = _xOff;
        }
        else
            _positionPriority = -100;
        if ( _showQueueTime.getState() ) {
            setTextArea( _queueTime, _widthQueueTime );
            _positionQueueTime = _xOff;
        }
        else
            _positionQueueTime = -100;
        if ( _showCorrelationStart.getState() ) {
            setTextArea( _correlationStart, _widthCorrelationStart );
            _positionCorrelationStart = _xOff;
        }
        else
            _positionCorrelationStart = -100;
        if ( _showCorrelationEnd.getState() ) {
            setTextArea( _correlationEnd, _widthCorrelationEnd );
            _positionCorrelationEnd = _xOff;
        }
        else
            _positionCorrelationEnd = -100;
        if ( _showJobStart.getState() ) {
            setTextArea( _jobStart, _widthJobStart );
            _positionJobStart = _xOff;
        }
        else
            _positionJobStart = -100;
        if ( _showJobDuration.getState() ) {
            setTextArea( _jobDuration, _widthJobDuration );
            _positionJobDuration = _xOff;
        }
        else
            _positionJobDuration = -100;
        if ( _showInputFile.getState() ) {
            setTextArea( _inputFile, _widthInputFile );
            _positionInputFile = _xOff;
        }
        else
            _positionInputFile = -100;
        if ( _showOutputFile.getState() ) {
            setTextArea( _outputFile, _widthOutputFile );
            _positionOutputFile = _xOff;
        }
        else
            _positionOutputFile = -100;
        if ( _showOutputSize.getState() ) {
            setTextArea( _outputSize, _widthOutputSize );
            _positionOutputSize = _xOff;
        }
        else
            _positionOutputSize = -100;
        if ( _showDifxVersion.getState() ) {
            setTextArea( _difxVersion, _widthDifxVersion );
            _positionDifxVersion = _xOff;
        }
        else
            _positionDifxVersion = -100;
        if ( _showSpeedUpFactor.getState() ) {
            setTextArea( _speedUpFactor, _widthSpeedUpFactor );
            _positionSpeedUpFactor = _xOff;
        }
        else
            _positionSpeedUpFactor = -100;
        if ( _showNumAntennas.getState() ) {
            setTextArea( _numAntennas, _widthNumAntennas );
            _positionNumAntennas = _xOff;
        }
        else
            _positionNumAntennas = -100;
        if ( _showNumForeignAntennas.getState() ) {
            setTextArea( _numForeignAntennas, _widthNumForeignAntennas );
            _positionNumForeignAntennas = _xOff;
        }
        else
            _positionNumForeignAntennas = -100;
        if ( _showDutyCycle.getState() ) {
            setTextArea( _dutyCycle, _widthDutyCycle );
            _positionDutyCycle = _xOff;
        }
        else
            _positionDutyCycle = -100;
        if ( _showStatus.getState() ) {
            setTextArea( _status, _widthStatus );
            _positionStatus = _xOff;
        }
        else
            _positionStatus = -100;
        if ( _showActive.getState() ) {
            setTextArea( _active, _widthActive );
            _positionActive = _xOff;
        }
        else
            _positionActive = -100;
        if ( _showStatusId.getState() ) {
            setTextArea( _statusId, _widthStatusId );
            _positionStatusId = _xOff;
        }
        else
            _positionStatusId = -100;
        
    }
    
    public void setTextArea( Component area, int xSize ) {
        area.setBounds( _xOff + 1, 1, xSize - 2, _ySize - 2);
        _xOff += xSize;
    }

    @Override
    public void paintComponent( Graphics g ) {
        //  Use anti-aliasing on the text (looks much better)
        Graphics2D g2 = (Graphics2D)g;
        g2.setRenderingHint( RenderingHints.KEY_ANTIALIASING,
                     RenderingHints.VALUE_ANTIALIAS_ON );
        super.paintComponent( g );
    }
    
    public void initializeDisplaySettings() {
        _showNetworkActivity.setState( true );
        _showName.setState( true );
        _showProgressBar.setState( true );
        _showState.setState( true );
        _showExperiment.setState( false );
        _showPass.setState( false );
        _showPriority.setState( false );
        _showQueueTime.setState( false );
        _showCorrelationStart.setState( false );
        _showCorrelationEnd.setState( false );
        _showJobStart.setState( false );
        _showJobDuration.setState( false );
        _showInputFile.setState( true );
        _showOutputFile.setState( false );
        _showOutputSize.setState( false );
        _showDifxVersion.setState( false );
        _showSpeedUpFactor.setState( false );
        _showNumAntennas.setState( false );
        _showNumForeignAntennas.setState( false );
        _showDutyCycle.setState( false );
        _showStatus.setState( false );
        _showActive.setState( false );
        _showStatusId.setState( false );
        _showWeights.setState( true );
        _showWeightsAsPlots.setState( true );
        _showWeightsAsNumbers.setState( false );
    }
    
    /*
     * Set the widths of the various columns.  These are all default widths
     * now, but in theory a settings structure could be handed to this function
     * to set widths specified by users.
     */
    public void setColumnWidths() {
        _widthName = 150;
        _widthProgressBar = 200;
        _widthState = 100;
        _widthExperiment = 100;
        _widthPass = 100;
        _widthPriority = 100;
        _widthQueueTime = 170;
        _widthCorrelationStart = 100;
        _widthCorrelationEnd = 100;
        _widthJobStart = 100;
        _widthJobDuration = 100;
        _widthInputFile = 400;
        _widthOutputFile = 100;
        _widthOutputSize = 100;
        _widthDifxVersion = 100;
        _widthSpeedUpFactor = 100;
        _widthNumAntennas = 100;
        _widthNumForeignAntennas = 100;
        _widthDutyCycle = 100;
        _widthStatus = 100;
        _widthActive = 100;
        _widthStatusId = 100;
        _widthWeights = 200;
    }
    
    /*
     * This functions propogates current column widths to all jobs.
     */
    public void setJobColumnWidths() {
        for ( Iterator<JobNode> iter = _jobs.iterator(); iter.hasNext(); ) {
            JobNode thisJob = iter.next();
            //  Change the settings on these items to match our current specifications.
            thisJob.widthName( _widthName );
            thisJob.widthProgressBar( _widthProgressBar );
            thisJob.widthState( _widthState );
            thisJob.widthExperiment( _widthExperiment );
            thisJob.widthPass( _widthPass );
            thisJob.widthPriority( _widthPriority );
            thisJob.widthQueueTime( _widthQueueTime );
            thisJob.widthCorrelationStart( _widthCorrelationStart );
            thisJob.widthCorrelationEnd( _widthCorrelationEnd );
            thisJob.widthJobStart( _widthJobStart );
            thisJob.widthJobDuration( _widthJobDuration );
            thisJob.widthInputFile( _widthInputFile );
            thisJob.widthOutputFile( _widthOutputFile );
            thisJob.widthOutputSize( _widthOutputSize );
            thisJob.widthDifxVersion( _widthDifxVersion );
            thisJob.widthSpeedUpFactor( _widthSpeedUpFactor );
            thisJob.widthNumAntennas( _widthNumAntennas );
            thisJob.widthNumForeignAntennas( _widthNumForeignAntennas );
            thisJob.widthDutyCycle( _widthDutyCycle );
            thisJob.widthStatus( _widthStatus );
            thisJob.widthActive( _widthActive );
            thisJob.widthStatusId( _widthStatusId );
            thisJob.widthWeights( _widthWeights );
            thisJob.updateUI();
        }
    }
    
    public void addJob( JobNode newNode ) {
        _jobs.add( newNode );
        setJobColumnWidths();
        updateDisplayedData();
    }
    
    /*
     * Check mouse move events to see if it is being positioned over one of the
     * joints between column headers.  This should change the cursor.  We also
     * record which item we are over.
     */
    @Override
    public void mouseMoved( MouseEvent e ) {
        this.setCursor( _normalCursor );
        _adjustName = false;
        _adjustProgressBar = false;
        _adjustState = false;
        _adjustExperiment = false;
        _adjustPass = false;
        _adjustPriority = false;
        _adjustQueueTime = false;
        _adjustCorrelationStart = false;
        _adjustCorrelationEnd = false;
        _adjustJobStart = false;
        _adjustJobDuration = false;
        _adjustInputFile = false;
        _adjustOutputFile = false;
        _adjustOutputSize = false;
        _adjustDifxVersion = false;
        _adjustSpeedUpFactor = false;
        _adjustNumAntennas = false;
        _adjustNumForeignAntennas = false;
        _adjustDutyCycle = false;
        _adjustStatus = false;
        _adjustActive = false;
        _adjustStatusId = false;
        _adjustWeights = false;
        if ( e.getX() > _positionName - 3 && e.getX() < _positionName + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustName = true;
        }
        else if ( e.getX() > _positionProgressBar - 3 && e.getX() < _positionProgressBar + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustProgressBar = true;
        }
        else if ( e.getX() > _positionState - 3 && e.getX() < _positionState + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustState = true;
        }
        else if ( e.getX() > _positionExperiment - 3 && e.getX() < _positionExperiment + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustExperiment = true;
        }
        else if ( e.getX() > _positionPass - 3 && e.getX() < _positionPass + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustPass = true;
        }
        else if ( e.getX() > _positionPriority - 3 && e.getX() < _positionPriority + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustPriority = true;
        }
        else if ( e.getX() > _positionQueueTime - 3 && e.getX() < _positionQueueTime + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustQueueTime = true;
        }
        else if ( e.getX() > _positionCorrelationStart - 3 && e.getX() < _positionCorrelationStart + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustCorrelationStart = true;
        }
        else if ( e.getX() > _positionCorrelationEnd - 3 && e.getX() < _positionCorrelationEnd + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustCorrelationEnd = true;
        }
        else if ( e.getX() > _positionJobStart - 3 && e.getX() < _positionJobStart + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustJobStart = true;
        }
        else if ( e.getX() > _positionJobDuration - 3 && e.getX() < _positionJobDuration + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustJobDuration = true;
        }
        else if ( e.getX() > _positionInputFile - 3 && e.getX() < _positionInputFile + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustInputFile = true;
        }
        else if ( e.getX() > _positionOutputFile - 3 && e.getX() < _positionOutputFile + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustOutputFile = true;
        }
        else if ( e.getX() > _positionOutputSize - 3 && e.getX() < _positionOutputSize + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustOutputSize = true;
        }
        else if ( e.getX() > _positionDifxVersion - 3 && e.getX() < _positionDifxVersion + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustDifxVersion = true;
        }
        else if ( e.getX() > _positionSpeedUpFactor - 3 && e.getX() < _positionSpeedUpFactor + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustSpeedUpFactor = true;
        }
        else if ( e.getX() > _positionNumAntennas - 3 && e.getX() < _positionNumAntennas + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustNumAntennas = true;
        }
        else if ( e.getX() > _positionNumForeignAntennas - 3 && e.getX() < _positionNumForeignAntennas + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustNumForeignAntennas = true;
        }
        else if ( e.getX() > _positionDutyCycle - 3 && e.getX() < _positionDutyCycle + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustDutyCycle = true;
        }
        else if ( e.getX() > _positionStatus - 3 && e.getX() < _positionStatus + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustStatus = true;
        }
        else if ( e.getX() > _positionActive - 3 && e.getX() < _positionActive + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustActive = true;
        }
        else if ( e.getX() > _positionStatusId - 3 && e.getX() < _positionStatusId + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustStatusId = true;
        }
        else if ( e.getX() > _positionWeights - 3 && e.getX() < _positionWeights + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustWeights = true;
        }
        else {
            super.mouseMoved( e );
        }
    }
    
    /*
     * Mouse pressed events record the size of a column (if we are in a position
     * to adjust the column).
     */
    @Override
    public void mousePressed( MouseEvent e ) {
        if ( _adjustName ) {
            _startWidth = _widthName;
            _startX = e.getX();
        }
        else if ( _adjustProgressBar ) {
            _startWidth = _widthProgressBar;
            _startX = e.getX();
        }
        else if ( _adjustState ) {
            _startWidth = _widthState;
            _startX = e.getX();
        }
        else if ( _adjustExperiment ) {
            _startWidth = _widthExperiment;
            _startX = e.getX();
        }
        else if ( _adjustPass ) {
            _startWidth = _widthPass;
            _startX = e.getX();
        }
        else if ( _adjustPriority ) {
            _startWidth = _widthPriority;
            _startX = e.getX();
        }
        else if ( _adjustQueueTime ) {
            _startWidth = _widthQueueTime;
            _startX = e.getX();
        }
        else if ( _adjustCorrelationStart ) {
            _startWidth = _widthCorrelationStart;
            _startX = e.getX();
        }
        else if ( _adjustCorrelationEnd ) {
            _startWidth = _widthCorrelationEnd;
            _startX = e.getX();
        }
        else if ( _adjustJobStart ) {
            _startWidth = _widthJobStart;
            _startX = e.getX();
        }
        else if ( _adjustJobDuration ) {
            _startWidth = _widthJobDuration;
            _startX = e.getX();
        }
        else if ( _adjustInputFile ) {
            _startWidth = _widthInputFile;
            _startX = e.getX();
        }
        else if ( _adjustOutputFile ) {
            _startWidth = _widthOutputFile;
            _startX = e.getX();
        }
        else if ( _adjustOutputSize ) {
            _startWidth = _widthOutputSize;
            _startX = e.getX();
        }
        else if ( _adjustDifxVersion ) {
            _startWidth = _widthDifxVersion;
            _startX = e.getX();
        }
        else if ( _adjustSpeedUpFactor ) {
            _startWidth = _widthSpeedUpFactor;
            _startX = e.getX();
        }
        else if ( _adjustNumAntennas ) {
            _startWidth = _widthNumAntennas;
            _startX = e.getX();
        }
        else if ( _adjustNumForeignAntennas ) {
            _startWidth = _widthNumForeignAntennas;
            _startX = e.getX();
        }
        else if ( _adjustDutyCycle ) {
            _startWidth = _widthDutyCycle;
            _startX = e.getX();
        }
        else if ( _adjustStatus ) {
            _startWidth = _widthStatus;
            _startX = e.getX();
        }
        else if ( _adjustActive ) {
            _startWidth = _widthActive;
            _startX = e.getX();
        }
        else if ( _adjustStatusId ) {
            _startWidth = _widthStatusId;
            _startX = e.getX();
        }
        else if ( _adjustWeights ) {
            _startWidth = _widthWeights;
            _startX = e.getX();
        }
        else
            super.mousePressed( e );
    }
    
    /*
     * Drag events might be used to change the width of columns.
     */
    @Override
    public void mouseDragged( MouseEvent e ) {
        if ( _adjustName ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthName = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustProgressBar ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthProgressBar = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustState ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthState = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustExperiment ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthExperiment = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustPass ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthPass = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustPriority ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthPriority = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustQueueTime ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthQueueTime = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustCorrelationStart ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthCorrelationStart = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustCorrelationEnd ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthCorrelationEnd = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustJobStart ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthJobStart = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustJobDuration ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthJobDuration = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustInputFile ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthInputFile = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustOutputFile ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthOutputFile = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustOutputSize ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthOutputSize = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustDifxVersion ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthDifxVersion = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustSpeedUpFactor ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthSpeedUpFactor = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustNumAntennas ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthNumAntennas = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustNumForeignAntennas ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthNumForeignAntennas = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustDutyCycle ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthDutyCycle = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustStatus ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthStatus = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustActive ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthActive = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustStatusId ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthStatusId = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustWeights ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthWeights = _startWidth + e.getX() - _startX;
        }
        setJobColumnWidths();
    }
    
    public void updateDisplayedData() {
        //  Run through the list of all "child" nodes, which are all of the listed
        //  cluster nodes.
        for ( Iterator<JobNode> iter = _jobs.iterator(); iter.hasNext(); ) {
            JobNode thisJob = (JobNode)(iter.next());
            //  Change the settings on these items to match our current specifications.
            thisJob.showNetworkActivity( _showNetworkActivity.getState() );
            thisJob.showName( _showName.getState() );
            thisJob.showProgressBar( _showProgressBar.getState() );
            thisJob.showState( _showState.getState() );
            thisJob.showExperiment( _showExperiment.getState() );
            thisJob.showPass( _showPass.getState() );
            thisJob.showPriority( _showPriority.getState() );
            thisJob.showQueueTime( _showQueueTime.getState() );
            thisJob.showCorrelationStart( _showCorrelationStart.getState() );
            thisJob.showCorrelationEnd( _showCorrelationEnd.getState() );
            thisJob.showJobStart( _showJobStart.getState() );
            thisJob.showJobDuration( _showJobDuration.getState() );
            thisJob.showInputFile( _showInputFile.getState() );
            thisJob.showOutputFile( _showOutputFile.getState() );
            thisJob.showOutputSize( _showOutputSize.getState() );
            thisJob.showDifxVersion( _showDifxVersion.getState() );
            thisJob.showSpeedUpFactor( _showSpeedUpFactor.getState() );
            thisJob.showNumAntennas( _showNumAntennas.getState() );
            thisJob.showNumForeignAntennas( _showNumForeignAntennas.getState() );
            thisJob.showDutyCycle( _showDutyCycle.getState() );
            thisJob.showStatus( _showStatus.getState() );
            thisJob.showActive( _showActive.getState() );
            thisJob.showStatusId( _showStatusId.getState() );
            thisJob.showWeights( _showWeights.getState() );
            thisJob.showWeightsAsPlots( _showWeightsAsPlots.getState() );
            thisJob.updateUI();
        }
        //  Update the headers as well.
        _nameArea.setVisible( _showName.getState() );
        _progressBar.setVisible( _showProgressBar.getState() );
        _state.setVisible( _showState.getState() );
        _experiment.setVisible( _showExperiment.getState() );
        _pass.setVisible( _showPass.getState() );
        _priority.setVisible( _showPriority.getState() );
        _queueTime.setVisible( _showQueueTime.getState() );
        _correlationStart.setVisible( _showCorrelationStart.getState() );
        _correlationEnd.setVisible( _showCorrelationEnd.getState() );
        _jobStart.setVisible( _showJobStart.getState() );
        _jobDuration.setVisible( _showJobDuration.getState() );
        _inputFile.setVisible( _showInputFile.getState() );
        _outputFile.setVisible( _showOutputFile.getState() );
        _outputSize.setVisible( _showOutputSize.getState() );
        _difxVersion.setVisible( _showDifxVersion.getState() );
        _speedUpFactor.setVisible( _showSpeedUpFactor.getState() );
        _numAntennas.setVisible( _showNumAntennas.getState() );
        _numForeignAntennas.setVisible( _showNumForeignAntennas.getState() );
        _dutyCycle.setVisible( _showDutyCycle.getState() );
        _status.setVisible( _showStatus.getState() );
        _active.setVisible( _showActive.getState() );
        _statusId.setVisible( _showStatusId.getState() );
        _weights.setVisible( _showWeights.getState() );
        this.updateUI();
        
    }
    
    protected JCheckBoxMenuItem _showNetworkActivity;
    
    protected ColumnTextArea _nameArea;
    protected int _widthName;
    protected JCheckBoxMenuItem _showName;
    protected int _positionName;
    protected boolean _adjustName;
    
    protected ColumnTextArea _progressBar;
    protected int _widthProgressBar;
    protected JCheckBoxMenuItem _showProgressBar;
    protected int _positionProgressBar;
    protected boolean _adjustProgressBar;
    
    protected ColumnTextArea _state;
    protected int _widthState;
    protected JCheckBoxMenuItem _showState;
    protected int _positionState;
    protected boolean _adjustState;
    
    protected ColumnTextArea _experiment;
    protected int _widthExperiment;
    protected JCheckBoxMenuItem _showExperiment;
    protected int _positionExperiment;
    protected boolean _adjustExperiment;
    
    protected ColumnTextArea _pass;
    protected int _widthPass;
    protected JCheckBoxMenuItem _showPass;
    protected int _positionPass;
    protected boolean _adjustPass;
    
    protected ColumnTextArea _priority;
    protected int _widthPriority;
    protected JCheckBoxMenuItem _showPriority;
    protected int _positionPriority;
    protected boolean _adjustPriority;
    
    protected ColumnTextArea _queueTime;
    protected int _widthQueueTime;
    protected JCheckBoxMenuItem _showQueueTime;
    protected int _positionQueueTime;
    protected boolean _adjustQueueTime;
    
    protected ColumnTextArea _correlationStart;
    protected int _widthCorrelationStart;
    protected JCheckBoxMenuItem _showCorrelationStart;
    protected int _positionCorrelationStart;
    protected boolean _adjustCorrelationStart;
    
    protected ColumnTextArea _correlationEnd;
    protected int _widthCorrelationEnd;
    protected JCheckBoxMenuItem _showCorrelationEnd;
    protected int _positionCorrelationEnd;
    protected boolean _adjustCorrelationEnd;
    
    protected ColumnTextArea _jobStart;
    protected int _widthJobStart;
    protected JCheckBoxMenuItem _showJobStart;
    protected int _positionJobStart;
    protected boolean _adjustJobStart;
    
    protected ColumnTextArea _jobDuration;
    protected int _widthJobDuration;
    protected JCheckBoxMenuItem _showJobDuration;
    protected int _positionJobDuration;
    protected boolean _adjustJobDuration;
    
    protected ColumnTextArea _inputFile;
    protected int _widthInputFile;
    protected JCheckBoxMenuItem _showInputFile;
    protected int _positionInputFile;
    protected boolean _adjustInputFile;
    
    protected ColumnTextArea _outputFile;
    protected int _widthOutputFile;
    protected JCheckBoxMenuItem _showOutputFile;
    protected int _positionOutputFile;
    protected boolean _adjustOutputFile;
    
    protected ColumnTextArea _outputSize;
    protected int _widthOutputSize;
    protected JCheckBoxMenuItem _showOutputSize;
    protected int _positionOutputSize;
    protected boolean _adjustOutputSize;
    
    protected ColumnTextArea _difxVersion;
    protected int _widthDifxVersion;
    protected JCheckBoxMenuItem _showDifxVersion;
    protected int _positionDifxVersion;
    protected boolean _adjustDifxVersion;
    
    protected ColumnTextArea _speedUpFactor;
    protected int _widthSpeedUpFactor;
    protected JCheckBoxMenuItem _showSpeedUpFactor;
    protected int _positionSpeedUpFactor;
    protected boolean _adjustSpeedUpFactor;
    
    protected ColumnTextArea _numAntennas;
    protected int _widthNumAntennas;
    protected JCheckBoxMenuItem _showNumAntennas;
    protected int _positionNumAntennas;
    protected boolean _adjustNumAntennas;
    
    protected ColumnTextArea _numForeignAntennas;
    protected int _widthNumForeignAntennas;
    protected JCheckBoxMenuItem _showNumForeignAntennas;
    protected int _positionNumForeignAntennas;
    protected boolean _adjustNumForeignAntennas;
    
    protected ColumnTextArea _dutyCycle;
    protected int _widthDutyCycle;
    protected JCheckBoxMenuItem _showDutyCycle;
    protected int _positionDutyCycle;
    protected boolean _adjustDutyCycle;
    
    protected ColumnTextArea _status;
    protected int _widthStatus;
    protected JCheckBoxMenuItem _showStatus;
    protected int _positionStatus;
    protected boolean _adjustStatus;
    
    protected ColumnTextArea _active;
    protected int _widthActive;
    protected JCheckBoxMenuItem _showActive;
    protected int _positionActive;
    protected boolean _adjustActive;
    
    protected ColumnTextArea _statusId;
    protected int _widthStatusId;
    protected JCheckBoxMenuItem _showStatusId;
    protected int _positionStatusId;
    protected boolean _adjustStatusId;
    
    protected ColumnTextArea _weights;
    protected int _widthWeights;
    protected JCheckBoxMenuItem _showWeights;
    protected int _positionWeights;
    protected boolean _adjustWeights;
    JCheckBoxMenuItem _showWeightsAsNumbers;
    JCheckBoxMenuItem _showWeightsAsPlots;
    
    protected int _xOff;
    
    protected Cursor _columnAdjustCursor;
    protected Cursor _normalCursor;
    
    ArrayDeque<JobNode> _jobs;
    
    protected int _startWidth;
    protected int _startX;
    
    
}
