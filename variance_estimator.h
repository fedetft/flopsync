/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef VARIANCE_ESTIMATOR_H
#define VARIANCE_ESTIMATOR_H

class VarianceEstimator
{
public:
    static const int numSamples=60;
    
    static const int fp=64; //Fixed point, log2(fp) bits are the decimal part
    
    VarianceEstimator() : sum(0), squareSum(0), var(w), count(0) {}
    
    /**
     * Set sync window to a given value
     * \param current variance
     */
    void setWindow(int v)
    {
        sum=squareSum=count=0; //Restart computing the variance from now
        var=v;
    }

    void update(int e)
    {
        sum+=e*fp;
        squareSum+=e*e*fp;
        if(++count<numSamples) return;
        count=0;
        sum/=numSamples;
        var=squareSum/numSamples-sum*sum/fp;
        var*=3; //Set the window size to three sigma
        var/=fp;
        sum=0;
        squareSum=0;
    }
    
    int window() const { return var; }
    
private:
    int sum, squareSum, var;
    int count;
};

#endif //VARIANCE_ESTIMATOR_H
