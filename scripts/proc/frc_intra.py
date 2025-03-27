#!/usr/bin/env python

#############################################################################
#                                                                           #
# FSC calculation and map postprocessing tools                 				#
#                                                                           #
# (C) 2dx.org, GNU Public License. 	                                        #
#                                                                           #
# Created..........: 09/09/2016                                             #
# Last Modification: 21/10/2017                                             #
# Author...........: Ricardo Righetto                                       #
# Author FCC...........: Robb McLeod                                        #
#																			#
# This program is inspired on the relion_postprocess program:				#
# http://www2.mrc-lmb.cam.ac.uk/relion 										#
# Reference: Scheres, JMB 2012												#
#																			#
# Single-Particle Wiener filter application is based on FREALIGN:			#
# http://grigoriefflab.janelia.org/frealign 								#
# References: 																#
# Grigorieff, Meth. Enzymol. 2016											#
# Sindelar & Grigorieff, JSB 2012											#
# 																			#
# High-Resolution Noise Substitution:										#
# Chen et al., Ultramicroscopy 2013											#
# 																			#
# Additional reference for FSC weighting and map sharpening: 				#
# Rosenthal & Henderson, JMB 2003											#
# 																			#
#############################################################################

import sys
import os
import numpy as np
import matplotlib.pyplot as plt
import focus_utilities as util
from mrcz import ioMRC
from optparse import OptionParser


