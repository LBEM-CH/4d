#
#############################################################################
#                                                                           #
# This is not an executable script.                                         #
#                                                                           #
# This should be sourced from the calling script.                           #
#                                                                           #
# (C) 2dx.org, GNU Plublic License.                                         #
#                                                                           #
# Created..........: 01/03/2007                                             #
# Last Modification: 01/03/2007                                             #
# Author...........: 2dx.org                                                #
#                                                                           #
#############################################################################
#
echo bin_2dx = ${bin_2dx}
echo proc_2dx = ${proc_2dx}
echo ccp4 = ${ccp4}
echo bin_ccp4 = ${bin_ccp4}
#
# this is for a later option:
set mode = 0
#
if ( ${mode} == 0 ) then
  #
  if ( ${scalimamp3d_rref} != "0" ) then
    #
    #############################################################################
    ${proc_2dx}/linblock "SCALIMAMP3D - to scale image amplitudes to selected reference data"
    #############################################################################
    #
    if ( ${scalimamp3d_rref} == "1" ) then 
      set scalimamp3d_refdata = FF
    endif
    if ( ${scalimamp3d_rref} == "2" ) then 
      set scalimamp3d_refdata = BT
    endif
    if ( ${scalimamp3d_rref} == "3" ) then 
      set scalimamp3d_refdata = LZ
      echo ":: "
      ${proc_2dx}/linblock "WARNING: SCALIMAMP3D option LZ is not yet working"
      echo ":: "
    endif
    if ( ${scalimamp3d_rref} == "4" ) then 
      set scalimamp3d_refdata = BR
    endif
    set bextra = 0.0
    #
    \rm -f SCALIMAMP3D.DAT
    set scalimamp_out = APH/merge_after_scalimamp3d.aph
    setenv OUT ${scalimamp_out}
    \rm -f ${scalimamp_out}
    #
    set RESMAX_local = 2.0
    echo ": "
    echo ": calling 2dx_scalimamp3d.exe with the following parameters:"
    echo ": ${scalimamp3d_refdata}"
    echo ": 0,F,${scalimamp3d_BXYMINMAX},${scalimamp3d_BZMINMAX} ! NPROG,TWOFOLD,BXYMIN,BXYMAX,BZMIN,BZMAX"
    echo ": ${RESMAX_local}, ${zstarrange_real}, ${scalimamp3d_BEXTRA} ! RESLIMXY, RESLIMZ, BEXTRA"
    echo ": APH/merge.aph"
    echo ": ${realcell}, ${realang}, ${RESMAX} !  A,B,GAMMA,RESOL"
    echo ":  "
    #
    ${bin_2dx}/2dx_scalimamp3d.exe << eot 
${scalimamp3d_refdata}
0,F,${scalimamp3d_BXYMINMAX},${scalimamp3d_BZMINMAX} ! NPROG,TWOFOLD,BXYMIN,BXYMAX,BZMIN,BZMAX
${RESMAX_local}, ${zstarrange_real}, ${scalimamp3d_BEXTRA} ! RESLIMXY, RESLIMZ, BEXTRA
APH/merge.aph
${realcell}, ${realang}, ${RESMAX_local} !  A,B,GAMMA,RESOL
eot
    #
    if ( ! -e ${scalimamp_out} ) then
      ${proc_2dx}/protest "ERROR: Problem in scalimamp3d.exe."
    else    
      \mv -f APH/merge.aph APH/merge_before_scalimamp3d.aph
      \mv -f ${scalimamp_out} APH/merge.aph
      echo "# IMAGE: APH/merge.aph <APH: merge.aph after scalimamp3d>" >> LOGS/${scriptname}.results
    endif
    #
    echo " "
    ${proc_2dx}/lin "-"
    echo " "
    #
  endif
  #
  #############################################################################
  ${proc_2dx}/linblock "LATLINPRESCAL - to apply CTF correction and weight calculation"
  #############################################################################
  #
  \rm -f fort.1
  \rm -f fort.3
  #
  \ln -s APH/merge.aph fort.1
  #
  echo "Calling: " > LOGS/latlinprescal.log
  echo "======== " >> LOGS/latlinprescal.log
  echo " " >> LOGS/latlinprescal.log
  #
  echo "  ${bin_2dx}/2dx_latlinprescal.exe << eot >> LOGS/latlinprescal.log " >> LOGS/latlinprescal.log
  echo "1001,${zminmax} ! NSER,ZMIN,ZMAX " >> LOGS/latlinprescal.log
  echo "${MergeIQMAX}               ! IQMAX " >> LOGS/latlinprescal.log
  echo "${max_amp_correction}       ! Max_Amp_Correction" >> LOGS/latlinprescal.log
  echo "${AMP_Scale_Factor}         ! AMP_Background_Scale Factor" >> LOGS/latlinprescal.log
  echo "eot " >> LOGS/latlinprescal.log
  echo " " >> LOGS/latlinprescal.log
  echo "Running: " >> LOGS/latlinprescal.log
  echo "======== " >> LOGS/latlinprescal.log
  #
  ${bin_2dx}/2dx_latlinprescal.exe << eot >> LOGS/latlinprescal.log
