
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>   //inet_addr
#include <netinet/tcp.h> // TCP_NODELAY
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "mqtt.h"
#include "unity.h"
#include "../help/help.h"

extern MQTTErrorCodes_t mqtt_connect_parse_ack(uint8_t * a_message_in_ptr);
extern bool g_auto_state_connection_completed_;
extern bool g_auto_state_subscribe_completed_;
extern bool socket_OK_;

void test_sm_subscrbe_with_receive()
{
    uint8_t buffer[1024];
    MQTT_shared_data_t shared;

    TEST_ASSERT_TRUE_MESSAGE(0 < open_mqtt_socket_(), "Failed to open socket");

    shared.state             = ACTION_DISCONNECT;
    shared.buffer            = buffer;
    shared.buffer_size       = sizeof(buffer);
    shared.out_fptr          = &data_stream_out_fptr_;
    shared.connected_cb_fptr = &connected_cb_;
    shared.subscribe_cb_fptr = &subscrbe_cb_;

    g_auto_state_connection_completed_ = false;
    g_auto_state_subscribe_completed_  = false;

    MQTT_action_data_t action;
    action.action_argument.shared_ptr = &shared;
    MQTTErrorCodes_t state = mqtt(ACTION_INIT,
                                  &action);

    MQTT_connect_t connect_params;
    uint8_t clientid[] = "JAMKtest test_sm_subscrbe_with_receive";
    uint8_t aparam[]   = "\0";

    connect_params.client_id                    = clientid;
    connect_params.last_will_topic              = aparam;
    connect_params.last_will_message            = aparam;
    connect_params.username                     = aparam;
    connect_params.password                     = aparam;
    connect_params.keepalive                    = 0;
    connect_params.connect_flags.clean_session  = true;
    connect_params.connect_flags.last_will_qos  = 0;
    connect_params.connect_flags.permanent_will = false;

    action.action_argument.connect_ptr = &connect_params;

    state = mqtt(ACTION_CONNECT,
                 &action);

    TEST_ASSERT_EQUAL_INT(Successfull, state);

    // Wait response and request parse for it
    // Parse will call given callback which will set global flag to true
    int rcv = data_stream_in_fptr_(buffer, sizeof(MQTT_fixed_header_t));


    if (0 < rcv) {
        MQTT_input_stream_t input;
        input.data = buffer;
        input.size_of_data = (uint32_t)rcv;
        action.action_argument.input_stream_ptr = &input;

        state = mqtt(ACTION_PARSE_INPUT_STREAM,
                     &action);
    } else {
        TEST_ASSERT(0);
    }

    do {
        /* Wait callback */
        TEST_ASSERT_TRUE(socket_OK_ == true);
        asleep(1);
    } while( false == g_auto_state_connection_completed_ );

    MQTT_subscribe_t subscribe;
    subscribe.topic_ptr = NULL;
    subscribe.topic_length = 0;
    subscribe.qos = QoS0;

    /* Test with long topic name (over 256B) */
    const char topic[] = "test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/test/msg/";
    subscribe.topic_ptr = (uint8_t*) topic;
    subscribe.topic_length = strlen(topic);

    action.action_argument.subscribe_ptr = &subscribe;

    state = mqtt(ACTION_SUBSCRIBE,
                 &action);

    TEST_ASSERT_EQUAL_INT(Successfull, state);

    // Wait response and request parse for it
    // Parse will call given callback which will set global flag to true
    rcv = data_stream_in_fptr_(buffer, sizeof(MQTT_fixed_header_t));

    MQTT_input_stream_t input;

    if (0 < rcv) {
        input.data = buffer;
        input.size_of_data = (uint32_t)rcv;
        action.action_argument.input_stream_ptr = &input;

        state = mqtt(ACTION_PARSE_INPUT_STREAM,
                     &action);
    } else {
        TEST_ASSERT(0);
    }

    while (false == g_auto_state_subscribe_completed_) {
        TEST_ASSERT_TRUE(true == socket_OK_);
        asleep(10);
    }

    const   char message[]      = "FooBarMessage2";

    MQTT_publish_t publish;
    publish.flags.dup           = false;
    publish.flags.retain        = false;
    publish.flags.qos           = QoS0;
    publish.topic_ptr           = (uint8_t*) topic;
    publish.topic_length        = strlen(topic);
    publish.message_buffer_ptr  = (uint8_t*)message;
    publish.message_buffer_size = strlen(message);
    publish.output_buffer_ptr   = NULL;
    publish.output_buffer_size   = 0;

    hex_print((uint8_t *) publish.message_buffer_ptr, publish.message_buffer_size);

    g_auto_state_subscribe_completed_ = false;

    action.action_argument.publish_ptr = &publish;
    state = mqtt(ACTION_PUBLISH,
                 &action);

    TEST_ASSERT_EQUAL_INT(Successfull, state);

    g_auto_state_subscribe_completed_ = false;
    rcv = data_stream_in_fptr_(buffer, sizeof(MQTT_fixed_header_t));

    // MQTT_input_stream_t input;

    if (0 < rcv) {
        input.data = buffer;
        input.size_of_data = (uint32_t)rcv;
        action.action_argument.input_stream_ptr = &input;

        state = mqtt(ACTION_PARSE_INPUT_STREAM,
                     &action);
    } else {
        TEST_ASSERT(0);
    }

    TEST_ASSERT_EQUAL_INT(Successfull, state);

    while (false == g_auto_state_subscribe_completed_)
        asleep(10);

    state = mqtt(ACTION_DISCONNECT,
                 NULL);

    TEST_ASSERT_EQUAL_INT(Successfull, state);

    close_mqtt_socket_();
}
/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{
    UnityBegin("State Maschine PubSub");
    unsigned int tCntr = 1;
    RUN_TEST(test_sm_subscrbe_with_receive,                 tCntr++);
    return (UnityEnd());
}
