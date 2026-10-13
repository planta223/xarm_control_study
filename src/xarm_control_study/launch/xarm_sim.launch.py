"""공식 단일 xArm7 예제의 simulation 경로만 사용하는 얇은 wrapper."""
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, SetLaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():
    if os.environ.get('ROS_DOMAIN_ID') != '87' or os.environ.get('ROS_LOCALHOST_ONLY') != '1':
        raise RuntimeError('먼저 source scripts/study_env.bash 를 실행하세요.')

    # 기존 launch의 실행 코드 DOF=7과 기록의 XS1305를 근거로 xArm7을 사용합니다.
    # 공식 xacro -> robot_description -> robot_state_publisher / Gazebo spawn 순서.
    # spawn이 끝나면 공식 launch가 두 controller의 spawner를 실행합니다.
    # 이 값들은 고정하여 real hardware plugin이나 추가 장비로 전환하지 않습니다.
    settings = {
        'gz_type': 'gazebo',
        'ros2_control_plugin': 'gazebo_ros2_control/GazeboSystem',
        'load_controller': 'true',
        'show_rviz': 'false',
        'add_gripper': 'false',
        'add_vacuum_gripper': 'false',
        'add_bio_gripper': 'false',
        'add_realsense_d435i': 'false',
        'add_d435i_links': 'false',
        'add_other_geometry': 'false',
        'prefix': '',
        'ros_namespace': '',
        'robot_ip': '',
        'robot_sn': 'XS1305',
        'moveit_config_dump': '',
    }
    return LaunchDescription([
        *[SetLaunchConfiguration(key, value) for key, value in settings.items()],
        # URDFを自作せず、公式xarm7 launchとそのcontroller YAMLを再利用します。
        IncludeLaunchDescription(PythonLaunchDescriptionSource(os.path.join(
            get_package_share_directory('xarm_gazebo'), 'launch',
            'xarm7_beside_table_gazebo.launch.py'))),
    ])
