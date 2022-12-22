#include "common_test.h"
#include <interface/core_test_interface.h>
#include <core/publisher.h>
#include <core/subscriber.h>
#include <dronecan_msgs.h>
#include <gtest/gtest.h>
#include <core/service_server.h>
#include <core/service_client.h>

using namespace CubeFramework;

namespace StaticCoreTest {

///////////// TESTS for Subscriber and Publisher //////////////
static bool called_handle_node_status = false;
static uavcan_protocol_NodeStatus sent_msg;
static CanardRxTransfer last_transfer;
static void handle_node_status(const CanardRxTransfer &transfer, const uavcan_protocol_NodeStatus &msg) {
    called_handle_node_status = true;
    last_transfer = transfer;
    // check if message is correct
    ASSERT_EQ(memcmp(&msg, &sent_msg, sizeof(uavcan_protocol_NodeStatus)), 0);
}

static CF_CORE_TEST_INTERFACE_DEFINE(0);
static CF_CORE_TEST_INTERFACE_DEFINE(1);

TEST(StaticCoreTest, test_publish_subscribe) {
    // create publisher for message uavcan_protocol_NodeStatus on interface CoreTestInterface
    // with callback function handle_node_status
    CF_PUBLISHER(CF_CORE_TEST_INTERFACE(0), node_status_pub0, uavcan_protocol_NodeStatus);
    // create publisher for message uavcan_protocol_NodeStatus on different interface instance CoreTestInterface
    // with callback function handle_node_status
    CF_PUBLISHER(CF_CORE_TEST_INTERFACE(1), node_status_pub1, uavcan_protocol_NodeStatus);

    // create subscriber for message uavcan_protocol_NodeStatus on interface CoreTestInterface
    CF_SUBSCRIBE_MSG(node_status_sub0, uavcan_protocol_NodeStatus, handle_node_status);
    // create subscriber for message uavcan_protocol_NodeStatus on different interface instance CoreTestInterface
    // with callback function handle_node_status
    CF_SUBSCRIBE_MSG_INDEXED(1, node_status_sub1, uavcan_protocol_NodeStatus, handle_node_status);

    // set node id for interfaces
    CF_CORE_TEST_INTERFACE(0).set_node_id(1);
    CF_CORE_TEST_INTERFACE(1).set_node_id(2);

    // create message
    uavcan_protocol_NodeStatus msg {};
    msg.uptime_sec = 1;
    msg.health = 2;
    msg.mode = 3;
    msg.sub_mode = 4;
    msg.vendor_specific_status_code = 5;
    sent_msg = msg;

    called_handle_node_status = false;
    // publish message
    ASSERT_TRUE(node_status_pub0.broadcast(msg));
    // check if message was received
    ASSERT_TRUE(called_handle_node_status);
}

// test multiple subscribers

// default test subscriber without
class TestSubscriber0 {
public:
    TestSubscriber0() {
        CF_BIND(node_status_sub, this);
    }
    void handle_node_status(const CanardRxTransfer &transfer, const uavcan_protocol_NodeStatus &msg);
    static int call_counts;

private:
    CF_SUBSCRIBE_MSG_CLASS(node_status_sub, uavcan_protocol_NodeStatus, TestSubscriber0, &TestSubscriber0::handle_node_status);
};

void TestSubscriber0::handle_node_status(const CanardRxTransfer &transfer, const uavcan_protocol_NodeStatus &msg) {
    called_handle_node_status = true;
    last_transfer = transfer;
    // check if message is correct
    ASSERT_EQ(memcmp(&msg, &sent_msg, sizeof(uavcan_protocol_NodeStatus)), 0);
    call_counts++;
}

int TestSubscriber0::call_counts = 0;

// test subsriber with index 1
class TestSubscriber1 {
public:
    TestSubscriber1() {
        CF_BIND(node_status_sub, this);
    }
    void handle_node_status(const CanardRxTransfer &transfer, const uavcan_protocol_NodeStatus &msg) {
        called_handle_node_status = true;
        last_transfer = transfer;
        // check if message is correct
        ASSERT_EQ(memcmp(&msg, &sent_msg, sizeof(uavcan_protocol_NodeStatus)), 0);
        call_counts++;
    }
    static int call_counts;
private:
    CF_SUBSCRIBE_MSG_CLASS_INDEX(1, node_status_sub, uavcan_protocol_NodeStatus, TestSubscriber1, &TestSubscriber1::handle_node_status);
};

int TestSubscriber1::call_counts = 0;

// Create 5 subscribers and check if all of them are called
TEST(StaticCoreTest, test_multiple_subscribers) {
    // create publisher for message uavcan_protocol_NodeStatus on interface CoreTestInterface
    // with callback function handle_node_status
    CF_PUBLISHER(CF_CORE_TEST_INTERFACE(0), node_status_pub0, uavcan_protocol_NodeStatus);
    // create publisher for message uavcan_protocol_NodeStatus on different interface instance CoreTestInterface
    // with callback function handle_node_status
    CF_PUBLISHER(CF_CORE_TEST_INTERFACE(1), node_status_pub1, uavcan_protocol_NodeStatus);

    TestSubscriber0 test_subscriber0[5] __attribute__((unused)) {};
    TestSubscriber1 test_subscriber1[5] __attribute__((unused)) {};

    // set node id for interfaces
    CF_CORE_TEST_INTERFACE(0).set_node_id(1);
    CF_CORE_TEST_INTERFACE(1).set_node_id(2);

    // create message
    uavcan_protocol_NodeStatus msg {};
    msg.uptime_sec = 1;
    msg.health = 2;
    msg.mode = 3;
    msg.sub_mode = 4;
    msg.vendor_specific_status_code = 5;
    sent_msg = msg;

    // publish message
    ASSERT_TRUE(node_status_pub0.broadcast(msg));
    // check if message was received 5 times
    ASSERT_EQ(TestSubscriber1::call_counts, 5);
    // publish message
    ASSERT_TRUE(node_status_pub1.broadcast(msg));
    // check if message was received 5 times
    ASSERT_EQ(TestSubscriber0::call_counts, 5);
}

//////////// TESTS FOR SERVICE //////////////

// test single server single client
bool handle_get_node_info_response_called = false;
void handle_get_node_info_response(const CanardRxTransfer &transfer, const uavcan_protocol_GetNodeInfoResponse &res) {
    ASSERT_EQ(res.status.uptime_sec, 1);
    ASSERT_EQ(res.status.health, 2);
    ASSERT_EQ(res.status.mode, 3);
    ASSERT_EQ(res.status.sub_mode, 4);
    ASSERT_EQ(res.status.vendor_specific_status_code, 5);
    ASSERT_EQ(res.software_version.major, 1);
    ASSERT_EQ(res.software_version.minor, 2);
    ASSERT_EQ(res.hardware_version.major, 3);
    ASSERT_EQ(res.hardware_version.minor, 4);
    ASSERT_EQ(res.name.len, strlen("helloworld"));
    ASSERT_EQ(memcmp(res.name.data, "helloworld", res.name.len), 0);
    handle_get_node_info_response_called = true;
}

void handle_get_node_info_request0(const CanardRxTransfer &transfer, const uavcan_protocol_GetNodeInfoRequest &req);
CF_CREATE_SERVER(CF_CORE_TEST_INTERFACE(0), get_node_info_server0, uavcan_protocol_GetNodeInfo, handle_get_node_info_request0);
void handle_get_node_info_request1(const CanardRxTransfer &transfer, const uavcan_protocol_GetNodeInfoRequest &req);
CF_CREATE_SERVER_INDEX(CF_CORE_TEST_INTERFACE(1), 1, get_node_info_server1, uavcan_protocol_GetNodeInfo, handle_get_node_info_request1);

void handle_get_node_info_request0(const CanardRxTransfer &transfer, const uavcan_protocol_GetNodeInfoRequest &req) {
    uavcan_protocol_GetNodeInfoResponse res {};
    res.status.uptime_sec = 1;
    res.status.health = 2;
    res.status.mode = 3;
    res.status.sub_mode = 4;
    res.status.vendor_specific_status_code = 5;
    res.software_version.major = 1;
    res.software_version.minor = 2;
    res.hardware_version.major = 3;
    res.hardware_version.minor = 4;
    res.name.len = strlen("helloworld");
    memcpy(res.name.data, "helloworld", res.name.len);
    get_node_info_server0.respond(transfer, res);
}

void handle_get_node_info_request1(const CanardRxTransfer &transfer, const uavcan_protocol_GetNodeInfoRequest &req) {
    uavcan_protocol_GetNodeInfoResponse res {};
    res.status.uptime_sec = 1;
    res.status.health = 2;
    res.status.mode = 3;
    res.status.sub_mode = 4;
    res.status.vendor_specific_status_code = 5;
    res.software_version.major = 1;
    res.software_version.minor = 2;
    res.hardware_version.major = 3;
    res.hardware_version.minor = 4;
    res.name.len = strlen("helloworld");
    memcpy(res.name.data, "helloworld", res.name.len);
    get_node_info_server1.respond(transfer, res);
}

TEST(StaticCoreTest, test_service) {
    // create client for service uavcan_protocol_GetNodeInfo on interface CoreTestInterface
    // with response callback function handle_get_node_info_response
    CF_CREATE_CLIENT(CF_CORE_TEST_INTERFACE(0), get_node_info_client0, uavcan_protocol_GetNodeInfo, handle_get_node_info_response);
    // create client for service uavcan_protocol_GetNodeInfo on different interface instance CoreTestInterface
    // with response callback function handle_get_node_info_response
    CF_CREATE_CLIENT_INDEX(CF_CORE_TEST_INTERFACE(1), 1, get_node_info_client1, uavcan_protocol_GetNodeInfo, handle_get_node_info_response);

    // set node id for interfaces
    CF_CORE_TEST_INTERFACE(0).set_node_id(1);
    CF_CORE_TEST_INTERFACE(1).set_node_id(2);

    // create request
    uavcan_protocol_GetNodeInfoRequest req {};
    handle_get_node_info_response_called = false;
    // send request
    ASSERT_TRUE(get_node_info_client0.request(2, req));
    // check if response was received
    ASSERT_TRUE(handle_get_node_info_response_called);

    handle_get_node_info_response_called = false;
    // send request
    ASSERT_TRUE(get_node_info_client1.request(1, req));
    // check if response was received
    ASSERT_TRUE(handle_get_node_info_response_called);
}

// test single server multiple clients
class TestClient0 {
public:
    TestClient0() {
        CF_BIND(get_node_info_client, this);
    }
    static int call_counts;
    void handle_get_node_info_response(const CanardRxTransfer &transfer, const uavcan_protocol_GetNodeInfoResponse &res) {
        ASSERT_EQ(res.status.uptime_sec, 1);
        ASSERT_EQ(res.status.health, 2);
        ASSERT_EQ(res.status.mode, 3);
        ASSERT_EQ(res.status.sub_mode, 4);
        ASSERT_EQ(res.status.vendor_specific_status_code, 5);
        ASSERT_EQ(res.software_version.major, 1);
        ASSERT_EQ(res.software_version.minor, 2);
        ASSERT_EQ(res.hardware_version.major, 3);
        ASSERT_EQ(res.hardware_version.minor, 4);
        ASSERT_EQ(res.name.len, strlen("helloworld"));
        ASSERT_EQ(memcmp(res.name.data, "helloworld", res.name.len), 0);
        call_counts++;
    }
    CF_CREATE_CLIENT_CLASS(CF_CORE_TEST_INTERFACE(0), // interface name
                           get_node_info_client, // client name
                           uavcan_protocol_GetNodeInfo, // service name
                           TestClient0, // class name
                           &TestClient0::handle_get_node_info_response); // callback function
};

class TestClient1 {
public:
    TestClient1() {
        CF_BIND(get_node_info_client, this);
    }
    static int call_counts;
    void handle_get_node_info_response(const CanardRxTransfer &transfer, const uavcan_protocol_GetNodeInfoResponse &res) {
        ASSERT_EQ(res.status.uptime_sec, 1);
        ASSERT_EQ(res.status.health, 2);
        ASSERT_EQ(res.status.mode, 3);
        ASSERT_EQ(res.status.sub_mode, 4);
        ASSERT_EQ(res.status.vendor_specific_status_code, 5);
        ASSERT_EQ(res.software_version.major, 1);
        ASSERT_EQ(res.software_version.minor, 2);
        ASSERT_EQ(res.hardware_version.major, 3);
        ASSERT_EQ(res.hardware_version.minor, 4);
        ASSERT_EQ(res.name.len, strlen("helloworld"));
        ASSERT_EQ(memcmp(res.name.data, "helloworld", res.name.len), 0);
        call_counts++;
    }
    CF_CREATE_CLIENT_CLASS_INDEX(CF_CORE_TEST_INTERFACE(1), // interface name
                                 1, // interface index
                                 get_node_info_client, // client name
                                 uavcan_protocol_GetNodeInfo, // service name
                                 TestClient1, // class name
                                 &TestClient1::handle_get_node_info_response); // callback function
};

int TestClient0::call_counts = 0;
int TestClient1::call_counts = 0;

class TestServer0 {
public:
    TestServer0() {
        CF_BIND(get_node_info_server, this);
    }
    static int call_counts;
    void handle_get_node_info_request(const CanardRxTransfer &transfer, const uavcan_protocol_GetNodeInfoRequest &req) {
        call_counts++;
        uavcan_protocol_GetNodeInfoResponse res {};
        res.status.uptime_sec = 1;
        res.status.health = 2;
        res.status.mode = 3;
        res.status.sub_mode = 4;
        res.status.vendor_specific_status_code = 5;
        res.software_version.major = 1;
        res.software_version.minor = 2;
        res.hardware_version.major = 3;
        res.hardware_version.minor = 4;
        res.name.len = strlen("helloworld");
        memcpy(res.name.data, "helloworld", res.name.len);
        get_node_info_server.respond(transfer, res);
    }
    CF_CREATE_SERVER_CLASS(CF_CORE_TEST_INTERFACE(0), // interface name
                            get_node_info_server, // server name
                            uavcan_protocol_GetNodeInfo, // service name
                            TestServer0, // class name
                            &TestServer0::handle_get_node_info_request); // callback function
};

class TestServer1 {
    CF_CREATE_SERVER_CLASS_INDEX(CF_CORE_TEST_INTERFACE(1), // interface name
                                  1, // node index
                                  get_node_info_server, // server name
                                  uavcan_protocol_GetNodeInfo, // service name
                                  TestServer1, // class name
                                  &TestServer1::handle_get_node_info_request); // callback function
public:
    TestServer1() {
        CF_BIND(get_node_info_server, this);
    }
    static int call_counts;
    void handle_get_node_info_request(const CanardRxTransfer &transfer, const uavcan_protocol_GetNodeInfoRequest &req) {
        call_counts++;
        uavcan_protocol_GetNodeInfoResponse res {};
        res.status.uptime_sec = 1;
        res.status.health = 2;
        res.status.mode = 3;
        res.status.sub_mode = 4;
        res.status.vendor_specific_status_code = 5;
        res.software_version.major = 1;
        res.software_version.minor = 2;
        res.hardware_version.major = 3;
        res.hardware_version.minor = 4;
        res.name.len = strlen("helloworld");
        memcpy(res.name.data, "helloworld", res.name.len);
        get_node_info_server.respond(transfer, res);
    }
};

int TestServer0::call_counts = 0;
int TestServer1::call_counts = 0;

TEST(StaticCoreTest, test_multiple_clients) {
    // create multiple clients
    TestClient0 client0[2];
    TestClient1 client1[2];
    // create servers
    TestServer0 server0 __attribute__((unused));
    TestServer1 server1 __attribute__((unused));

    // set node id
    CF_CORE_TEST_INTERFACE(0).set_node_id(1);
    CF_CORE_TEST_INTERFACE(1).set_node_id(2);

    // send request
    uavcan_protocol_GetNodeInfoRequest req {};
    EXPECT_TRUE(client0[0].get_node_info_client.request(2, req));
    ASSERT_EQ(TestClient0::call_counts, 1);
    EXPECT_TRUE(client0[1].get_node_info_client.request(2, req));
    ASSERT_EQ(TestClient0::call_counts, 2);
    ASSERT_EQ(TestServer1::call_counts, 2);

    EXPECT_TRUE(client1[0].get_node_info_client.request(1, req));
    ASSERT_EQ(TestClient1::call_counts, 1);
    EXPECT_TRUE(client1[1].get_node_info_client.request(1, req));
    ASSERT_EQ(TestClient1::call_counts, 2);
    ASSERT_EQ(TestServer0::call_counts, 2);
}

} // namespace StaticCoreTest