#!/bin/tcsh -x
#
#   ctftilt
#
time ./ctftilt.exe << eof
micrograph.mrc
montage.pow
2.0,200.0,0.07,60000,7.0,2				!CS[mm],HT[kV],AmpCnst,XMAG,DStep[um],PAve
128,200.0,8.0,5000.0,30000.0,1000.0,100.0,0.0,2.5	!Box,ResMin[A],ResMax[A],dFMin[A],dFMax[A],FStep[A],dAst[A],TiltA[deg],TiltR[deg]
eof
#

