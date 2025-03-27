#
#  2dx_unbend_movie_sub.com
#
# Attention:
# This is not an independent csh file.
# This file has to be sourced from another csh script.
#
#
#
#
#
#
set imagecenterx = `echo ${imagesidelength} | awk '{ s = int( $1 / 2 ) } END { print s }'`
set imagecentery = ${imagecenterx}
#


        set valspotscan = '1'

        ###############################################################
        ${proc_2dx}/linblock "Processing frame average #${i}"
        ###############################################################
                

        set prog_num = `echo ${irunner} ${movie_imagenumber_superframes} | awk '{ s = 30 + int( 50 * $1 / $2 ) } END { print s }'` 
        echo "<<@progress: ${prog_num}>>"
        
        
        #########################################################################
        ${proc_2dx}/lin "TWOFILE - Cross-correlate frame and reference in Fourier-space, with ref A"
        #########################################################################
        setenv IN1 ${frame_folder}/f${i}/${iname}_fft.mrc
        setenv IN2 SCRATCH/reference_fft.mrc
        setenv OUT ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB_fft.mrc
        \rm -f     ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB_fft.mrc
        ${bin_2dx}/twofile.exe << eot
        2 ! ICOMB = 2
        2 0 0 ${imagecenterx} ${imagecentery} ! IORIGIN,OXA,OYA,OXB,OYB  Origin shifts to FFTs
eot

        #########################################################################
        ${proc_2dx}/lin "FFTRANS - Inverse FFT to obtain cross-correlation profile (raw frame with reference A)"
        #########################################################################
        setenv IN ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB_fft.mrc
        setenv OUT ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc
        \rm -f     ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc
        ${bin_2dx}/2dx_fftrans.exe
        

        \rm -f SPIDERCOORD.spi
        \rm -f CCPLOT.PS

        setenv PROFILE  SCRATCH/${iname}_fou_unbend2_fft_msk_fft_cro_aut_cro.mrc
        setenv PROFDATA ${frame_folder}/f${i}/SCRATCH/prof${nonmaskimagename}.dat
        \rm -f          ${frame_folder}/f${i}/SCRATCH/prof${nonmaskimagename}.dat
        setenv ERRORS   SCRATCH/errout2${iname}.dat

        set createmask = '0'
        
        if ( ${movie_masking_mode} != '0' && ${i} != "0" ) then
                
           if ( ${movie_masking_mode} == '1' && ${use_masked_image} == "y" ) then
                #
                #########################################################################
                ${proc_2dx}/lin "QUADSERCH - masking with external mask-info (IPASS=2)"
                #########################################################################
                #
                if ( ! -e ${maskfile}.mrc ) then
                  ${proc_2dx}/protest "ERROR: ${maskfile}.mrc not found. First run UNBEND-II with masking option."
                endif 
                \cp -f ${maskfile}.mrc TMP_quadserch_7.mrc
                #
                  ${bin_2dx}/2dx_quadserchk-2.exe << eot
