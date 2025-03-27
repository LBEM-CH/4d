#
#
#
#
#
#
if ( ${ctfcor_imode}x == 9x ) then
  set iname = image_ctfcor_multiplied
else
  set iname = image_ctfcor
endif
#
set date = `date`
echo date = ${date}
#
source ${proc_2dx}/2dx_makedirs
#
echo "<<@progress: 1>>"
#
#################################################################################
${proc_2dx}/linblock "Verifying some parameters"
#################################################################################
#
set spot_source_begin = `echo ${spot_source} | awk '{ s = $1 } END { print s }'`
if ( ${spot_source_begin} == "FFT" ) then
  set spot_source = "0"
  echo "set spot_source = ${spot_source}" >> LOGS/${scriptname}.results
endif
#
set imagecenterx = `echo ${imagesidelength} | awk '{ s = int( $1 / 2 ) } END { print s }'`
set imagecentery = ${imagecenterx}
#
set outputfile = findspots.tmp
\rm -f ${outputfile}
#
${bin_2dx}/2dx_calcmag.exe << eot
${realcell}
${realang}
${TLTAXIS}
${TLTANG}
${lattice}
${imagesidelength}
${magnification}
${stepdigitizer}
${outputfile}
eot
#
if ( ! -e ${outputfile} ) then
  ${proc_2dx}/protest "ERROR in 2dx_calcmag.exe"
endif
#
#
set theormag = `cat ${outputfile} | head -n 1`
\rm -f ${outputfile}
#
${proc_2dx}/linblock "Theoretical magnification is ${theormag}, given magnification is ${magnification}"
#
echo "set CALCULATEDMAG = ${theormag}" >> LOGS/${scriptname}.results
#
echo "<<@progress: 5>>"
#
if ( ! -e ${nonmaskimagename}.spt ) then
  set ProtSpotList = "n"
  echo "::SpotList will not be protected... because it does not yet exist."
endif
#
if ( ${ProtSpotList} != "y" ) then
  if ( ${ProtSpotList} != "n" ) then
    # Sometimes, ProtSpotList is not defined, and is simply empty:
    set ProtSpotList = "n"
    echo "set ProtSpotList = n" >> LOGS/${scriptname}.results
  endif
endif
#
if ( ${ProtSpotList} == "n" ) then
  set spotname = ${nonmaskimagename}.spt
else
  set spotname = TMPspotlist.spt
endif
#
if ( ${imagename} != ${nonmaskimagename} ) then
  \rm -f ${imagename}.spt
endif
#
echo "<<@progress: 20>>"
#
# Cutoff radius is:
#
#                       Size * stepsize * 10000
# Cutoff radius = ---------------------------------
#                 Resolution[A] * Magnification
#
#
# Example:
#
#    1024 * 17.0 * 10000
#  --------------------- = 241
#   16 * 45000
#
#
#
set RINNER = 1
set ROUTER = `echo ${imagesidelength} ${stepdigitizer} ${magnification} ${spotlist_RESMAX} | awk '{ s = ( $1 * $2 *10000 ) / ( $3 * $4 ) } END { print s }'`
${proc_2dx}/linblock "Outer radius = ${ROUTER}"
#
set FFT_FILE = "none"
if ( ${spot_source} == "0" ) then  
  if ( -e FFTIR/${iname}_fou_unbend2_fft.mrc ) then
    set FFT_FILE = FFTIR/${iname}_fou_unbend2_fft.mrc
  else
    set FFT_FILE = FFTIR/${iname}_unbend1_fft.mrc
  endif
  if ( ! -e ${FFT_FILE} ) then
    set FFT_FILE = FFTIR/${iname}_fft.mrc
  endif
endif
if ( ${spot_source} == "1" ) then
  set FFT_FILE = FFTIR/${iname}_unbend1_fft.mrc
  if ( ! -e ${FFT_FILE} ) then
    set FFT_FILE = FFTIR/${iname}_fft.mrc
  endif
endif
if ( ${spot_source} == "2" ) then
  set FFT_FILE = FFTIR/${iname}_fft.mrc
endif
#
if ( ${spot_source} == "0" || ${spot_source} == "1" || ${spot_source} == "2" ) then
  #
  if ( ${FFT_FILE} == "none" ) then
    ${proc_2dx}/linblock "ERROR: ${FFT_FILE} does not exist."
    ${proc_2dx}/protest "First run previous scripts."
  endif
  if ( ! -e ${FFT_FILE} ) then
    ${proc_2dx}/linblock "ERROR: ${FFT_FILE} does not exist."
    ${proc_2dx}/protest "First run previous scripts."
  endif 
  #
  ${proc_2dx}/linblock "Using the unbent data ${FFT_FILE} as source of spots."
  echo "# IMAGE-IMPORTANT: ${FFT_FILE} <FFT source: ${FFT_FILE}>" >> LOGS/${scriptname}.results
  #
  #############################################################################
  ${proc_2dx}/linblock "2dx_mmboxa: To get the spot list from the FFT of the image."
  #############################################################################
  #
  if ( ${scriptname} == "2dx_getspots1" ) then
    set IQMAX_local = ${IQMAX1}
  else
    set IQMAX_local = ${IQMAX}
  endif
  #
  \rm -f ${spotname}
  ${bin_2dx}/2dx_mmboxa.exe << eot
