      SUBROUTINE sla_DVN (V, UV, VM)
*+
*     - - - - 
*      D V N
*     - - - -
*
*  Normalises a 3-vector also giving the modulus (double precision)
*
*  Given:
*     V       dp(3)      vector
*
*  Returned:
*     UV      dp(3)      unit vector in direction of V
*     VM      dp         modulus of V
*
*  If the modulus of V is zero, UV is set to zero as well
*
*  P.T.Wallace   Starlink   November 1984
*-

      IMPLICIT NONE

      DOUBLE PRECISION V(3),UV(3),VM

      INTEGER I
      DOUBLE PRECISION W1,W2


*  Modulus
      W1=0D0
      DO I=1,3
         W2=V(I)
         W1=W1+W2*W2
      END DO
      W1=SQRT(W1)
      VM=W1

*  Normalise the vector
      IF (W1.LE.0D0) W1=1D0
      DO I=1,3
         UV(I)=V(I)/W1
      END DO

      END
