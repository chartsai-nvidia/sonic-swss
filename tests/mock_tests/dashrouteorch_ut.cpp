#define private public
#include "directory.h"
#undef private
#define protected public
#include "orch.h"
#undef protected
#include "ut_helper.h"
#include "mock_orchagent_main.h"
#include "mock_sai_api.h"
#include "mock_dash_orch_test.h"
#include "dash_api/appliance.pb.h"
#include "dash_api/route_type.pb.h"
#include "dash_api/route.pb.h"
#include "dash_api/eni.pb.h"
#include "dash_api/qos.pb.h"
#include "dash_api/eni_route.pb.h"
#include "crmorch.h"

EXTERN_MOCK_FNS
namespace dashrouteorch_test
{
    DEFINE_SAI_API_MOCK(dash_outbound_routing, outbound_routing);
    using namespace mock_orch_test;
    using ::testing::InSequence;
    using ::testing::Return;
    using ::testing::DoAll;
    using ::testing::SetArrayArgument;

    class DashRouteOrchTest : public MockDashOrchTest
    {
    protected:
        int GetCrmUsedCount(CrmResourceType type)
        {
            return gCrmOrch->m_resourcesMap.at(type).countersMap["STATS"].usedCounter;
        }

        void PostSetUp()
        {
            CreateApplianceEntry();
            CreateVnet();
        }

        void ApplySaiMock()
        {
            INIT_SAI_API_MOCK(dash_outbound_routing);
            MockSaiApis();
        }

        void PreTearDown()
        {
            RestoreSaiApis();
            DEINIT_SAI_API_MOCK(dash_outbound_routing);
        }
    };

    TEST_F(DashRouteOrchTest, RouteWithMissingTunnelNotAdded)
    {
        {
            InSequence seq;
            EXPECT_CALL(*mock_sai_dash_outbound_routing_api, create_outbound_routing_entries).Times(0);
            EXPECT_CALL(*mock_sai_dash_outbound_routing_api, create_outbound_routing_entries).Times(1);
        }
        AddOutboundRoutingGroup();
        AddOutboundRoutingEntry(false);
        
        AddTunnel();
        AddOutboundRoutingEntry();
    }

    TEST_F(DashRouteOrchTest, RemoveNonexistOutboundRoutingDoesNotDecrementCrm)
    {
        AddOutboundRoutingGroup();
        AddTunnel();

        int baselineUsed = GetCrmUsedCount(CrmResourceType::CRM_DASH_IPV4_OUTBOUND_ROUTING);
        AddOutboundRoutingEntry();
        EXPECT_EQ(GetCrmUsedCount(CrmResourceType::CRM_DASH_IPV4_OUTBOUND_ROUTING), baselineUsed + 1);

        RemoveOutboundRoutingEntry();
        EXPECT_EQ(GetCrmUsedCount(CrmResourceType::CRM_DASH_IPV4_OUTBOUND_ROUTING), baselineUsed);

        // Remove non-existent outbound routing entry should return SAI_STATUS_ITEM_NOT_FOUND and not decrement the CRM used count
        std::vector<sai_status_t> exp_status = {SAI_STATUS_ITEM_NOT_FOUND};
        EXPECT_CALL(*mock_sai_dash_outbound_routing_api, remove_outbound_routing_entries)
            .Times(1).WillOnce(DoAll(SetArrayArgument<3>(exp_status.begin(), exp_status.end()), Return(SAI_STATUS_SUCCESS)));
        RemoveOutboundRoutingEntry();
        EXPECT_EQ(GetCrmUsedCount(CrmResourceType::CRM_DASH_IPV4_OUTBOUND_ROUTING), baselineUsed);
    }
}