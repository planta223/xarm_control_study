# xArm Control Study

## Purpose

실제 Pipe Auto Align 전에 **xArm ROS2/Gazebo 제어 구조를 학습하는 최소 프로젝트**입니다.
목표는 xArm 한 대를 보고, 현재 관절 위치를 읽고, 직접 만든 C++ node에서
joint1만 현재 위치 기준 +5° 움직이는 것입니다.

## Scope

- 포함: single xArm7, Gazebo, joint states, joint command, ros2_control.
- 제외: dual arm, FoundationPose, Pipe, D405, MoveIt Servo, Cartesian control,
  Shared Control, Teleop, RoHand, real robot.

직접 작성한 ROS 패키지는 `src/xarm_control_study` 하나입니다.
URDF, 로봇 mesh, controller 설정은 공식 소스를 재사용합니다.

## 확인한 환경 (2026-10-13, KST)

| 항목 | 실제 확인 결과 |
|---|---|
| OS | Ubuntu 22.04.5 LTS (jammy), amd64 |
| ROS distro | Humble, `/opt/ros/humble/bin/ros2` |
| Gazebo | Gazebo Classic 11.10.2 |
| xArm model | xArm7, 기존 실행 기록의 model 식별자 XS1305 |
| xarm_ros2 path | `~/pipe_align_control_kkb/xarm_control_study/vendor/xarm_ros2` |
| controller | `xarm7_traj_controller`: `joint_trajectory_controller/JointTrajectoryController` |
| hardware plugin | `gazebo_ros2_control/GazeboSystem` |
| command interfaces | position, velocity (joint1 ~ joint7, claimed) |
| state interfaces | position, velocity |
| 격리 환경 | ROS_DOMAIN_ID=87, ROS_LOCALHOST_ONLY=1, Gazebo master port 11387 |

모델을 추측하지 않았습니다. 기존 안내 `../how to use/000_slave켜기.txt`에 나온
`hyeongjun_xarm_control/dual_arm_xarm.launch.py`를 읽고, 그 launch가 실행하는
`/home/rico/HyeongJun_Workspace/teleop_xarm/teleop_to_xarm.cpp:46`의
`DOF = 7` 및 xArm7 주석을 확인했습니다. 같은 안내의 이미 기록된 SDK 출력에도
`DETAIL: 7,7,XS1305,...`가 있습니다. 실제 로봇 launch/SDK는 실행하지 않았습니다.

최초 `ros2 pkg list`에는 `xarm_sdk`만 있었고, 홈과 현재 폴더에 공식
`xarm_ros2`, `xarm_description`, `xarm_gazebo`, `xarm_moveit_config`는 없었습니다.
ros2_control 핵심 라이브러리는 `/opt/ros/humble`에 있었지만 Gazebo ROS 플러그인,
trajectory controller, control CLI 등이 빠져 있었습니다.

