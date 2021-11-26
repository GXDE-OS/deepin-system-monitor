/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     xuezifan<xuezifan@uniontech.com>
*
* Maintainer: xuezifan<xuezifan@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//self
#include "service/service_manager.h"
#include "application.h"
#include "dbus/dbus_common.h"
#include "dbus/dbus_properties_interface.h"
#include "dbus/environment_file.h"
#include "dbus/systemd1_manager_interface.h"
#include "dbus/systemd1_service_interface.h"
#include "dbus/systemd1_unit_interface.h"
#include "dbus/unit_file_info.h"
#include "dbus/unit_info.h"
#include "service/system_service_entry.h"
#include "service/service_manager_worker.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

/***************************************STUB begin*********************************************/


/***************************************STUB end**********************************************/

class UT_ServiceManager : public ::testing::Test
{
public:
    UT_ServiceManager() : m_tester(nullptr) , m_tester1(nullptr) {}

public:
    virtual void SetUp()
    {

        m_tester = new ServiceManager();
        m_tester1 = new CustomTimer(m_tester,nullptr);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        if (m_tester1) {
            delete m_tester1;
            m_tester = nullptr;
        }
    }

protected:
    ServiceManager *m_tester;
    CustomTimer *m_tester1;

};

TEST_F(UT_ServiceManager, initTest)
{

}

TEST_F(UT_ServiceManager, test_Customer_01)
{
    QString path;
    m_tester1->start(path);
}

TEST_F(UT_ServiceManager, test_updateServiceList_01)
{
    m_tester->beginUpdateList();
}

TEST_F(UT_ServiceManager, test_normalizeServiceID_01)
{
    m_tester->normalizeServiceId("id","param");
    EXPECT_EQ(m_tester->normalizeServiceId("id").isEmpty(),false);
}

TEST_F(UT_ServiceManager, test_startService_01)
{
    m_tester->startService("");
}

TEST_F(UT_ServiceManager, test_stopService_01)
{
    m_tester->stopService("");
}

TEST_F(UT_ServiceManager, test_restartService_01)
{
    m_tester->restartService("");
}

TEST_F(UT_ServiceManager, test_setServiceStartupMode_01)
{
    //m_tester->setServiceStartupMode("",false);
}

TEST_F(UT_ServiceManager, test_updateServiceEntry_01)
{
    m_tester->updateServiceEntry("");
}
