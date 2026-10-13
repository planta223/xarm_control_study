# source scripts/study_env.bash : 기존 teleop overlay를 제거한 학습용 shell 환경.
STUDY_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
unset AMENT_PREFIX_PATH CMAKE_PREFIX_PATH COLCON_PREFIX_PATH ROS_PACKAGE_PATH
unset PYTHONPATH LD_LIBRARY_PATH
source /opt/ros/humble/setup.bash
STUDY_RUNTIME="$STUDY_ROOT/vendor/runtime/opt/ros/humble"
export AMENT_PREFIX_PATH="$STUDY_ROOT/vendor/resources:$STUDY_RUNTIME:$AMENT_PREFIX_PATH"
export PYTHONPATH="$STUDY_ROOT/vendor/xarm_ros2/uf_ros_lib:$STUDY_RUNTIME/local/lib/python3.10/dist-packages:$STUDY_RUNTIME/lib/python3.10/site-packages:$PYTHONPATH"
export LD_LIBRARY_PATH="$STUDY_RUNTIME/lib:$LD_LIBRARY_PATH"
export GAZEBO_PLUGIN_PATH="$STUDY_RUNTIME/lib:/usr/lib/x86_64-linux-gnu/gazebo-11/plugins"
export GAZEBO_MODEL_PATH="$STUDY_ROOT/vendor/models:/usr/share/gazebo-11/models"
export GAZEBO_RESOURCE_PATH="/usr/share/gazebo-11"
export GAZEBO_MODEL_DATABASE_URI=""
# 기존 real-robot domain 8과 분리. 모든 명령 터미널에서 동일하게 source합니다.
export ROS_DOMAIN_ID=87
export ROS_LOCALHOST_ONLY=1
export GAZEBO_MASTER_URI=http://127.0.0.1:11387
export ROS_LOG_DIR="$STUDY_ROOT/log/ros"
export TMPDIR="$STUDY_ROOT/log/tmp"
mkdir -p "$ROS_LOG_DIR" "$TMPDIR"
unset STUDY_RUNTIME
