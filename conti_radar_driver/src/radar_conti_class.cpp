#include "radar_conti_ars408_component.hpp"
#include <iostream>
Radar_Conti::Radar_Conti(const ros::NodeHandle &nh) : nh(nh) {};

void Radar_Conti::init(can::DriverInterfaceSharedPtr &driver_)
{
    // pub_marker = nh.advertise<visualization_msgs::MarkerArray>("radar_objects_marker",0);
    pub_objects = nh.advertise<radar_conti::ObjectList>("radar_object_list",0);
    collison_obj_pub = nh.advertise<radar_conti::CollisonList>("radar_obj_collison",0);
    pub_cluster = nh.advertise<visualization_msgs::MarkerArray>("radar_cluster_markers",0);
    pub_cluster_list = nh.advertise<radar_conti::ClusterList>("radar_cluster_list",0);
    pub_cloud = nh.advertise<sensor_msgs::PointCloud2>("point_cloud",0);
    this->driver_ = driver_;
    this->frame_listener_ = driver_->createMsgListenerM(this,&Radar_Conti::can_frame_callback);
}

void Radar_Conti::can_frame_callback(const can::Frame &msg)
{
    if (msg.id == ID_RadarState) {
        operation_mode_ =CALC_RadarState_RadarState_OutputTypeCfg(GET_RadarState_RadarState_OutputTypeCfg(msg.data),1.0);
    }
    if (operation_mode_ == 0x00)
        return;

    else if(operation_mode_ == 0x01)
        handle_object_list(msg);

    else if(operation_mode_ == 0x02)
        handle_cluster_list(msg);
}
void Radar_Conti::handle_object_list(const can::Frame &msg)
{
    if (msg.id == ID_Obj_0_Status) {

        publish_object_map();

        object_list_.header.stamp = ros::Time::now();
        object_list_.object_count.data = GET_Obj_0_Status_Obj_NofObjects(msg.data);

        object_map_.clear();
        collison_objects.clear();

    }


    if (msg.id == ID_Obj_1_General) {

        radar_conti::Object o;
        //object ID
        int id = GET_Obj_1_General_Obj_ID(msg.data);
        o.obj_id.data = GET_Obj_1_General_Obj_ID(msg.data);

        // //RCLCPP_INFO(this->get_logger(), "Object_ID: 0x%04x", o.obj_id.data);

        //longitudinal distance
        o.object_general.obj_distlong.data =
                CALC_Obj_1_General_Obj_DistLong(GET_Obj_1_General_Obj_DistLong(msg.data), 1.0);

        //lateral distance
        o.object_general.obj_distlat.data =
                CALC_Obj_1_General_Obj_DistLat(GET_Obj_1_General_Obj_DistLat(msg.data), 1.0);

        //relative longitudinal velocity
        o.object_general.obj_vrellong.data =
                CALC_Obj_1_General_Obj_VrelLong(GET_Obj_1_General_Obj_VrelLong(msg.data), 1.0);

        //relative lateral velocity
        o.object_general.obj_vrellat.data =
                CALC_Obj_1_General_Obj_VrelLat(GET_Obj_1_General_Obj_VrelLat(msg.data), 1.0);

        o.object_general.obj_dynprop.data =
                CALC_Obj_1_General_Obj_DynProp(GET_Obj_1_General_Obj_DynProp(msg.data), 1.0);

        o.object_general.obj_rcs.data = 
                CALC_Obj_1_General_Obj_RCS(GET_Obj_1_General_Obj_RCS(msg.data), 1.0);

        //insert object into map
        
        object_map_.insert(std::pair<int, radar_conti::Object>(id, o));
    }

    if (msg.id == ID_Obj_2_Quality) {

        int id = GET_Obj_2_Quality_Obj_ID(msg.data);

        object_map_[id].object_quality.obj_distlong_rms.data =
                CALC_Obj_2_Quality_Obj_DistLong_rms(GET_Obj_2_Quality_Obj_DistLong_rms(msg.data), 1.0);

        object_map_[id].object_quality.obj_distlat_rms.data =
                CALC_Obj_2_Quality_Obj_DistLat_rms(GET_Obj_2_Quality_Obj_DistLat_rms(msg.data), 1.0);

        object_map_[id].object_quality.obj_vrellong_rms.data =
                CALC_Obj_2_Quality_Obj_VrelLong_rms(GET_Obj_2_Quality_Obj_VrelLong_rms(msg.data), 1.0);

        object_map_[id].object_quality.obj_vrellat_rms.data =
                CALC_Obj_2_Quality_Obj_VrelLat_rms(GET_Obj_2_Quality_Obj_VrelLat_rms(msg.data), 1.0);

    }
    if (msg.id == ID_Obj_3_Extended) {
        int id = GET_Obj_3_Extended_Obj_ID(msg.data);


        object_map_[id].object_extended.obj_arellong.data =
                CALC_Obj_3_Extended_Obj_ArelLong(GET_Obj_3_Extended_Obj_ArelLong(msg.data), 1.0);

        object_map_[id].object_extended.obj_arellat.data =
                CALC_Obj_3_Extended_Obj_ArelLat(GET_Obj_3_Extended_Obj_ArelLat(msg.data), 1.0);

        object_map_[id].object_extended.obj_class.data =
                CALC_Obj_3_Extended_Obj_Class(GET_Obj_3_Extended_Obj_Class(msg.data), 1.0);

        int obj_class = CALC_Obj_3_Extended_Obj_Class(GET_Obj_3_Extended_Obj_Class(msg.data), 1.0);

        object_map_[id].object_extended.obj_orientationangle.data =
                CALC_Obj_3_Extended_Obj_OrientationAngle(GET_Obj_3_Extended_Obj_OrientationAngle(msg.data),
                                                            1.0);

        object_map_[id].object_extended.obj_length.data =
                CALC_Obj_3_Extended_Obj_Length(GET_Obj_3_Extended_Obj_Length(msg.data), 1.0);

        object_map_[id].object_extended.obj_width.data =
                CALC_Obj_3_Extended_Obj_Width(GET_Obj_3_Extended_Obj_Width(msg.data), 1.0);
    }
    if(msg.id == ID_Obj_4_Warning)
    {
        const int obj_war = CALC_Obj_4_Warning_Obj_ID(GET_Obj_4_Warning_Obj_ID(msg.data),1.0);
        collison_objects.insert(obj_war); 
    }
    publish_object_map();

}
void Radar_Conti::handle_cluster_list(const can::Frame &msg)
{
    if(msg.id == ID_Cluster_0_Status)
    {
        publish_cluster_map();

        cluster_list.header.stamp = ros::Time::now();
        cluster_list.cluster_count.data = GET_Cluster_0_Status_Cluster_NofClustersNear(msg.data);

        cluster_map_.clear();
    }
    if(msg.id == ID_Cluster_1_General)
    {
        radar_conti::Cluster c;

        int id = GET_Cluster_1_General_Cluster_ID(msg.data);
        c.cluster_id.data = id;

        c.cluster_general.cluster_distlong.data = 
                CALC_Cluster_1_General_Cluster_DistLong(GET_Cluster_1_General_Cluster_DistLong(msg.data),1.0);
        
        c.cluster_general.cluster_distlat.data = 
                CALC_Cluster_1_General_Cluster_DistLat(GET_Cluster_1_General_Cluster_DistLat(msg.data),1.0);

        c.cluster_general.cluster_vrellong.data = 
                CALC_Cluster_1_General_Cluster_VrelLong(GET_Cluster_1_General_Cluster_VrelLong(msg.data),1.0);

        c.cluster_general.cluster_vrellat.data = 
                CALC_Cluster_1_General_Cluster_VrelLat(GET_Cluster_1_General_Cluster_VrelLat(msg.data),1.0);

        c.cluster_general.cluster_dynprop.data = 
                CALC_Cluster_1_General_Cluster_DynProp(GET_Cluster_1_General_Cluster_DynProp(msg.data),1.0);

        c.cluster_general.cluster_rcs.data = 
                CALC_Cluster_1_General_Cluster_RCS(GET_Cluster_1_General_Cluster_RCS(msg.data),1.0);

        cluster_map_.insert(std::pair<int, radar_conti::Cluster>(id, c));
    }
    if(msg.id == ID_Cluster_2_Quality)
    {
        int id = GET_Cluster_2_Quality_Cluster_ID(msg.data);

        cluster_map_[id].cluster_quality.cluster_distlat_rms.data = 
                CALC_Cluster_2_Quality_Cluster_DistLat_rms(GET_Cluster_2_Quality_Cluster_DistLat_rms(msg.data),1.0);
        cluster_map_[id].cluster_quality.cluster_distlong_rms.data =
                CALC_Cluster_2_Quality_Cluster_DistLong_rms(GET_Cluster_2_Quality_Cluster_DistLong_rms(msg.data),1.0);
        cluster_map_[id].cluster_quality.cluster_vrellong_rms.data =
                CALC_Cluster_2_Quality_Cluster_VrelLong_rms(GET_Cluster_2_Quality_Cluster_VrelLong_rms(msg.data),1.0);
        cluster_map_[id].cluster_quality.cluster_vrellat_rms.data = 
                CALC_Cluster_2_Quality_Cluster_VrelLat_rms(GET_Cluster_2_Quality_Cluster_VrelLat_rms(msg.data),1.0);
        cluster_map_[id].cluster_quality.cluster_pdh0.data = 
                CALC_Cluster_2_Quality_Cluster_PdH0(GET_Cluster_2_Quality_Cluster_PdH0(msg.data),1.0);
        cluster_map_[id].cluster_quality.cluster_ambigstate.data = 
                CALC_Cluster_2_Quality_Cluster_AmbigState(GET_Cluster_2_Quality_Cluster_AmbigState(msg.data),1.0);
        cluster_map_[id].cluster_quality.cluster_invalidstate.data = 
                CALC_Cluster_2_Quality_Cluster_InvalidState(GET_Cluster_2_Quality_Cluster_InvalidState(msg.data),1.0);
    }



}
void Radar_Conti::publish_object_map() {
        
        std::map<int, radar_conti::Object>::iterator itr;

        visualization_msgs::MarkerArray marker_array;

        //marker for ego car
        visualization_msgs::Marker mEgoCar;

        mEgoCar.header.stamp = ros::Time::now();
        mEgoCar.header.frame_id = frame_id_;
        mEgoCar.ns = "ego";
        mEgoCar.id = 999;

        //if you want to use a cube comment out the next 2 line
        mEgoCar.type = 1; // cube
        mEgoCar.action = 0; // add/modify
        mEgoCar.pose.position.x = 0.0;
        mEgoCar.pose.position.y = 0.0;
        mEgoCar.pose.position.z = 0.0;

        tf2::Quaternion myQuaternion;
        myQuaternion.setRPY(0, 0, M_PI/2);

        mEgoCar.pose.orientation.w = myQuaternion.getW();
        mEgoCar.pose.orientation.x = myQuaternion.getX();
        mEgoCar.pose.orientation.y = myQuaternion.getY();
        mEgoCar.pose.orientation.z = myQuaternion.getZ();
        mEgoCar.scale.x = 0.2;
        mEgoCar.scale.y = 0.2;
        mEgoCar.scale.z = 0.2;
        mEgoCar.color.r = 0.0;
        mEgoCar.color.g = 0.0;
        mEgoCar.color.b = 1.0;
        mEgoCar.color.a = 0.4;
        mEgoCar.lifetime = ros::Duration(0.2);
        mEgoCar.frame_locked = false;

        marker_array.markers.push_back(mEgoCar);

        for (itr = object_map_.begin(); itr != object_map_.end(); ++itr) {

                visualization_msgs::Marker mobject;
                visualization_msgs::Marker mtext;

                mtext.header.stamp = ros::Time::now();
                mtext.header.frame_id = frame_id_;
                mtext.ns = "text";
                mtext.id = (itr->first+100);
                mtext.type = 1; //Cube
                mtext.action = 0; // add/modify
                mtext.pose.position.x = itr->second.object_general.obj_distlong.data;
                mtext.pose.position.y = itr->second.object_general.obj_distlat.data;
                mtext.pose.position.z = 4.0;

        
                //myQuaternion.setRPY(M_PI / 2, 0, 0);
                myQuaternion.setRPY(0, 0, 0);

                mtext.pose.orientation.w = myQuaternion.getW();
                mtext.pose.orientation.x = myQuaternion.getX();
                mtext.pose.orientation.y = myQuaternion.getY();
                mtext.pose.orientation.z = myQuaternion.getZ();
                // mtext.scale.x = 1.0;
                // mtext.scale.y = 1.0;
                mtext.scale.z = 1.0;
                mtext.color.r = 1.0;
                mtext.color.g = 1.0;
                mtext.color.b = 1.0;
                mtext.color.a = 1.0;
                mtext.lifetime = ros::Duration(0.2);
                mtext.frame_locked = false;
                mtext.type=9;


                mobject.header.stamp = ros::Time::now();
                mobject.header.frame_id = frame_id_;
                mobject.ns = "objects";
                mobject.id = itr->first;
                mobject.type = 1; //Cube
                mobject.action = 0; // add/modify
                mobject.pose.position.x = itr->second.object_general.obj_distlong.data;
                mobject.pose.position.y = itr->second.object_general.obj_distlat.data;
                mobject.pose.position.z = 0.0;

                myQuaternion.setRPY(0, 0, itr->second.object_extended.obj_orientationangle.data);
                // TODO: itr->second.object_extended.obj_orientationangle.data is always 0
                //myQuaternion.setRPY(0, 0, 2.0);

                mobject.pose.orientation.w = myQuaternion.getW();
                mobject.pose.orientation.x = myQuaternion.getX();
                mobject.pose.orientation.y = myQuaternion.getY();
                mobject.pose.orientation.z = myQuaternion.getZ();
                mobject.scale.x = itr->second.object_extended.obj_length.data;
                mobject.scale.y = itr->second.object_extended.obj_width.data;
                mobject.scale.z = 1.0;
                if(collison_objects.find(itr->first) != collison_objects.end())
                {
                        mobject.color.r = 1.0;
                        mobject.color.g = 0.0;
                        mtext.text= "object_" + std::to_string(itr->first) + ": \n" ;
                        //+ " RCS: " + std::to_string(itr->second.object_general.obj_rcs.data) + "dBm^2" + " \n" 
                        //+ " V_long: " +   std::to_string(itr->second.object_general.obj_vrellong.data) + "m/s" + " \n" 
                        //+ " V_lat: " + std::to_string(itr->second.object_general.obj_vrellat.data) + "m/s" + " \n" 
                        // + " Orientation: " + std::to_string(itr->second.object_extended.obj_orientationangle.data) + "degree" + " \n"
                        //+ " Class: " + object_classes[itr->second.object_extended.obj_class.data] +"\n"
                        //+ " Collison with Object\n"
                        //+ "Obj_ProbOfExist:" +   std::to_string(value);
                        radar_conti::CollisonObj collison_item;
                        collison_item.obj_id = itr->second.obj_id;
                        coll_list.objects.push_back(collison_item);
                }
                else
                {
                        double value;
                        if (itr->second.object_quality.obj_probofexist.data==7)
                        {
                                value=100.0;
                        }
                        else if (itr->second.object_quality.obj_probofexist.data==6)
                        {
                                value=99.9;
                        }
                        else if (itr->second.object_quality.obj_probofexist.data==5)
                        {
                                value=99.0;
                        }
                        else if (itr->second.object_quality.obj_probofexist.data==4)
                        {
                                value=90.0;
                        }
                        else if (itr->second.object_quality.obj_probofexist.data==3)
                        {
                                value=75.0;
                        }
                        else if (itr->second.object_quality.obj_probofexist.data==2)
                        {
                                value=50.0;
                        }
                        else if (itr->second.object_quality.obj_probofexist.data==1)
                        {
                                value=25.0;
                        }
                        else if (itr->second.object_quality.obj_probofexist.data==0)
                        {
                                value=0.0;
                        }
                        
                        std::stringstream ss;
                        ss << "object_" << std::fixed << std::setprecision(1) << itr->first  << "\n" 
                        //+ " RCS: " + std::to_string(itr->second.object_general.obj_rcs.data) // + "dBm^2" + " \n" 
                        //+ " V_long: " +   std::to_string(itr->second.object_general.obj_vrellong.data) + "m/s" + " \n" 
                        //+ " V_lat: " + std::to_string(itr->second.object_general.obj_vrellat.data) + "m/s" + " \n" 
                        //+ " Orientation: " + std::to_string(itr->second.object_extended.obj_orientationangle.data); //+ "degree" + " \n"
                        <<"PO " << value;
                        mtext.text = ss.str();
                        //+ " Class: " + object_classes[itr->second.object_extended.obj_class.data] + "\n";
                }
                // assign colors to each object ID (deterministic pseudo-random colors)
                int i = int(itr->first);
                int j = 100 - int(itr->first);
                int k = itr->first + 5;
                double r = double(i % 3) / 2.1;
                double g = double(j % 21) / 21;
                double b = double(k % 30) / 30;
                mobject.color.r = r;
                mobject.color.g = g;
                mobject.color.b = b;
                mobject.color.a = 0.6;
                mobject.lifetime = ros::Duration(0.2);
                mobject.frame_locked = false;

                object_list_.objects.push_back(itr->second);
                //if (TODO){
                        marker_array.markers.push_back(mobject);
                //}
                marker_array.markers.push_back(mtext);

        }
        collison_obj_pub.publish(coll_list);
        pub_objects.publish(object_list_);
        //pub_marker.publish(marker_array);

}
void Radar_Conti::publish_cluster_map()
{
        std::map<int, radar_conti::Cluster>::iterator itr;

        
        visualization_msgs::MarkerArray marker_array;

        //marker for ego car
        visualization_msgs::Marker mEgoCar;

        mEgoCar.header.stamp = ros::Time::now();
        mEgoCar.header.frame_id = frame_id_;
        mEgoCar.ns = "ego";
        mEgoCar.id = 999;

        //if you want to use a cube comment out the next 2 line
        mEgoCar.type = 1; // cube
        mEgoCar.action = 0; // add/modify
        mEgoCar.pose.position.x = 0.0;
        mEgoCar.pose.position.y = 0.0;
        mEgoCar.pose.position.z = 0.0;

        tf2::Quaternion myQuaternion;
        myQuaternion.setRPY(0, 0, M_PI/2);

        mEgoCar.pose.orientation.w = myQuaternion.getW();
        mEgoCar.pose.orientation.x = myQuaternion.getX();
        mEgoCar.pose.orientation.y = myQuaternion.getY();
        mEgoCar.pose.orientation.z = myQuaternion.getZ();
        mEgoCar.scale.x = 0.2;
        mEgoCar.scale.y = 0.2;
        mEgoCar.scale.z = 0.2;
        mEgoCar.color.r = 0.0;
        mEgoCar.color.g = 0.0;
        mEgoCar.color.b = 1.0;
        mEgoCar.color.a = 1.0;
        mEgoCar.lifetime = ros::Duration(0.2);
        mEgoCar.frame_locked = false;

        marker_array.markers.push_back(mEgoCar);

        for (itr = cluster_map_.begin(); itr != cluster_map_.end(); ++itr) {

                visualization_msgs::Marker mobject;
                visualization_msgs::Marker mtext;

                mtext.header.stamp = ros::Time::now();
                mtext.header.frame_id = frame_id_;
                mtext.ns = "text";
                mtext.id = (itr->first+100);
                mtext.type = 1; //Cube
                mtext.action = 0; // add/modify
                mtext.pose.position.x = itr->second.cluster_general.cluster_distlong.data;
                mtext.pose.position.y = itr->second.cluster_general.cluster_distlat.data;
                mtext.pose.position.z = 4.0;

        
                //myQuaternion.setRPY(M_PI / 2, 0, 0);
                myQuaternion.setRPY(0, 0, 0);

                mtext.pose.orientation.w = myQuaternion.getW();
                mtext.pose.orientation.x = myQuaternion.getX();
                mtext.pose.orientation.y = myQuaternion.getY();
                mtext.pose.orientation.z = myQuaternion.getZ();
                // mtext.scale.x = 1.0;
                // mtext.scale.y = 1.0;
                mtext.scale.z = 2.0;
                mtext.color.r = 1.0;
                mtext.color.g = 1.0;
                mtext.color.b = 1.0;
                mtext.color.a = 1.0;
                mtext.lifetime = ros::Duration(0.2);
                mtext.frame_locked = false;
                mtext.type=9;
                mtext.text= "Cluster" + std::to_string(itr->first) + ": \n" 
                + " RCS: " + std::to_string(itr->second.cluster_general.cluster_rcs.data) + "dBm^2"; // + " \n" 
                //+ " V_long: " +   std::to_string(itr->second.cluster_general.cluster_vrellong.data) + "m/s" + " \n" 
                //+ " V_lat: " + std::to_string(itr->second.cluster_general.cluster_vrellat.data) + "m/s" + " \n";
                //+ " Orientation: " + std::to_string(itr->second.cluster_general.cluster_vrellon);


                marker_array.markers.push_back(mtext);



                mobject.header.stamp = ros::Time::now();
                mobject.header.frame_id = frame_id_;
                mobject.ns = "objects";
                mobject.id = itr->first;
                mobject.type = 1; //Cube
                mobject.action = 0; // add/modify
                mobject.pose.position.x = itr->second.cluster_general.cluster_distlong.data;
                mobject.pose.position.y = itr->second.cluster_general.cluster_distlat.data;
                mobject.pose.position.z = 1.0;

                myQuaternion.setRPY(0, 0, 0);

                mobject.pose.orientation.w = myQuaternion.getW();
                mobject.pose.orientation.x = myQuaternion.getX();
                mobject.pose.orientation.y = myQuaternion.getY();
                mobject.pose.orientation.z = myQuaternion.getZ();
                mobject.scale.x = 0.2;
                mobject.scale.y = 0.2;
                mobject.scale.z = 0.1;
                mobject.color.r = 0.0;
                mobject.color.g = 1.0;
                mobject.color.b = 0.0;
                mobject.color.a = 1.0;
                mobject.lifetime = ros::Duration(0.2);
                mobject.frame_locked = false;

                cluster_list.clusters.push_back(itr->second);

                marker_array.markers.push_back(mobject);


        }
        pub_cluster_list.publish(cluster_list);
        pub_cluster.publish(marker_array);

}



void Radar_Conti::publish_cluster_pointcloud()
{
        std::map<int, radar_conti::Cluster>::iterator itr;
        pcl::PointCloud<pcl::PointXYZ> cloud;
        
        for (itr = cluster_map_.begin(); itr != cluster_map_.end(); ++itr) 
        {
                pcl::PointXYZ pcl_point;
                pcl_point.x = itr->second.cluster_general.cluster_distlong.data;
                pcl_point.y = itr->second.cluster_general.cluster_distlat.data;
                pcl_point.z = 1.0;
                cloud.points.push_back(pcl_point);
        }
        cloud.header.frame_id = frame_id_;
        pcl_conversions::toPCL(ros::Time::now(), cloud.header.stamp);
        sensor_msgs::PointCloud2 output;
        pcl::toROSMsg(cloud, output);
        pub_cloud.publish(output);
}
