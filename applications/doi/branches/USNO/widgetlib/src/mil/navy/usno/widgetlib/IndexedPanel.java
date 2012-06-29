/*
 * This widget is a collapsable panel that can contain anything a JPanel can.  It
 * inherits the BrowserNode class so that it has a little button in the upper left
 * corner and can be contained in a NodeBrowserPane.  However unlike the BrowserNode
 * it is not expected to contain child Browser nodes.
 * 
 * When "closed" the panel is resized to the "closed" sizes, and when "open" it
 * takes the "open" sizes.  The idea is that when closed the panel will show its
 * title and the open/close arrow icon only, but when open it will show all of the
 * other stuff it contains as well.
 */
package mil.navy.usno.widgetlib;

import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Color;

import java.util.ArrayList;
import java.util.Iterator;

import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;

/**
 *
 * @author jspitzak
 */
public class IndexedPanel extends BrowserNode {
    
    public IndexedPanel( String name ) {
        super( name );
        _drawFrame = true;
        _highlightColor = Color.WHITE;
        this.resizeOnTopBar( true );
    }
    
    /*
     * Find (and set) the dimensions of this object based on whether it is
     * open.  This is an override of a function in the BrowserNode that does
     * something slightly different.
     */
    @Override
    public int setDrawConditions( int yOffset, boolean open ) {
        int height = 0;
        int width = 0;
        if ( _open || _alwaysOpen )
            height = _openHeight;
        else
            height = _closedHeight;
        //  Set the bounds of this object.
        this.setBounds( 0, yOffset, this.getWidth(), height );
        //  Make the label as wide as the object itself.
        this.labelWidth( this.getWidth() );
        //  Determine whether it is inside the boundary.
        if ( yOffset + height < 0 )
            inBounds( false );
        else
            inBounds( true );
        return height;
    }

    public void openHeight( int h ) { _openHeight = h; }
    public int openHeight() { return _openHeight; }
    public void closedHeight( int h ) { 
        _closedHeight = h;
        resizeTopBarSize( _closedHeight );
    }
    public int closedHeight() { return _closedHeight; }
    
    public void drawFrame( boolean newVal ) { 
        _drawFrame = newVal;
        this.updateUI(); 
    }
    public boolean drawFrame() { return _drawFrame; }
    public void alwaysOpen( boolean newVal ) { _alwaysOpen = newVal; }
    public void noArrow( boolean newVal ) { _noArrow = newVal; }
    
    @Override
    public void paintComponent( Graphics g ) {
        if ( !_inBounds )
            return;
        //  If the panel is closed and the mouse is in it, highlight it.
        if ( !_open && !_alwaysOpen && _mouseIn ) {
            g.setColor( _highlightColor );
            g.fillRect( 0, 0, this.getWidth(), this.getHeight() );
        }
        else {
            g.setColor( this.getBackground() );
            g.fillRect( 0, 0, this.getWidth(), this.getHeight() );
        }
        //  This draws a frame around the panel, if requested.
        if ( _drawFrame ) {
            int w = this.getWidth();
            //  Not entirely convinced what looks better here.
            int h = _openHeight; //this.getHeight();
            int x = 0;
            int y = 0;
            for ( int i = 0; i <2; ++i ) {
                g.draw3DRect( x++, y++, w, h, true );
                w -= 2;
                h -= 2;
            }
        }
        //  Draw the open/close arrow.
        int xpts[] = new int[3];
        int ypts[] = new int[3];
        int levelOffset = ( _level - 1 ) * _levelOffset;
        if ( !_noArrow ) {
            if ( _open || _alwaysOpen ) {
                //  "Open" objects have an arrow that points down.
                xpts[0] = levelOffset + 10;
                xpts[1] = levelOffset + 22;
                xpts[2] = levelOffset + 16;
                ypts[0] = 5;
                ypts[1] = 5;
                ypts[2] = 15;
            } else {
                xpts[0] = levelOffset + 11;
                xpts[1] = levelOffset + 11;
                xpts[2] = levelOffset + 21;
                ypts[0] = 4;
                ypts[1] = 16;
                ypts[2] = 10;
            }
        }
        g.setColor( Color.GRAY );
        g.fillPolygon( xpts, ypts, 3 );
    }
        
    /*
     * Add a scroll pane as a child.  This will trigger resize callbacks.
     */
    public void addScrollPane( NodeBrowserScrollPane newChild ) {
        this.add( newChild );
        if ( _scrollPanes == null )
            _scrollPanes = new ArrayList<NodeBrowserScrollPane>();
        _scrollPanes.add( newChild );
        newChild.addResizeEventListener(new ActionListener() {
            public void actionPerformed( ActionEvent e ) {
                respondToResizeEvent();
            }
        });
        measureOpenHeight();
    }
    
    /*
     * This function responds to a resize event in a child.  It overrides a similar
     * method in the BrowserNode.
     */
    public void respondToResizeEvent() {
        measureOpenHeight();
        super.respondToResizeEvent();
    }
    
    /*
     * Measure the height of this panel including all added scroll panes.  If there
     * are other things that add to the height, this method needs to be inherited,
     * or the _staticHeight value needs to be set.
     */
    public void measureOpenHeight() {
        _openHeight = _closedHeight + _staticHeight;
        if ( _scrollPanes != null ) {
            for ( Iterator<NodeBrowserScrollPane> iter = _scrollPanes.iterator(); iter.hasNext(); ) {
                _openHeight += iter.next().browserHeight();
            }
        }
    }
    
    /*
     * This sets the "static height".  The static height is the height of items that
     * are not contained in child scroll panes.
     */
    public void staticHeight( int newVal ) { 
        _staticHeight = newVal;
        //measureOpenHeight();
        respondToResizeEvent();
    }
    
    protected int _openHeight;
    protected int _closedHeight;
    protected int _staticHeight;
    protected boolean _drawFrame;
    protected boolean _alwaysOpen;
    protected boolean _noArrow;
    protected ArrayList<NodeBrowserScrollPane> _scrollPanes;
    
}