1001,${zminmax} ! NSER,ZMIN,ZMAX
${MergeIQMAX}               ! IQMAX
${max_amp_correction}       ! Max_Amp_Correction
${AMP_Scale_Factor}         ! AMP_Background_Scale Factor
eot
  #
  \rm -f fort.1
  echo "################################################"
  echo "################################################"
  echo "output in file LOGS/latlinprescal.log"
  echo "################################################"
  echo "################################################"
  #
  if ( -e fort.3 ) then
    \mv -f fort.3 APH/latlines.dat
    echo "# IMAGE: LOGS/latlinprescal.log <LOG: latlinprescal output>" >> LOGS/${scriptname}.results
    echo "# IMAGE: APH/latlines.dat <Latline after prescal [H,K,Z,A,P,SigA,SigP,IQ]>" >> LOGS/${scriptname}.results
  else
    ${proc_2dx}/protest "ERROR: latlines.dat does not exist."
  endif
  #
  set New_MergeLatLine_MaxAmpFactor = `echo ${MergeLatLine_MaxAmpFactor} | awk ' { if ( $1 < 1.0 ) { s = 0.0 } else { s = $1 } } END { printf "%.1f", s }'`
  if ( ${New_MergeLatLine_MaxAmpFactor}x != ${MergeLatLine_MaxAmpFactor}x ) then
    set MergeLatLine_MaxAmpFactor = ${New_MergeLatLine_MaxAmpFactor}
    echo "::Correcting MergeLatLine_MaxAmpFactor to ${MergeLatLine_MaxAmpFactor}"
    echo "set MergeLatLine_MaxAmpFactor = ${MergeLatLine_MaxAmpFactor}" >> LOGS/${scriptname}.results
  endif
  echo ": MergeLatLine_MaxAmpFactor = ${MergeLatLine_MaxAmpFactor}"
  if ( ${MergeLatLine_MaxAmpFactor}x == "0.0x" ) then
    set RMAXAMP = 0.0
    echo ":Using variable lattice line amplitude scaling."
  else
    if ( -e TMP_RMAXAMP.dat ) then
      set TMPVAL = `cat TMP_RMAXAMP.dat`
      \rm -f TMP_RMAXAMP.dat
      set RMAXAMP = `echo ${TMPVAL} ${MergeLatLine_MaxAmpFactor} | awk ' { s = $1 / $2 } END { print s }'`
      echo "::Using static lattice line amplitude scaling, ${MergeLatLine_MaxAmpFactor} below highest amplitude."
    else
      set RMAXAMP = 0.0
      echo ":Using variable lattice line amplitude scaling."
    endif
  endif
  #
  if ( ${MergeLatLine_RFOMSCALE}x == "0.0x" ) then
    set MergeLatLine_RFOMSCALE = 1.0
    echo "::Correcting MergeLatLine_RFOMSCALE to ${MergeLatLine_RFOMSCALE}"
    echo "set MergeLatLine_RFOMSCALE = ${MergeLatLine_RFOMSCALE}" >> LOGS/${scriptname}.results
  endif
  echo MergeLatLine_RFOMSCALE = ${MergeLatLine_RFOMSCALE}
  #
  echo " "
  ${proc_2dx}/lin "-"
  echo " "
  #
