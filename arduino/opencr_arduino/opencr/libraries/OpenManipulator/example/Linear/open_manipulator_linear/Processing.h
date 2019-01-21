/*******************************************************************************
* Copyright 2018 ROBOTIS CO., LTD.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/* Authors: Darby Lim, Hye-Jong KIM, Ryan Shim, Yong-Ho Na */

#ifndef PROCESSING_H_
#define PROCESSING_H_

#include <Linear.h>
#include "Demo.h"

typedef struct _MotionWayPoint
{
  std::vector<double> angle;
  double path_time;
  double gripper_value;
} MotionWayPoint;

std::vector<MotionWayPoint> motion_way_point_buf;

bool processing_motion_flag = false;
char hand_motion_cnt = 0;
bool hand_motion_repeat_flag = false;

//---------------------------------------------------------------------------------------------------- 
/* Initialize baudrate for Processing */
void initProcessing()
{ 
  Serial.begin(57600);
}

//---------------------------------------------------------------------------------------------------- 
/* Split data by separator */
void split(String data, char separator, String* temp)
{
  int cnt = 0;
  int get_index = 0;

  String copy = data;
  
  while(true)
  {
    get_index = copy.indexOf(separator);

	if(-1 != get_index)
	{
	  temp[cnt] = copy.substring(0, get_index);
  	  copy = copy.substring(get_index + 1);
	}
	else
	{
      temp[cnt] = copy.substring(0, copy.length());
	  break;
	}
	  ++cnt;
  }
}

/* Parse data received from Processing */
String* parseProcessingData(String get) 
{
  String cmd[50];
  get.trim();
  split(get, ',', cmd);
  
  return cmd;
}

/* Send joint data(values) to Processing */
void sendJointDataToProcessing(std::vector<WayPoint> joint_angle_vector) 
{
  Serial.print("angle");

  for (int i = 0; i < (int)joint_angle_vector.size(); i++)
  {
    Serial.print(",");
    Serial.print(joint_angle_vector.at(i).value,3);
  }
  Serial.print("\n");
}

/* Send tool data(on or off) to Processing */
void sendToolDataToProcessing(bool onoff)
{
  Serial.print("tool");
  Serial.print(",");
  Serial.print(onoff);
  Serial.print("\n");
}

/* Send tool data(values) to Processing */
void sendToolDataToProcessing(double value)
{
  Serial.print("tool");
  Serial.print(",");
  Serial.print(value*10);
  Serial.print("\n");
}

/* Send data(values) to Processing */
void sendDataToProcessing(Linear *linear)
{
  // sendJointDataToProcessing(linear->getAllActiveJointValue());
  // sendToolDataToProcessing(linear->getToolValue("tool"));
}

