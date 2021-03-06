/*
 * TestCommand1Impl.cpp
 *
 *  Created on: Mar 28, 2014
 *      Author: tcanham
 */

#include <Autocoders/test/serialize_user/ExampleComponentImpl.hpp>
#include <Fw/Types/EightyCharString.hpp>
#include <cstdio>

#if FW_OBJECT_NAMES == 1
ExampleComponentImpl::ExampleComponentImpl(const char* name) :  ExampleComponents::ExampleComponentComponentBase(name)
#else
ExampleComponentImpl::ExampleComponentImpl() :  ExampleComponents::ExampleComponentComponentBase()
#endif
{
}

void ExampleComponentImpl::init(NATIVE_INT_TYPE queueDepth) {
    ExampleComponents::ExampleComponentComponentBase::init(queueDepth);
}

ExampleComponentImpl::~ExampleComponentImpl() {
}

void ExampleComponentImpl::exampleInput_handler(NATIVE_INT_TYPE portNum, I32 arg1, ANameSpace::UserSerializer arg2) {

    Fw::EightyCharString str;
    arg2.toString(str);
    printf("ARG: %s\n",str.toChar());
}
