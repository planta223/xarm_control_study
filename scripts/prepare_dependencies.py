#!/usr/bin/env python3
"""이 PC의 누락된 runtime만 vendor에 준비. sudo/시스템 설치/SDK 빌드 없음."""
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / "vendor"
COMMIT = "62936f7ea1846a85f7350de2c4c18f39e6d19715"
VENDOR.mkdir(exist_ok=True)
(VENDOR / "COLCON_IGNORE").touch()

upstream = VENDOR / "xarm_ros2"
if not upstream.exists():
    url = f"https://codeload.github.com/xArm-Developer/xarm_ros2/tar.gz/{COMMIT}"
    print("Download official xarm_ros2:", COMMIT, flush=True)
    with urllib.request.urlopen(url, timeout=120) as response:
        with tarfile.open(fileobj=io.BytesIO(response.read()), mode="r:gz") as archive:
            archive.extractall(VENDOR)
    (VENDOR / f"xarm_ros2-{COMMIT}").rename(upstream)

# apt의 package manifest에 있는 SHA256을 확인하고 로컬에만 압축을 풉니다.
# 이미 설치된 /opt/ros/humble 라이브러리와 함께 쓰며, 전체 OS 설치기는 아닙니다.
for item in json.loads((ROOT / "scripts/dependencies.json").read_text()):
    deb = VENDOR / Path(item["url"]).name
    if not deb.exists():
        print("Download:", item["package"], flush=True)
        with urllib.request.urlopen(item["url"], timeout=120) as response:
            deb.write_bytes(response.read())
    if hashlib.sha256(deb.read_bytes()).hexdigest() != item["sha256"]:
        raise RuntimeError(f"Checksum mismatch: {deb}")
    subprocess.run(["dpkg-deb", "-x", str(deb), str(VENDOR / "runtime")], check=True)

# 공식 launch/URDF/YAML은 소스 그대로 등록합니다. controller C++/real driver는
# 빌드하지 않습니다. 실제 제어기는 배포판의 joint_trajectory_controller입니다.
prefix = VENDOR / "resources"
index = prefix / "share/ament_index/resource_index/packages"
index.mkdir(parents=True, exist_ok=True)
for package in ("xarm_description", "xarm_gazebo", "xarm_controller", "uf_ros_lib"):
    (index / package).touch()
    link = prefix / "share" / package
    if not link.exists():
        link.symlink_to(upstream / package, target_is_directory=True)
# 공식 world가 참조하는 table만 다운로드하여 Gazebo의 온라인 모델 검색을 피합니다.
table = VENDOR / "models/table"
table.mkdir(parents=True, exist_ok=True)
model_commit = "8163eb4b5e7e21985c6591d1c0bfb56468c0093f"
for name in ("model.config", "model.sdf", "model-1_2.sdf", "model-1_3.sdf", "model-1_4.sdf"):
    path = table / name
    if not path.exists():
        url = f"https://raw.githubusercontent.com/osrf/gazebo_models/{model_commit}/table/{name}"
        with urllib.request.urlopen(url, timeout=60) as response:
            path.write_bytes(response.read())
print("Dependencies ready under", VENDOR)
