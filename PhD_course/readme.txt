
This directory contains an example of control synthesis to compensate for
clock skew and drift used as part of the "Control-theoretical methods and tools
for the design of computing systems" PhD course.

------------------------------------------------------------------
It is a toy example, its performance is far inferior to FLOPSYNC-2
------------------------------------------------------------------

Model of the process

e(k) = localtime - globaltime

e(k+1) = e(k) + d(k) + u(k)

E(z) = 1/(z-1) (D(Z)+U(z))

                  D
      +------+ U  |   +------+
  +-->| R(z) |--->o-->| P(z) |--+ E
  |   +------+        +------+  |
  +-----------------------------+

Control is synthesized by assigning the transfer function between D and E.
Using Scilab to see possible responses, we select

D(z)/E(z) = (z-1)/(z*(z-0.4))

    P
 ------- = (z-1)/(z*(z-0.4))
 1 + R*P

R = - (8/5-z)/(z-1)

The control law is thus

u(k) = u(k-1) -8/5*e(k) + e(k-1)
