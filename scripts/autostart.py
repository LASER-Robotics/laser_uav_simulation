#!/usr/bin/env python3

import os
import sys
import time

import rclpy
from laser_msgs.msg import ApiPx4Diagnostics
from nav_msgs.msg import Odometry
from rclpy.node import Node
from std_srvs.srv import Trigger


class AutoStartNode(Node):
    def __init__(self):
        uav_env = str(os.environ.get('UAV_NAME', 'uav1')).strip('/').lower()

        super().__init__('autostart', namespace=uav_env)

        self.uav_name = uav_env
        self.get_logger().info(f"AutoStart active for: /{self.uav_name}")

        self.arm_client = self.create_client(Trigger, f'/{self.uav_name}/hw_api/arm')
        self.takeoff_client = self.create_client(
            Trigger, f'/{self.uav_name}/control_manager/takeoff')

        self.diag_sub = self.create_subscription(
            ApiPx4Diagnostics,
            f'/{self.uav_name}/hw_api/diagnostics',
            self.diag_callback,
            10)

        self.odom_sub = self.create_subscription(
            Odometry,
            f'/{self.uav_name}/estimation_manager/estimation',
            self.odom_callback,
            10)

        self.current_diag = None
        self.estimation_received = False
        self.arm_requested = False
        self.takeoff_requested = False

        self.create_timer(0.5, self.control_loop)

    def diag_callback(self, msg):
        self.current_diag = msg

    def odom_callback(self, msg):
        if not self.estimation_received:
            self.get_logger().info("Estimation data received! Position lock confirmed.")
            self.estimation_received = True

    def call_trigger(self, client, label):
        if not client.service_is_ready():
            self.get_logger().warn(f"Waiting for {label} service...", throttle_duration_sec=5.0)
            return False

        self.get_logger().info(f"Calling {label}...")
        client.call_async(Trigger.Request())
        return True

    def control_loop(self):
        if self.current_diag is None:
            return

        if not self.estimation_received:
            self.get_logger().warn("Waiting for Estimation/Odometry topic...", throttle_duration_sec=5.0)
            return

        if not self.current_diag.preflight_checks_passed:
            self.get_logger().warn("Pre-flight checks failing. Logic paused.", throttle_duration_sec=5.0)
            return

        if not self.current_diag.armed and not self.arm_requested:
            if self.call_trigger(self.arm_client, "ARM"):
                self.arm_requested = True
            return

        if self.current_diag.armed and self.current_diag.offboard_mode:
            if not self.takeoff_requested:
                if self.call_trigger(self.takeoff_client, "TAKEOFF"):
                    self.takeoff_requested = True
                    self.get_logger().info("Takeoff command sent! Mission starting.")
                    time.sleep(3.0)
                    sys.exit(0)
        else:
            if self.arm_requested and not self.takeoff_requested:
                if not self.current_diag.armed:
                    self.get_logger().info("Waiting for ARM confirmation...")
                elif not self.current_diag.offboard_mode:
                    self.get_logger().warn("Armed but NOT in OFFBOARD mode.")


def main(args=None):
    rclpy.init(args=args)
    node = AutoStartNode()
    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, SystemExit):
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
