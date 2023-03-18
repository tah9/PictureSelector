// IMyAidlInterface.aidl
package com.school.demo2_23;

import com.school.demo2_23.PcPathBean;

// Declare any non-default types here with import statements

interface IMyAidlInterface {
    /**
     * Demonstrates some basic types that you can use as parameters
     * and return values in AIDL.
     */
    void sendMessage(in PcPathBean pic);
}