endif
#
set Reflections_Unique = `wc -l APH/latlines.dat | cut -d\  -f1`
echo "set Reflections_Unique = ${Reflections_Unique}" >> LOGS/${scriptname}.results
echo "::Unique Reflections = ${Reflections_Unique}"
#
echo "<<@progress: +5>>"  
#
##########################################################################################################################################################
##########################################################################################################################################################
##########################################################################################################################################################
##########################################################################################################################################################
if ( ${merge3D_algo}x == "0x" ) then
  #############################################################################
  ${proc_2dx}/linblock "LATLINEK - to fit lattice lines to merged data (D.Agard's program)"
  #############################################################################  
  #
  \rm -f PLOT.PS
  \rm -f PS/latline.ps
  \rm -f SCRATCH/latfitteds.dat
  \rm -f SCRATCH/guess.dat
  \rm -f latline.statistics
  #
  set iplterr = 0
  set idoh = 0
  set idok = 0
  set IAQP2 = 0
  set iplterr = 1         
  set imaxIQplot = ${MergeIQMAX}
  set MergeIGUESS = 1
  #
  if ( ${tempkeep} == 'y' ) then
    set iverbose = 1
  else
    set iverbose = 3
  endif
  #
  setenv  OBS   APH/latlines.dat
  setenv  OUT   SCRATCH/latfitteds.dat
  setenv  GUESS SCRATCH/guess.dat
  #
  echo " "
  echo " Parameters for latline are:"
  echo " "
  echo "${spcgrp}                            # IPG (Plane group number 1-17)"
  echo "${MergeIPAT}                             # IPAT, 0=F & Phase, 1=Intensity data"
  echo "${MergeAK},${MergeIWF_VAL},${MergeIWP_VAL}                      # AK,IWF,IWP - relative weights, + individual sigmas"
  echo "${ALAT},${zminmax},${MergeDELPLT}        # ALAT,ZMIN,ZMAX,DELPLT "
  echo "${MergeDELPRO},${MergeRminRmax},${MergeRCUT},${MergePFACT}        # DELPRO,RMIN,RMAX,RCUT,PFACT"
  echo "${MergeIGUESS},${MergeBINSIZ}                       # IGUESS,BINSIZ"
  echo "${MergeNCYCLS},${MergeMPRINT}                          # NCYCLS,MPRINT"
  echo "${idoh},${idok}		                          # H,K indices to plot. 0 0 = all."
  echo "${IAQP2},${iplterr},${imaxIQplot}                         # IAQP2: 1=y,0=n, iplterr=1:errbar in PHS, maxIQ for PSplot"
  echo "${RMAXAMP}"
  echo " " 
  #
  ${bin_2dx}/2dx_latlinek.exe << eot > LOGS/2dx_latlinek.log