2,${quadpredb}                     ! IPASS,NRANGE
${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc
${imagesidelength},${imagesidelength}     ! SIZE OF TRANSFORM (ISIZEX, ISIZEY)
${lattice},F                       ! Lattice vectors
-200,200,-200,200               ! NUMBER UNIT CELLS TO SEARCH
${movie_quadradax},${movie_quadraday}         ! RADIUS OF CORR SEARCH, search offset in pixels
${imagecenterx},${imagecentery}           ! POSN OF START SEARCH ORIGIN  0,0 IS ORIGIN
N                               ! YES/NO FOR DETAILED PRINTOUT
${radlim}                       ! RADLIM IN PROFILE GRID UNITS
${valspotscan},${RMAG},${LCOLOR}          ! prohibit fractures in crystal (1=y,0=n),RMAG,LCOLOR
${createmask}                   ! dont create manual Masking information
1                               ! Do mask the image directly
${frame_folder}/f${i}/${iname}.mrc
${frame_folder}/f${i}/${iname}_mask.mrc
-1                              ! create output images (0=no, <0 = one, >0 =size)
1                               ! use external masking template (0=no, 1=yes, 2=no with 2nd image, 3=yes with 2nd image)
TMP_quadserch_7.mrc
eot
                #
           else
                #
                #########################################################################
                ${proc_2dx}/lin "QUADSERCH - masking frame individually (IPASS=2)"
                #########################################################################
                #
                ${bin_2dx}/2dx_quadserchk-2.exe << eot
2,${quadpredb}                     ! IPASS,NRANGE
${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc
${imagesidelength},${imagesidelength}     ! SIZE OF TRANSFORM (ISIZEX, ISIZEY)
${lattice},F                       ! Lattice vectors
-200,200,-200,200               ! NUMBER UNIT CELLS TO SEARCH
${movie_quadradax},${movie_quadraday}           ! RADIUS OF CORR SEARCH, search offset in pixels
${imagecenterx} ${imagecentery}           ! POSN OF START SEARCH ORIGIN  0,0 IS ORIGIN
N                               ! YES/NO FOR DETAILED PRINTOUT
${radlim}                       ! RADLIM IN PROFILE GRID UNITS
${valspotscan},${RMAG},${LCOLOR}          ! prohibit fractures in crystal (1=y,0=n),RMAG,LCOLOR
${createmask}                   ! dont create manual Masking information
1                               ! Do mask the image directly
${frame_folder}/f${i}/${iname}.mrc
${frame_folder}/f${i}/${iname}_mask.mrc
-1                              ! create output images (0=no, <0 = one, >0 =size)
0                               ! use external masking template (0=no, 1=yes, 2=no with 2nd image, 3=yes with 2nd image)
eot
                #
           endif

           ######################################################
           ${proc_2dx}/lin "FFT of masked frame average"
           ######################################################
           setenv IN ${frame_folder}/f${i}/${iname}_mask.mrc
           setenv OUT ${frame_folder}/f${i}/${iname}_mask_fft.mrc
           \rm -f     ${frame_folder}/f${i}/${iname}_mask_fft.mrc
           ${bin_2dx}/2dx_fftrans.exe

        else
           ######################################################
           ${proc_2dx}/lin "Not masking frame"
           ######################################################
           set olddir = $PWD
           cd ${frame_folder}/f${i}
           \rm -f              ${iname}_mask.mrc
           \ln -s ${iname}.mrc ${iname}_mask.mrc
           \rm -f                  ${iname}_mask_fft.mrc
           \ln -s ${iname}_fft.mrc ${iname}_mask_fft.mrc
           cd ${olddir}
        endif


        ###########################################################################
        ${proc_2dx}/lin "TWOFILE - Cross-correlation with references in Fourier-space"
        ###########################################################################
        setenv IN1 ${frame_folder}/f${i}/${iname}_mask_fft.mrc
        setenv IN2 SCRATCH/reference_fft.mrc
        setenv OUT ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB_fft.mrc
        \rm -f     ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB_fft.mrc
        ${bin_2dx}/twofile.exe << eot
        2 ! ICOMB = 2
        2 0 0 ${imagecenterx} ${imagecentery} ! IORIGIN,OXA,OYA,OXB,OYB  Origin shifts to FFTs
eot


        if ( ${movie_refboxb} != "0" ) then       
          setenv IN1 ${frame_folder}/f${i}/${iname}_mask_fft.mrc
          setenv IN2 SCRATCH/reference_fft.mrc
          setenv OUT ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMBb_fft.mrc
          \rm -f     ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMBb_fft.mrc
          ${bin_2dx}/twofile.exe << eot
          2 ! ICOMB = 2
          2 0 0 ${imagecenterx} ${imagecentery} ! IORIGIN,OXA,OYA,OXB,OYB  Origin shifts to FFTs
eot
        endif 

        ###########################################################################
        ${proc_2dx}/lin "FFTRANS - IFFT to obtain cross correlation profiles"
        ###########################################################################
        setenv IN ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB_fft.mrc
        setenv OUT ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc
        \rm -f     ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc
        ${bin_2dx}/2dx_fftrans.exe
        
        if ( ${movie_refboxb} != "0" ) then
          setenv IN ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMBb_fft.mrc
          setenv OUT ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMBb.mrc
          \rm -f     ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMBb.mrc
          ${bin_2dx}/2dx_fftrans.exe
        endif


        \cp -f ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc ${frame_folder}/CCmap-${i}.mrc

if ( 1 == 2 ) then

set rtempx1 = 2048
set rtempy1 = 2048
set rtempx2 = 2048
set rtempy2 = 2048
@ rtempx1 -= 200
@ rtempx2 += 199
@ rtempy1 -= 200
@ rtempy2 += 199
set boxlabel = ${rtempx1},${rtempx2},${rtempy1},${rtempy2}
echo boxlabel = ${boxlabel}
\rm -f ${frame_folder}/CC-CE-f${i}.mrc
${bin_2dx}/labelh.exe << eot
${frame_folder}/CCmap-${i}.mrc
1
${frame_folder}/CC-CE-f${i}.mrc
${boxlabel}
eot
#
${bin_2dx}/mrc2tif.exe << eot
${frame_folder}/CC-CE-f${i}.mrc
${frame_folder}/CC-CE-f${i}.tif
eot
#
set rtempx1 = 500
set rtempy1 = 500
set rtempx2 = 500
set rtempy2 = 500
@ rtempx1 -= 200
@ rtempx2 += 199
@ rtempy1 -= 200
@ rtempy2 += 199
set boxlabel = ${rtempx1},${rtempx2},${rtempy1},${rtempy2}
echo boxlabel = ${boxlabel}
\rm -f ${frame_folder}/CC-BL-f${i}.mrc
${bin_2dx}/labelh.exe << eot
${frame_folder}/CCmap-${i}.mrc
1
${frame_folder}/CC-BL-f${i}.mrc
${boxlabel}
eot
#
${bin_2dx}/mrc2tif.exe << eot
${frame_folder}/CC-BL-f${i}.mrc
${frame_folder}/CC-BL-f${i}.tif
eot
#
set rtempx1 = 500
set rtempy1 = 3000
set rtempx2 = 500
set rtempy2 = 3000
@ rtempx1 -= 200
@ rtempx2 += 199
@ rtempy1 -= 200
@ rtempy2 += 199
set boxlabel = ${rtempx1},${rtempx2},${rtempy1},${rtempy2}
echo boxlabel = ${boxlabel}
\rm -f ${frame_folder}/CC-TL-f${i}.mrc
${bin_2dx}/labelh.exe << eot
${frame_folder}/CCmap-${i}.mrc
1
${frame_folder}/CC-TL-f${i}.mrc
${boxlabel}
eot
#
${bin_2dx}/mrc2tif.exe << eot
${frame_folder}/CC-TL-f${i}.mrc
${frame_folder}/CC-TL-f${i}.tif
eot
#
set rtempx1 = 3000
set rtempy1 = 3000
set rtempx2 = 3000
set rtempy2 = 3000
@ rtempx1 -= 200
@ rtempx2 += 199
@ rtempy1 -= 200
@ rtempy2 += 199
set boxlabel = ${rtempx1},${rtempx2},${rtempy1},${rtempy2}
echo boxlabel = ${boxlabel}
\rm -f ${frame_folder}/CC-TR-f${i}.mrc
${bin_2dx}/labelh.exe << eot
${frame_folder}/CCmap-${i}.mrc
1
${frame_folder}/CC-TR-f${i}.mrc
${boxlabel}
eot
#
${bin_2dx}/mrc2tif.exe << eot
${frame_folder}/CC-TR-f${i}.mrc
${frame_folder}/CC-TR-f${i}.tif
eot
#
set rtempx1 = 3000
set rtempy1 =  500
set rtempx2 = 3000
set rtempy2 =  500
@ rtempx1 -= 200
@ rtempx2 += 199
@ rtempy1 -= 200
@ rtempy2 += 199
\rm -f ${frame_folder}/CC-BR-f${i}.mrc
set boxlabel = ${rtempx1},${rtempx2},${rtempy1},${rtempy2}
echo boxlabel = ${boxlabel}
${bin_2dx}/labelh.exe << eot
${frame_folder}/CCmap-${i}.mrc
1
${frame_folder}/CC-BR-f${i}.mrc
${boxlabel}
eot
#
${bin_2dx}/mrc2tif.exe << eot
${frame_folder}/CC-BR-f${i}.mrc
${frame_folder}/CC-BR-f${i}.tif
eot
#

endif


        ###########################################################################
        ${proc_2dx}/lin "QUADSERCH - Search unbending profile with first reference"
        ###########################################################################

        # \cp -f ${frame_folder}/f${i}/m${nonmaskimagename}_${i}.mrc ${frame_folder}/f${i}/m${nonmaskimagename}_${i}_ori.mrc

        setenv PROFILE  SCRATCH/${iname}_fou_unbend2_fft_msk_fft_cro_aut_cro.mrc
        setenv PROFDATA ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}.dat
        setenv ERRORS   SCRATCH/errout2${iname}.dat
        setenv ERROUT   SCRATCH/errout3${iname}.dat

        set valspotscan = '0'
        set createmask = '0'

        if ( 1 == 1 ) then
          # IPASS = 13   ! same as IPASS=3, but align images also
          set IPASS = 13

          \rm -f ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}.dat
          \rm -f SPIDERCOORD.spi
          \rm -f SCRATCH/errout3${iname}.dat
          \rm -f CCPLOT.PS
          ${bin_2dx}/2dx_quadserchk-2.exe << eot
