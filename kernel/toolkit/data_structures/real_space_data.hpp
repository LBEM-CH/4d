/* 
 * @license GNU Public License
 * @author Nikhil Biyani (nikhilbiyani@gmail.com)
 * 
 */


#ifndef REAL_SPACE_DATA_HPP
#define	REAL_SPACE_DATA_HPP

namespace tdx
{
    namespace data
    {
        /**
         * A class to sore real space data in a double array format.
         */
        class RealSpaceData
        {
        public:
            
            /**
             * Default constructor setting the data to null
             */
            RealSpaceData();
            
            /**
             * Constructor initializing the data with given size
             * @param nx (Default=1)
             * @param ny (Default=1)
             * @param nz (Default=1)
             */
            RealSpaceData(int nx, int ny, int nz);
            
            /**
             * Copy constructor
             */
            RealSpaceData(const RealSpaceData& other);
            
            /**
             * Destructor
             */
            ~RealSpaceData();
            
            RealSpaceData& operator=(const RealSpaceData& rhs);
            
            /**
             * Addition operator definition
             */
            RealSpaceData operator+(const RealSpaceData& rhs) const;
            
            /**
             * Multiplication of all the densities by a factor
             */
            RealSpaceData operator*(double factor) const;
            
            /**
             * Resets the data with the other data
             */
            void reset(const RealSpaceData& other);
            
            /**
             * Clears the data
             */
            void clear();
            
            /**
             * Sets the data from the fftw transformed real data
             */
            void set_from_fftw(double* fftw_real);
            
            /**
             * Returns the FFTW allocated data
             */
            double* get_data_for_fftw();
            
            /**
             * Fetches the value at the given index
             * @param x
             * @param y
             * @param z
             * @return value
             */
            double get_value_at(int x, int y, int z) const ;
            
             /**
             * Fetches the value at the given index
             * @param id
             * @return value
             */
            double get_value_at(int id) const ;
            
            /**
             * Assigns the value to the given index
             * @param x
             * @param y
             * @param z
             * @param value
             */
            void set_value_at(int x, int y, int z, double value);
            
            /**
             * Assigns the value to the given index
             * @param id
             * @param value
             */
            void set_value_at(int id, double value);
            
            /**
             * x-size of the data
             * @return x-size of the data
             */
            size_t nx() const;
            
            /**
             * y-size of the data
             * @return y-size of the data
             */
            size_t ny() const;
            
            /**
             * z-size of the data
             * @return z-size of the data
             */
            size_t nz() const;
            
            /**
             * Evaluates minimum density value
             * @return minimum density value
             */
            double min() const;
            
            /**
             * Evaluates maximum density value
             * @return maximum density value
             */
            double max() const;
            
            /**
             * Evaluates mean density value
             * @return mean density value
             */
            double mean() const;
            
            /**
             * The the current size of the array
             * @return size
             */
            size_t size() const ;
            
             /**
             * Fetches the memory location of the x, y, z
             * @param x
             * @param y
             * @param z
             * @return 
             */
            size_t memory_id(int x, int y, int z) const ;
            
            /**
             * Returns the sum of the squared densities of all voxels
             */
            double squared_sum() const;
            
            /**
             * Merges the data from other real spaced data. The input data's 
             * center will be placed at the location x, y, z in the current data
             * @param to_be_merged : Real space data to be merged
             * @param x - Location x where the input map's x-center will be merged
             * @param y - Location y where the input map's y-center will be merged
             * @param z - Location z where the input map's z-center will be merged
             */
            void merge_data(const RealSpaceData& to_be_merged, int x, int y, int z);
            
            /**
             * Sorts the real space data and returns the sorted id's
             * @return the sorted id's
             */
            int* density_sorted_ids();
            