def main():

	progname = os.path.basename(sys.argv[0])
	usage = progname + """ <half-map1> <half-map2> [options] 

	Given two unmasked and unfiltered reconstructions from random halves of your dataset, calculates the FSC between them and applies additional postprocessing filters.

	Output:

			-FSC plot(s) in PNG format
			-Text file containing description of FSC and filters applied (_data.fsc)
			-Masked and unmasked postprocessed maps 

	"""

	parser = OptionParser(usage)

	parser.add_option("--out", metavar="postprocess", type="string", default='postprocess', help="Output rootname.")

	parser.add_option("--angpix", metavar=1.0, type="float", help="Pixel size in Angstroems")

	parser.add_option("--fsc_threshold", metavar=0.143, default=0.143, type="float", help="Display the resolution at which the FSC curve crosses this value.")

	parser.add_option("--lowpass", metavar='"auto"', default='auto', help="Resolution (in Angstroems) at which to low-pass filter the final map. A negative value will skip low-pass filtering. Default is to low-pass at resolution determined from FSC threshold.")

	parser.add_option("--mask", metavar="MyMask.mrc", type="string", help="A file containing the mask to be applied to the half-maps before calculating the FSC.")

	parser.add_option("--force_mask", action="store_true", help="Force using this mask even if it has strange properties such as values outside the range [0,1].", default=False)

	parser.add_option("--mask_radius", metavar=0.5, default=None, type="float", help="If not specifying a mask file, a soft spherical mask can be created. This is the radius of this mask, in pixels or fraction of the box size.")
	
	parser.add_option("--mask_edge_width", metavar=6.0, default=None, type="float", help="This is the width of the cosine edge for the mask to be created, in pixels or fraction of the box size.")

	parser.add_option("--mask_center", metavar="0,0,0", default=None, type="string", help="Three numbers describing where the center of spherical mask should be placed within the 3D box (in pixels). Default is the middle of the box: (0,0,0). Can be positive or negative.")

	parser.add_option("--mw", metavar=1000.0, type="float", help="Molecular mass in kDa of particle or helical segment comprised within the mask. Needed to calculate volume-normalized Single-Particle Wiener filter (Sindelar & Grigorieff, JSB 2012). If not specified, will do conventional FSC weighting on the final map (Rosenthal & Henderson, JMB 2003).")

	parser.add_option("--mw_ignore", metavar=0.0, type="float", default=0.0, help="EXPERIMENTAL OPTION: Molecular mass in kDa present within the mask that needs to be ignored. Needed to calculate an adaptation of the volume-normalized Single-Particle Wiener filter (Sindelar & Grigorieff, JSB 2012). Only used if --mw is also specified. May be useful if your particles are extracted from 2D crystals.")

	parser.add_option("--skip_fsc_weighting", action="store_true", help="Do NOT apply FSC weighting (Rosenthal & Henderson, JMB 2003) to the final map, nor the Single-Particle Wiener filter (Sindelar & Grigorieff, JSB 2012).", default=False)

	parser.add_option("--apply_fsc2", action="store_true", help="Apply the square of the FSC curve as a filter. Generally should be used together with the --skip_fsc_weighting option.", default=False)

	parser.add_option("--gaussian", action="store_true", help="Apply a Gaussian low-pass filter to the map, with cutoff defined by --lowpass. (default)", default=True)

	parser.add_option("--cosine", action="store_true", help="Apply a cosine-edge instead of Gaussian low-pass filter to the map, with cutoff defined by --lowpass. The width of the cosine edge can be specified with the option --edge_width.", default=False)

	parser.add_option("--cosine_edge_width", metavar=2.0, type="float", help="Width of the cosine-edge filter (in Fourier pixels). If set to zero, becomes a top-hat filter.", default=2.0)

	parser.add_option("--tophat", action="store_true", help="Apply a top-hat low-pass filter to the final map. Equivalent to specifying --cosine with --edge_width=0.", default=False)

	parser.add_option("--mtf", type="string", help="File containing the detector MTF for sharpening of the final map.")

	parser.add_option("--auto_bfac", metavar="10.0,0.0", default=None, type="string", help="Estimate B-factor automatically using information in this resolution range, in Angstroems (lowres,highres). This works based on the Guinier plot, which should ideally be a straight line from ~10.0 A and beyond (Rosenthal & Henderson, JMB 2003).'If you set lowres and/or maxres to -1, these values will be calculated automatically. MTF and FSC weighting information are employed, if not ommitted.")

	parser.add_option("--adhoc_bfac", metavar=0.0, type="float", help="Apply an ad-hoc B-factor to the map (in Angstroems^2). Can be positive (smoothing) or negative (sharpening).", default=0.0)

	parser.add_option("--randomize_below_fsc", metavar=0.8, type="float", help="If provided, will randomize phases for all Fourier shells beyond where the FSC drops below this value, to assess correlations introduced by the mask by High-Resolution Noise Substitution (Chen et al, Ultramicroscopy 2013). Be aware that this procedure may introduce a 'dip' in the FSC curves at the corresponding resolution value.")

	# parser.add_option("--evaluate_spw_random", action="store_true", default=False, help="If both --mw and --randomize_below_fsc are provided, will evalute the performance of the Single-Particle Wiener filter (Sindelar & Grigorieff, JSB 2012) on the phase-randomized maps. Useful to assess how much artifacts (e.g. ringing) are being amplified by this filter.")

	# parser.add_option("--cone_aperture", metavar=90.0, type="float", help="Instead of the FSC, calculate the Fourier Conical Correlation, within a cone with this aperture, in degrees.")

	parser.add_option("--cone_aperture", metavar=90.0, type="float", help="If data contains a missing cone, use this option to exclude it from FSC calculations. A missing cone may introduce artificially high correlations in the FSC. The cone aperture (2*Theta) is given in degrees.")

	parser.add_option("--xy_only", action="store_true", default=False, help="CAUTION! EXPERIMENTAL OPTION: Evaluate average resolution along X,Y planes only.")

	parser.add_option("--z_only", action="store_true", default=False, help="CAUTION! EXPERIMENTAL OPTION: Evaluate average resolution along the Z direction only.")

	parser.add_option("--refine_res_lim", metavar=10.0, type="float", help="Resolution limit in Angstroems used during the refinement, to be displayed on the FSC plots.")

	parser.add_option("--resample", metavar=1.0, type="float", help="Resample the final result in Fourier space to this pixel size (in Angstroems) in order to make the volume larger or smaller.")

	# parser.add_option("--three_sigma", action="store_true", help="Show the 3-Sigma criterion curve on the FSC plots (van Heel, Ultramicroscopy 1987).", default=False)

	parser.add_option("--dpi", metavar=300, type="int", default=300, help="Resolution of the PNG files to be saved with the FSC curves (dots per inch).")

	(options, args) = parser.parse_args()

	command = ' '.join(sys.argv)

	# Do some sanity checks:
	if len(sys.argv) < 3:

		# print len(sys.argv), args
		print ( 'You must specify at least two map files to compute an FSC:' )
		print ( usage )
		sys.exit(1)

	if options.lowpass != 'auto':

		options.lowpass = float(options.lowpass)

	if options.mw != None and options.mw < 0.0:

		print ( 'Molecular mass cannot be negative!' )
		sys.exit(1)

	if options.mw_ignore != None and options.mw_ignore < 0.0:

		print ( 'Molecular mass to be ignored cannot be negative!' )
		sys.exit(1)

	if options.angpix == None:

		print ( '\nWARNING: Pixel size was not specified. Assuming 1.0 A/pixel.' )
		options.angpix = 1.0 

	elif options.angpix <= 0.0:

		print ( 'Pixel size must be greater than zero!' )
		sys.exit(1)

	if options.cosine_edge_width != None and options.cosine_edge_width < 0.0:

		print ( '\nCosine edge width cannot be negative!' )
		sys.exit(1)

	if options.refine_res_lim != None and options.refine_res_lim <= 0.0:

		print ( 'Refinement resolution limit must be greater than zero!' )
		sys.exit(1)

	if options.randomize_below_fsc != None and (options.randomize_below_fsc < -1.0 or options.randomize_below_fsc > 1.0):

		print ( 'FSC values for phase-randomization must be in the range [-1,1]!' )
		sys.exit(1)

	if options.cone_aperture != None:

		options.cone_aperture = float(options.cone_aperture)/2

	if options.tophat:

		options.gaussian = False
		options.cosine = True
		options.cosine_edge_width = 0.0

	if options.mask_center == None:

		mask_center = [0,0,0]

	else:

		mask_center = np.array( map(float, options.mask_center.split( ',' ) ) )

	if options.resample != None and options.resample <= 0.0:

		print( 'Resampling pixel size must be greater than zero! options.resample = %f A' % options.resample )
		sys.exit(1)


	# Read in the two maps:
	sys.stdout = open(os.devnull, "w") # Suppress output
	map1 = ioMRC.readMRC( args[0] )[0]
	map2 = ioMRC.readMRC( args[1] )[0]
	map3 = ioMRC.readMRC( args[2] )[0]
	map4 = ioMRC.readMRC( args[3] )[0]
	sys.stdout = sys.__stdout__

	# We check if there is a mask file and if not, should we create one?
	if options.mask != None:

		sys.stdout = open(os.devnull, "w") # Suppress output
		mask = ioMRC.readMRC( options.mask )[0]
		sys.stdout = sys.__stdout__

	elif options.mask_radius != None or options.mask_edge_width != None:

		if options.mask_radius == None:

			options.mask_radius = 0.5

		if options.mask_edge_width == None:

			options.mask_edge_width = 6.0

		mask = util.SoftMask( map1.shape, radius = options.mask_radius, width = options.mask_edge_width, xyz=mask_center )

		sys.stdout = open(os.devnull, "w") # Suppress output
		ioMRC.writeMRC( mask, options.out+'-mask.mrc', dtype='float32', pixelsize=options.angpix, quickStats=True )
		sys.stdout = sys.__stdout__

		options.mask = options.out+'-mask.mrc'

	# Resolution range to estimate B-factor:
	if options.auto_bfac != None:

		resrange = options.auto_bfac.split(',')
		minres = float( resrange[0] ) 
		maxres = float( resrange[1] )

	NSAM = np.round( np.sqrt( np.sum( np.power( map1.shape, 2 ) ) ) / 2.0 / np.sqrt( len( map1.shape ) ) ).astype('int') + 1 # For cubic volumes this is just half the box size + 1.
	freq = ( np.arange( NSAM ) / ( 2.0 * ( NSAM - 1 ) * options.angpix ) ).reshape( NSAM, 1 )
	freq[0] = 1.0/999 # Just to avoid dividing by zero later
	freq2 = freq * freq

	if np.any( map1.shape != map2.shape ):

		print ( 'Input maps must be the same size!' )
		sys.exit(1)

	print ( '\nCalculating unmasked FSC...' )

	if options.cone_aperture == None:

		fsc12 = util.FCC( map1, map2, xy_only = options.xy_only, z_only = options.z_only )
		fsc34 = util.FCC( map3, map4, xy_only = options.xy_only, z_only = options.z_only )

	else:

		# l = NSAM/2 + 1
		# if np.mod(NSAM, 2):

		# 	freq[1:] = np.arange( float(l) ) / (NSAM - 1)

		# else:

		# 	freq[1:] = np.arange( float(l) ) / NSAM


		fsc = util.FCC( map1 , map2 , [ options.cone_aperture ], invertCone = True, xy_only = options.xy_only, z_only = options.z_only )

	# if options.three_sigma:

	# 	three_sigma_curve = 3.0 / np.sqrt( np.reshape(fscmat[:,2], (l, 1)) )

	dat = np.append(1.0/freq, freq,  axis=1) # Start creating the matrix that will be written to an output file
	head = 'Res       \t1/Res     \t' # Header of the output file describing the data columns

	res12 = ResolutionAtThreshold(freq[1:], fsc12[1:NSAM], options.fsc_threshold)
	res34 = ResolutionAtThreshold(freq[1:], fsc34[1:NSAM], options.fsc_threshold)
	print ( 'FSC 1-2 >= %.3f up to %.3f A (unmasked)' % (options.fsc_threshold, res12) )
	print ( 'FSC 3-4 >= %.3f up to %.3f A (unmasked)' % (options.fsc_threshold, res34) )

	# Plot
	plt.figure()
	# if options.three_sigma:

	# 	plt.plot(freq[1:], fsc, freq[1:], three_sigma_curve)

	# else:

	# 	plt.plot(freq[1:], fsc)
	plt.plot(freq[1:], fsc12[1:NSAM],freq[1:], fsc34[1:NSAM])
	plt.title('Fourier Ring Correlation')
	plt.ylabel('FRC')
	plt.xlabel('Spatial frequency (1/A)')
	plt.legend(['4k4k','8k4k'])
	plt.ylim([0.0,1.0])
	plt.minorticks_on()
	plt.grid(b=True, which='both')
	plt.savefig(options.out+'_frc-intra.png', dpi=options.dpi)
	plt.close()

	# # 	plt.plot(freq[1:], fsc)
	# map1amps = np.abs( np.fft.fftshift( np.fft.fftn( map1 ) ) )
	# map2amps = np.abs( np.fft.fftshift( np.fft.fftn( map2 ) ) )
	# map1ps = util.RadialProfile( map1amps )
	# map2ps = util.RadialProfile( map2amps )

	# plt.plot(freq[1:], map1ps[1:NSAM], freq[1:], map2ps[1:NSAM] )
	# plt.title('1D Amplitude Spectrum')
	# plt.ylabel('Amplitudes (a.u.)')
	# plt.xlabel('Spatial frequency (1/A)')
	# # plt.ylim([0.0,1.0])
	# plt.minorticks_on()
	# # ax = plt.gca()
	# # ax.set_yticks([options.fsc_threshold], minor=True)
	# # ax.set_yticklabels([str(options.fsc_threshold)], minor=True)
	# # if options.refine_res_lim != None:
	# # 	ax.axvline(1.0/options.refine_res_lim, linestyle='dashed', linewidth=0.75, color='m')
	# plt.legend([args[0], args[1]])
	# plt.grid(b=True, which='both')
	# plt.savefig(options.out+'_amps.png', dpi=options.dpi)
	# plt.close()

	# plt.plot(freq[1:], map1ps[1:NSAM]**2, freq[1:], map2ps[1:NSAM]**2 )
	# plt.title('1D Power Spectrum [Amps^2]')
	# plt.ylabel('Power (a.u.)')
	# plt.xlabel('Spatial frequency (1/A)')
	# # plt.ylim([0.0,1.0])
	# plt.minorticks_on()
	# # ax = plt.gca()
	# # ax.set_yticks([options.fsc_threshold], minor=True)
	# # ax.set_yticklabels([str(options.fsc_threshold)], minor=True)
	# # if options.refine_res_lim != None:
	# # 	ax.axvline(1.0/options.refine_res_lim, linestyle='dashed', linewidth=0.75, color='m')
	# plt.legend([args[0], args[1]])
	# plt.grid(b=True, which='both')
	# plt.savefig(options.out+'_ps.png', dpi=options.dpi)
	# plt.close()

	# plt.plot(freq[1:], np.log( map1ps[1:NSAM] ), freq[1:], np.log( map2ps[1:NSAM] ) )
	# plt.title('Guinier plot [ log(Amps) ]')
	# plt.ylabel('log(Amps)')
	# plt.xlabel('Spatial frequency (1/A)')
	# # plt.ylim([0.0,1.0])
	# plt.minorticks_on()
	# # ax = plt.gca()
	# # ax.set_yticks([options.fsc_threshold], minor=True)
	# # ax.set_yticklabels([str(options.fsc_threshold)], minor=True)
	# # if options.refine_res_lim != None:
	# # 	ax.axvline(1.0/options.refine_res_lim, linestyle='dashed', linewidth=0.75, color='m')
	# plt.legend([args[0], args[1]])
	# plt.grid(b=True, which='both')
	# plt.savefig(options.out+'_guinier.png', dpi=options.dpi)
	# plt.close()

	dat = np.append(dat, fsc12[:NSAM], axis=1) # Append the unmasked FSC
	head += 'FSC12-unmasked\t'
	dat = np.append(dat, fsc34[:NSAM], axis=1) # Append the unmasked FSC
	head += 'FSC34-unmasked\t'

	# Now we go to the mask-related operations which are activated if a mask or MW are specified. If only
	if options.mask != None or options.mw != None:

		if options.mask == None: # If MW is specified but no mask, we issue a warning:

			print ( '\nWARNING: You specified MW without a mask. This may produce inaccurate results!') 

			# rmin = np.float( np.min( map1.shape ) ) / 2.0
			# mask = util.SoftMask( map1.shape, radius = rmin - 4.0, width = 6.0 )
			mask = np.ones( map1.shape, dtype='float' )

			sys.stdout = open(os.devnull, "w") # Suppress output
			ioMRC.writeMRC( mask, options.out+'-mask.mrc', dtype='float32', pixelsize=options.angpix, quickStats=True )
			sys.stdout = sys.__stdout__

			options.mask = options.out+'-mask.mrc'

		if options.force_mask == False:

			if (mask.min() < -0.001 or mask.max() > 1.001):

				print ( '\nMask values not in range [0,1]! Min: %.6f, Max: %.6f' % (mask.min(), mask.max()) )
				sys.exit(1)

		else:

			print ( '\nWARNING: You are forcing a mask that may have strange properties. Use at your own risk!!!' )

		map1masked = map1 * mask
		map2masked = map2 * mask

		print ( '\nCalculating masked FSC...' )

		if options.cone_aperture == None:

			fsc_mask = util.FCC( map1masked, map2masked, xy_only = options.xy_only, z_only = options.z_only )

		else:

			fsc_mask = util.FCC( map1masked , map2masked , [ options.cone_aperture ], invertCone = True, xy_only = options.xy_only, z_only = options.z_only )

		res_mask = ResolutionAtThreshold(freq[1:], fsc_mask[1:NSAM], options.fsc_threshold)
		print ( 'FSC >= %.3f up to %.3f A (masked)' % (options.fsc_threshold, res_mask) )

		dat = np.append(dat, fsc_mask[:NSAM], axis=1) # Append the masked FSC
		head += 'FSC-masked\t'

		if options.randomize_below_fsc == None:

			# Plot
			plt.figure()
			plt.plot(freq[1:], fsc_mask[1:NSAM])
			plt.title('Fourier Shell Correlation - masked')
			plt.ylabel('FSC')
			plt.xlabel('Spatial frequency (1/A)')
			plt.minorticks_on()
			ax = plt.gca()
			ax.set_yticks([options.fsc_threshold], minor=True)
			ax.set_yticklabels([str(options.fsc_threshold)], minor=True)
			if options.refine_res_lim != None:
				ax.axvline(1.0/options.refine_res_lim, linestyle='dashed', linewidth=0.75, color='m')
			plt.grid(b=True, which='both')
			plt.savefig(options.out+'_fsc-masked.png', dpi=options.dpi)
			plt.close()

		else:

			rand_res = ResolutionAtThreshold(freq[1:], fsc[1:NSAM], options.randomize_below_fsc)
			print ( '\nRandomizing phases beyond %.2f A...\n' % rand_res )
			rand_freq = 1.0/rand_res

			np.random.seed( seed=123 ) # We have to enforce the random seed otherwise different runs would not be comparable
			map1randphase = util.HighResolutionNoiseSubstitution( map1, lp = rand_res, apix = options.angpix )

			np.random.seed( seed=1234 ) # Cannot use same random seed for both maps!!!
			map2randphase = util.HighResolutionNoiseSubstitution( map2, lp = rand_res, apix = options.angpix )

			# We mask the phase-randomized maps:
			map1randphasemasked = map1randphase * mask
			map2randphasemasked = map2randphase * mask

			print ( '\nCalculating masked FSC for phase-randomized maps...' )

			if options.cone_aperture == None:

				fsc_mask_rnd = util.FCC( map1randphasemasked, map2randphasemasked, xy_only = options.xy_only, z_only = options.z_only )

			else:

				fsc_mask_rnd = util.FCC( map1randphasemasked , map2randphasemasked , [ options.cone_aperture ], invertCone = True, xy_only = options.xy_only, z_only = options.z_only )

			# We compute FSCtrue following (Chen et al, Ultramicroscopy 2013). For masked maps this will correct the FSC for eventual refinement overfitting, including from the mask:

			# fsc_mask_true[freq >= rand_freq] = (fsc_mask[freq >= rand_freq] - fsc_mask_rnd[freq >= rand_freq]) / (1 - fsc_mask_rnd[freq >= rand_freq])
			fsc_mask_true = ( ( fsc_mask - fsc_mask_rnd ) / ( 1.0 - fsc_mask_rnd ) )
			fsc_mask_true[:NSAM][freq < rand_freq] = fsc_mask[:NSAM][freq < rand_freq]
			fsc_mask_true = np.nan_to_num( fsc_mask_true )

			res_mask_true = ResolutionAtThreshold(freq[1:], fsc_mask_true[1:NSAM], options.fsc_threshold)
			print ( 'FSC >= %.3f up to %.3f A (masked - true)' % (options.fsc_threshold, res_mask_true) )

			dat = np.append(dat, fsc_mask_true[:NSAM], axis=1) # Append the true masked FSC
			head += 'FSC-masked_true\t'

			# Plot
			plt.figure()
			plt.plot(freq[1:], fsc_mask[1:NSAM], freq[1:], fsc_mask_rnd[1:NSAM], freq[1:], fsc_mask_true[1:NSAM])
			plt.title('Fourier Shell Correlation - masked')
			plt.ylabel('FSC')
			plt.xlabel('Spatial frequency (1/A)')
			plt.legend(['FSC', 'FSC - phase randomized', 'FSC - true'])
			plt.minorticks_on()
			ax = plt.gca()
			ax.set_yticks([options.fsc_threshold], minor=True)
			ax.set_yticklabels([str(options.fsc_threshold)], minor=True)
			if options.refine_res_lim != None:
				ax.axvline(1.0/options.refine_res_lim, linestyle='dashed', linewidth=0.75, color='m')
			plt.grid(b=True, which='both')
			plt.savefig(options.out+'_fsc-masked_true.png', dpi=options.dpi)
			plt.close()

			res_mask = res_mask_true

		if options.mw == None and options.randomize_below_fsc == None:

			# Plot
			plt.figure()
			plt.plot(freq[1:], fsc[1:NSAM], freq[1:], fsc_mask[1:NSAM])
			plt.title('Fourier Shell Correlation')
			plt.ylabel('FSC')
			plt.xlabel('Spatial frequency (1/A)')
			plt.legend(['unmasked', 'masked'])
			plt.minorticks_on()
			ax = plt.gca()
			ax.set_yticks([options.fsc_threshold], minor=True)
			ax.set_yticklabels([str(options.fsc_threshold)], minor=True)
			if options.refine_res_lim != None:
				ax.axvline(1.0/options.refine_res_lim, linestyle='dashed', linewidth=0.75, color='m')
			plt.grid(b=True, which='both')
			plt.savefig(options.out+'_fsc.png', dpi=options.dpi)
			plt.close()

		elif options.mw == None and options.randomize_below_fsc != None:

			# Plot
			plt.figure()
			plt.plot(freq[1:], fsc[1:NSAM], freq[1:], fsc_mask_true[1:NSAM])
			plt.title('Fourier Shell Correlation')
			plt.ylabel('FSC')
			plt.xlabel('Spatial frequency (1/A)')
			plt.legend(['unmasked', 'masked - true'])
			plt.minorticks_on()
			ax = plt.gca()
			ax.set_yticks([options.fsc_threshold], minor=True)
			ax.set_yticklabels([str(options.fsc_threshold)], minor=True)
			if options.refine_res_lim != None:
				ax.axvline(1.0/options.refine_res_lim, linestyle='dashed', linewidth=0.75, color='m')
			plt.grid(b=True, which='both')
			plt.savefig(options.out+'_fsc.png', dpi=options.dpi)
			plt.close()


		if options.mw != None:

			DALT = 0.81 # Da/A^3

			# Estimate fraction of volume occupied by the molecule:
			fpart = 1000.0 * options.mw / DALT / (options.angpix * 2*NSAM)**3

			fignore = 1000.0 * options.mw_ignore / DALT / (options.angpix * 2*NSAM)**3

			# Fraction of the volume occupied by the mask:
			maskvoxsum = np.sum(mask)
			fmask = maskvoxsum / (2*NSAM)**3

			print ( '\nCalculating Single-Particle Wiener filter...' )
			print ( '\nFraction of particle within the volume (Fpart): %.6f' % fpart )
			print ( 'Fraction of mask within the volume (Fmask): %.6f' % fmask )
			if options.mw_ignore > 0.0:

				print ( 'Fraction of densities to be ignored within the volume (Fignore): %.6f' % fignore )
				print ( 'Fpart/(Fmask-Fignore) ratio: %.6f' % (fpart/(fmask-fignore)) )

				if (fpart/(fmask-fignore)) >= 1.0:

					print ( '\nWARNING: Your particle occupies a volume bigger than the mask. Mask is probably too tight or even too small!' )

			else:

				print ( 'Fpart/Fmask ratio: %.6f' % (fpart/fmask) )

				if (fpart/fmask) >= 1.0:

					print ( '\nWARNING: Your particle occupies a volume bigger than the mask. Mask is probably too tight or even too small!' )

			# Let's do Single-Particle Wiener filtering following (Sindelar & Grigorieff, 2012):

			if options.randomize_below_fsc == None:

				fsc_spw = fsc_mask / (fsc_mask + (fpart / (fmask - fignore)) * (1.0 - fsc_mask))

			else:

				fsc_spw = fsc_mask_true / (fsc_mask_true + (fpart / (fmask - fignore)) * (1.0 - fsc_mask_true))

			res_spw = ResolutionAtThreshold(freq[1:], fsc_spw[1:NSAM], options.fsc_threshold)
			print ( '\nFSC >= %.3f up to %.3f A (volume-normalized)' % (options.fsc_threshold, res_spw) )

			dat = np.append(dat, fsc_spw[:NSAM], axis=1) # Append the FSC-SPW
			head += 'FSC-SPW   \t'

			# Plot
			plt.figure()
			plt.plot(freq[1:], fsc_spw[1:NSAM])
			plt.title('Fourier Shell Correlation - Single-Particle Wiener filter')
			plt.ylabel('FSC')
			plt.xlabel('Spatial frequency (1/A)')
			plt.minorticks_on()
			ax = plt.gca()
			ax.set_yticks([options.fsc_threshold], minor=True)
			ax.set_yticklabels([str(options.fsc_threshold)], minor=True)
			if options.refine_res_lim != None:
				ax.axvline(1.0/options.refine_res_lim, linestyle='dashed', linewidth=0.75, color='m')
			plt.grid(b=True, which='both')
			plt.savefig(options.out+'_fsc-spw.png', dpi=options.dpi)
			plt.close()

			if options.randomize_below_fsc != None:

				# Plot
				plt.figure()
				plt.plot(freq[1:], fsc[1:NSAM], freq[1:], fsc_mask[1:NSAM], freq[1:], fsc_spw[1:NSAM], freq[1:], fsc_mask_true[1:NSAM])
				plt.title('Fourier Shell Correlation')
				plt.ylabel('FSC')
				plt.xlabel('Spatial frequency (1/A)')
				plt.legend(['unmasked', 'masked', 'masked - SPW', 'masked - true'])
				plt.minorticks_on()
				ax = plt.gca()
				ax.set_yticks([options.fsc_threshold], minor=True)
				ax.set_yticklabels([str(options.fsc_threshold)], minor=True)
				if options.refine_res_lim != None:
					ax.axvline(1.0/options.refine_res_lim, linestyle='dashed', linewidth=0.75, color='m')
				plt.grid(b=True, which='both')
				plt.savefig(options.out+'_fsc.png', dpi=options.dpi)
				plt.close()

			else:

				# Plot
				plt.figure()
				plt.plot(freq[1:], fsc[1:NSAM], freq[1:], fsc_mask[1:NSAM], freq[1:], fsc_spw[1:NSAM])
				plt.title('Fourier Shell Correlation')
				plt.ylabel('FSC')
				plt.xlabel('Spatial frequency (1/A)')
				plt.legend(['unmasked', 'masked', 'masked - SPW'])
				plt.minorticks_on()
				ax = plt.gca()
				ax.set_yticks([options.fsc_threshold], minor=True)
				ax.set_yticklabels([str(options.fsc_threshold)], minor=True)
				if options.refine_res_lim != None:
					ax.axvline(1.0/options.refine_res_lim, linestyle='dashed', linewidth=0.75, color='m')
				plt.grid(b=True, which='both')
				plt.savefig(options.out+'_fsc.png', dpi=options.dpi)
				plt.close()