2dx_merge_generate3D.com, ${date}
${spcgrp}                                                    ! IPG (Plane group number 1-17)
${MergeIPAT}                                                            ! IPAT, 0=F & Phase, 1=Intensity data
${MergeAK},${MergeIWF_VAL},${MergeIWP_VAL}                   ! AK,IWF,IWP - relative weights, + individual sigmas
${ALAT},${zminmax},${MergeDELPLT}                            ! ALAT,ZMIN,ZMAX,DELPLT
${MergeDELPRO},${MergeRminRmax},${MergeRCUT},${MergePFACT}   ! DELPRO,RMIN,RMAX,RCUT,PFACT
${MergeIGUESS},${MergeBINSIZ}                                ! IGUESS,BINSIZ
${MergeNCYCLS},${MergeMPRINT}                                ! NCYCLS,MPRINT
${idoh},${idok}                                              ! H,K indices to plot. 0 0 = all.
${IAQP2},${iplterr},${imaxIQplot}                            ! IAQP2: 1=y,0=n, iplterr=1:errbar in PHS, maxIQ for PSplot
${RMAXAMP},${MergeLatLine_RFOMSCALE}                         ! RMAXAMP: maximum amplitude; RFOMSCALE: increase in FOM for symmetry-restricted lattice lines
eot
  #
  echo "################################################"
  echo "################################################"
  echo "output in file LOGS/2dx_latlinek.log"
  echo "################################################"
  echo "################################################"    
  #
  echo "# IMAGE: LOGS/2dx_latlinek.log <LOG: 2dx_latlinek output>" >> LOGS/${scriptname}.results
  echo "# IMAGE: SCRATCH/latline_stat.dat <Lattice line statistics>" >> LOGS/${scriptname}.results
  echo "# IMAGE: SCRATCH/latfitteds.dat <Lattice line fit data [H,K,Z,A,PHI,SIGF,SIGP,FOM]>" >> LOGS/${scriptname}.results
  if ( -e SCRATCH/guess.dat ) then
    echo "# IMAGE: SCRATCH/guess.dat <Lattice line guess data>" >> LOGS/${scriptname}.results
  endif
  if ( ! -e latline.statistics ) then
    ${proc_2dx}/linblock "#"
    ${proc_2dx}/linhash "3D modus, but do you have 3D (i.e. tilted) data?"
    ${proc_2dx}/protest "ERROR in latlinek. Check logfile."
  endif
  \mv -f latline.statistics SCRATCH/latline_stat.dat
  set num_amplitudes_observed = `cat SCRATCH/latline_stat.dat | grep "Number of amplitudes observed" | cut -c55-`
  set num_phases_observed = `cat SCRATCH/latline_stat.dat | grep "Number of phases observed" | cut -c55-`
  set num_reflections_fitted = `cat SCRATCH/latfitteds.dat | wc -l`
  set overall_R_factor =  `cat SCRATCH/latline_stat.dat | grep "Overall R-factor" | cut -c55-`
  set overall_phase_residual =  `cat SCRATCH/latline_stat.dat | grep "Overall phase residual" | cut -c55-`
  set overall_weighted_R_factor =  `cat SCRATCH/latline_stat.dat | grep "Overall weighted R-factor" | cut -c55-`
  set overall_weighted_phase_residual =  `cat SCRATCH/latline_stat.dat | grep "Overall weighted phase residual" | cut -c55-`
  echo "set num_amplitudes_observed = ${num_amplitudes_observed}" >> LOGS/${scriptname}.results
  echo "set num_phases_observed = ${num_phases_observed}" >> LOGS/${scriptname}.results
  echo "set num_reflections_fitted = ${num_reflections_fitted}" >> LOGS/${scriptname}.results 
  echo "set overall_R_factor = ${overall_R_factor}" >> LOGS/${scriptname}.results
  echo "set overall_phase_residual = ${overall_phase_residual}" >> LOGS/${scriptname}.results
  echo "set overall_weighted_R_factor = ${overall_weighted_R_factor}" >> LOGS/${scriptname}.results
  echo "set overall_weighted_phase_residual = ${overall_weighted_phase_residual}" >> LOGS/${scriptname}.results
  #
  if ( ! -e PLOT.PS ) then
    ${proc_2dx}/protest "2dx_latlinek: ERROR occured."
  endif 
  #
  \mv -f PLOT.PS PS/latline.ps 
  echo "# IMAGE-IMPORTANT: PS/latline.ps <PS: Lattice lines>" >> LOGS/${scriptname}.results
  #
  if ( ${latline_pdf} == 'y' ) then
    \rm -f PS/latline.pdf
    ps2pdf PS/latline.ps PS/latline.pdf
    echo "# IMAGE-IMPORTANT: PS/latline.pdf <PDF: Lattice lines>" >> LOGS/${scriptname}.results
  endif  
  #
  #############################################################################
  ${proc_2dx}/linblock "2dx_plotresolution - Plotting resolution curves"
  #############################################################################
  #
  \rm -f PLOTCUR.PS
  if ( -e SCRATCH/latfitteds_limit.dat ) then   
    \rm -f SCRATCH/latfitteds_limit.dat
  endif
  #
  ${bin_2dx}/2dx_plotresolution.exe << eot