${IPASS},${quadpredb}                     ! IPASS,NRANGE
${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc
${frame_folder}/f${i}/${iname}_mask.mrc
${frame_folder}/f${i}/${iname}_mask_aligned.mrc
${imagesidelength},${imagesidelength}     ! SIZE OF TRANSFORM (ISIZEX, ISIZEY)
${lattice},F                       ! Lattice vectors
-200,200,-200,200               ! NUMBER UNIT CELLS TO SEARCH
${movie_quadradax},${movie_quadraday},500,500           ! RADIUS OF CORR SEARCH, search offset in pixels
${imagecenterx},${imagecentery}           ! POSN OF START SEARCH ORIGIN  0,0 IS ORIGIN
N                               ! YES/NO FOR DETAILED PRINTOUT
${radlim}                       ! RADLIM IN PROFILE GRID UNITS
${valspotscan},${RMAG},${LCOLOR}          ! prohibit fractures in crystal (1=y,0=n),RMAG,LCOLOR
${createmask}                   ! create manual Masking information (0=n,1=y)
0                               ! Do mask the image directly
eot
          #
          if ( ${movie_refboxb} != "0" ) then
            \rm -f ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}.dat
            \rm -f SPIDERCOORD.spi
            \rm -f SCRATCH/errout3${iname}.dat
            \rm -f CCPLOT.PS
            ${bin_2dx}/2dx_quadserchk-2.exe << eot