${FFT_FILE}
${imagenumber}
Y                               ! Use grid units?
Y                               ! Generate grid from lattice?
N                               ! Generate points from lattice?
2 3 0 50 50 12 12               ! IPIXEL,IOUT,NUMSPOT,NOH,NOK,NHOR,NVERT
${spotname}
${IQMAX_local}                              ! IQMAX
${imagecenterx} ${imagecentery}         ! XORIG,YORIG
${RINNER} ${ROUTER} 0 ${realcell} ${ALAT} ${realang} ! RINNER,ROUTER,IRAD,A,B,W,ABANG
${lattice}                         ! Lattice vectors
eot
  #
endif
#
echo "<<@progress: 40>>"
#
if ( ${spot_source} == "3" ) then
  #############################################################################
  ${proc_2dx}/linblock "2dx_spotgenerator: To create a synthetic full spot list."
  #############################################################################
  #
  \rm -f ${spotname}
  ${bin_2dx}/2dx_mmboxa.exe << eot
FFTIR/${iname}_fft.mrc
${imagenumber}
Y                               ! Use grid units?
Y                               ! Generate grid from lattice?
N                               ! Generate points from lattice?
2 3 0 50 50 12 12               ! IPIXEL,IOUT,NUMSPOT,NOH,NOK,NHOR,NVERT
${spotname}
9                              ! IQMAX
${imagecenterx} ${imagecentery}         ! XORIG,YORIG
${RINNER} ${ROUTER} 0 ${realcell} ${ALAT} ${realang} ! RINNER,ROUTER,IRAD,A,B,W,ABANG
${lattice}                         ! Lattice vectors
eot
  #
endif
#
echo "<<@progress: 60>>"
#
if ( ${spot_filter} == 'y' ) then
  #
  set spotnum = `cat ${spotname} | wc -l`
  ${proc_2dx}/linblock "So far, found ${spotnum} spots."
  #
  #############################################################################
  ${proc_2dx}/linblock "2dx_spotfilter: To weed out the spots on second lattice."
  #############################################################################
  #
  \rm -f GOODSPOT.spt
  \rm -f SCRATCH/BADSPOT.spt
  \rm -f SCRATCH/COUNTSPOT.tmp
  \rm -f SCRATCH/CORRECTEDSPOT.spt
  #
  set imodspot = 1
  #
  ${bin_2dx}/2dx_spotfilter.exe << eot
${lattice}
${secondlattice}
${spotrefine_IHKmax}
${spot_exclusion_radius}
${imodspot}
${spotname}
GOODSPOT.spt
SCRATCH/BADSPOT.spt
SCRATCH/COUNTSPOT.tmp
SCRATCH/CORRECTEDSPOT.spt
eot
  #
  echo "# IMAGE-IMPORTANT: GOODSPOT.spt <SPT: Good Spot List>" >> LOGS/${scriptname}.results
  echo "# IMAGE-IMPORTANT: SCRATCH/BADSPOT.spt <SPT: Bad Spot List>" >> LOGS/${scriptname}.results
  #
  if ( ! -e SCRATCH/COUNTSPOT.tmp ) then
    ${proc_2dx}/protest "ERROR in 2dx_spotfilter"
  endif
  set igood = `head -1 SCRATCH/COUNTSPOT.tmp`
  set ibad  = `head -2 SCRATCH/COUNTSPOT.tmp | tail -1`
  set ispt  = `head -3 SCRATCH/COUNTSPOT.tmp | tail -1`
  set irest = `head -4 SCRATCH/COUNTSPOT.tmp | tail -1`
  echo "::`echo 2dx_spotfilter: Generated ${igood} good and ${ibad} bad spots.`"
  echo ::From ${ispt} spots in ${spotname} survived ${irest} spots.
  echo ::Survived spots are copied onto ${spotname}
  \rm -f ${spotname}
  \mv SCRATCH/CORRECTEDSPOT.spt ${spotname}
  echo ::Good spots are in GOODSPOT.spt
  \rm -f SCRATCH/COUNTSPOT.tmp
  #
  set spotnum = `cat ${spotname} | wc -l`
  ${proc_2dx}/linblock "Now you have ${spotnum} spots."
  #
else
  #
  set spotnum = `cat ${spotname} | wc -l`
  ${proc_2dx}/linblock "You have ${spotnum} spots."
  #
endif
#
echo " " >> History.dat
echo ":Get Spotlist: You have ${spotnum} spots." >> History.dat
#
echo "<<@progress: 80>>"
#
echo "# IMAGE-IMPORTANT: ${spotname} <SPT: Fourier Spot List>" >> LOGS/${scriptname}.results
#
echo "# IMAGE-IMPORTANT: FFTIR/${iname}_fft.mrc <FFT of Image>" >> LOGS/${scriptname}.results
echo "# IMAGE-IMPORTANT: FFTIR/${iname}_red_fft.mrc <FFT of Downsampled Image>" >> LOGS/${scriptname}.results
#
cp ${spotname} ${spotname}.bak
if ( ${ProtSpotList} == "y" ) then
  ${proc_2dx}/linblock "Spotlist left untouched, since it is protected."
  \rm -f TMPspotlist.spt
endif
#
echo "set SPOTS_done = y" >> LOGS/${scriptname}.results
echo "set UNBENDING_done = n" >> LOGS/${scriptname}.results
echo "set ML_done = n" >> LOGS/${scriptname}.results
echo "set CTF_done = n" >> LOGS/${scriptname}.results
echo "set MERGING_done = n" >> LOGS/${scriptname}.results
#
\rm -f fort.3
#
#
