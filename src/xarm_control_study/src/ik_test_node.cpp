#include <chrono>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include <tf2/time.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>

using namespace std::chrono_literals;

class IKTestNode : public rclcpp::Node
{
public:
  IKTestNode()
  : Node("ik_test_node"),
    tf_buffer_(this->get_clock()),
    tf_listener_(tf_buffer_)
  {
    // TF가 들어올 시간을 조금 기다렸다가 확인
    timer_ = create_wall_timer(
      500ms,
      std::bind(&IKTestNode::read_eef_pose, this));
  }

private:
  void read_eef_pose()
  {
    try {
      // link_base 기준 link_eef의 현재 pose
      auto tf = tf_buffer_.lookupTransform(
        "link_base",
        "link_eef",
        tf2::TimePointZero);

      double x = tf.transform.translation.x;
      double y = tf.transform.translation.y;
      double z = tf.transform.translation.z;

      RCLCPP_INFO(
        get_logger(),
        "Current EEF: x=%.3f, y=%.3f, z=%.3f",
        x, y, z);

      // 현재 위치에서 x 방향으로 +5 cm
      double target_x = x + 0.05;
      double target_y = y;
      double target_z = z;

      RCLCPP_INFO(
        get_logger(),
        "Target EEF : x=%.3f, y=%.3f, z=%.3f",
        target_x, target_y, target_z);

      rclcpp::shutdown();
    }
    catch (const tf2::TransformException & ex) {
      RCLCPP_WARN(
        get_logger(),
        "TF not ready: %s",
        ex.what());
    }
  }

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<IKTestNode>();
  rclcpp::spin(node);

  return 0;
}