
#ifndef RTT_ESTIMATOR_H
#define RTT_ESTIMATOR_H

#include "protocol_constants.h"
#include "../drivers/cc2520.h"
#include "../drivers/timer.h"
#include "../drivers/BarraLed.h"

#ifdef _BOARD_STM32VLDISCOVERY

class RttEstimator
{
public:
    /**
     * Constructor
     * \param hopCount the node's hopCount in the overall system
     * \param transceiver pointer to node's transceiver object
     * \param timer pointer to node's VHT
     */    
    RttEstimator(char hopCount, Cc2520& transceiver, Timer& timer);   
    
    /**
     * Used by nodes to measure the distance between them and the previous ones (in hopCoun terms)
     * \param frameStart absolute time that triggers the rtt measurement process
     * \return a pair last hop propagation delay, and cumulated propagation delay
     */
    std::pair<int, int> rttClient(unsigned long long frameStart);
    
    /**
     * Used by nodes to serve a rtt request coming from the next ones (in hopCoun terms)
     * \param frameStart absolute time that triggers the rtt measurement process
     * \param cumulatedPropagationDelay rtt delay from this node and the root node
     */    
    void rttServer(unsigned long long frameStart, int cumulatedPropagationDelay);
        
private:
    
    char hopCount;
    Cc2520& transceiver;
    Timer& timer;   
};

#endif //_BOARD_STM32VLDISCOVERY

#endif //RTT_ESTIMATOR_H