${IPASS},${quadpredb}                     ! IPASS,NRANGE
${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc
${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMBb.mrc
${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMBb_aligned.mrc
${imagesidelength},${imagesidelength}     ! SIZE OF TRANSFORM (ISIZEX, ISIZEY)
${lattice},F                       ! Lattice vectors
-200,200,-200,200               ! NUMBER UNIT CELLS TO SEARCH
${movie_quadradax},${movie_quadraday},500,500           ! RADIUS OF CORR SEARCH, search offset in pixels
${imagecenterx},${imagecentery}           ! POSN OF START SEARCH ORIGIN  0,0 IS ORIGIN
N                               ! YES/NO FOR DETAILED PRINTOUT
${radlim}                       ! RADLIM IN PROFILE GRID UNITS
${valspotscan},${RMAG},${LCOLOR}          ! prohibit fractures in crystal (1=y,0=n),RMAG,LCOLOR
${createmask}                   ! create manual Masking information (0=n,1=y)
0                               ! Do mask the image directly
eot
          endif
          #
          \rm -f ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}.dat
          \rm -f SPIDERCOORD.spi
          \rm -f SCRATCH/errout3${iname}.dat
          \rm -f CCPLOT.PS
          ${bin_2dx}/2dx_quadserchk-2.exe << eot
${IPASS},${quadpredb}                     ! IPASS,NRANGE
${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc
${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc
${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB_aligned.mrc
${imagesidelength},${imagesidelength}     ! SIZE OF TRANSFORM (ISIZEX, ISIZEY)
${lattice},F                       ! Lattice vectors
-200,200,-200,200               ! NUMBER UNIT CELLS TO SEARCH
${movie_quadradax},${movie_quadraday},500,500           ! RADIUS OF CORR SEARCH, search offset in pixels
${imagecenterx},${imagecentery}           ! POSN OF START SEARCH ORIGIN  0,0 IS ORIGIN
N                               ! YES/NO FOR DETAILED PRINTOUT
${radlim}                       ! RADLIM IN PROFILE GRID UNITS
${valspotscan},${RMAG},${LCOLOR}          ! prohibit fractures in crystal (1=y,0=n),RMAG,LCOLOR
${createmask}                   ! create manual Masking information (0=n,1=y)
0                               ! Do mask the image directly
eot
          #
          #
          if ( ! -e ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB_aligned.mrc ) then
            ${proc_2dx}/protest "ERROR in 2dx_quadserchk-2" 
          else
            \mv -f ${frame_folder}/f${i}/${iname}_mask_aligned.mrc ${frame_folder}/f${i}/${iname}_mask.mrc
            \mv -f ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB_aligned.mrc ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc
            if ( ${movie_refboxb} != "0" ) then
              \mv -f ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMBb_aligned.mrc ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMBb.mrc
            endif
          endif
          #
        endif

        #
        set IPASS = 3
        echo "PWD =" `pwd`
        setenv | grep ERRORS
        setenv | grep ERROUT
        setenv | grep PROFDATA
        setenv | grep PROFILE
        
        \rm -f ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}.dat
        \rm -f SPIDERCOORD.spi
        \rm -f SCRATCH/errout3${iname}.dat
        \rm -f CCPLOT.PS
        ${bin_2dx}/2dx_quadserchk-2.exe << eot
${IPASS},${quadpredb}                     ! IPASS,NRANGE
${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc
${imagesidelength},${imagesidelength}     ! SIZE OF TRANSFORM (ISIZEX, ISIZEY)
${lattice},F                       ! Lattice vectors
-200,200,-200,200               ! NUMBER UNIT CELLS TO SEARCH
${movie_quadradax},${movie_quadraday}          ! RADIUS OF CORR SEARCH, search offset in pixels
${imagecenterx},${imagecentery}           ! POSN OF START SEARCH ORIGIN  0,0 IS ORIGIN
N                               ! YES/NO FOR DETAILED PRINTOUT
${radlim}                       ! RADLIM IN PROFILE GRID UNITS
${valspotscan},${RMAG},${LCOLOR}          ! prohibit fractures in crystal (1=y,0=n),RMAG,LCOLOR
${createmask}                   ! create manual Masking information (0=n,1=y)
0                               ! Do mask the image directly
eot
        #
        
        if ( ${movie_refboxb} != "0" ) then

          ###########################################################################
          ${proc_2dx}/lin "QUADSERCH - Refine unbending profile with second reference (2nd round, IPASS=3)"
          ###########################################################################

          \mv -f SCRATCH/errout3${iname}.dat SCRATCH/errout2${iname}.dat

          \rm -f ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}.dat
          \rm -f SPIDERCOORD.spi
          \rm -f CCPLOT.PS

          setenv PROFILE  SCRATCH/${iname}_fou_unbend2_fft_msk_fft_cro_aut_cro.mrc
          setenv PROFDATA ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}.dat
          setenv ERRORS   SCRATCH/errout2${iname}.dat
          setenv ERROUT   SCRATCH/errout3${iname}.dat

          set createmask = '0'
          set IPASS = 3

          ${bin_2dx}/2dx_quadserchk-2.exe << eot
${IPASS},${quadpredb}                     ! IPASS,NRANGE
${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMBb.mrc
${imagesidelength},${imagesidelength}     ! SIZE OF TRANSFORM (ISIZEX, ISIZEY)
${lattice},F                       ! Lattice vectors
-200,200,-200,200               ! NUMBER UNIT CELLS TO SEARCH
${movie_quadradbx},${movie_quadradby}           ! RADIUS OF CORR SEARCH, search offset in pixels
${imagecenterx},${imagecentery}           ! POSN OF START SEARCH ORIGIN  0,0 IS ORIGIN
N                               ! YES/NO FOR DETAILED PRINTOUT
${radlim}                       ! RADLIM IN PROFILE GRID UNITS
${valspotscan},${RMAG},${LCOLOR}          ! prohibit fractures in crystal (1=y,0=n),RMAG,LCOLOR
${createmask}                   ! create manual Masking information (0=n,1=y)
0                               ! Do mask the image directly
eot
        endif

        if ( -e TMP_quadserch3_autocor.mrc ) then
          \mv -f TMP_quadserch3_autocor.mrc ${frame_folder}/f${i}/SCRATCH/TMP_quadserch3_autocor.mrc
        endif 

        if ( ! -e SCRATCH/errout3${iname}.dat ) then
          ${proc_2dx}/protest "ERROR: Problem in 2dx_quadserchk-2: SCRATCH/errout3${iname}.dat does not exist."
        endif
 
        # The other frames are subsequently used as refinement reference:
        \mv -f SCRATCH/errout3${iname}.dat SCRATCH/errout2${iname}.dat


        ###########################################################################
        ${proc_2dx}/lin "Store distortion-vector-field for visual inspection"
        ###########################################################################
        if ( ! -e CCPLOT.PS ) then
          ${proc_2dx}/protest "ERROR: CCPLOT.PS missing"
        endif
        #
        \cp -f CCPLOT.PS ${frame_folder}/f${i}/PS/${nonmaskimagename}-quadserch.ps


        # The high-contrast frame-0 is only used as refinement reference  
        if ( ${i} != "0" ) then

            if ( ${movie_ghostscript_installed} == "y" ) then
              if ( ${irunner} == '1' ) then
                if ( ${ps2pdf} == "pstopdf" ) then
                  ${ps2pdf} CCPLOT.PS 
                  \mv -f CCPLOT.pdf PS/MB_quadserch.pdf
                else
                   ${ps2pdf} CCPLOT.PS PS/MB_quadserch.pdf 
                endif
                pdftk A=PS/MB_quadserch.pdf cat A1 output out.pdf
                \mv -f out.pdf PS/MB_quadserch.pdf
              else
                if ( ${ps2pdf} == "pstopdf" ) then
                  ${ps2pdf} CCPLOT.PS  
                else
                  ${ps2pdf} CCPLOT.PS CCPLOT.pdf 
                endif
                if ( ${iforward} == '1' ) then
                  pdftk A=PS/MB_quadserch.pdf B=CCPLOT.pdf cat A1-end B1 output out.pdf 
                else
                  pdftk A=CCPLOT.pdf B=PS/MB_quadserch.pdf cat A1 B1-end output out.pdf 
                endif
                \mv -f out.pdf PS/MB_quadserch.pdf
              endif

            endif

            ###########################################################################
            ${proc_2dx}/lin "CCUNBEND - Unbend the CCmap for verfication"
            ###########################################################################
            setenv CCORDATA ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}.dat
            setenv TABLEOUT ${frame_folder}/f${i}/SCRATCH/ccunbend-table-m${nonmaskimagename}.dat
            set ITYPE = 0
            set ROFFSET = 50.0
            set NNUM = 6
            \rm -f CCPLOT.PS
            if ( ${movie_refboxb} != "0" ) then
              set CCmap = ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMBb.mrc
            else
              set CCmap = ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB.mrc
            endif

            ${bin_2dx}/2dx_ccunbendk.exe << eot
${CCmap}
${ITYPE},1,${IMAXCOR},${ISTEP},F,40,T       !ITYPE,IOUT,IMAXCOR,ISTEP,LTAPER,RTAPER,LTABOUT
30,52,0.001,${movie_facthreshb},${TLTAXIS},${RMAG},${LCOLOR}     !IKX,IKY,EPS,FACTOR,TLTAXIS,RMAG,LCOLOR
${imagename}, Movie-Mode UNBEND, ${date}
${frame_folder}/f${i}_CCmapMBb_unbent.mrc
CCUNBEND, f${i}/m${nonmaskimagename}_${i}.mrc, ${date}
eot




            ###########################################################################
            ${proc_2dx}/lin "CCUNBEND - Unbend the frame average"
            ###########################################################################
            setenv CCORDATA ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}.dat
            setenv TABLEOUT ${frame_folder}/f${i}/SCRATCH/ccunbend-table-m${nonmaskimagename}.dat
            set ITYPE = 0
            set ROFFSET = 50.0
            set NNUM = 6
            \rm -f CCPLOT.PS

            ${bin_2dx}/2dx_ccunbendk.exe << eot
${frame_folder}/f${i}/image_ctfcor_mask.mrc
${ITYPE},1,${IMAXCOR},${ISTEP},F,40,T       !ITYPE,IOUT,IMAXCOR,ISTEP,LTAPER,RTAPER,LTABOUT
30,52,0.001,${movie_facthreshb},${TLTAXIS},${RMAG},${LCOLOR}     !IKX,IKY,EPS,FACTOR,TLTAXIS,RMAG,LCOLOR
${imagename}, Movie-Mode UNBEND, ${date}
${frame_folder}/CCUNBEND_f${i}_notap.mrc
CCUNBEND, f${i}/m${nonmaskimagename}_${i}.mrc, ${date}
eot

            echo "# IMAGE-IMPORTANT: ${frame_folder}/CCUNBEND_f${i}_notap.mrc <Unbent Frame ${i}>" >> LOGS/${scriptname}.results


            if ( ${show_frame_CCmap_marked}x == "yx" ) then
              \rm -f ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB_marked.mrc
              ${bin_2dx}/2dx_mark_spots.exe << eot
${CCmap}
${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB_marked.mrc
${nonmaskimagename}_profile.dat
2
eot
              echo "# IMAGE: ${frame_folder}/f${i}/SCRATCH/${nonmaskimagename}_CCmapMB_marked.mrc <CCmap, marked, frame ${i}>" >> LOGS/${scriptname}.results 
              # 
            endif




            ###########################################################################
            ${proc_2dx}/lin "Extract trajectory for region drift plotting"
            ###########################################################################
            ${app_python} ${proc_2dx}/movie/deleteZeros.py ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}.dat ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}_nz.dat
                  
            set x_1_4 = `echo ${imagesidelength} | awk '{ s = int( $1 / 4 ) } END { print s }'`
            set x_3_4 = `echo ${imagesidelength} | awk '{ s = int( 3 * $1 / 4 ) } END { print s }'`
          
            ${app_python} ${proc_2dx}/movie/getClosestPeaks.py ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}_nz.dat ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}_closest_I.dat   ${x_1_4} ${x_1_4} ${num_dia} MB/MB_peaks_I.dat
            ${app_python} ${proc_2dx}/movie/getClosestPeaks.py ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}_nz.dat ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}_closest_II.dat  ${x_3_4} ${x_1_4} ${num_dia} MB/MB_peaks_II.dat
            ${app_python} ${proc_2dx}/movie/getClosestPeaks.py ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}_nz.dat ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}_closest_III.dat ${x_1_4} ${x_3_4} ${num_dia} MB/MB_peaks_III.dat
            ${app_python} ${proc_2dx}/movie/getClosestPeaks.py ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}_nz.dat ${frame_folder}/f${i}/SCRATCH/profm${nonmaskimagename}_closest_IV.dat  ${x_3_4} ${x_3_4} ${num_dia} MB/MB_peaks_IV.dat
    
            ###########################################################################
            ${proc_2dx}/lin "Store distortion-vector-field for visual inspection"
            ###########################################################################
            if ( ! -e CCPLOT.PS ) then
              ${proc_2dx}/protest "ERROR: CCPLOT.PS missing"
            endif
            #
            \cp -f CCPLOT.PS ${frame_folder}/f${i}/PS/${nonmaskimagename}-ccunbend.ps

            if ( ${movie_ghostscript_installed} == "y" ) then
              if ( ${irunner} == '1' ) then
                if ( ${ps2pdf} == "pstopdf" ) then
                  ${ps2pdf} CCPLOT.PS  
                  \mv -f CCPLOT.pdf PS/MB_unbending.pdf
                else
                   ${ps2pdf} CCPLOT.PS PS/MB_unbending.pdf 
                endif
                pdftk A=PS/MB_unbending.pdf cat A1 output out.pdf 
                \mv -f out.pdf PS/MB_unbending.pdf
              else
                if ( ${ps2pdf} == "pstopdf" ) then
                  ${ps2pdf} CCPLOT.PS 
                else
                  ${ps2pdf} CCPLOT.PS CCPLOT.pdf 
                endif
                if ( ${iforward} == 1 ) then
                  pdftk A=PS/MB_unbending.pdf B=CCPLOT.pdf cat A1-end B1 output out.pdf 
                else
                  pdftk A=CCPLOT.pdf B=PS/MB_unbending.pdf cat A1 B1-end output out.pdf 
                endif
                \mv -f out.pdf PS/MB_unbending.pdf
              endif
            endif
        
            ###########################################################################
            ${proc_2dx}/lin "TAPEREDGE - correct unbent frame average for taper edges"
            ###########################################################################
            setenv IN  ${frame_folder}/CCUNBEND_f${i}_notap.mrc
            setenv OUT ${frame_folder}/CCUNBEND_f${i}.mrc
            \rm -f     ${frame_folder}/CCUNBEND_f${i}.mrc
            ${bin_2dx}/2dx_taperedgek.exe << eot
