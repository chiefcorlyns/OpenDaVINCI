/**
 * OpenDaVINCI - Portable middleware for distributed components.
 * Copyright (C) 2008 - 2015 Christian Berger, Bernhard Rumpe
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef CONTEXT_RUNTIMECONTROLTESTSUITESETUPNOTCALLED_H_
#define CONTEXT_RUNTIMECONTROLTESTSUITESETUPNOTCALLED_H_

#include <cmath>                        // for sqrt
#include <iostream>                     // for operator<<, basic_ostream, etc
#include <string>                       // for string, operator<<, etc

#include "cxxtest/TestSuite.h"          // for TS_ASSERT, TestSuite

#include "opendavinci/core/opendavinci.h"
#include "opendavinci/context/base/DirectInterface.h"  // for DirectInterface
#include "opendavinci/context/base/RuntimeControl.h"  // for RuntimeControl, etc
#include "opendavinci/context/base/RuntimeEnvironment.h"  // for RuntimeEnvironment
#include "opendavinci/context/base/SystemFeedbackComponent.h"
#include "opendavinci/core/base/KeyValueConfiguration.h"  // for KeyValueConfiguration
#include "opendavinci/core/base/Thread.h"
#include "opendavinci/core/base/module/TimeTriggeredConferenceClientModule.h"
#include "opendavinci/generated/coredata/dmcp/ModuleExitCodeMessage.h"

namespace odcontext { namespace base { class SendContainerToSystemsUnderTest; } }
namespace core { namespace wrapper { class Time; } }

using namespace std;
using namespace core::base;
using namespace core::base::module;
using namespace odcontext::base;

class RuntimeControlTestModule : public TimeTriggeredConferenceClientModule {
    public:
        RuntimeControlTestModule(const int32_t &argc, char **argv) :
            TimeTriggeredConferenceClientModule(argc, argv, "RuntimeControlTestModule"),
            m_cycleCounter(0),
            m_config() {}

        virtual void setUp() {}

        virtual void tearDown() {}

        virtual coredata::dmcp::ModuleExitCodeMessage::ModuleExitCode body() {
            m_config = getKeyValueConfiguration();

            m_config.getValue<string>("runtimecontroltestmodule.key1");

            while (getModuleStateAndWaitForRemainingTimeInTimeslice() == coredata::dmcp::ModuleStateMessage::RUNNING) {
                m_cycleCounter++;
            }

            return coredata::dmcp::ModuleExitCodeMessage::OKAY;
        }

        KeyValueConfiguration getConfiguration() {
            return m_config;
        }

        uint32_t getCycleCounter() const {
            return m_cycleCounter;
        }

    private:
        uint32_t m_cycleCounter;
        KeyValueConfiguration m_config;
};

class RuntimeControlTestDummySystemPart : public SystemFeedbackComponent {
    public:
        RuntimeControlTestDummySystemPart() :
            m_freq(1),
            m_setupCalled(false),
            m_tearDownCalled(false) {}

        float getFrequency() const {
            return m_freq;
        }

        virtual void setup() {
            m_setupCalled = true;
        }

        virtual void tearDown() {
            m_tearDownCalled = true;
        }

        virtual void step(const core::wrapper::Time &t, SendContainerToSystemsUnderTest &/*sender*/) {
            clog << "Call to RuntimeControlTestDummySystemPart for t = " << t.getSeconds() << "." << t.getPartialMicroseconds() << ", containing " << getFIFO().getSize() << " containers." << endl;
        }

        const float m_freq;
        bool m_setupCalled;
        bool m_tearDownCalled;
};

class RuntimeControlTest : public CxxTest::TestSuite {
    public:

        void testRuntimeControlRegularRunSetupNotCalled() {
            // Setup configuration.
            stringstream sstr;
            sstr << "runtimecontroltestmodule.key1 = value1" << endl
                 << "runtimecontroltestmodule:241280.key2 = value2" << endl
                 << "othermodule.key2 = value2" << endl;

            DirectInterface di("225.0.0.100", 100, sstr.str());
            RuntimeControl sc(di);

            // Setup application.
            string argv0("runtimecontroltestmodule");
            string argv1("--cid=100");
            int32_t argc = 2;
            char **argv;
            argv = new char*[2];
            argv[0] = const_cast<char*>(argv0.c_str());
            argv[1] = const_cast<char*>(argv1.c_str());

            RuntimeControlTestModule rctm(argc, argv);

            RuntimeControlTestDummySystemPart rctdsc;

            RuntimeEnvironment rte;
            rte.add(rctm);
            rte.add(rctdsc);

            TS_ASSERT(rte.getListOfTimeTriggeredConferenceClientModules().size() == 1);
            TS_ASSERT(rte.getListOfSystemFeedbackComponents().size() == 1);

            TS_ASSERT(!rctdsc.m_setupCalled);
            TS_ASSERT(!rctdsc.m_tearDownCalled);

            // Run application under supervision of RuntimeControl for ten cycles.
            TS_ASSERT(sc.run(rte, 10) == RuntimeControl::SETUP_NOT_CALLED);

            TS_ASSERT(!rctdsc.m_setupCalled);
            TS_ASSERT(!rctdsc.m_tearDownCalled);

            // Try to add more modules after executing (should fail).
            rte.add(rctm);
            rte.add(rctdsc);

            // Adding still possible since call to run failed!
            TS_ASSERT(rte.getListOfTimeTriggeredConferenceClientModules().size() == 2);
            TS_ASSERT(rte.getListOfSystemFeedbackComponents().size() == 2);

            sc.tearDown();
        }
};

#endif /*CONTEXT_RUNTIMECONTROLTESTSUITESETUPNOTCALLED_H_*/