//---------------------------------------------------------------------------------------------------- 
/* Receive data from Processing */
void receiveDataFromProcessing(Linear *linear) 
{
  if (!linear->getReceiveDataFlag())
  {
    if (Serial.available())
    {
      linear->setReceiveDataFlag(true);  

      String serialInput = Serial.readStringUntil('\n');
      String *cmd = parseProcessingData(serialInput);

      // Actuator control tab 
      if (cmd[0] == "actuator")
      {
        if (cmd[1] == "on")
        {
          if(linear->getHardwareFlag())
          {
            linear->allActuatorEnable();
            sendDataToProcessing(linear);  // how..?? can this be used here..??
          }
        }
        else if (cmd[1] == "off")
        {
          if (linear->getHardwareFlag())
          {
            linear->allActuatorDisable();
          }
        }
      }

      // Joint space control tab 
      else if (cmd[0] == "joint")
      {

        // Torque On/Off
        if (cmd[1] == "on")
        {
          if (linear->getHardwareFlag())    
          {
            linear->allJointActuatorEnable();
            sendJointDataToProcessing(linear->getAllActiveJointValue());
          }
        }
        else if (cmd[1] == "off")
        {
          if (linear->getHardwareFlag())    
            linear->allJointActuatorDisable();
        }

        //
        else
        {
          std::vector<double> goal_position;

          for (uint8_t index = 0; index < DXL_SIZE; index++)
          {
            goal_position.push_back((double)cmd[index + 1].toFloat());
          }
          linear->jointTrajectoryMove(goal_position, 1.0); 
        }
      }
      
      // Tool control tab 
      else if (cmd[0] == "tool")
      {
        //
        if (cmd[1] == "y")
          linear->toolMove("tool", -0.007);
        else if (cmd[1] == "n")
          linear->toolMove("tool", 0.007);

        // Torque On/Off
        else if (cmd[1] == "on")
        {
          if (linear->getHardwareFlag())    
            linear->allToolActuatorEnable();
        }
        else if (cmd[1] == "off")
        {
          if (linear->getHardwareFlag())    
            linear->allToolActuatorDisable();
        }

        //
        else
          linear->toolMove("tool", (double)cmd[1].toFloat());
      }

      // Task space control tab 
      else if (cmd[0] == "task")
      {
        if (cmd[1] == "f")
          linear->taskTrajectoryMoveToPresentPose("tool", RM_MATH::makeVector3(0.006, 0.0, 0.0), 0.2);
        else if (cmd[1] == "b")
          linear->taskTrajectoryMoveToPresentPose("tool", RM_MATH::makeVector3(-0.006, 0.0, 0.0), 0.2);
        else if (cmd[1] == "l")
          linear->taskTrajectoryMoveToPresentPose("tool", RM_MATH::makeVector3(0.0, 0.006, 0.0), 0.2);
        else if (cmd[1] == "r")
          linear->taskTrajectoryMoveToPresentPose("tool", RM_MATH::makeVector3(0.0, -0.006, 0.0), 0.2);
        else
          linear->taskTrajectoryMoveToPresentPose("tool", RM_MATH::makeVector3(0.0, 0.0, 0.0), 0.2);
      }

      // Hand teaching tab 
      else if (cmd[0] == "hand")
      {
        if (cmd[1] == "clear")  // motion clear
        {
          processing_motion_flag = false;
          motion_way_point_buf.clear();
          hand_motion_cnt = 0;
        }
        else if (cmd[1] == "pose")  // save pose
        {
          MotionWayPoint read_value;
          read_value.angle = linear->getManipulator()->getAllActiveJointValue();
          read_value.path_time = 2.0;
          // read_value.gripper_value = linear->getManipulator()->getToolGoalValue("tool");
          motion_way_point_buf.push_back(read_value);  
          hand_motion_cnt = 0;
        }
        else if (cmd[1] == "on")  // save gripper on
        {
          linear->toolMove("tool", -0.01);
        }
        else if (cmd[1] == "off")  // save gripper off
        {
          linear->toolMove("tool", 0.01);
        }
      }
      else if (cmd[0] == "hand")
      {
        if (cmd[1] == "once") // play motion (once)
        {
          processing_motion_flag = true;//processing_motion_flag;
        }
        else if (cmd[1] == "repeat") // play motion (repeat)
        {
          hand_motion_repeat_flag = true;
        }
        else if (cmd[1] == "stop") // play motion (stop)
        {
          hand_motion_repeat_flag = false;
          processing_motion_flag = false;
          hand_motion_cnt = 0;
        }
      }

      // Motion Control tab 
      else if (cmd[0] == "motion")
      {
        if (cmd[1] == "start")
        {
          Pose present_pose = linear->getManipulator()->getComponentPoseFromWorld("tool");
          WayPoint draw_goal_pose[6];
          draw_goal_pose[0].value = present_pose.position(0) + 0.02;
          draw_goal_pose[1].value = present_pose.position(1) + 0.02;
          draw_goal_pose[2].value = present_pose.position(2) - 0.02;
          draw_goal_pose[3].value = RM_MATH::convertRotationToRPY(present_pose.orientation)[0];
          draw_goal_pose[4].value = RM_MATH::convertRotationToRPY(present_pose.orientation)[1];
          draw_goal_pose[5].value = RM_MATH::convertRotationToRPY(present_pose.orientation)[2];

          void *p_draw_goal_pose = &draw_goal_pose;
          
          linear->drawingTrajectoryMove(DRAWING_LINE, "tool", p_draw_goal_pose, 1.0);
        }
        else if (cmd[1] == "stop")
        {
          double draw_circle_arg[3];
          draw_circle_arg[0] = 0.03; // radius (m)
          draw_circle_arg[1] = 2;    // revolution
          draw_circle_arg[2] = 0.0;  // start angle position (rad)
          void* p_draw_circle_arg = &draw_circle_arg;
          linear->drawingTrajectoryMove(DRAWING_CIRCLE, "tool", p_draw_circle_arg, 4.0);
        }
      }

      linear->setPrevReceiveTime(millis()/1000.0); // instead of curr_time...?
    }
  }
  else 
  {
    // Check if running demo now
    if (linear->getRunDemoFlag())
    {
      runDemo(linear); 
    }

    // Check if ???
    else if (millis()/1000.0 - linear->getPrevReceiveTime() >= RECEIVE_RATE)
    {
      linear->setReceiveDataFlag(false);   //received <--
      initRC100();
    }
  }
}

//---------------------------------------------------------------------------------------------------- 
void playProcessingMotion(Linear *linear)
{
  if(!linear->isMoving() && processing_motion_flag)
  {
    if(motion_way_point_buf.size() == 0)
      return;

    linear->toolMove("tool", motion_way_point_buf.at(hand_motion_cnt).gripper_value);
    linear->jointTrajectoryMove(motion_way_point_buf.at(hand_motion_cnt).angle, motion_way_point_buf.at(hand_motion_cnt).path_time); 
    hand_motion_cnt ++;
    if(hand_motion_cnt >= motion_way_point_buf.size())
    {
      hand_motion_cnt = 0;
      if(!hand_motion_repeat_flag)
        processing_motion_flag = false;
    }
  }
}


#endif