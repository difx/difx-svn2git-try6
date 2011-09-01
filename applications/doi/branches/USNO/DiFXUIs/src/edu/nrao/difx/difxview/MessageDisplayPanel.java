/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.nrao.difx.difxview;

import java.awt.Color;
import java.util.Date;
import java.util.Calendar;
import java.util.Iterator;
import java.util.ArrayDeque;

import javax.swing.JPanel;
import javax.swing.JButton;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JCheckBoxMenuItem;
import javax.swing.JMenuBar;
import javax.swing.Box;
import javax.swing.JSeparator;
import javax.swing.JOptionPane;


/**
 *
 * @author jspitzak
 */
public class MessageDisplayPanel extends JPanel { //extends NodeBrowserScrollPane {
    
    public MessageDisplayPanel() {
        initComponents();
        _browser.setBackground( Color.BLACK );
    }
    
    public void initComponents() {
        setLayout( null );
        _browser = new NodeBrowserScrollPane();
        this.add( _browser );
        _menuBar = new JMenuBar();
        this.add( _menuBar );
        _showMenu = new JMenu( "  Show  " );
        _menuBar.add( _showMenu );
        _includeMenu = new JMenu( "  Include  " );
        _menuBar.add( _includeMenu );
        _filterMenu = new JMenu( "  Filter  " );
        _menuBar.add( _filterMenu );
        _menuBar.add( Box.createHorizontalGlue() );
        _navigateMenu = new JMenu( "  Navigate  " );
        _menuBar.add( _navigateMenu );
        _clearMenu = new JMenu( "  Clear  " );
        _menuBar.add( _clearMenu );
        
        //  Show menu items
        _showErrors = new JCheckBoxMenuItem( "Errors", true );
        _showErrors.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                showErrorsAction( e );
            }
        });
        _showMenu.add( _showErrors );
        _showWarnings = new JCheckBoxMenuItem( "Warnings", true );
        _showWarnings.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                showWarningsAction( e );
            }
        });
        _showMenu.add( _showWarnings );
        _showMessages = new JCheckBoxMenuItem( "Messages", true );
        _showMessages.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                showMessagesAction( e );
            }
        });
        _showMenu.add( _showMessages );
        _showChecked = new JCheckBoxMenuItem( "Checked", true );
        _showChecked.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                showCheckedAction( e );
            }
        });
        _showMenu.add( _showChecked );
        _showUnchecked = new JCheckBoxMenuItem( "Unchecked", true );
        _showUnchecked.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                showUncheckedAction( e );
            }
        });
        _showMenu.add( _showUnchecked );

        //  Include menu items
        _showDate = new JCheckBoxMenuItem( "Date", false );
        _showDate.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                showDateAction( e );
            }
        });
        _includeMenu.add( _showDate );
        _showTime = new JCheckBoxMenuItem( "Time", true );
        _showTime.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                showTimeAction( e );
            }
        });
        _includeMenu.add( _showTime );
        _showSource = new JCheckBoxMenuItem( "Source", true );
        _showSource.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                showSourceAction( e );
            }
        });
        _includeMenu.add( _showSource );

        //  Filter menu items
        _currentFilter = null;
        _filterSource = new JCheckBoxMenuItem( "Filter Source", false );
        _filterSource.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                runFilter();
            }
        });
        _filterMenu.add( _filterSource );
        _filterMessage = new JCheckBoxMenuItem( "Filter Message", true );
        _filterMessage.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                runFilter();
            }
        });
        _filterMenu.add( _filterMessage );
        _filterMenu.add( new JSeparator() );
        _containingMenu = new JMenu( "Search for String..." );
        _filterMenu.add( _containingMenu );
        _filters = new ArrayDeque<String>();
        _filters.add( "*" );
        buildFilterMenu();

        //  Navigate menu items
        _navigateTop = new JMenuItem( "Top" );
        _navigateTop.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                navigateTopAction( e );
            }
        });
        _navigateMenu.add( _navigateTop );
        _navigateBottom = new JMenuItem( "Bottom" );
        _navigateBottom.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                navigateBottomAction( e );
            }
        });
        _navigateMenu.add( _navigateBottom );
        _previousError = new JMenuItem( "Previous Error" );
        _previousError.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                previousErrorAction( e );
            }
        });
        _navigateMenu.add( _previousError );
        _nextError = new JMenuItem( "Next Error" );
        _nextError.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                nextErrorAction( e );
            }
        });
        _navigateMenu.add( _nextError );
        _previousWarning = new JMenuItem( "Previous Warning or Error" );
        _previousWarning.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                previousWarningAction( e );
            }
        });
        _navigateMenu.add( _previousWarning );
        _nextWarning = new JMenuItem( "Next Warning or Error" );
        _nextWarning.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                nextWarningAction( e );
            }
        });
        _navigateMenu.add( _nextWarning );
        _previousChecked = new JMenuItem( "Previous Checked" );
        _previousChecked.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                previousCheckedAction( e );
            }
        });
        _navigateMenu.add( _previousChecked );
        _nextChecked = new JMenuItem( "Next Checked" );
        _nextChecked.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                nextCheckedAction( e );
            }
        });
        _navigateMenu.add( _nextChecked );

        //  Clear menu items
        _clearAll = new JMenuItem( "Errors, Warnings and Messages" );
        _clearAll.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                clearAllAction( e );
            }
        });
        _clearMenu.add( _clearAll );
        _clearWarningsAndMessages = new JMenuItem( "Warnings and Messages" );
        _clearWarningsAndMessages.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                clearWarningsAndMessagesAction( e );
           }
        });
        _clearMenu.add( _clearWarningsAndMessages );
        _clearMessages = new JMenuItem( "Messages" );
        _clearMessages.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                clearMessagesAction( e );
            }
        });
        _clearMenu.add( _clearMessages );
        _clearChecked = new JMenuItem( "Checked" );
        _clearChecked.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                clearCheckedAction( e );
            }
        });
        _clearMenu.add( _clearChecked );
        _clearUnchecked = new JMenuItem( "Unchecked" );
        _clearUnchecked.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                clearUncheckedAction( e );
            }
        });
        _clearMenu.add( _clearUnchecked );
    }
    
    @Override
    public void setBounds( int x, int y, int w, int h ) {
        _browser.setBounds( 0, 20, w, h - 20 );
        _menuBar.setBounds( 0, 0, w, 20 );
        super.setBounds( x, y, w, h );
    }
    
    /*
     * The clear button generates submenus that can be used to clear specific
     * items.
     */
    protected void clearButtonAction( java.awt.event.ActionEvent e ) {
        _clearMenu.setVisible( true );
    }
        
    protected void clearAllAction( java.awt.event.ActionEvent e ) {
        _browser.clear();
    }
    
    /*
     * Search through all messages and eliminate those that are warnings or
     * informational.
     */
    protected void clearWarningsAndMessagesAction( java.awt.event.ActionEvent e ) {
        BrowserNode topNode = _browser.browserTopNode();
        ArrayDeque<MessageNode> removeList = new ArrayDeque<MessageNode>();
        for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
            MessageNode thisMessage = (MessageNode)( iter.next() );
            if ( thisMessage.severity() != MessageNode.ERROR )
                removeList.add( thisMessage );
        }
        for ( Iterator<MessageNode> iter = removeList.iterator(); iter.hasNext(); ) {
                topNode.removeChild( iter.next() );
        }
        _browser.listChange();
    }
    
    protected void clearMessagesAction( java.awt.event.ActionEvent e ) {
        BrowserNode topNode = _browser.browserTopNode();
        ArrayDeque<MessageNode> removeList = new ArrayDeque<MessageNode>();
        for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
            MessageNode thisMessage = (MessageNode)( iter.next() );
            if ( thisMessage.severity() == MessageNode.INFO )
                removeList.add( thisMessage );
        }
        for ( Iterator<MessageNode> iter = removeList.iterator(); iter.hasNext(); ) {
                topNode.removeChild( iter.next() );
        }
        _browser.listChange();
    }
    
    protected void clearCheckedAction( java.awt.event.ActionEvent e ) {
        BrowserNode topNode = _browser.browserTopNode();
        ArrayDeque<MessageNode> removeList = new ArrayDeque<MessageNode>();
        for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
            MessageNode thisMessage = (MessageNode)( iter.next() );
            if ( thisMessage.starred() )
                removeList.add( thisMessage );
        }
        for ( Iterator<MessageNode> iter = removeList.iterator(); iter.hasNext(); ) {
                topNode.removeChild( iter.next() );
        }
        _browser.listChange();
    }
    
    protected void clearUncheckedAction( java.awt.event.ActionEvent e ) {
        BrowserNode topNode = _browser.browserTopNode();
        ArrayDeque<MessageNode> removeList = new ArrayDeque<MessageNode>();
        for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
            MessageNode thisMessage = (MessageNode)( iter.next() );
            if ( !thisMessage.starred() )
                removeList.add( thisMessage );
        }
        for ( Iterator<MessageNode> iter = removeList.iterator(); iter.hasNext(); ) {
                topNode.removeChild( iter.next() );
        }
        _browser.listChange();
    }
    
    protected void showDateAction( java.awt.event.ActionEvent e ) {
        BrowserNode topNode = _browser.browserTopNode();
        for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
            MessageNode thisMessage = (MessageNode)( iter.next() );
            thisMessage.showDate( _showDate.isSelected() );
        }
        this.updateUI();
    }
    
    protected void showTimeAction( java.awt.event.ActionEvent e ) {
        BrowserNode topNode = _browser.browserTopNode();
        for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
            MessageNode thisMessage = (MessageNode)( iter.next() );
            thisMessage.showTime( _showTime.isSelected() );
        }
        this.updateUI();
    }
    
    protected void showSourceAction( java.awt.event.ActionEvent e ) {
        BrowserNode topNode = _browser.browserTopNode();
        for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
            MessageNode thisMessage = (MessageNode)( iter.next() );
            thisMessage.showSource( _showSource.isSelected() );
        }
        this.updateUI();
    }
    
    protected void showErrorsAction( java.awt.event.ActionEvent e ) {
        BrowserNode topNode = _browser.browserTopNode();
        for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
            MessageNode thisMessage = (MessageNode)( iter.next() );
            thisMessage.showErrors( _showErrors.isSelected() );
        }
        _browser.listChange();
    }
    
    protected void showWarningsAction( java.awt.event.ActionEvent e ) {
        BrowserNode topNode = _browser.browserTopNode();
        for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
            MessageNode thisMessage = (MessageNode)( iter.next() );
            thisMessage.showWarnings( _showWarnings.isSelected() );
        }
        _browser.listChange();
    }
    
    protected void showMessagesAction( java.awt.event.ActionEvent e ) {
        BrowserNode topNode = _browser.browserTopNode();
        for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
            MessageNode thisMessage = (MessageNode)( iter.next() );
            thisMessage.showMessages( _showMessages.isSelected() );
        }
        _browser.listChange();
    }
    
    protected void showCheckedAction( java.awt.event.ActionEvent e ) {
        BrowserNode topNode = _browser.browserTopNode();
        for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
            MessageNode thisMessage = (MessageNode)( iter.next() );
            thisMessage.showChecked( _showChecked.isSelected() );
        }
        _browser.listChange();
    }
    
    protected void showUncheckedAction( java.awt.event.ActionEvent e ) {
        BrowserNode topNode = _browser.browserTopNode();
        for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
            MessageNode thisMessage = (MessageNode)( iter.next() );
            thisMessage.showUnchecked( _showUnchecked.isSelected() );
        }
        _browser.listChange();
    }
    
    protected void buildFilterMenu() {
        _containingMenu.removeAll();
        for ( Iterator<String> iter = _filters.iterator(); iter.hasNext(); ) {
            final String thisFilter = new String( iter.next() );
            JCheckBoxMenuItem newFilter = new JCheckBoxMenuItem( thisFilter );
            newFilter.addActionListener( new java.awt.event.ActionListener() {
                public void actionPerformed( java.awt.event.ActionEvent e ) {
                    setFilterAction( e, thisFilter );
                }
            });
            if ( thisFilter.equals( _currentFilter ) ) {
                newFilter.setSelected( true );
            }
            else {
                newFilter.setSelected( false );
            }
            _containingMenu.add( newFilter );
        }
        _containingMenu.add( new JSeparator() );
        _newFilter = new JMenuItem( "Add New Search String" );
        _newFilter.addActionListener( new java.awt.event.ActionListener() {
            public void actionPerformed( java.awt.event.ActionEvent e ) {
                addNewFilterAction( e );
            }
        });
        _containingMenu.add( _newFilter );
    }
    
    protected void addNewFilterAction( java.awt.event.ActionEvent e ) {
        String newFilter = JOptionPane.showInputDialog( this, "New Filter:" );
        if ( newFilter != null ) {
            _filters.add( newFilter );
            _currentFilter = newFilter;
            //  The menu of filter options needs to be rewitten to include this
            //  item.  We do this (instead of just adding items to the end) so we
            //  can have "New Filter" at the end, where it belongs.
            this.buildFilterMenu();
            this.runFilter();
        }
    }
    
    protected void setFilterAction( java.awt.event.ActionEvent e, String newFilter ) {
        _currentFilter = newFilter;
        this.changeFilterMenu();
        this.runFilter();
    }
    
    protected void changeFilterMenu() {
        for ( int i = 0; i < _containingMenu.getItemCount() - 2; ++i ) {
            if ( _containingMenu.getItem( i ).getText().equals( _currentFilter ) )
                ((JCheckBoxMenuItem)(_containingMenu.getItem( i ))).setSelected( true );
            else
                ((JCheckBoxMenuItem)(_containingMenu.getItem( i ))).setSelected( false );
        }
    }
    
    /*
     * Run the current filter instructions on the existing browser items and
     * display the results.
     */
    protected void runFilter() {
        BrowserNode topNode = _browser.browserTopNode();
        for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
            MessageNode thisMessage = (MessageNode)( iter.next() );
            thisMessage.applyFilter( _filterSource.isSelected(), _filterMessage.isSelected(),
                    _currentFilter );
        }
        _browser.listChange();
    }
    
        public void navigateTopAction( java.awt.event.ActionEvent e ) {
            _browser.scrollToTop();
        }
        
        public void navigateBottomAction( java.awt.event.ActionEvent e ) {
            _browser.scrollToEnd();
        }
        
        /*
         * Locate the first error that is above the current location, and move
         * the browser to it.
         */
        public void previousErrorAction( java.awt.event.ActionEvent e ) {
            int yOffset = _browser.getYOffset();
            MessageNode goToNode = null;
            int newOffset = 0;
            int nodeOffset = 0;
            BrowserNode topNode = _browser.browserTopNode();
            for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
                MessageNode thisMessage = (MessageNode)( iter.next() );
                //  Measure the vertical offset of each message.  Make sure each
                //  is visible before we consider it.
                if ( thisMessage._showThis ) {
                    //  Now see if the message is of the correct type - error in
                    //  this case.
                    if ( thisMessage.severity() == MessageNode.ERROR ) {
                        if ( -nodeOffset > yOffset ) {
                            goToNode = thisMessage;
                            newOffset = -nodeOffset;
                        }
                    }
                    nodeOffset += thisMessage.setDrawConditions( nodeOffset, true );
                }
            }
            if ( goToNode != null )
                _browser.setYOffset( newOffset );
        }
        
        /*
         * Locate the first error below the current location and move the browser
         * to it.
         */
        public void nextErrorAction( java.awt.event.ActionEvent e ) {
            int yOffset = _browser.getYOffset();
            MessageNode goToNode = null;
            int newOffset = 0;
            int nodeOffset = 0;
            BrowserNode topNode = _browser.browserTopNode();
            for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
                MessageNode thisMessage = (MessageNode)( iter.next() );
                //  Measure the vertical offset of each message.  Make sure each
                //  is visible before we consider it.
                if ( thisMessage._showThis ) {
                    //  Now see if the message is of the correct type - error in
                    //  this case.
                    if ( thisMessage.severity() == MessageNode.ERROR ) {
                        if ( goToNode == null && -nodeOffset < yOffset ) {
                            goToNode = thisMessage;
                            newOffset = -nodeOffset;
                        }
                    }
                    nodeOffset += thisMessage.setDrawConditions( nodeOffset, true );
                }
            }
            if ( goToNode != null )
                _browser.setYOffset( newOffset );
        }
        
        public void previousWarningAction( java.awt.event.ActionEvent e ) {
            int yOffset = _browser.getYOffset();
            MessageNode goToNode = null;
            int newOffset = 0;
            int nodeOffset = 0;
            BrowserNode topNode = _browser.browserTopNode();
            for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
                MessageNode thisMessage = (MessageNode)( iter.next() );
                //  Measure the vertical offset of each message.  Make sure each
                //  is visible before we consider it.
                if ( thisMessage._showThis ) {
                    //  Now see if the message is of the correct type - error in
                    //  this case.
                    if ( thisMessage.severity() == MessageNode.ERROR ||
                         thisMessage.severity() == MessageNode.WARNING ) {
                        if ( -nodeOffset > yOffset ) {
                            goToNode = thisMessage;
                            newOffset = -nodeOffset;
                        }
                    }
                    nodeOffset += thisMessage.setDrawConditions( nodeOffset, true );
                }
            }
            if ( goToNode != null )
                _browser.setYOffset( newOffset );
        }
        
        public void nextWarningAction( java.awt.event.ActionEvent e ) {
            int yOffset = _browser.getYOffset();
            MessageNode goToNode = null;
            int newOffset = 0;
            int nodeOffset = 0;
            BrowserNode topNode = _browser.browserTopNode();
            for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
                MessageNode thisMessage = (MessageNode)( iter.next() );
                //  Measure the vertical offset of each message.  Make sure each
                //  is visible before we consider it.
                if ( thisMessage._showThis ) {
                    //  Now see if the message is of the correct type - error in
                    //  this case.
                    if ( thisMessage.severity() == MessageNode.ERROR ||
                         thisMessage.severity() == MessageNode.WARNING ) {
                        if ( goToNode == null && -nodeOffset < yOffset ) {
                            goToNode = thisMessage;
                            newOffset = -nodeOffset;
                        }
                    }
                    nodeOffset += thisMessage.setDrawConditions( nodeOffset, true );
                }
            }
            if ( goToNode != null )
                _browser.setYOffset( newOffset );
        }
        
        public void previousCheckedAction( java.awt.event.ActionEvent e ) {
            int yOffset = _browser.getYOffset();
            MessageNode goToNode = null;
            int newOffset = 0;
            int nodeOffset = 0;
            BrowserNode topNode = _browser.browserTopNode();
            for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
                MessageNode thisMessage = (MessageNode)( iter.next() );
                //  Measure the vertical offset of each message.  Make sure each
                //  is visible before we consider it.
                if ( thisMessage._showThis ) {
                    //  Now see if the message is of the correct type - error in
                    //  this case.
                    if ( thisMessage.starred() ) {
                        if ( -nodeOffset > yOffset ) {
                            goToNode = thisMessage;
                            newOffset = -nodeOffset;
                        }
                    }
                    nodeOffset += thisMessage.setDrawConditions( nodeOffset, true );
                }
            }
            if ( goToNode != null )
                _browser.setYOffset( newOffset );
        }
        
        public void nextCheckedAction( java.awt.event.ActionEvent e ) {
            int yOffset = _browser.getYOffset();
            MessageNode goToNode = null;
            int newOffset = 0;
            int nodeOffset = 0;
            BrowserNode topNode = _browser.browserTopNode();
            for ( Iterator<BrowserNode> iter = topNode.childrenIterator(); iter.hasNext(); ) {
                MessageNode thisMessage = (MessageNode)( iter.next() );
                //  Measure the vertical offset of each message.  Make sure each
                //  is visible before we consider it.
                if ( thisMessage._showThis ) {
                    //  Now see if the message is of the correct type - error in
                    //  this case.
                    if ( thisMessage.starred() ) {
                        if ( goToNode == null && -nodeOffset < yOffset ) {
                            goToNode = thisMessage;
                            newOffset = -nodeOffset;
                        }
                    }
                    nodeOffset += thisMessage.setDrawConditions( nodeOffset, true );
                }
            }
            if ( goToNode != null )
                _browser.setYOffset( newOffset );
        }
        
    /*
     * Set a "handler" to sponge up log messages and display them on this
     * message panel.  Logging can be broken into individual logs, which are
     * named by the given string.  You can use "global" as the string to grab
     * everything (which is generally what I do).
     */
    public void captureLogging( String logName ) {
        internalLoggingHandler = new InternalLoggingHandler( this );
        java.util.logging.Logger.getLogger( logName ).addHandler( internalLoggingHandler );
    }
    
    public void message( long time, String source, String newText ) {
        boolean scrollToEnd = _browser.scrolledToEnd();
        if ( time == 0 ) {
            Calendar cal = Calendar.getInstance();
            time = cal.getTimeInMillis();
        }
        if ( source == null ) {
            source = "GUI Internal";
        }
        MessageNode node = new MessageNode( time, MessageNode.INFO, source, newText );
        node.showDate( _showDate.isSelected() );
        node.showTime( _showTime.isSelected() );
        node.showSource( _showSource.isSelected() );
        node.showErrors( _showErrors.isSelected() );
        node.showWarnings( _showWarnings.isSelected() );
        node.showMessages( _showMessages.isSelected() );
        node.showChecked( _showChecked.isSelected() );
        node.showUnchecked( _showUnchecked.isSelected() );
        node.applyFilter( _filterSource.isSelected(), _filterMessage.isSelected(), _currentFilter );
        _browser.addNode( node );
        if ( scrollToEnd )
            _browser.scrollToEnd();
    }

    public void warning( long time, String source, String newText ) {
        boolean scrollToEnd = _browser.scrolledToEnd();
        if ( time == 0 ) {
            Calendar cal = Calendar.getInstance();
            time = cal.getTimeInMillis();
        }
        if ( source == null ) {
            source = "GUI Internal";
        }
        MessageNode node = new MessageNode( time, MessageNode.WARNING, source, newText );
        node.showDate( _showDate.isSelected() );
        node.showTime( _showTime.isSelected() );
        node.showSource( _showSource.isSelected() );
        node.showErrors( _showErrors.isSelected() );
        node.showWarnings( _showWarnings.isSelected() );
        node.showMessages( _showMessages.isSelected() );
        node.showChecked( _showChecked.isSelected() );
        node.showUnchecked( _showUnchecked.isSelected() );
        node.applyFilter( _filterSource.isSelected(), _filterMessage.isSelected(), _currentFilter );
        _browser.addNode( node );
        if ( scrollToEnd )
            _browser.scrollToEnd();
    }

    public void error( long time, String source, String newText ) {
        boolean scrollToEnd = _browser.scrolledToEnd();
        if ( time == 0 ) {
            Calendar cal = Calendar.getInstance();
            time = cal.getTimeInMillis();
        }
        if ( source == null ) {
            source = "GUI Internal";
        }
        MessageNode node = new MessageNode( time, MessageNode.ERROR, source, newText );
        node.showDate( _showDate.isSelected() );
        node.showTime( _showTime.isSelected() );
        node.showSource( _showSource.isSelected() );
        node.showErrors( _showErrors.isSelected() );
        node.showWarnings( _showWarnings.isSelected() );
        node.showMessages( _showMessages.isSelected() );
        node.showChecked( _showChecked.isSelected() );
        node.showUnchecked( _showUnchecked.isSelected() );
        node.applyFilter( _filterSource.isSelected(), _filterMessage.isSelected(), _currentFilter );
        _browser.addNode( node );
        if ( scrollToEnd )
            _browser.scrollToEnd();
    }
    
    private InternalLoggingHandler internalLoggingHandler;
    /*
     * This class allows the message panel to sponge up logging messages.  Logging
     * is a bit complex and messy in Java, but for our purposes, you can dump
     * something like the following any place in the code:
     * 
     *      java.util.logging.Logger.getLogger("global").log(java.util.logging.Level.SEVERE, "this is a message" );
     * 
     * This will cause the message to appear in the output console as well as to
     * be captured by this logging handler (and from there printed to the panel).
     * 
     * Logging messages are given a "level", which can be SEVERE, WARNING, INFO,
     * CONFIG, FINE, FINER, or FINEST.  The logging system can be adjusted to
     * ignore levels.  We don't use this system to its full extent - SEVERE log
     * messages are treated as "errors", WARNINGS are treated as "warnings" and
     * everything else is treated as "messages".
     * 
     * You can add a "Throwable" as a third parameter to a "log()" call if you
     * wish.  The "publish()" method below knows what to do with those.  You can
     * also add Objects, but publish() ignores those.
     */
    private class InternalLoggingHandler extends java.util.logging.Handler {
                public InternalLoggingHandler( MessageDisplayPanel newOutput ) {
            output = newOutput;
            //includeTime = true;
            //includeSource = true;
            //includeDate = false;
        }
        //public void includeDate( boolean newVal ) {
        //    includeDate = newVal;
        //}
        //public void includeTime( boolean newVal ) {
        //    includeTime = newVal;
        //}
        //public void includeSource( boolean newVal ) {
        //    includeSource = newVal;
        //}
        @Override
        public void flush() {}        
        @Override
        public void close() {}        
        @Override
        public void publish( java.util.logging.LogRecord newRecord ) {
            //  Build a new message string including components as specified.
            String outText = new String( "" );
            //  Print out the string version of a Throwable, if one is included.
            //  These are sometimes big, but useful to see.
            if ( newRecord.getThrown() != null ) {
                outText += newRecord.getThrown().toString() + " ";
            }
            if ( newRecord.getMessage() != null ) {
                outText += newRecord.getMessage();
            }
            long time = newRecord.getMillis();
            String source = "GUI:" + newRecord.getSourceClassName().substring( newRecord.getSourceClassName().lastIndexOf(".") + 1 ) + 
                        "." + newRecord.getSourceMethodName() + "(): ";
            if ( newRecord.getLevel() == java.util.logging.Level.SEVERE )
                output.error( time, source, outText + "\n" );
            else if ( newRecord.getLevel() == java.util.logging.Level.WARNING )
                output.warning( time, source, outText + "\n" );
            else
                output.message( time, source, outText + "\n" );
        }       
        private MessageDisplayPanel output;
    }
    
    protected NodeBrowserScrollPane _browser;
    protected JButton _clearButton;
    protected JMenuBar _menuBar;
    protected JMenu _clearMenu;
    protected JMenuItem _clearAll;
    protected JMenuItem _clearWarningsAndMessages;
    protected JMenuItem _clearMessages;
    protected JMenuItem _clearChecked;
    protected JMenuItem _clearUnchecked;
    protected JCheckBoxMenuItem _showDate;
    protected JCheckBoxMenuItem _showTime;
    protected JCheckBoxMenuItem _showSource;
    protected JCheckBoxMenuItem _showErrors;
    protected JCheckBoxMenuItem _showWarnings;
    protected JCheckBoxMenuItem _showMessages;
    protected JCheckBoxMenuItem _showChecked;
    protected JCheckBoxMenuItem _showUnchecked;
    protected JMenuItem _newFilter;
    protected JCheckBoxMenuItem _filterSource;
    protected JCheckBoxMenuItem _filterMessage;
    protected JMenu _containingMenu;
    protected JMenu _showMenu;
    protected JMenu _includeMenu;
    protected JMenu _filterMenu;
    protected JMenu _navigateMenu;
    protected JMenuItem _navigateTop;
    protected JMenuItem _navigateBottom;
    protected JMenuItem _previousError;
    protected JMenuItem _nextError;
    protected JMenuItem _previousWarning;
    protected JMenuItem _nextWarning;
    protected JMenuItem _previousChecked;
    protected JMenuItem _nextChecked;
    protected ArrayDeque<String> _filters;
    protected String _currentFilter;

}