30,30,100,30       ! IAVER,ISMOOTH,ITAPER
eot

            ###########################################################################
            ${proc_2dx}/lin "FFTRANS - Obtain Fourier-transform of unbent and masked frame average"
            ###########################################################################
            setenv IN  ${frame_folder}/CCUNBEND_f${i}.mrc
            setenv OUT ${frame_folder}/CCUNBEND_f${i}_fft.mrc
            \rm -f     ${frame_folder}/CCUNBEND_f${i}_fft.mrc
            ${bin_2dx}/2dx_fftrans.exe

            echo "# IMAGE-IMPORTANT: ${frame_folder}/CCUNBEND_f${i}.mrc <Unbent Frame ${i}>" >> LOGS/${scriptname}.results
            echo "# IMAGE-IMPORTANT: ${frame_folder}/CCUNBEND_f${i}_fft.mrc <Unbent Frame ${i} (FFT)>" >> LOGS/${scriptname}.results

            ###########################################################################
            ${proc_2dx}/lin "MMBOX - evaluate AMPlitudes and PHAses from unbent movie"
            ###########################################################################
            ${bin_2dx}/2dx_mmboxa.exe << eot
${frame_folder}/CCUNBEND_f${i}_fft.mrc
${imagenumber} ${nonmaskimagename}, Unbend2, ${date}
Y                               ! Use grid units?
Y                               ! Generate grid from lattice?
N                               ! Generate points from lattice?
2,2,0,50,50,19,19               ! IPIXEL,IOUT,NUMSPOT,NOH,NOK,NHOR,NVERT
${frame_folder}/aph_${i}.fou.nolimit.aph
SCRATCH/TMP9873.dat
UMB
${imagecenterx},${imagecentery}           ! XORIG,YORIG
200.0,1.5,1,${realcell},${ALAT},${realang} ! RINNER,ROUTER,IRAD,A,B,W,ABANG
${lattice}                         ! Lattice vectors
eot
            #
            #
            #
        endif
        #

