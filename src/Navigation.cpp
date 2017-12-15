#include "Navigation.hpp"

Navigation::Navigation() {
    velocity = nh.advertise <geometry_msgs::Twist>
 ("/mobile_base/commands/velocity", 1000);
    depthBuffer = nh.subscribe <sensor_msgs::LaserScan>
 ("/scan", 50, &Turtlebotstates::callDepth, &depth);
    msg.linear.x = 0.0;
    msg.linear.y = 0.0;
    msg.linear.z = 0.0;
    msg.angular.x = 0.0;
    msg.angular.y = 0.0;
    msg.angular.z = 0.0;
    // publish velocity values for turtlebot
    velocity.publish(msg);
}

Navigation::~Navigation() {
    // stop the robot motion
    msg.linear.x = 0.0;
    msg.linear.y = 0.0;
    msg.linear.z = 0.0;
    msg.angular.x = 0.0;
    msg.angular.y = 0.0;
    msg.angular.z = 0.0;
    velocity.publish(msg);
}

void Navigation::navmain(std::string path) {
  int totalEpisode = 3000;
  double epsilonDiscount = 0.99;  // 0.9986
  int highestReward = 0;
  // double initialEpsilon = q.returnEpsilon();
  std::vector<int> state;
  int stateIndex = 0;
  int nextStateIndex;
  int chosenAction;
  int epiCount = 0;
  ros::Rate loop_rate(10);
  while (ros::ok()) {
  // while(epiCount < totalEpisode)  {
  if (epiCount < totalEpisode) {
      // std::cout<<"episode"<<epiCount<<"\n";
      bool collision = false;
      int cumulatedReward = 0;
      envReset();

      if (q.returnEpsilon() > 0.05) {
          q.setEpsilon(q.returnEpsilon() * epsilonDiscount);
      }
      // std::cout<<"epsilon"<<q.returnEpsilon();

      state = depth.returnLaserState();
      stateIndex = findStateIndex(state);

      int innerLoopCount = 0;

      while (innerLoopCount < 700) {
          chosenAction = q.chooseAction(stateIndex);
          action(chosenAction, collision, reward, nextStateIndex);
          cumulatedReward += reward;

          if (highestReward < cumulatedReward) {
              highestReward = cumulatedReward;
          }
          q.learn(stateIndex, chosenAction, reward, nextStateIndex);
          if (collision) {
              envReset();
              break;
          } else {
              stateIndex = nextStateIndex;
          }
      innerLoopCount++;
      ros::spinOnce();
      }
      ROS_INFO_STREAM("Epsilon: " << q.returnEpsilon() << " Episode Number: " << epiCount << " Cum. reward: " << cumulatedReward);
//      std::cout << "Epsilon: " << q.returnEpsilon() << " Episode number: "
// << epiCount << " Cum. reward: " << cumulatedReward << "\t" <<
// innerLoopCount << "\n";
          epiCount++;
     }
      ros::spinOnce();
  }
  q.storeQtable(path);
}

int Navigation::findStateIndex(std::vector<int> state) {
    int tempIndex = 0;
    int tempState[4] = {0, 0, 0, 0};
    for (auto i : state) {
        tempState[tempIndex++] += i-1;
    }
    return (tempState[3] + tempState[2] * 6 + tempState[1] * 36 + (tempState[0])*216);
}

void Navigation::envReset() {

    std_srvs::Empty resetWorldSrv;
    ros::service::call("/gazebo/reset_world", resetWorldSrv);
    envUnPause();
    while (depth.collisionCheck()) {
     ros::spinOnce();
    }
    envPause();
}

void Navigation::envPause() {
    std_srvs::Empty resetWorldSrv;
    ros::service::call("/gazebo/pause_physics", resetWorldSrv);
}

void Navigation::envUnPause() {
    std_srvs::Empty resetWorldSrv;
    ros::service::call("/gazebo/unpause_physics", resetWorldSrv);
}

void Navigation::action(int action, bool &colStat, int &reward, int &nextState) {
    std::vector<int> tempState;
    envUnPause();
    if (action == 0) {
        msg.linear.x = 0.2;
        msg.angular.z = 0.0;
        velocity.publish(msg);
    } else if (action == 1) {
        msg.linear.x = 0.05;
        msg.angular.z = 0.3;
        velocity.publish(msg);
    } else if (action == 2) {
        msg.linear.x = 0.05;
        msg.angular.z = -0.3;
        velocity.publish(msg);
    }
    sensor_msgs::LaserScan pc;
    sensor_msgs::LaserScanConstPtr msg1 = ros::topic::waitForMessage<sensor_msgs::LaserScan>("/scan", ros::Duration(10));
    if (msg1 == NULL)
    ROS_ERROR("Waiting for laser scan data");
    envPause();
    tempState = depth.returnLaserState();
    nextState = findStateIndex(tempState);
    colStat = depth.collisionCheck();
    if (colStat == false) {
        if (action == 0) {
            reward = 5;
        } else {
            reward = 1;
        }
    } else {
        reward = -200;
    }
}

void Navigation::navmain2(std::string path) {
    std::vector<int> state;
    ros::Rate loop_rate(10);
    Qlearning qqq;
    qqq.loadQtable(path);
    envReset();
    while (ros::ok()) {
        int stateIndex = 0;
        int chosenAction;
        qqq.setEpsilon(-1);
        state = depth.returnLaserState();
        stateIndex = findStateIndex(state);
        chosenAction = qqq.demo(stateIndex);
        demoAction(chosenAction);
        if (depth.collisionCheck()) {
            envReset();
        }
        ros::spinOnce();
    }
}

void Navigation::demoAction(int action) {
    envUnPause();
    if (action == 0) {
        msg.linear.x = 0.2;
        msg.angular.z = 0.0;
        velocity.publish(msg);
    } else if (action == 1) {
        msg.linear.x = 0.05;
        msg.angular.z = 0.3;
        velocity.publish(msg);
    } else if (action == 2) {
        msg.linear.x = 0.05;
        msg.angular.z = -0.3;
        velocity.publish(msg);
    }
    sensor_msgs::LaserScan pc;
    sensor_msgs::LaserScanConstPtr msg1 = ros::topic::waitForMessage<sensor_msgs::LaserScan>("/scan", ros::Duration(10));
    if (msg1 == NULL) {
        ROS_ERROR("Waiting for laser scan data");
    }
    envPause();
}