1
${realcell},${ALAT},${realang}
SCRATCH/latfitteds.dat
SCRATCH/latfitteds_limit.dat
APH/latlines.dat
${RESMAX}
${zstarrange_real}
${resolutionplot_RESMAX}
${MergeLatLine_REDUCAC}
${resolutionplot_bins}
eot
  #
  if ( ! -e PLOTCUR.PS ) then
    ${proc_2dx}/protest "ERROR in 2dx_plotresolution"
  else
    \mv -f PLOTCUR.PS PS/2dx_plotresolution.ps
    echo "# IMAGE-IMPORTANT: PS/2dx_plotresolution.ps <PS: Resolution Plot>" >> LOGS/${scriptname}.results
    echo "# IMAGE: SCRATCH/latfitteds_limit.dat <Lattice line limited  [H,K,Z,A,PHI,SIGF,SIGP,FOM]>" >> LOGS/${scriptname}.results
  endif
  #
  #############################################################################
  ${proc_2dx}/linblock "PREPMKLCF - Program to convert fitted data to CCP4 format"
  #############################################################################
  #
  \rm -f APH/latfitted_nosym.hkl
  \rm -f 2dx_prepmklcf.statistics
  #
  setenv IN SCRATCH/latfitteds_limit.dat
  setenv OUT APH/latfitted_nosym.hkl
  setenv REFHKL APH/latfittedref_nosym.hkl
  #
  # (1.0 to leave FOM values as they are, or 1.5 = to add a 60deg phase error)
  set REDUCAC = ${MergeLatLine_REDUCAC}   
  echo REDUCAC = ${REDUCAC}
  #
  echo "# IMAGE: LOGS/prepmklcf.log <LOG: prepmklcf output>" >> LOGS/${scriptname}.results
  #
  ${bin_2dx}/2dx_prepmklcf.exe << eot > LOGS/prepmklcf.log
${RESMAX},${REDUCAC}                   ! RESOLUTION,REDUCAC
${realcell},${realang},${ALAT}         ! a,b,gamma,c
0.0                                    ! SCALE (automatic scaling to max(AMP)=32000.0)
1				       ! 1=Calculate FOM from SIGF and SIGP. 0=Calculate FOM only from SIGP (this was the original version) 
eot
  #
  echo "################################################"
  echo "################################################"
  echo "output in file LOGS/prepmklcf.log"
  echo "################################################"
  echo "################################################"
  #
  if ( ! -e 2dx_prepmklcf.statistics ) then
    ${proc_2dx}/linblock "ERROR: 2dx_prepmklcf.statistics file is missing."
  else
    set num_reflections_FOM1 = `cat 2dx_prepmklcf.statistics | sed 's/ /_/g' | grep 'Number_of_phases_with_FOM_over_1' | sed s'/_/ /g' | cut -d= -f2`
    set num_reflections_FOM50 = `cat 2dx_prepmklcf.statistics | sed 's/ /_/g' | grep 'Number_of_phases_with_FOM_over_50' | sed s'/_/ /g' | cut -d= -f2`
    echo "::Number of phases with FOM>1% is ${num_reflections_FOM1}"
    echo "::Number of phases with FOM>50% is ${num_reflections_FOM50}"
    echo "set num_reflections_FOM1 = ${num_reflections_FOM1}" >> LOGS/${scriptname}.results
    echo "set num_reflections_FOM50 = ${num_reflections_FOM50}" >> LOGS/${scriptname}.results
    \mv -f 2dx_prepmklcf.statistics SCRATCH
  endif
  if ( ${tempkeep} == "y" ) then
    echo "# IMAGE: APH/latfitted_nosym.hkl <APH: Latline for vol after prepmklcf [H,K,L,F,P,FOM]>" >> LOGS/${scriptname}.results
    echo "# IMAGE: APH/latfittedref_nosym.hkl <APH: Latline for ref after prepmklcf [H,K,L,F,P,FOM,SIGF]>" >> LOGS/${scriptname}.results
  endif  
  #
  #############################################################################
  ${proc_2dx}/linblock "2dx_processor.exe - to expand symmetry to full P1 space"
  #############################################################################  
  #
  set infile = APH/latfitted_nosym.hkl
  set outfile = APH/latfitted.hkl
  #
  \rm -f ${outfile}
  #
  set nh_max = ${MergeHKMAX}
  set nk_max = ${MergeHKMAX}
  set nl_max = ${ALAT}
  #
  ${bin_2dx}/2dx_processor.exe --hklin ${infile} -s ${SYM_NAME} -X ${nh_max} -Y ${nk_max} -Z ${nl_max} --hklout ${outfile} --full-fourier
  echo "# IMAGE: ${infile} <HKL: HKL file before symmetrization [H,K,L,A,PHI,FOM]>" >> LOGS/${scriptname}.results
  echo "# IMAGE: ${outfile} <HKL: HKL file after symmetrization [H,K,L,A,PHI,FOM]>" >> LOGS/${scriptname}.results
  #
  ##########################################################################################################################################################
  ##########################################################################################################################################################
  ##########################################################################################################################################################
  ##########################################################################################################################################################
