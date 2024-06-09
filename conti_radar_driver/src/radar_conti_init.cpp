#include "radar_conti_ars408_component.hpp"
#include <string>


int main(int argc, char** argv)
{
    ros::init(argc, argv, "lidar_filt");
    ros::NodeHandle nh;

    can::DriverInterfaceSharedPtr driver = std::make_shared<can::ThreadedSocketCANInterface> ();

    std::string can_device;
    ros::param::get("can_port", can_device);

    if(!driver->init(can_device,0)) 
    {
        ROS_FATAL("Failed to initialize can_device at %s",can_device.c_str());
        return 1;
    }
    else ROS_INFO("Successfully connected to %s.", can_device.c_str());
    Radar_Conti node(nh);

    ROS_INFO("Successfully innitialize node");
    node.init(driver);
    ROS_INFO("Successfully innitialize driver");

    ros::spin();

    driver->shutdown();
    driver.reset();

    ros::waitForShutdown();

    return 0;
}