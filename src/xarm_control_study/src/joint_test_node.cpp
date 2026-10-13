#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <std_msgs/msg/string.hpp>
#include <trajectory_msgs/msg/joint_trajectory.hpp>

using namespace std::chrono_literals;

// 이 node는 '상태 읽기 -> 목표 만들기 -> 표준 controller에 명령'을 보여줍니다.
// Gazebo 물리 엔진/저수준 제어를 직접 구현하지 않습니다. 실제 robot에서는
// GazeboSystem 대신 hardware driver가 SDK/통신으로 모터를 구동하지만,
// 여기서는 SDK, robot IP, real hardware plugin을 사용하지 않습니다.
class JointTestNode : public rclcpp::Node
{
public:
  JointTestNode() : Node("joint_test_node")
  {
    move_ = declare_parameter("move", false);  // 기본 실행은 읽기만 합니다.
    if (!get_parameter("use_sim_time").as_bool()) {
      throw std::runtime_error("use_sim_time:=true is required");
    }
    for (int i = 1; i <= 7; ++i) {names_.push_back("joint" + std::to_string(i));}

    // robot_state_publisher의 latched URDF로 Gazebo용 description인지 확인합니다.
    // 별도 domain/localhost 환경과 함께 실물 경로에 실수로 명령하는 것을 막습니다.
    description_ = create_subscription<std_msgs::msg::String>(
      "/robot_description", rclcpp::QoS(1).transient_local().reliable(),
      [this](std_msgs::msg::String::ConstSharedPtr msg) {
        simulation_ = msg->data.find("<plugin>gazebo_ros2_control/GazeboSystem</plugin>")
          != std::string::npos && msg->data.find("UFRobotSystemHardware") == std::string::npos;
      });

    // 현재 joint state는 GazeboSystem -> joint_state_broadcaster -> /joint_states로
    // 전달됩니다. callback은 새 feedback이 올 때 실행되며, 이름으로 순서를 맞춥니다.
    // 메시지 배열 순서가 항상 joint1...7이라고 가정하면 잘못된 축을 움직일 수 있습니다.
    subscription_ = create_subscription<sensor_msgs::msg::JointState>(
      "/joint_states", rclcpp::SensorDataQoS(),
      [this](sensor_msgs::msg::JointState::ConstSharedPtr msg) {on_state(*msg);});

    // 표준 JointTrajectory topic은 이미 실행 중인 controller의 command 입력입니다.
    // 읽기 모드에서는 publisher조차 만들지 않습니다. action은 이번 예제에서 쓰지 않습니다.
    if (move_) {
      command_ = create_publisher<trajectory_msgs::msg::JointTrajectory>(
        "/xarm7_traj_controller/joint_trajectory", 10);
    }
    // /clock이 멈추거나 feedback이 없을 때도 wall time으로 종료합니다.
    watchdog_ = create_wall_timer(20s, [this]() {finish(false, "Timed out waiting for simulation/feedback");});
  }