else
  #
  #############################################################################
  ${proc_2dx}/linblock "2dx Processor : creating FOM-weighted averaged HKL file"
  #############################################################################  
  #
  \rm -f PLOT.PS
  \rm -f PS/latline.ps
  \rm -f PS/latline.pdf
  \rm -f SCRATCH/latfitteds.dat
  \rm -f SCRATCH/guess.dat
  \rm -f latline.statistics
  #
  set split = ($realcell:as/,/ /)
  set cellx = $split[1]
  set celly = $split[2]
  #
  echo "cellx = ${cellx}"
  echo "celly = ${celly}"
  echo "cellz = ${ALAT}" 
  #
  echo ":Launching ${bin_2dx}/2dx_processor.exe --hkzin APH/latlines.dat -s ${SYM_NAME} -X ${cellx} -Y ${celly} -Z ${ALAT} --gamma ${realang} --res ${RESMAX} --hklout APH/latfitted.hkl --threshold 0 --normalize-grey"
  ${bin_2dx}/2dx_processor.exe --hkzin APH/latlines.dat -s ${SYM_NAME} -X ${cellx} -Y ${celly} -Z ${ALAT} --gamma ${realang} --res ${RESMAX} --hklout APH/latfitted.hkl --threshold 0 --normalize-grey
  #
  echo "# IMAGE: APH/latfitted.hkl <HKL: Generated HKL [H,K,L,A,PHI,FOM]>" >> LOGS/${scriptname}.results
  #
  #############################################################################
  ${proc_2dx}/linblock "2dx_plotresolution - Plotting resolution curves"
  #############################################################################
  #
  \rm -f PLOTCUR.PS
  if ( -e APH/latfitteds_limit.dat ) then   
    \rm -f APH/latfitteds_limit.dat
  endif
  #
  ${bin_2dx}/2dx_plotresolution.exe << eot
2
${realcell},${ALAT},${realang}
APH/latfitted.hkl
APH/latfitteds_limit.dat
APH/latlines.dat
${RESMAX}
${zstarrange_real}
${resolutionplot_RESMAX}
${MergeLatLine_REDUCAC}
${resolutionplot_bins}
eot
  #
  if ( ! -e PLOTCUR.PS ) then
    ${proc_2dx}/protest "ERROR in 2dx_plotresolution"
  else
    \mv -f PLOTCUR.PS PS/2dx_plotresolution.ps
    echo "# IMAGE-IMPORTANT: PS/2dx_plotresolution.ps <PS: Resolution Plot>" >> LOGS/${scriptname}.results
    echo "# IMAGE: APH/latfitteds_limit.dat <Lattice line limited  [H,K,Z,A,PHI,SIGF,SIGP,FOM]>" >> LOGS/${scriptname}.results
  endif
  #
  #############################################################################
  ${proc_2dx}/linblock "PREPMKLCF - Program to convert fitted data to CCP4 format"
  #############################################################################
  #
  \rm -f APH/latfitted_nosym.hkl
  \rm -f 2dx_prepmklcf.statistics
  #
  setenv IN APH/latfitteds_limit.dat
  setenv OUT APH/latfitted_nosym.hkl
  setenv REFHKL APH/latfittedref_nosym.hkl
  #
  # (1.0 to leave FOM values as they are, or 1.5 = to add a 60deg phase error)
  set REDUCAC = ${MergeLatLine_REDUCAC}   
  echo REDUCAC = ${REDUCAC}
  #
  echo "# IMAGE: LOGS/prepmklcf.log <LOG: prepmklcf output>" >> LOGS/${scriptname}.results
  #
  ${bin_2dx}/2dx_prepmklcf.exe << eot > LOGS/prepmklcf.log
