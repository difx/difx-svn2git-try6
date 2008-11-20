      subroutine VAXR4 (rv, rb)
*-----------------------------------------------------------------------
*   VAXR4 converts from VAX REAL to big-endian IEEE REAL.
*
*   $Id: cvt_ieee.f,v 1.4 2001/02/08 05:09:39 mcalabre Exp $
*-----------------------------------------------------------------------
      real      rb, rv
*-----------------------------------------------------------------------
      call RV2B (rv, rb)
      return
      end



      subroutine VAXI4 (iv, ib)
*-----------------------------------------------------------------------
*   VAXI4 converts VAX INTEGER to big-endian INTEGER.
*-----------------------------------------------------------------------
      integer   ib, iv
*-----------------------------------------------------------------------
      call IV2B (iv, ib)
      return
      end



      subroutine R4VAX (rb, rv)
*-----------------------------------------------------------------------
*   R4VAX converts from big-endian IEEE REAL to VAX REAL.
*-----------------------------------------------------------------------
      real      rb, rv
*-----------------------------------------------------------------------
      call RB2V (rb, rv)
      return
      end



      subroutine I4VAX (ib, iv)
*-----------------------------------------------------------------------
*   I4VAX converts big-endian INTEGER to VAX INTEGER.
*-----------------------------------------------------------------------
      integer   ib, iv
*-----------------------------------------------------------------------
      call IB2V (ib, iv)
      return
      end