  int result() const {return result_;}

private:
  void on_state(const sensor_msgs::msg::JointState & msg)
  {
    // 오래된 state로 목표를 만들지 않습니다. /clock과 state는 같은 simulation 시간입니다.
    const auto stamp = rclcpp::Time(msg.header.stamp, RCL_ROS_TIME);
    const double age = (now() - stamp).seconds();
    if (!simulation_ || stamp.nanoseconds() == 0 || age < 0.0 || age > 0.5) {return;}
    std::vector<double> current;
    for (const auto & name : names_) {
      auto it = std::find(msg.name.begin(), msg.name.end(), name);
      if (it == msg.name.end()) {return;}
      const auto index = static_cast<size_t>(std::distance(msg.name.begin(), it));
      if (index >= msg.position.size() || !std::isfinite(msg.position[index])) {return;}
      current.push_back(msg.position[index]);
    }

    if (!printed_) {
      std::ostringstream values;
      for (size_t i = 0; i < names_.size(); ++i) {
        values << names_[i] << ": " << current[i] << " rad\n";
      }
      RCLCPP_INFO(get_logger(), "Current joint positions:\n%s", values.str().c_str());
      printed_ = true;  // 1000 Hz feedback을 받아도 최초 위치는 한 번만 출력합니다.
    }
    if (!move_) {finish(true, "Joint state read PASS"); return;}

    if (!sent_) {
      if (command_->get_subscription_count() != 1) {return;}
      // 현재 자세를 먼저 복사해야 나머지 축을 0으로 돌려버리지 않습니다.
      // 목표는 joint1만 현재값 +5도, 나머지는 현재값 유지입니다. ROS 각도 단위는 rad.
      initial_ = current;
      target_ = current;
      target_[0] += 5.0 * std::acos(-1.0) / 180.0;
      target_[1] += 10.0 * std::acos(-1.0) / 180.0;
      if (target_[0] > 2.0 * std::acos(-1.0) || target_[0] < -2.0 * std::acos(-1.0)) {
        finish(false, "joint1 target exceeds official URDF limits"); return;
      }
      trajectory_msgs::msg::JointTrajectory trajectory;
      trajectory.joint_names = names_;
      trajectory_msgs::msg::JointTrajectoryPoint start, goal;
      start.positions = initial_;
      goal.positions = target_;
      // controller는 position+velocity를 요구합니다. 두 끝점의 속도를 0으로 두어
      // 3초 동안 부드럽게 보간하고 정지하게 합니다. time_from_start의 단위는 초입니다.
      start.velocities.assign(7, 0.0);
      goal.velocities.assign(7, 0.0);
      goal.time_from_start.sec = 3;
      trajectory.points = {start, goal};
      // stamp=0은 수신 즉시 시작. controller가 궤적을 보간하고 ros2_control의
      // position/velocity interface -> GazeboSystem -> Gazebo joint에 전달합니다.
      command_->publish(trajectory);
      sent_ = true;  // 매 feedback마다 +5도를 누적하지 않고 한 번만 보냅니다.
      sent_at_ = now();
      RCLCPP_INFO(get_logger(), "Initial joint1: %.9f rad; Target joint1: %.9f rad (+5 deg)",
        initial_[0], target_[0]);
      return;
    }

    // publish 성공은 motion 성공이 아닙니다. 새 /joint_states로 도달 여부를 확인합니다.
    // 나머지 6축도 원래 위치와 비교하여 한 축만 움직였는지 검증합니다.
    if ((stamp - sent_at_).seconds() >= 3.5) {
      double error = 0.0, other_drift = 0.0;
      for (size_t i = 0; i < target_.size(); ++i) {
        error = std::max(error, std::abs(current[i] - target_[i]));
        if (i > 0) {other_drift = std::max(other_drift, std::abs(current[i] - initial_[i]));}
      }
      RCLCPP_INFO(get_logger(), "Final joint1: %.9f rad; other joints max drift: %.9f rad",
        current[0], other_drift);
      finish(error < 0.001, error < 0.001 ? "Joint command PASS" : "Joint command FAIL (tolerance 0.001 rad)");
    }
  }

  void finish(bool success, const char * message)
  {
    result_ = success ? 0 : 1;
    if (success) {RCLCPP_INFO(get_logger(), "%s", message);}
    else {RCLCPP_ERROR(get_logger(), "%s", message);}
    rclcpp::shutdown();
  }

  bool move_{false}, simulation_{false}, printed_{false}, sent_{false};
  int result_{1};
  std::vector<std::string> names_;
  std::vector<double> initial_, target_;
  rclcpp::Time sent_at_{0, 0, RCL_ROS_TIME};
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr description_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr subscription_;
  rclcpp::Publisher<trajectory_msgs::msg::JointTrajectory>::SharedPtr command_;
  rclcpp::TimerBase::SharedPtr watchdog_;
};

int main(int argc, char ** argv)
{
  // 실물 환경(domain 8)에서 단독 ros2 run을 잘못 실행하는 경우도 차단합니다.
  const char * domain = std::getenv("ROS_DOMAIN_ID");
  const char * localhost = std::getenv("ROS_LOCALHOST_ONLY");
  if (!domain || std::string(domain) != "87" || !localhost || std::string(localhost) != "1") {
    std::fprintf(stderr, "Source scripts/study_env.bash first (domain 87, localhost only).\n");
    return 1;
  }
  rclcpp::init(argc, argv);
  try {
    auto node = std::make_shared<JointTestNode>();
    rclcpp::spin(node);
    return node->result();
  } catch (const std::exception & error) {
    std::fprintf(stderr, "%s\n", error.what());
    rclcpp::shutdown();
    return 1;
  }
}
