/*
 * TiR5Task.cpp
 *
 *  Created on: Apr 8, 2019
 *      Author: tcanham
 */

#include <Os/Task.hpp>

namespace Os {


    STATIC bool s_inISR = false;

    bool Task::isISR(void) {
        return s_inISR;
    }

    void Task::setIsISR(bool is) {
        s_inISR = is;
    }

}
