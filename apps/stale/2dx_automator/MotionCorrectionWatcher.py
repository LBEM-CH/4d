from WatcherBaseClass import *
from EMAN2  import *
from sparx  import *

from pylab import plt, plot, subplot, figure, hist

import os

class MotionCorrectionWatcher(WatcherBase):
	
	def generateDriftPlot(self, filename):
		command = "driftplotter.py " + filename + " " + self.outfolder
		print command
		os.system(command)
		
	def __init__(self, refresh_time, wait_time, infolder, outfolder, log_file_name, first_frame=0, last_frame=0):
		self.refresh_time = refresh_time
		self.wait_time = wait_time
		self.infolder = infolder
		self.outfolder = outfolder
		self.log_file_name = log_file_name
		self.first_frame = first_frame
		self.last_frame = last_frame
		self.lock_compute = threading.Lock()
		self.fod = 7
		self.waittime = 40
		self.binning = 1
		self.bfac = 150
		self.mode = 0
		self.storestack = 0
		self.storelocation = "-"
		self.input_mode = 1
		self.filter_string = ".mrc"
	
	def file_filter(self, filename):
		return ((filename.endswith(self.filter_string)) and not (filename.endswith("_ready.mrc")) and not (filename.endswith("_ready_SumCorr.mrc")))
	
	def getFileCoreName(self, filename):
		return filename[:-4]
	
	def image_added(self, filename, do_wait = True):
		print "motion_correction for", filename
		filecorename = self.getFileCoreName(filename)
		
		if do_wait:
			time.sleep(15)
			old_size = os.path.getsize(self.infolder + "/" + filename)
			new_size = old_size + 1
			while old_size != new_size:
				print "file stil changing"
				print old_size
				time.sleep(1)
				old_size = new_size
				new_size = os.path.getsize(self.infolder + "/" + filename)
				time.sleep(2)
			print os.path.getsize(self.infolder + "/" + filename)
			
		print "*** In case you see this for a long time consider troubleshooting the automation ***"
		
		self.lock_compute.acquire()
		time.sleep(3)
		
		if self.input_mode == 1:
			shutil.copyfile(self.infolder + "/" + filename, self.tmp_location + "/mc_tmp.mrc")
		else:
			shutil.copyfile(self.infolder + "/" + filename, self.tmp_location + "/mc_tmp.dm4")
		
		if self.input_mode == 1:
			eman2_command = "e2proc2d.py " + self.tmp_location + "/mc_tmp.mrc" + " " + self.tmp_location + "/mc_ready.mrc --threed2threed"
		else:
			eman2_command = "e2proc2d.py " + self.tmp_location + "/mc_tmp.dm4" + " " + self.tmp_location + "/mc_ready.mrc --twod2threed --process xform.flip:axis=y"
		os.system(eman2_command)
		
		old_path = os.getcwd()
		os.chdir(self.tmp_location)
		
		if self.mode == 0:
			align_to = 1
		else:
			align_to = 0
		
		motion_command = "motioncorr " + self.tmp_location + "/mc_ready.mrc" + " -nst " + str(self.first_frame) + " -ned " + str(self.last_frame) + " -fod " + str(self.fod) + " -bin " + str(self.binning) + " -bft " + str(self.bfac) + " -atm " + str(align_to)
		
		if self.storestack == 1 and self.storelocation != "-" and self.mode==0:
			motion_command += " -ssc 1 -fct " + self.storelocation + "/" + filecorename + "_aligned.mrc"
			
		os.system(motion_command)
		
		if self.binning == 1:
			shutil.copyfile(self.tmp_location + "/mc_ready_SumCorr.mrc", self.outfolder + "/" + filecorename + "_aligned.mrc")
		else:
			shutil.copyfile(self.tmp_location + "/mc_ready_2x_SumCorr.mrc", self.outfolder + "/" + filecorename + "_aligned.mrc")
	
		if self.mode == 1:
			motion_command = "motioncorr " + self.tmp_location + "/mc_ready.mrc" + " -nst " + str(self.first_frame) + " -ned " + str(0) + " -fod " + str(15) + " -bin " + str(self.binning) + " -bft " + str(self.bfac) + " -atm " + str(align_to)	
			if self.storestack == 1 and self.storelocation != "-":
				motion_command += " -fct " + self.storelocation
			os.system(motion_command)
			if self.binning == 1:
				shutil.copyfile(self.tmp_location + "/mc_ready_SumCorr.mrc", self.outfolder + "/" + filecorename + "_fullaligned.mrc")
			else:
				shutil.copyfile(self.tmp_location + "/mc_ready_2x_SumCorr.mrc", self.outfolder + "/" + filecorename + "_fullaligned.mrc")
	
		os.chdir(old_path)
		
		if not os.path.exists(self.outfolder + "/dosef_quick"):
			os.makedirs(self.outfolder + "/dosef_quick")
		
		if self.binning == 1:
			shutil.copyfile(self.tmp_location + "/mc_ready_Log.txt", self.outfolder + "/dosef_quick/" + filecorename + "_ready_Log.txt")
			shutil.copyfile(self.tmp_location + "/dosef_quick/mc_ready_CorrFFT.mrc", self.outfolder + "/dosef_quick/" + filecorename + "_ready_CorrFFT.mrc")
			shutil.copyfile(self.tmp_location + "/dosef_quick/mc_ready_CorrSum.mrc", self.outfolder + "/dosef_quick/" + filecorename + "_ready_CorrSum.mrc")
			shutil.copyfile(self.tmp_location + "/dosef_quick/mc_ready_RawFFT.mrc", self.outfolder + "/dosef_quick/" + filecorename + "_ready_RawFFT.mrc")
			
			try:
				os.remove(self.tmp_location + "/mc_ready_SumCorr.mrc")
			except:
				pass
				
			try:
				os.remove(self.tmp_location + "/mc_ready_Log.txt")
			except:
				pass
				
			try:
				os.remove(self.tmp_location + "/dosef_quick/mc_ready_CorrFFT.mrc")
			except:
				pass
				
			try:
				os.remove(self.tmp_location + "/dosef_quick/mc_ready_CorrSum.mrc")
			except:
				pass
				
			try:
				os.remove(self.tmp_location + "/dosef_quick/mc_ready_RawFFT.mrc")
			except:
				pass
				
				
		else:
			shutil.copyfile(self.tmp_location + "/mc_ready_2x_Log.txt", self.outfolder + "/dosef_quick/" + filecorename + "_ready_Log.txt")
			shutil.copyfile(self.tmp_location + "/dosef_quick/mc_ready_2x_CorrFFT.mrc", self.outfolder + "/dosef_quick/" + filecorename + "_ready_CorrFFT.mrc")
			shutil.copyfile(self.tmp_location + "/dosef_quick/mc_ready_2x_CorrSum.mrc", self.outfolder + "/dosef_quick/" + filecorename + "_ready_CorrSum.mrc")
			shutil.copyfile(self.tmp_location + "/dosef_quick/mc_ready_2x_RawFFT.mrc", self.outfolder + "/dosef_quick/" + filecorename + "_ready_RawFFT.mrc")
			
			try:
				os.remove(self.tmp_location + "/mc_ready_2x_SumCorr.mrc")
			except:
				pass
				
			try:
				os.remove(self.tmp_location + "/mc_ready_2x_Log.txt")
			except:
				pass
				
			try:
				os.remove(self.tmp_location + "/dosef_quick/mc_ready_2x_CorrFFT.mrc")
			except:
				pass
				
			try:
				os.remove(self.tmp_location + "/dosef_quick/mc_ready_2x_CorrSum.mrc")
			except:
				pass
				
			try:
				os.remove(self.tmp_location + "/dosef_quick/mc_ready_2x_RawFFT.mrc")
			except:
				pass
		
		self.convert_mrc_to_png(filename)
		self.generateDriftPlot(filename)
				
		self.lock_compute.release()
				
		print "motion_correction done for", filename
		
	def setFirstFrame(self, rhs):
		self.first_frame = rhs
		
	def setLastFrame(self, rhs):
		self.last_frame = rhs
		
	def setFOD(self, rhs):
		self.fod = rhs

	def setWaittime(self, rhs):
		self.waittime = rhs
		
	def setBinning(self, rhs):
		self.binning = rhs
		
	def setBFactor(self, rhs):
		self.bfac = rhs
		
	def setMode(self, rhs):
		self.mode = rhs
		
		
	def convert_mrc_to_png(self, filename):
		old_path = os.getcwd()
		os.chdir(self.outfolder + "/dosef_quick")
		
		corename = self.getFileCoreName(filename)
		
		os.system("e2proc2d.py " + corename + "_ready_CorrFFT.mrc" + " " + corename + "_ready_CorrFFT.png" )
		os.system("convert " + corename + "_ready_CorrFFT" + ".png " + corename + "_ready_CorrFFT" + ".gif" )
		
		os.system("e2proc2d.py " + corename + "_ready_RawFFT.mrc" + " " + corename + "_ready_RawFFT.png" )
		os.system("convert " + corename + "_ready_RawFFT" + ".png " + corename + "_ready_RawFFT" + ".gif" )
		
		os.system("e2proc2d.py " + corename + "_ready_CorrSum.mrc" + " " + corename + "_ready_CorrSum.png" )
		os.system("convert " + corename + "_ready_CorrSum" + ".png " + corename + "_ready_CorrSum" + ".gif" )
		
		os.chdir(old_path)
		
	