[공식 Humble 안내](https://github.com/xArm-Developer/xarm_ros2/blob/humble/ReadMe.md)의
Gazebo 예제를 사용하며, 소스는 Humble commit
`62936f7ea1846a85f7350de2c4c18f39e6d19715`로 고정했습니다.
관리자 권한 없이 필요한 바이너리만 `vendor/runtime`에 풀었습니다.
시스템 apt 설치/업그레이드와 기존 workspace 빌드는 하지 않았습니다.

## 파일 구조

```text
xarm_control_study/
├── .gitignore
├── README.md
├── scripts/
│   ├── dependencies.json          # 확인한 ROS deb 버전/URL/SHA256
│   ├── prepare_dependencies.py    # 공식 소스 + 누락 runtime 로컬 준비
│   └── study_env.bash             # 기존 overlay 제거, simulation domain 설정
├── src/xarm_control_study/
│   ├── CMakeLists.txt
│   ├── package.xml
│   ├── launch/xarm_sim.launch.py
│   └── src/joint_test_node.cpp
├── vendor/                       # 다운로드 의존성, Git/colcon 대상 제외
├── build/                        # 이 workspace만의 빌드 결과
├── install/
└── log/                          # 검증 로그와 GUI 화면
```

`vendor/resources`는 공식 `xarm_description`, `xarm_gazebo`, `xarm_controller`,
`uf_ros_lib`의 runtime resource를 원본 소스에 연결한 ament index입니다.
공식 xarm_controller의 C++나 real driver는 빌드하지 않습니다.
MoveIt 패키지 등 전체 upstream archive에 있는 나머지 코드는 실행하지 않습니다.
`vendor/COLCON_IGNORE`로 colcon이 외부 소스를 빌드하지 않게 합니다.

## Build

아래 명령은 **이 PC에서 실제 실행하여 성공한 명령**입니다.
다른 Ubuntu/ROS 버전을 위한 범용 설치 스크립트는 아닙니다.
이미 설치된 Humble, Gazebo Classic, colcon, compiler, joint_state_broadcaster,
robot_state_publisher, xacro 등의 시스템 의존성을 사용합니다.

```bash
cd ~/pipe_align_control_kkb/xarm_control_study
python3 scripts/prepare_dependencies.py
source scripts/study_env.bash
colcon build --symlink-install
source install/setup.bash
```

의존성 준비는 재실행할 수 있습니다. 다운로드는 `vendor/`에만 저장하고 deb는
`dependencies.json`의 SHA256을 검증합니다. 공식 apt 저장소의 해당 버전이
향후 삭제된다면 다운로드가 실패할 수 있으므로 현재 `vendor/`를 보존하면 좋습니다.
소스 archive에는 `.git`이 없으며 `git init`도 하지 않습니다.

colcon은 두 로컬 resource/runtime 경로에 `local_setup.*`가 없다는 WARNING을
표시할 수 있습니다. 필요한 경로는 `study_env.bash`가 설정하며 빌드는 정상 완료됩니다.

## Run

모든 터미널에서 먼저 같은 workspace와 simulation 환경을 설정합니다.
`study_env.bash`는 현재 shell의 기존 teleop overlay 환경을 제거하므로 별도 터미널을 사용합니다.
기존 `pipe_auto_align_ws/install/setup.bash`는 source하지 않습니다.

터미널 1: Gazebo GUI에 단일 xArm7을 띄웁니다. 제어 node는 자동 실행하지 않습니다.

```bash
cd ~/pipe_align_control_kkb/xarm_control_study
source scripts/study_env.bash
source install/setup.bash
ros2 launch xarm_control_study xarm_sim.launch.py
```

공식 예제의 table world를 그대로 사용합니다. 공식 world의 gravity는 0이므로
이 결과는 중력/하중 성능 검증이 아닌 command 흐름 학습 결과입니다.
Gazebo GUI의 재생 상태에서 다음 명령을 실행합니다.

터미널 2: node/topic/controller 및 simulation hardware를 확인합니다.

```bash
cd ~/pipe_align_control_kkb/xarm_control_study
source scripts/study_env.bash
source install/setup.bash
ros2 node list
ros2 topic list
ros2 control list_controllers
ros2 control list_hardware_components
ros2 control list_hardware_interfaces
```

두 controller가 `active`이고 hardware가 `gazebo_ros2_control/GazeboSystem`인지 확인합니다.

현재 위치만 읽기: joint1 ~ joint7을 한 번 출력하고 종료합니다.

```bash
ros2 run xarm_control_study joint_test_node --ros-args -p use_sim_time:=true
```

joint1을 현재값에서 +5° 움직이기: 3초 궤적을 한 번 보내고 feedback을 검증한 뒤 종료합니다.

```bash
ros2 run xarm_control_study joint_test_node --ros-args -p use_sim_time:=true -p move:=true
```

**move 명령을 다시 실행할 때마다 현재값에서 +5°가 추가됩니다.**
나머지 6축 목표는 명령 직전 현재값입니다. 초기 0°부터 다시 보려면 터미널 1에서
Ctrl+C로 simulation을 종료하고 같은 launch를 다시 실행합니다.
성공은 exit code 0, timeout/목표 불일치/환경 오류는 nonzero입니다.
기본 read 모드에는 command publisher가 없습니다.

## Architecture / 코드 흐름

```text
Gazebo xArm7 joint position
    ↓ gazebo_ros2_control/GazeboSystem (state interface)
joint_state_broadcaster
    ↓ /joint_states [sensor_msgs/msg/JointState]
joint_test_node
    ↓ /xarm7_traj_controller/joint_trajectory [trajectory_msgs/msg/JointTrajectory]
xarm7_traj_controller [JointTrajectoryController]
    ↓ ros2_control position + velocity command interfaces
gazebo_ros2_control/GazeboSystem
    ↓
Gazebo xArm7 motion
```

1. **Gazebo**: 공식 URDF/mesh로 만든 로봇의 joint 위치와 움직임을 시뮬레이션합니다.
   GazeboSystem이 simulation 상태를 ros2_control state interface로 전달합니다.
2. **joint_state_broadcaster**: state interface를 읽어 `/joint_states`로 발행합니다.
   목표를 만드는 제어기는 아니며 관절 이름·위치·속도를 외부 node에 전달합니다.
3. **/joint_states**: `name`과 `position` 배열이 대응합니다. 실제 배열 순서는
   joint2, joint3, joint5, joint6, joint1, joint4, joint7이므로 이름으로 찾아야 합니다.
4. **joint_test_node**: subscriber callback에서 fresh feedback을 읽고 joint1만 +5°인
   목표를 만듭니다. 나머지는 현재값을 복사하여 의도하지 않은 다른 축 이동을 피합니다.
5. **JointTrajectory publisher**: 7개 joint 이름과 두 trajectory point를 보냅니다.
   첫 point는 현재 위치(0초), 두 번째는 목표(3초)이며 양 끝의 속도는 0입니다.
6. **xarm7_traj_controller**: 받은 궤적을 시간에 따라 보간하여 position·velocity command
   interface에 씁니다. 공식 YAML의 설정을 사용하며 이 예제에서 새 protocol을 만들지 않습니다.
7. **GazeboSystem → motion**: command interface 값을 Gazebo joint에 반영합니다.
   node는 3.5초 이후 새 feedback에서 모든 축 오차가 0.001 rad 미만인지 검증합니다.

실제 로봇은 같은 상위 ROS 개념을 사용하더라도 마지막 hardware driver가 SDK/네트워크를
통해 물리 모터에 명령합니다. 여기서는 GazeboSystem만 선택하고 real driver를 로드하지 않습니다.
launch와 node는 domain 87/localhost를 요구하며 node는 use_sim_time 및
`/robot_description`의 Gazebo hardware plugin도 확인합니다. 이는 실수 방지용 검사입니다.

## Learned Concepts

| 개념 | 이 프로젝트에서의 역할 |
|---|---|
| URDF/xacro | link, joint, mesh, 관절 제한, hardware plugin을 기술하는 로봇 모델. xacro는 모델을 파라미터로 조합합니다. |
| robot_state_publisher | URDF와 joint state로 link 간 TF를 계산하며 `/robot_description`을 제공합니다. |
| joint_state_broadcaster | ros2_control의 현재 state를 읽어 표준 JointState로 발행합니다. |
| controller | trajectory 목표를 시각별 관절 명령으로 변환합니다. 여기서는 JointTrajectoryController입니다. |
| joint state | 현재 측정/시뮬레이션 관절 상태. position의 단위는 rad입니다. |
| joint command | 앞으로 도달할 목표. 이 예제는 이름·위치·속도·도달시간이 있는 JointTrajectory입니다. |
| subscriber / callback | `/joint_states`가 도착할 때 `on_state()`를 호출해 feedback을 처리합니다. |
| publisher | 기존 controller의 command topic에 trajectory를 한 번 발행합니다. |
| use_sim_time | `/clock`에 맞춰 simulation 시간으로 state 신선도와 motion 시간을 판단합니다. |

핵심 코드는 주석 포함 168줄의 `joint_test_node.cpp` 한 파일입니다.
생성자에서 통신 경로를 만들고, `on_state()`에서 읽기·목표 생성·전송·결과 확인이 이어집니다.
`sent_`는 고주기 callback마다 +5° 명령이 반복되는 것을 막습니다.
20초 wall-time watchdog은 Gazebo가 멈췄을 때도 기다림을 끝냅니다.

## Dependencies

| dependency | 이유 |
|---|---|
| ament_cmake | ROS 패키지 빌드 및 설치 등록 |
| rclcpp | C++ node, subscriber, publisher, timer, ROS clock |
| sensor_msgs | `/joint_states`의 JointState |
| trajectory_msgs | controller가 받는 JointTrajectory/JointTrajectoryPoint |
| std_msgs | `/robot_description`의 String을 읽어 simulation plugin 확인 |
| launch / launch_ros / ament_index_python | 공식 launch include 및 패키지 경로 탐색 |
| xarm_description / xarm_gazebo / xarm_controller / uf_ros_lib | 공식 URDF·mesh·world·launch·controller YAML 및 launch helper |
| gazebo_ros / gazebo_msgs / gazebo_ros2_control | Gazebo 실행, entity spawn, ROS 메시지, simulation hardware |
| controller_manager / joint_state_broadcaster / joint_trajectory_controller | controller 로딩, 상태 발행, 궤적 실행 |
| launch_param_builder | 공식 launch의 xacro 처리 helper |
| ros2controlcli | `ros2 control ...` 상태 조사 명령 |

`control_msgs`의 FollowJointTrajectory action도 controller에서 제공하지만,
이 작은 node는 topic publisher만 사용하므로 action client 코드는 없습니다.

## 확인된 실행 상태

Gazebo spawn 직후의 `ros2 node list`:

```text
/controller_manager
/gazebo
/gazebo_ros2_control
/joint_state_broadcaster
/robot_state_publisher
/xarm7_traj_controller
```

학습 node 실행 중에는 `/joint_test_node`가 추가되고 완료 후 사라집니다.
확인된 주요 topic/action:

| 이름 | 용도 |
|---|---|
| `/joint_states` | 실제 simulation JointState feedback |
| `/dynamic_joint_states` | ros2_control dynamic state |
| `/clock` | Gazebo simulation clock |
| `/robot_description` | 공식 xacro에서 생성한 URDF |
| `/tf`, `/tf_static` | 로봇 link 좌표 변환 |
| `/xarm7_traj_controller/joint_trajectory` | 이번 node가 사용하는 표준 command topic |
| `/xarm7_traj_controller/controller_state` | trajectory controller 상태 |
| `/xarm7_traj_controller/state` | controller가 함께 제공하는 기존 상태 topic |
| `/xarm7_traj_controller/follow_joint_trajectory` | 제공되는 action; 이번 node에서는 사용 안 함 |

`ros2 control list_controllers` 실제 결과:

```text
joint_state_broadcaster  joint_state_broadcaster/JointStateBroadcaster          active
xarm7_traj_controller    joint_trajectory_controller/JointTrajectoryController  active
```

## Milestone / 실측 결과

환경 조사 → skeleton → Gazebo 단일 spawn/GUI 확인 → controller 확인 →
joint state 읽기 → joint1 +5° 테스트 순서로 진행했습니다.

```text
Initial joint1: 0.000000000 rad (0°)
Target joint1:  0.087266463 rad (+5°)
Final joint1:   0.087266463 rad (+5°)
Other joints maximum drift: 0.000000000 rad
```

| 검사 | 결과 |
|---|---|
| Build | PASS (1 package) |
| Gazebo spawn | PASS (GUI에서 xArm7 한 대 확인) |
| Controller | PASS (두 controller active, GazeboSystem) |
| Joint state read | PASS (7축 이름과 현재 position 출력) |
| Joint command | PASS (+5°, 나머지 축 유지, 새 feedback으로 확인) |

검증 자료는 `log/`에 있습니다(Git 제외):
`gazebo-session.log`, `nodes.txt`, `topics.txt`, `controllers.txt`, `hardware.txt`,
`interfaces.txt`, `command-topic.txt`, `joint-read.txt`, `joint-test.txt`,
`final-joint-states.yaml`, `gazebo-screen.png`, `gazebo-after.png`.
최종 joint1 원시 측정값은 `0.08726646259971638 rad`였습니다.

## Git / 변경 범위

작업 전후 `cd ~/pipe_align_control_kkb && git status --short`를 확인했으나
이 환경의 해당 폴더에는 `.git`이 없어 `fatal: not a git repository`였습니다.
따라서 Git diff로 변경 범위를 검증할 수는 없으며 새 Git 저장소도 만들지 않았습니다.
프로젝트 파일 생성/수정은 `xarm_control_study/` 안에서만 수행했습니다.
`KITECH_sharedcontrol/`, `xarm_teleop_hypertelepresence/`, `pipe_auto_align_ws/`,
`PIPE_AUTO_ALIGN_PROGRESS.md`는 수정하지 않았습니다.

## 다음 학습

여러 joint 목표 → EEF pose 확인 → Cartesian control.
이 세 단계는 이번 프로젝트에서 구현하지 않았습니다.
