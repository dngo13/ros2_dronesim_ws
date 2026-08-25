#include <chrono>
#include <memory>
#include <thread>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/wait_for_message.hpp"

#include "ardupilot_msgs/srv/mode_switch.hpp"
#include "ardupilot_msgs/srv/arm_motors.hpp"
#include "ardupilot_msgs/srv/takeoff.hpp"

#include "geometry_msgs/msg/pose_stamped.hpp"

using namespace std::chrono_literals;

// ArduCopter flight mode numbers.
// https://mavlink.io/en/messages/ardupilotmega.html#COPTER_MODE
constexpr uint8_t COPTER_MODE_GUIDED = 4; // constexpr is a keyword for variable to evaluated at compile time to improve performance 
constexpr uint8_t COPTER_MODE_LAND = 9;
constexpr float TAKEOFF_ALTITUDE_M = 10.0;

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv); // initialize ROS 2 client library
    auto node = std::make_shared<rclcpp::Node>("flight_mission_node"); // create a node named "flight_mission_node"
    auto logger = node->get_logger(); //creates a logger instance

    // Switch to GUIDED mode to allow autonomous control of the drone
    auto mode_client = node->create_client<ardupilot_msgs::srv::ModeSwitch>("/ap/mode_switch");
    mode_client->wait_for_service();

    auto guided_request = std::make_shared<ardupilot_msgs::srv::ModeSwitch::Request>(); // create a request object for the ModeSwitch service
    guided_request->mode = COPTER_MODE_GUIDED;
    auto guided_future = mode_client->async_send_request(guided_request); // send the request asynchronously to the service
    rclcpp::spin_until_future_complete(node, guided_future); // wait for the service response to complete
    RCLCPP_INFO(logger, "Switched to GUIDED mode: %s", guided_future.get()->status ? "ok" : "FAILED");

    // Arm the motors to prepare for takeoff
    auto arm_client = node->create_client<ardupilot_msgs::srv::ArmMotors>("/ap/arm_motors");
    arm_client->wait_for_service();

    auto arm_request = std::make_shared<ardupilot_msgs::srv::ArmMotors::Request>(); // create a request object for the ArmMotors service
    arm_request->arm = true;
    auto arm_future = arm_client->async_send_request(arm_request); // send the request asynchronously to the service
    rclcpp::spin_until_future_complete(node, arm_future); // wait for the service response to complete
    bool armed = arm_future.get()->result;  // call .get() exactly once, save it
    RCLCPP_INFO(logger, "Armed motors: %s", armed ? "ok" : "FAILED");

    // Take off to 10 meters
    auto takeoff_client = node->create_client<ardupilot_msgs::srv::Takeoff>("/ap/experimental/takeoff"); // create a client for the Takeoff service
    takeoff_client->wait_for_service();

    auto takeoff_request = std::make_shared<ardupilot_msgs::srv::Takeoff::Request>(); // create a request object for the Takeoff service
    takeoff_request->alt = TAKEOFF_ALTITUDE_M;

    
    if (armed) { // check if the motors were successfully armed before attempting to take off
        auto takeoff_future = takeoff_client->async_send_request(takeoff_request); // send the request asynchronously to the service
        rclcpp::spin_until_future_complete(node, takeoff_future);
        bool took_off = takeoff_future.get()->status; // status variable to check if the takeoff was successful
        RCLCPP_INFO(logger, "Takeoff requested: %s",  took_off ? "ok" : "FAILED");

        double current_altitude_m = 0.0; // variable to store the current altitude of the drone
        geometry_msgs::msg::PoseStamped pose_msg; // variable to store the pose message received from the /ap/pose/filtered topic
        constexpr double ALTITUDE_TOLERANCE_M = 0.5;

        while (rclcpp::ok() && std::abs(current_altitude_m - TAKEOFF_ALTITUDE_M) > ALTITUDE_TOLERANCE_M) { // loop until the drone reaches the target altitude within the specified tolerance
            if (rclcpp::wait_for_message(pose_msg, node, "/ap/pose/filtered", 1s)) {
                current_altitude_m = pose_msg.pose.position.z;
            }
        }
        RCLCPP_INFO(logger, "Reached altitude: %.2fm", current_altitude_m);

        // Hover for 30 seconds
        std::this_thread::sleep_for(30s);
    }

    // Land
    auto land_request = std::make_shared<ardupilot_msgs::srv::ModeSwitch::Request>(); // create a request object for the ModeSwitch service
    land_request->mode = COPTER_MODE_LAND;
    auto land_future = mode_client->async_send_request(land_request); // send the request asynchronously to the service
    rclcpp::spin_until_future_complete(node, land_future); // wait for the service response to complete
    RCLCPP_INFO(logger, "Switched to LAND mode: %s", land_future.get()->status ? "ok" : "FAILED");

    rclcpp::shutdown();
    return 0;
}