#### MAP FILTERING STEPS:

	# 1. Sum the two half-reconstructions:
	print ( '\nAveraging the two half-maps...' )
	fullmap = 0.5 * ( map1 + map2 )

	# 2. Apply FSC weighting or SPW filter to the final map, accordingly:
	if options.skip_fsc_weighting == False:

		print ( 'Applying FSC weighting (Cref) to the map...' )
		if options.mask == None and options.mw == None:
			
			# Derive weights from unmasked FSC
			fsc_weights = np.sqrt(2 * np.abs(fsc) / (1 + np.abs(fsc)))

		elif options.mw == None:

			# Derive weights from masked FSC
			if options.randomize_below_fsc != None:

				fsc_weights = np.sqrt(2 * np.abs(fsc_mask_true) / (1 + np.abs(fsc_mask_true)))

			else:

				fsc_weights = np.sqrt(2 * np.abs(fsc_mask) / (1 + np.abs(fsc_mask)))

		else:

			fsc_weights = np.sqrt(2 * np.abs(fsc_spw) / (1 + np.abs(fsc_spw)))
			
		fullmap = util.RadialFilter( fullmap, fsc_weights, return_filter = False )

		dat = np.append(dat, fsc_weights[:NSAM], axis=1) # Append the FSC weighting
		head += 'Cref_Weights\t'

	# 3. Sharpen map by recovering amplitudes from detector's MTF:
	if options.mtf != None:

		print ( 'Dividing map by the detector MTF...' )

		try:

			mtf = np.loadtxt(options.mtf)

			ignore_mtf = False

		except ValueError:

			if options.mtf[-5:] == '.star':

				mtf = np.loadtxt(options.mtf, skiprows=4)

				ignore_mtf = False

			else:

				print ( 'Could not read MTF file! Ignoring MTF...' )
				ignore_mtf = True

		if ignore_mtf == False:

			# We need to know the MTF values at the Fourier bins of our map. So we interpolate from the MTF description available:

			NSAMfull = np.ceil( np.sqrt( np.sum( np.power( map1.shape, 2 ) ) ) / 2.0 + 1).astype('int') # For cubic volumes this is just half the box size multiplied by sqrt(2).
			freqfull = ( np.arange( NSAMfull ) / ( 2.0 * NSAM * options.angpix ) ).reshape( NSAMfull, 1 )
			freqfull[0] = 1.0/999 # Just to avoid dividing by zero later

			interp_mtf = np.interp(freqfull, mtf[:,0], mtf[:,1])

			# print len(interp_mtf),len(freq[1:]full)

			# Divide Fourier components by the detector MTF:
			inv_mtf = 1.0/interp_mtf

			fullmap = util.RadialFilter( fullmap, inv_mtf, return_filter = False )

			dat = np.append(dat, inv_mtf[:NSAM], axis=1) # Append the inverse MTF applied
			head += 'InverseMTF\t'

	# 4. Perform automatic sharpening based on the Guinier plot:

	##### GUINIER PLOT ##### 
	if options.auto_bfac != None:

		# Here we use the same method as relion_postprocess. Note there is a difference in the normalization of the FFT, but that doesn't affect the results (only the intercept of fit).
		# NOTE: the bfactor.exe and EM-BFACTOR programs use a different fitting method.
		radamp = util.RadialProfile( np.abs( np.fft.fftshift( np.fft.fftn( fullmap ) ) ) )[:NSAM]
		lnF = np.log( radamp )
		if minres == -1.0:
			minres = 10.0
		if maxres == -1.0:
			if options.mw != None:

				maxres = res_spw

			elif options.mask != None:

				maxres = res_mask

			else:

				maxres = res

		print ( '\nEstimating contrast decay (B-factor) from Guinier plot between %.2f A and %.2f A...\n' % (minres,maxres)) 

		hirange = 1./freq <= minres 
		lorange = 1./freq >= maxres
		resrange = hirange * lorange
		resrange = resrange[:,0]
		fit = np.polyfit( freq2[resrange,0], lnF[resrange], deg=1)
		fitline = fit[0] * freq2 + fit[1]
		print ( 'Slope of fit: %.4f' % (fit[0]) )
		print ( 'Intercept of fit: %.4f' % (fit[1]) )
		print ( 'Correlation of fit: %.5f' % ( np.corrcoef( lnF[resrange], fitline[resrange,0] )[0,1] ) )
		print ( 'B-factor for contrast restoration: %.4f A^2\n' % ( 4.0 * fit[0] ) )

		fullmap = util.FilterBfactor( fullmap, apix=options.angpix, B = 4.0 * fit[0], return_filter = False )
		guinierfilt = np.exp( - fit[0] * freq2  ) # Just for appending to the output data file
		dat = np.append(dat, guinierfilt[:NSAM], axis=1) # Append the B-factor filter derived from the Guinier plot
		head += 'Auto_B-factor\t'

		radampnew = util.RadialProfile( np.abs( np.fft.fftshift( np.fft.fftn( fullmap ) ) ) )[:NSAM]
		lnFnew = np.log( radampnew )

		# Plot
		plt.figure()
		plt.plot( freq2, lnF, freq2[lorange[:,0],0], lnFnew[lorange[:,0]], freq2[resrange,0], fitline[resrange]  )
		plt.title('Guinier Plot')
		plt.ylabel('ln(F)')
		plt.xlabel('Spatial frequency^2 (1/A^2)')
		plt.legend(['Exp.', 'Exp. sharpened', 'Fit'])
		ax = plt.gca()
		ax.axvline(1.0/minres**2, linestyle='dashed', linewidth=0.75, color='m')
		ax.axvline(1.0/maxres**2, linestyle='dashed', linewidth=0.75, color='m')
		plt.grid(b=True, which='both')
		plt.savefig(options.out+'_guinier.png', dpi=options.dpi)
		plt.close()

		if options.cosine == False:

			print ( '\nWARNING: You should probably specify --cosine option to low-pass filter your map after sharpening!\n' )

	# 5. Apply an ad-hoc B-factor for smoothing or sharpening the map, if provided:
	if options.adhoc_bfac != 0.0:
		print ( 'Applying ad-hoc B-factor to the map...' )

		fullmap = util.FilterBfactor( fullmap, apix=options.angpix, B=options.adhoc_bfac, return_filter = False )
		freq2 = freq * freq
		bfacfilt = np.exp( - options.adhoc_bfac * freq2 / 4.0  ) # Just for appending to the output data file

		dat = np.append(dat, bfacfilt[:NSAM], axis=1) # Append the ad-hoc B-factor filter applied
		head += 'Adhoc_B-factor\t'

		if options.cosine == False:

			print ( '\nWARNING: You should probably specify --cosine option to low-pass filter your map after sharpening!\n' )

	# 6. Apply FSC^2 weighting to the final map:
	if options.apply_fsc2:

		print ( 'Applying FSC^2 weighting to the map...' )
		if options.mask == None and options.mw == None:
			
			# Derive weights from unmasked FSC
			fsc2_weights = fsc**2

		elif options.mw == None:

			# Derive weights from masked FSC
			if options.randomize_below_fsc != None:

				fsc2_weights = fsc_mask_true**2

			else:

				fsc2_weights = fsc_mask**2

		else:

			fsc2_weights = fsc_spw**2
			
		fullmap = util.RadialFilter( fullmap, fsc2_weights, return_filter = False )

		dat = np.append(dat, fsc2_weights[:NSAM], axis=1) # Append the FSC weighting
		head += 'FSC^2_Weights\t'

	# 7. Impose a Gaussian or Cosine or Top-hat low-pass filter with cutoff at given resolution, or resolution determined from FSC threshold:
	if options.lowpass == 'auto':
		print ( 'Low-pass filtering the map at resolution cutoff...' )
		if options.mw != None:

			res_cutoff = res_spw

		elif options.mask != None:

			res_cutoff = res_mask

		else:

			res_cutoff = res
	
		if options.tophat == False and options.cosine == False: 

			fullmap = util.FilterGauss( fullmap, apix=options.angpix, lp=res_cutoff, return_filter = False )
			lp = np.exp( - res_cutoff ** 2 * freq2[:,0] / 2 )

		else:

			fullmap = util.FilterCosine( fullmap, apix=options.angpix, lp=res_cutoff, return_filter = False, width = options.cosine_edge_width )
			cosrad = np.argmin( np.abs( 1./freq - res_cutoff ) )
			rii = cosrad + options.cosine_edge_width/2
			rih = cosrad - options.cosine_edge_width/2
			lp = np.zeros( freq[:,0].shape )
			r = np.arange( len( freq ) )
			fill_idx = r <= rih
			lp[fill_idx] = 1.0
			rih_idx = r > rih
			rii_idx = r <= rii
			edge_idx = rih_idx * rii_idx
			lp[edge_idx] = ( 1.0 + np.cos( np.pi * ( r[edge_idx] - rih ) / options.cosine_edge_width ) ) / 2.0

		dat = np.append(dat, lp.reshape(NSAM,1), axis=1) # Append the low-pass filter applied
		head += 'Low-pass  \t'			
		

	elif options.lowpass >= 0.0:
		print ( 'Low-pass filtering the map at resolution cutoff...' )
		res_cutoff = options.lowpass

		if options.tophat == False and options.cosine == False: 

			fullmap = util.FilterGauss( fullmap, apix=options.angpix, lp=res_cutoff, return_filter = False )
			lp = np.exp( - res_cutoff ** 2 * freq2[:,0] / 2 )

		else:

			fullmap = util.FilterCosine( fullmap, apix=options.angpix, lp=res_cutoff, return_filter = False, width = options.cosine_edge_width )
			cosrad = np.where( freq <= 1./res_cutoff )[0][0]
			rii = cosrad + options.cosine_edge_width/2
			rih = cosrad - options.cosine_edge_width/2
			lp = np.zeros( freq[:,0].shape )
			r = np.arange( len( freq ) )
			fill_idx = r <= rih
			lp[fill_idx] = 1.0
			rih_idx = r > rih
			rii_idx = r <= rii
			edge_idx = rih_idx * rii_idx
			lp[edge_idx] = ( 1.0 + np.cos( np.pi * ( r[edge_idx] - rih ) / options.cosine_edge_width ) ) / 2.0

		dat = np.append(dat, lp.reshape(NSAM,1), axis=1) # Append the low-pass filter applied
		head += 'Low-pass  \t'				


	# 8. Apply mask, if provided:
	if options.mask != None or options.mw != None:
		print ( 'Masking the map...' )
		masked = fullmap * mask

		if options.resample == None:

			sys.stdout = open(os.devnull, "w") # Suppress output
			ioMRC.writeMRC( masked, options.out+'-masked.mrc', dtype='float32', pixelsize=options.angpix, quickStats=True )
			sys.stdout = sys.__stdout__

		else:

			masked = util.Resample( masked, apix = options.angpix, newapix = options.resample )
			sys.stdout = open(os.devnull, "w") # Suppress output
			ioMRC.writeMRC( masked, options.out+'-masked.mrc', dtype='float32', pixelsize=options.resample, quickStats=True )
			sys.stdout = sys.__stdout__

			mask = util.Resample( mask, apix = options.angpix, newapix = options.resample )
			sys.stdout = open(os.devnull, "w") # Suppress output
			ioMRC.writeMRC( mask, options.out+'-mask.mrc', dtype='float32', pixelsize=options.resample, quickStats=True )
			sys.stdout = sys.__stdout__

	# Write filtered, unmasked map
	if options.resample == None:

		sys.stdout = open(os.devnull, "w") # Suppress output
		ioMRC.writeMRC( fullmap, options.out+'-unmasked.mrc', dtype='float32', pixelsize=options.angpix, quickStats=True )
		sys.stdout = sys.__stdout__

	else:

		fullmap = util.Resample( fullmap, apix = options.angpix, newapix = options.resample )
		sys.stdout = open(os.devnull, "w") # Suppress output
		ioMRC.writeMRC( fullmap, options.out+'-unmasked.mrc', dtype='float32', pixelsize=options.resample, quickStats=True )
		sys.stdout = sys.__stdout__


	# Save output file with all relevant FSC data
	np.savetxt(options.out+'_data.fsc', np.matrix(dat), header=command+'\n'+head, delimiter='\t', fmt='%.6f')

	print ( '\nDone!' )


def ResolutionAtThreshold(freq, fsc, thr):
# Do a simple linear interpolation to get resolution value at the specified FSC threshold
# (DEPRECATED, RETURN THE RESOLUTION AT WHICH FSC IS STILL HIGHER THAN THRESHOLD)

	i = 0
	for f in fsc:

		# if f < thr and i > 0:
		if f < thr:

			break

		i += 1

	if i < len(fsc)-1 and i > 0:

		# y1 = fsc[i-1]
		# y0 = fsc[i-2]
		# x1 = freq[1:][i-1]
		x0 = freq[i-1]

		# delta = (y1-y0)/(x1-x0)

		# res_freq[1:] = x0 + (thr - y0) / delta
		
		# Just return the highest resolution bin at which FSC is still higher than threshold:
		res_freq = x0

	elif i == 0:

		res_freq = freq[i]

	else:

		print ( '\nFSC NEVER DROPS BELOW %.3f THRESHOLD. THERE IS SOMETHING WRONG!!!\n' % thr)
		res_freq = freq[-1]


	return 1/res_freq


if __name__ == "__main__":
	main()
