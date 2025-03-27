from pylab import *
import os
from numpy import array
from EMAN2 import *
from sparx import *
import matplotlib.image as mpimg
import matplotlib.pyplot as plt

if __name__ == "__main__":
	
	if len(sys.argv) != 3:
		if len(sys.argv) != 4:
			sys.exit("Usage: drift_plotter.py <Data-File> <PDF-output> [<background CCmap image>]")

	infile = sys.argv[1]
	outfile = sys.argv[2]
	width = 0
	if len(sys.argv) == 4:
		CCmap_file = sys.argv[3]
		CCmap = get_image(CCmap_file)
		print ( "Read CCmap " )
		s = info(CCmap)
		width = s[4]
		CCmapy = mirror(CCmap,"y")
		CCmapy.write_image('tmp.png')
		map = mpimg.imread('tmp.png')
			
	data_file = open(infile)
	
	# Skip header 
	for i in range(0,13):
		data_file.readline()
		
	x=[]; y=[];
	xstart=[]; ystart=[];
	xend=[]; yend=[];
	peak=[]
	
	for l in data_file:
		data_split = l.split()
		x.append(float(data_split[2]))
		y.append(float(data_split[3]))
		peak.append(float(data_split[4]))
		xstart.append(float(data_split[5]))
		ystart.append(float(data_split[6]))
		xend.append(float(data_split[7]))
		yend.append(float(data_split[8]))
	
	start_pos_x = np.array(x)+np.array(xstart)
	start_pos_y = np.array(y)+np.array(ystart)
	length_x = np.array(xend) - np.array(xstart)
	length_y = np.array(yend) - np.array(ystart)
	
	figure()
	ax = subplot(1,1,1)
	ax.set_aspect('equal')
	if len(sys.argv) == 4:
		plt.imshow(map, cmap = cm.summer, interpolation='nearest', origin='upper')
		plt.gca().invert_yaxis()
		

	ax.plot(np.array(x)+np.array(xstart),np.array(y)+np.array(ystart),'.',markersize=0.5,linewidth=0.5,markeredgecolor='white',markerfacecolor='white')

        plt.title('Drift profile of lattice nodes (10x exaggerated)')
	if width != 0:
		plt.xlim(1,width)
		plt.ylim(1,width)
	
	factor = 10
	for i in range(len(start_pos_x)):
		if peak[i] > 4:
			ax.arrow(start_pos_x[i], start_pos_y[i], factor*length_x[i], factor*length_y[i], linewidth=0.3, width=0.001, head_width=10, head_length=15, fc='k', ec='k')
	# head_width=20, head_length=30, 	

	plt.savefig(outfile)