            /**
             * Sorts the real space data according to density values
             * and returns the values
             * @return 
             */
            double* density_sorted_values();
            
            
            /**
             * Returns a mask of density slab in the vertical direction (z-axis) with the
             * fractional height.
             * @param height: height in fraction of z height
             * @param centered: Is the density centered along z-axis?
             * @return mask
             */
            RealSpaceData vertical_slab_mask(double height, bool centered);
            
            /**
             * Returns a mask with soft boundaries.
             * All densities above the threshold_higher will be given a value of
             * 1.0. All densities below this value will be given a value 0f 0.0.
             * Rest densities will be properly scaled.
             * @param threshold_higher
             * @param therehsold_lower
             * @return RealSpaceData
             */
            RealSpaceData threshold_soft_mask(double threshold_higher, double therehsold_lower) const;
            
            /**
             * Applies a density slab in the vertical direction (z-axis) with the
             * fractional height. The new densities are changed only with the 
             * fraction provided. fraction = 1.0 will completely change the map
             * @param height: height in fraction of z height
             * @param fraction: fraction, by which the densities are changed
             * @param centered: Is the density centered along z-axis?
             */
            void vertical_slab(double height, double fraction, bool centered);
            
            /**
             * Apply a density threshold. fraction = 1.0 will completely remove
             * densities below the limit
             * @param limit
             * @param fraction
             */
            void threshold(double limit = 0.0, double fraction=1.0);
            
            /**
             * Scale the densities such that the values of densities are between
             * min and max values provided.
             * @param min
             * @param max
             */
            void scale(double min, double max);
            
            /**
             * Generate a grey scaled volume. The densities are scaled form 0 to 255
             */
            void grey_scale();
            
            /**
             * Thresholds the volume, and returns a binary mask.
             * @param threshold - value of density threshold
             * @return mask
             */
            RealSpaceData threshold_mask(double threshold) const;
            
            /**
             * Thresholds the volume and returns a mask with all values below the
             * the threshold set to 1.0 and above set to 0.0
             */
            RealSpaceData threshold_below_mask(double thershold) const;
            
            /**
             * Dilates a binary (threshold 0.5) volume by radius pixels.
             * @param radius - radius of pixels around each px>0.5 to become 1
             * @return dilated_volume
             */
            RealSpaceData dilate(double radius) const;
            
            /**
             * Mask the real space data with a mask. It will fractionally delete all densities 
             * where the mask is <=0. Fraction=0.0 will not change at all, and fraction = 1.0
             * will do a complete masking.
             * @param mask
             * @param fraction
             */
            void apply_mask(const RealSpaceData& mask, double fraction = 1.0);
            
            /**
             * Multiplies the densities of the mask provided with the current data,
             * the size of the mask and current data should be the same.
             * @param mask
             */
            void multiply_mask(const RealSpaceData& mask);
            
            /**
             * Mask the real space data with a mask. It will fractionally delete all densities 
             * where the mask is <=0. Fraction=0.0 will not change at all, and fraction = 1.0
             * will do a complete masking.
             * @param mask
             * @param fraction
             * @return mask applied real space data
             * @see apply_mask
             */
            RealSpaceData mask_applied_data(const RealSpaceData& mask, double fraction = 1.0) const;
            
        private:
            
            /**
             * Generates a copy of elements required and returns it.
             * @param start - starting index of the element required
             * @param end - end index of the element required
             * @return copy of data
             */
            double* get_data_copy(int start, int end) const;
            
            /**
             * Checks if the indices are correct with the given data!
             * Should bypass the seg fault 11!
             * @param x
             * @param y
             * @param z
             * @return True for correct, false otherwise
             */
            bool indices_in_limit(int x, int y, int z) const ;
            
            /**
             * Data variable as a pointer to the double array
             * Data is stored with origin at the lower left corner looking 
             * down on the volume.
             */
            double* _data;
            
            /**
             * The size of the current data variable
             */
            size_t _nx, _ny, _nz;
            
            
        }; //class RealSpaceData
        
    } // namespace data_structures
    
} // namespace volume_processing_2dx

#endif	/* REAL_SPACE_DATA_HPP */

