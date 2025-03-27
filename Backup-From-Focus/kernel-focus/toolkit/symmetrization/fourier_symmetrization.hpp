/* 
 * @license GNU Public License
 * @author Nikhil Biyani (nikhilbiyani@gmail.com)
 * 
 */

#ifndef FOURIERSYMMETRIZATION_HPP
#define	FOURIERSYMMETRIZATION_HPP

#include <math.h>

#include "symmetry2dx.hpp"
#include "../data_structures/reflection_data.hpp"


namespace tdx
{
    namespace symmetrization
    {
        namespace fourier_symmetrization
        {
            /**
             * A Fourier space symmetrization function.
             * Takes in a map with miller indices mapped to the diffraction spot values
             * and changes the map in-place to generate symmetrized map.
             * @param[in/out] fourier_data
             * @param symmetry
             */
            void symmetrize(tdx::data::ReflectionData& fourier_data, 
                            const tdx::symmetrization::Symmetry2dx& symmetry);
            
        }
        
    }
    
}


#endif	/* FOURIERSYMMETRIZATION_HPP */