${RESMAX},${REDUCAC}                   ! RESOLUTION,REDUCAC
${realcell},${realang},${ALAT}         ! a,b,gamma,c
0.0                                    ! SCALE (automatic scaling to max(AMP)=32000.0)
1				       ! 1=Calculate FOM from SIGF and SIGP. 0=Calculate FOM only from SIGP (this was the original version) 
eot
  #
  echo "################################################"
  echo "################################################"
  echo "output in file LOGS/prepmklcf.log"
  echo "################################################"
  echo "################################################"
  #
  if ( ! -e 2dx_prepmklcf.statistics ) then
    ${proc_2dx}/linblock "ERROR: 2dx_prepmklcf.statistics file is missing."
  else
    set num_reflections_FOM1 = `cat 2dx_prepmklcf.statistics | sed 's/ /_/g' | grep 'Number_of_phases_with_FOM_over_1' | sed s'/_/ /g' | cut -d= -f2`
    set num_reflections_FOM50 = `cat 2dx_prepmklcf.statistics | sed 's/ /_/g' | grep 'Number_of_phases_with_FOM_over_50' | sed s'/_/ /g' | cut -d= -f2`
    echo "::Number of phases with FOM>1% is ${num_reflections_FOM1}"
    echo "::Number of phases with FOM>50% is ${num_reflections_FOM50}"
    echo "set num_reflections_FOM1 = ${num_reflections_FOM1}" >> LOGS/${scriptname}.results
    echo "set num_reflections_FOM50 = ${num_reflections_FOM50}" >> LOGS/${scriptname}.results
    \mv -f 2dx_prepmklcf.statistics SCRATCH
  endif
  if ( ${tempkeep} == "y" ) then
    echo "# IMAGE: APH/latfitted_nosym.hkl <APH: Latline for vol after prepmklcf [H,K,L,F,P,FOM]>" >> LOGS/${scriptname}.results
    echo "# IMAGE: APH/latfittedref_nosym.hkl <APH: Latline for ref after prepmklcf [H,K,L,F,P,FOM,SIGF]>" >> LOGS/${scriptname}.results
  endif  
  #
endif
##########################################################################################################################################################
##########################################################################################################################################################
##########################################################################################################################################################
##########################################################################################################################################################
#
echo "<<@progress: +5>>"
#
#############################################################################
${proc_2dx}/linblock "Sourcing 2dx_hkl_to_mtz.com to create reference mtz file"
#############################################################################  
#
set outfile_ref = merge3Dref_MRClefthanded.mtz
\rm -f ${outfile_ref}
#
# echo "::Calling:" source ${proc_2dx}/2dx_hkl_to_mtz.com APH/latfitted.hkl ${realcell} ${ALAT} ${realang} ${RESMIN} ${MergeResolution} ${outfile_ref}
source ${proc_2dx}/2dx_hkl_to_mtz.com APH/latfitted.hkl ${realcell} ${ALAT} ${realang} ${RESMIN} ${MergeResolution} ${outfile_ref}
#
echo "# IMAGE-IMPORTANT: ${outfile_ref} <MTZ: Reference 3D MTZ file (MRC lefthanded) [H,K,L,F,P,FOM,SIGF] >" >> LOGS/${scriptname}.results
#

