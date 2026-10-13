cmake_minimum_required(VERSION 3.8)
project(xarm_control_study)
find_package(ament_cmake REQUIRED)
# ROS node, joint feedback, 표준 trajectory command의 메시지 의존성입니다.
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(trajectory_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
add_executable(joint_test_node src/joint_test_node.cpp)
add_executable(ik_test_node src/ik_test_node.cpp)

ament_target_dependencies(
  ik_test_node
  rclcpp
  tf2_ros
)
target_compile_features(joint_test_node PUBLIC cxx_std_17)
target_compile_options(joint_test_node PRIVATE -Wall -Wextra -Wpedantic)
ament_target_dependencies(joint_test_node rclcpp sensor_msgs std_msgs trajectory_msgs)
# ros2 run / ros2 launch가 설치 공간에서 실행 파일과 launch를 찾게 합니다.
install(TARGETS
  joint_test_node
  ik_test_node
  DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})
ament_package()
