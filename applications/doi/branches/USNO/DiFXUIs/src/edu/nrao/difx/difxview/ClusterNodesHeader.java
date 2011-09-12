/*
 * This class acts as a container for a list of cluster nodes (used in the
 * HardwareMonitorPanel).  It has a popup menu for activating or removing
 * different data displays and headers for each of them.
 */
package edu.nrao.difx.difxview;

import javax.swing.JMenuItem;
import javax.swing.JPopupMenu;
import javax.swing.JSeparator;
import javax.swing.JCheckBoxMenuItem;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseEvent;
import java.awt.Cursor;
import java.awt.Component;
import java.awt.Container;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;

import java.util.Iterator;

public class ClusterNodesHeader extends BrowserNode {
    
    public ClusterNodesHeader( String name ) {
        super( name );
        _normalCursor = this.getCursor();
        _columnAdjustCursor = new Cursor( Cursor.W_RESIZE_CURSOR );
        initializeDisplaySettings();
        setChildColumnWidths();
        _popupButton.setVisible( true );
    }
    
    @Override
    public void createAdditionalItems() {
        _numCPUs = new ColumnTextArea( "CPUs" );
        _numCPUs.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showNumCPUs.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _numCPUs );
        _numCores = new ColumnTextArea( "Cores" );
        _numCores.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showNumCores.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _numCores );
        _bogusGHz = new ColumnTextArea( "GHz" );
        _bogusGHz.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showBogusGHz.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _bogusGHz );
        _type = new ColumnTextArea( "Type Code" );
        _type.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showType.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _type );
        _typeString = new ColumnTextArea( "Type" );
        _typeString.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showTypeString.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _typeString );
        _state = new ColumnTextArea( "State" );
        _state.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showState.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _state );
        _enabled = new ColumnTextArea( "Enabled" );
        _enabled.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showEnabled.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _enabled );
        _cpuLoad = new ColumnTextArea( "CPU Usage" );
        _cpuLoad.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showCpuLoad.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _cpuLoad );
        _cpuLoadPlot = new ColumnTextArea( "CPU Usage" );
        _cpuLoadPlot.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showCpuLoadPlot.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _cpuLoadPlot );
        _usedMem = new ColumnTextArea( "Used Mem" );
        _usedMem.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showUsedMem.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _usedMem );
        _totalMem = new ColumnTextArea( "Total Mem");
        _totalMem.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showTotalMem.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _totalMem );
        _memLoad = new ColumnTextArea( "Mem Usage" );
        _memLoad.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showMemLoad.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _memLoad );
        _memLoadPlot = new ColumnTextArea( "Mem Usage" );
        _memLoadPlot.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showMemLoadPlot.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _memLoadPlot );
        _netRxRate = new ColumnTextArea( "Rx Rate" );
        _netRxRate.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showNetRxRate.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _netRxRate );
        _netTxRate = new ColumnTextArea( "Tx Rate" );
        _netTxRate.addKillButton(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                _showNetTxRate.setState( false );
                updateDisplayedData();
            }
        });
        this.add( _netTxRate );
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
        _broadcastMonitor = new JCheckBoxMenuItem( "Broadcast Monitor" );
        _broadcastMonitor.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _broadcastMonitor );
        _showNumCPUs = new JCheckBoxMenuItem( "CPUs" );
        _showNumCPUs.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showNumCPUs );
        _showNumCores = new JCheckBoxMenuItem( "Cores" );
        _showNumCores.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showNumCores );
        _showBogusGHz = new JCheckBoxMenuItem( "GHz" );
        _showBogusGHz.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showBogusGHz );
        _showType = new JCheckBoxMenuItem( "Type (Numeric)" );
        _showType.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showType );
        _showTypeString = new JCheckBoxMenuItem( "Type" );
        _showTypeString.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showTypeString );
        _showState = new JCheckBoxMenuItem( "State" );
        _showState.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showState );
        _showEnabled = new JCheckBoxMenuItem( "Enabled" );
        _showEnabled.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showEnabled );
        _showCpuLoad = new JCheckBoxMenuItem( "CPU Usage" );
        _showCpuLoad.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showCpuLoad );
        _showCpuLoadPlot = new JCheckBoxMenuItem( "CPU Usage Plot" );
        _showCpuLoadPlot.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showCpuLoadPlot );
        _showUsedMem = new JCheckBoxMenuItem( "Used Memory" );
        _showUsedMem.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showUsedMem );
        _showTotalMem = new JCheckBoxMenuItem( "Total Memory" );
        _showTotalMem.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showTotalMem );
        _showMemLoad = new JCheckBoxMenuItem( "Memory Usage" );
        _showMemLoad.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showMemLoad );
        _showMemLoadPlot = new JCheckBoxMenuItem( "Memory Usage Plot" );
        _showMemLoadPlot.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showMemLoadPlot );
        _showNetRxRate = new JCheckBoxMenuItem( "Net Receive Rate" );
        _showNetRxRate.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showNetRxRate );
        _showNetTxRate = new JCheckBoxMenuItem( "Net Transmit Rate" );
        _showNetTxRate.addActionListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                updateDisplayedData();
            }
        });
        _popup.add( _showNetTxRate );
    }
    
    /*
     * This function makes everything in the popup menu visible.
     */
    public void activateAll() {
        _broadcastMonitor.setState( true );
        _showNumCPUs.setState( true );
        _showNumCores.setState( true );
        _showBogusGHz.setState( true );
        _showType.setState( true );
        _showTypeString.setState( true );
        _showState.setState( true );
        _showEnabled.setState( true );
        _showCpuLoad.setState( true );
        _showCpuLoadPlot.setState( true );
        _showUsedMem.setState( true );
        _showTotalMem.setState( true );
        _showMemLoad.setState( true );
        _showMemLoadPlot.setState( true );
        _showNetRxRate.setState( true );
        _showNetTxRate.setState( true );
        updateDisplayedData();
    }

    @Override
    public void positionItems() {
        super.positionItems();
        _xOff = 220;
        if ( _broadcastMonitor.getState() )
            _xOff += 14;
        _popupButton.setBounds( _xOff + 2, 2, 16, _ySize - 4 );
        _xOff += 20;
        if ( _showNumCPUs.getState() ) {
            setTextArea( _numCPUs, _widthNumCPUs );
            _positionNumCPUs = _xOff;
        }
        else
            _positionNumCPUs = -100;
        if ( _showNumCores.getState() ) {
            setTextArea( _numCores, _widthNumCores );
            _positionNumCores = _xOff;
        }
        else
            _positionNumCores = -100;
        if ( _showBogusGHz.getState() ) {
            setTextArea( _bogusGHz, _widthBogusGHz );
            _positionBogusGHz = _xOff;
        }
        else
            _positionBogusGHz = -100;
        if ( _showType.getState() ) {
            setTextArea( _type, _widthType );
            _positionType = _xOff;
        }
        else
            _positionType = -100;
        if ( _showTypeString.getState() ) {
            setTextArea( _typeString, _widthTypeString );
            _positionTypeString = _xOff;
        }
        else
            _positionTypeString = -100;
        if ( _showState.getState() ) {
            setTextArea( _state, _widthState );
            _positionState = _xOff;
        }
        else
            _positionState = -100;
        if ( _showEnabled.getState() ) {
            setTextArea( _enabled, _widthEnabled );
            _positionEnabled = _xOff;
        }
        else
            _positionEnabled = -100;
        if ( _showCpuLoad.getState() ) {
            setTextArea( _cpuLoad, _widthCpuLoad );
            _positionCpuLoad = _xOff;
        }
        else
            _positionCpuLoad = -100;
        if ( _showCpuLoadPlot.getState() ) {
            //  If the header "CPU Usage" is already displayed for the previous
            //  column, don't repeat it.
            if ( _showCpuLoad.getState() )
                _cpuLoadPlot.setText( "" );
            else
                _cpuLoadPlot.setText( "CPU Usage" );
            setTextArea( _cpuLoadPlot, _widthCpuLoadPlot );
            _positionCpuLoadPlot = _xOff;
        }
        else
            _positionCpuLoadPlot = -100;
        if ( _showUsedMem.getState() ) {
            setTextArea( _usedMem, _widthUsedMem );
            _positionUsedMem = _xOff;
        }
        else
            _positionUsedMem = -100;
        if ( _showTotalMem.getState() ) {
            setTextArea( _totalMem, _widthTotalMem );
            _positionTotalMem = _xOff;
        }
        else
            _positionTotalMem = -100;
        if ( _showMemLoad.getState() ) {
            setTextArea( _memLoad, _widthMemLoad );
            _positionMemLoad = _xOff;
        }
        else
            _positionMemLoad = -100;
        if ( _showMemLoadPlot.getState() ) {
            //  As with the CPU plot - don't repeat a column header if it is
            //  already there.
            if ( _showMemLoad.getState() )
                _memLoadPlot.setText( "" );
            else
                _memLoadPlot.setText( "Mem Usage" );
            setTextArea( _memLoadPlot, _widthMemLoadPlot );
            _positionMemLoadPlot = _xOff;
        }
        else
            _positionMemLoadPlot = -100;
        if ( _showNetRxRate.getState() ) {
            setTextArea( _netRxRate, _widthNetRxRate );
            _positionNetRxRate = _xOff;
        }
        else
            _positionNetRxRate = -100;
        if ( _showNetTxRate.getState() ) {
            setTextArea( _netTxRate, _widthNetTxRate );
            _positionNetTxRate = _xOff;
        }
        else
            _positionNetTxRate = -100;
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
        _broadcastMonitor.setState( true );
        _showNumCPUs.setState( false );
        _showNumCores.setState( false );
        _showBogusGHz.setState( false );
        _showType.setState( false );
        _showTypeString.setState( false );
        _showState.setState( true );
        _showEnabled.setState( false );
        _showCpuLoad.setState( false );
        _showCpuLoadPlot.setState( true );
        _showUsedMem.setState( false );
        _showTotalMem.setState( false );
        _showMemLoad.setState( false );
        _showMemLoadPlot.setState( true );
        _showNetRxRate.setState( true );
        _showNetTxRate.setState( true );
        setColumnWidths();
    }
    
    /*
     * Set the widths of the various columns.  These are all default widths
     * now, but in theory a settings structure could be handed to this function
     * to set widths specified by users.
     */
    public void setColumnWidths() {
        _widthNumCPUs = 70;
        _widthNumCores = 70;
        _widthBogusGHz = 70;
        _widthType = 70;
        _widthTypeString = 70;
        _widthState = 100;
        _widthEnabled = 70;
        _widthCpuLoad = 70;
        _widthCpuLoadPlot = 70;
        _widthUsedMem = 70;
        _widthTotalMem = 70;
        _widthMemLoad = 70;
        _widthMemLoadPlot = 70;
        _widthNetRxRate = 70;
        _widthNetTxRate = 70;
    }
    
    /*
     * This functions propogates current column widths to all children.
     */
    public void setChildColumnWidths() {
        for ( Iterator<BrowserNode> iter = _children.iterator(); iter.hasNext(); ) {
            ClusterNode thisNode = (ClusterNode)(iter.next());
            //  Change the settings on these items to match our current specifications.
            thisNode.widthNumCPUs( _widthNumCPUs );
            thisNode.widthNumCores( _widthNumCores );
            thisNode.widthBogusGHz( _widthBogusGHz );
            thisNode.widthType( _widthType );
            thisNode.widthTypeString( _widthTypeString );
            thisNode.widthState( _widthState );
            thisNode.widthEnabled( _widthEnabled );
            thisNode.widthCpuLoad( _widthCpuLoad );
            thisNode.widthCpuLoadPlot( _widthCpuLoadPlot );
            thisNode.widthUsedMem( _widthUsedMem );
            thisNode.widthTotalMem( _widthTotalMem );
            thisNode.widthMemLoad( _widthMemLoad );
            thisNode.widthMemLoadPlot( _widthMemLoadPlot );
            thisNode.widthNetRxRate( _widthNetRxRate );
            thisNode.widthNetTxRate( _widthNetTxRate );
            thisNode.updateUI();
        }
    }
    
    @Override
    public void addChild( BrowserNode newNode ) {
        super.addChild( newNode );
        setChildColumnWidths();
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
        _adjustNumCPUs = false;
        _adjustNumCores = false;
        _adjustBogusGHz = false;
        _adjustType = false;
        _adjustTypeString = false;
        _adjustState = false;
        _adjustEnabled = false;
        _adjustCpuLoad = false;
        _adjustCpuLoadPlot = false;
        _adjustUsedMem = false;
        _adjustTotalMem = false;
        _adjustMemLoad = false;
        _adjustMemLoadPlot = false;
        _adjustNetRxRate = false;
        _adjustNetTxRate = false;
        if ( e.getX() > _positionNumCPUs - 3 && e.getX() < _positionNumCPUs + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustNumCPUs = true;
        }
        else if ( e.getX() > _positionNumCores - 3 && e.getX() < _positionNumCores + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustNumCores = true;
        }
        else if ( e.getX() > _positionBogusGHz - 3 && e.getX() < _positionBogusGHz + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustBogusGHz = true;
        }
        else if ( e.getX() > _positionType - 3 && e.getX() < _positionType + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustType = true;
        }
        else if ( e.getX() > _positionTypeString - 3 && e.getX() < _positionTypeString + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustTypeString = true;
        }
        else if ( e.getX() > _positionState - 3 && e.getX() < _positionState + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustState = true;
        }
        else if ( e.getX() > _positionEnabled - 3 && e.getX() < _positionEnabled + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustEnabled = true;
        }
        else if ( e.getX() > _positionCpuLoad - 3 && e.getX() < _positionCpuLoad + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustCpuLoad = true;
        }
        else if ( e.getX() > _positionCpuLoadPlot - 3 && e.getX() < _positionCpuLoadPlot + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustCpuLoadPlot = true;
        }
        else if ( e.getX() > _positionUsedMem - 3 && e.getX() < _positionUsedMem + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustUsedMem = true;
        }
        else if ( e.getX() > _positionTotalMem - 3 && e.getX() < _positionTotalMem + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustTotalMem = true;
        }
        else if ( e.getX() > _positionMemLoad - 3 && e.getX() < _positionMemLoad + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustMemLoad = true;
        }
        else if ( e.getX() > _positionMemLoadPlot - 3 && e.getX() < _positionMemLoadPlot + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustMemLoadPlot = true;
        }
        else if ( e.getX() > _positionNetRxRate - 3 && e.getX() < _positionNetRxRate + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustNetRxRate = true;
        }
        else if ( e.getX() > _positionNetTxRate - 3 && e.getX() < _positionNetTxRate + 2 ) {
            setCursor( _columnAdjustCursor );
            _adjustNetTxRate = true;
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
        if ( _adjustNumCPUs ) {
            _startWidth = _widthNumCPUs;
            _startX = e.getX();
        }
        else if ( _adjustNumCores ) {
            _startWidth = _widthNumCores;
            _startX = e.getX();
        }
        else if ( _adjustBogusGHz ) {
            _startWidth = _widthBogusGHz;
            _startX = e.getX();
        }
        else if ( _adjustType ) {
            _startWidth = _widthType;
            _startX = e.getX();
        }
        else if ( _adjustTypeString ) {
            _startWidth = _widthTypeString;
            _startX = e.getX();
        }
        else if ( _adjustState ) {
            _startWidth = _widthState;
            _startX = e.getX();
        }
        else if ( _adjustEnabled ) {
            _startWidth = _widthEnabled;
            _startX = e.getX();
        }
        else if ( _adjustCpuLoad ) {
            _startWidth = _widthCpuLoad;
            _startX = e.getX();
        }
        else if ( _adjustCpuLoadPlot ) {
            _startWidth = _widthCpuLoadPlot;
            _startX = e.getX();
        }
        else if ( _adjustUsedMem ) {
            _startWidth = _widthUsedMem;
            _startX = e.getX();
        }
        else if ( _adjustTotalMem ) {
            _startWidth = _widthTotalMem;
            _startX = e.getX();
        }
        else if ( _adjustMemLoad ) {
            _startWidth = _widthMemLoad;
            _startX = e.getX();
        }
        else if ( _adjustMemLoadPlot ) {
            _startWidth = _widthMemLoadPlot;
            _startX = e.getX();
        }
        else if ( _adjustNetRxRate ) {
            _startWidth = _widthNetRxRate;
            _startX = e.getX();
        }
        else if ( _adjustNetTxRate ) {
            _startWidth = _widthNetTxRate;
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
        if ( _adjustNumCPUs ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthNumCPUs = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustNumCores ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthNumCores = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustBogusGHz ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthBogusGHz = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustType ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthType = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustTypeString ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthTypeString = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustState ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthState = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustEnabled ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthEnabled = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustCpuLoad ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthCpuLoad = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustCpuLoadPlot ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthCpuLoadPlot = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustUsedMem ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthUsedMem = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustTotalMem ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthTotalMem = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustMemLoad ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthMemLoad = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustMemLoadPlot ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthMemLoadPlot = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustNetRxRate ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthNetRxRate = _startWidth + e.getX() - _startX;
        }
        else if ( _adjustNetTxRate ) {
            if ( e.getX() - _startX + _startWidth > 5 )
                _widthNetTxRate = _startWidth + e.getX() - _startX;
        }
        setChildColumnWidths();
    }
    
   
    public void updateDisplayedData() {
        //  Run through the list of all "child" nodes, which are all of the listed
        //  cluster nodes.
        for ( Iterator<BrowserNode> iter = _children.iterator(); iter.hasNext(); ) {
            ClusterNode thisNode = (ClusterNode)(iter.next());
            //  Change the settings on these items to match our current specifications.
            thisNode.showNetworkActivity( _broadcastMonitor.getState() );
            thisNode.showNumCPUs( _showNumCPUs.getState() );
            thisNode.showNumCores( _showNumCores.getState() );
            thisNode.showBogusGHz( _showBogusGHz.getState() );
            thisNode.showType( _showType.getState() );
            thisNode.showTypeString( _showTypeString.getState() );
            thisNode.showState( _showState.getState() );
            thisNode.showEnabled( _showEnabled.getState() );
            thisNode.showCpuLoad( _showCpuLoad.getState() );
            thisNode.showCpuLoadPlot( _showCpuLoadPlot.getState() );
            thisNode.showUsedMem( _showUsedMem.getState() );
            thisNode.showTotalMem( _showTotalMem.getState() );
            thisNode.showMemLoad( _showMemLoad.getState() );
            thisNode.showMemLoadPlot( _showMemLoadPlot.getState() );
            thisNode.showNetRxRate( _showNetRxRate.getState() );
            thisNode.showNetTxRate( _showNetTxRate.getState() );
            thisNode.updateUI();
        }
        //  Update the headers as well.
        _numCPUs.setVisible( _showNumCPUs.getState() );
        _numCores.setVisible( _showNumCores.getState() );
        _bogusGHz.setVisible( _showBogusGHz.getState() );
        _type.setVisible( _showType.getState() );
        _typeString.setVisible( _showTypeString.getState() );
        _state.setVisible( _showState.getState() );
        _enabled.setVisible( _showEnabled.getState() );
        _cpuLoad.setVisible( _showCpuLoad.getState() );
        _cpuLoadPlot.setVisible( _showCpuLoadPlot.getState() );
        _usedMem.setVisible( _showUsedMem.getState() );
        _totalMem.setVisible( _showTotalMem.getState() );
        _memLoad.setVisible( _showMemLoad.getState() );
        _memLoadPlot.setVisible( _showMemLoadPlot.getState() );
        _netRxRate.setVisible( _showNetRxRate.getState() );
        _netTxRate.setVisible( _showNetTxRate.getState() );
        this.updateUI();
        
    }
    
    protected JCheckBoxMenuItem _broadcastMonitor;
    protected JCheckBoxMenuItem _showNumCPUs;
    protected JCheckBoxMenuItem _showNumCores;
    protected JCheckBoxMenuItem _showBogusGHz;
    protected JCheckBoxMenuItem _showType;
    protected JCheckBoxMenuItem _showTypeString;
    protected JCheckBoxMenuItem _showState;
    protected JCheckBoxMenuItem _showEnabled;
    protected JCheckBoxMenuItem _showCpuLoad;
    protected JCheckBoxMenuItem _showCpuLoadPlot;
    protected JCheckBoxMenuItem _showUsedMem;
    protected JCheckBoxMenuItem _showTotalMem;
    protected JCheckBoxMenuItem _showMemLoad;
    protected JCheckBoxMenuItem _showMemLoadPlot;
    protected JCheckBoxMenuItem _showNetRxRate;
    protected JCheckBoxMenuItem _showNetTxRate;
    
    ColumnTextArea _numCPUs;
    ColumnTextArea _numCores;
    ColumnTextArea _bogusGHz;
    ColumnTextArea _type;
    ColumnTextArea _typeString;
    ColumnTextArea _state;
    ColumnTextArea _enabled;
    ColumnTextArea _cpuLoad;
    ColumnTextArea _cpuLoadPlot;
    ColumnTextArea _usedMem;
    ColumnTextArea _totalMem;
    ColumnTextArea _memLoad;
    ColumnTextArea _memLoadPlot;
    ColumnTextArea _netRxRate;
    ColumnTextArea _netTxRate;

    int _widthNumCPUs;
    int _widthNumCores;
    int _widthBogusGHz;
    int _widthType;
    int _widthTypeString;
    int _widthState;
    int _widthEnabled;
    int _widthCpuLoad;
    int _widthCpuLoadPlot;
    int _widthUsedMem;
    int _widthTotalMem;
    int _widthMemLoad;
    int _widthMemLoadPlot;
    int _widthNetRxRate;
    int _widthNetTxRate;

    int _positionNumCPUs;
    int _positionNumCores;
    int _positionBogusGHz;
    int _positionType;
    int _positionTypeString;
    int _positionState;
    int _positionEnabled;
    int _positionCpuLoad;
    int _positionCpuLoadPlot;
    int _positionUsedMem;
    int _positionTotalMem;
    int _positionMemLoad;
    int _positionMemLoadPlot;
    int _positionNetRxRate;
    int _positionNetTxRate;

    boolean _adjustNumCPUs;
    boolean _adjustNumCores;
    boolean _adjustBogusGHz;
    boolean _adjustType;
    boolean _adjustTypeString;
    boolean _adjustState;
    boolean _adjustEnabled;
    boolean _adjustCpuLoad;
    boolean _adjustCpuLoadPlot;
    boolean _adjustUsedMem;
    boolean _adjustTotalMem;
    boolean _adjustMemLoad;
    boolean _adjustMemLoadPlot;
    boolean _adjustNetRxRate;
    boolean _adjustNetTxRate;

    protected int _xOff;
    
    protected Cursor _columnAdjustCursor;
    protected Cursor _normalCursor;
    
    protected int _startWidth;
    protected int _startX;
    
}
