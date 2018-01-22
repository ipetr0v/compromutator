#include "gtest/gtest.h"

#include "ExampleDependencyGraph.hpp"
#include "../src/network/DependencyGraph.hpp"
#include "../src/flow_predictor/FlowPredictor.hpp"

class StatsManagerTest : public ::testing::Test
                       , public SimpleTwoSwitchGraph
{
protected:
    virtual void SetUp() {
        initGraph();
        xid_generator = std::make_shared<RequestIdGenerator>();
        stats_manager = std::make_shared<StatsManager>(xid_generator);
    }

    virtual void TearDown() {
        stats_manager.reset();
        xid_generator.reset();
        destroyGraph();
    }

    std::shared_ptr<RequestIdGenerator> xid_generator;
    std::shared_ptr<StatsManager> stats_manager;
};

TEST_F(StatsManagerTest, BucketTest)
{
    StatsBucket bucket(xid_generator);
    auto stats = std::make_shared<RuleStats>(stats_manager->frontTime(), rule1);
    bucket.addStats(stats);
    EXPECT_TRUE(bucket.queriesExist());
    auto requests = bucket.getRequests();
    ASSERT_EQ(1u, requests.data.size());
    bucket.passRequest(requests.data[0]);
    EXPECT_TRUE(bucket.isFull());
    auto complete_stats = bucket.popStatsList();
    ASSERT_EQ(1u, complete_stats.size());
}

TEST_F(StatsManagerTest, BasicTest)
{
    uint64_t packet_count = 100u;
    PathId path_id = 1u;

    stats_manager->requestRule(rule1);
    stats_manager->requestPath(
        path_id, DomainPathDescriptor(nullptr), rule1, rule2
    );
    stats_manager->discardPathRequest(path_id);

    auto requests = stats_manager->getNewRequests();
    ASSERT_EQ(1u, requests.data.size());
    auto rule_request = RuleRequest::pointerCast(requests.data[0]);
    ASSERT_NE(nullptr, rule_request);
    EXPECT_EQ(rule1->id(), rule_request->rule->id());
    EXPECT_LT(rule_request->time.value, stats_manager->frontTime().value);
    rule_request->stats.packet_count = packet_count;
    stats_manager->passRequest(rule_request);
    auto stats_list = stats_manager->popStatsList();
    ASSERT_EQ(1u, stats_list.size());
    auto rule_stats = std::dynamic_pointer_cast<RuleStats>(*stats_list.begin());
    ASSERT_NE(nullptr, rule_stats);
    EXPECT_EQ(rule1->id(), rule_stats->rule->id());
    EXPECT_EQ(packet_count, rule_stats->stats_fields.packet_count);
}

class FlowPredictorTest : public ::testing::Test
                        , public SimpleTwoSwitchGraph
{
protected:
    virtual void SetUp() {
        initGraph();
        xid_generator = std::make_shared<RequestIdGenerator>();
        flow_predictor = std::make_shared<FlowPredictor>(dependency_graph,
                                                         xid_generator);
    }

    virtual void TearDown() {
        flow_predictor.reset();
        xid_generator.reset();
        destroyGraph();
    }

    std::shared_ptr<RequestIdGenerator> xid_generator;
    std::shared_ptr<FlowPredictor> flow_predictor;
};

TEST_F(FlowPredictorTest, CreationTest)
{
    installSpecialRules();

    flow_predictor->updateEdges(drop_diff);
    flow_predictor->updateEdges(controller_diff);

    flow_predictor->updateEdges(p11_source_diff);
    flow_predictor->updateEdges(p11_sink_diff);
    flow_predictor->updateEdges(p12_source_diff);
    flow_predictor->updateEdges(p12_sink_diff);

    flow_predictor->updateEdges(p21_source_diff);
    flow_predictor->updateEdges(p21_sink_diff);
    flow_predictor->updateEdges(p22_source_diff);
    flow_predictor->updateEdges(p22_sink_diff);

    auto instruction = flow_predictor->getInstruction();
    EXPECT_TRUE(instruction.interceptor_diff.empty());

    // TODO: implement other tests
}
